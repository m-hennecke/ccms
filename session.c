/*
 * Copyright (c) 2018 Markus Hennecke <markus-hennecke@markus-hennecke.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <db.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "session.h"

static const char b64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

static size_t
base64len(size_t _len)
{
	return((_len + 2) / 3 * 4) + 1;
}

static size_t
base64buf(char *_enc, const char *_str, size_t _len)
{
	size_t  i;
	char    *p;

	p = _enc;

	for (i = 0; i < _len - 2; i += 3) {
		*p++ = b64[(_str[i] >> 2) & 0x3F];
		*p++ = b64[((_str[i] & 0x3) << 4) |
			((int)(_str[i + 1] & 0xF0) >> 4)];
		*p++ = b64[((_str[i + 1] & 0xF) << 2) |
			((int)(_str[i + 2] & 0xC0) >> 6)];
		*p++ = b64[_str[i + 2] & 0x3F];
	}

	if (i < _len) {
		*p++ = b64[(_str[i] >> 2) & 0x3F];
		if (i == (_len - 1)) {
			*p++ = b64[((_str[i] & 0x3) << 4)];
			*p++ = '=';
		} else {
			*p++ = b64[((_str[i] & 0x3) << 4) |
				((int)(_str[i + 1] & 0xF0) >> 4)];
			*p++ = b64[((_str[i + 1] & 0xF) << 2)];
		}
		*p++ = '=';
	}

	*p++ = '\0';
	return(p - _enc);
}


struct session_data *
session_data_new(void)
{
	struct session_data *sd = calloc(1, sizeof(struct session_data));
	if (sd == NULL)
		err(1, NULL);
	sd->timeout = time(NULL) + 60 * 30;
	return sd;
}


bool
session_data_timeout(struct session_data *_s)
{
	time_t now = time(NULL);
	return (now > _s->timeout);
}


struct session *
session_new(void)
{
	char id[15];
	struct session *s = calloc(1, sizeof(struct session));
	if (s == NULL)
		err(1, NULL);
	s->data.timeout = time(NULL) + 60 * 30;
	arc4random_buf(id, sizeof(id));
	s->sid = malloc(base64len(sizeof(id)));
	base64buf(s->sid, id, sizeof(id));
	return s;
}


void
session_free(struct session *_s)
{
	if (_s) {
		free(_s->sid);
		free(_s);
	}
}


bool
session_save(struct session *_s, struct session_store *_store,
		bool _new)
{
	DBT key = { _s->sid, strlen(_s->sid) };
	DBT data = { &_s->data, sizeof(struct session_data) };

	int rc = _store->db->put(_store->db, &key, &data,
			(_new) ? R_NOOVERWRITE : 0);
	if (rc == -1)
		err(1, NULL);
	else if (_new && (rc == 1)) {
		errx(1, "Duplicate session key");
	}
	return true;
}


struct session *
session_load(char *_sid, struct session_store *_store)
{
	DBT key = { _sid, strlen(_sid) };
	struct session *s = calloc(1, sizeof(struct session));
	if (s == NULL)
		err(1, NULL);
	DBT data;

	int rc = _store->db->get(_store->db, &key, &data, 0);
	if (rc == -1)
		err(1, NULL);
	else if (rc == 1) {
		session_free(s);
		s = NULL;
	} else {
		memcpy(&s->data, data.data, sizeof(struct session_data));
		s->sid = strdup(_sid);
	}
	return s;
}


struct session_store *
session_store_new(const char *_filename)
{
	struct session_store *store = malloc(sizeof(struct session_store));
	if (store == NULL)
		err(1, NULL);
	store->db = dbopen(_filename, O_CREAT | O_RDWR | O_SHLOCK, 0600,
			DB_HASH, NULL);
	if (store->db == NULL)
		err(1, "%s", _filename);
	store->filename = strdup(_filename);
	return store;
}


void
session_store_free(struct session_store *_store)
{
	if (_store && _store->db) {
		_store->db->close(_store->db);
		free(_store->filename);
	}
	free(_store);
}


void
session_store_cleanup(struct session_store *_s)
{
	int rc;
	DBT key, data;
	while ((rc = _s->db->seq(_s->db, &key, &data, R_NEXT)) == 0) {
		if (session_data_timeout(data.data)) {
			if (-1 == _s->db->del(_s->db, &key, R_CURSOR)) {
				warn(NULL);
			}
		}
#if 0
		else {
			struct session_data *d
				= (struct session_data *)data.data;
			warnx("Session %.*s expires in %d seconds",
					(int )key.size,
					key.data, (
					int )(d->timeout - time(NULL)));
		}
#endif
	}
	if (rc == -1)
		warn(NULL);
}
