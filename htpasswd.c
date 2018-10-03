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

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "helper.h"
#include "htpasswd.h"

static struct htpasswd *_htpasswd_init(struct memmap *);


struct htpasswd *
_htpasswd_init(struct memmap *_file)
{
	struct htpasswd *htp = calloc(1, sizeof(struct htpasswd));
	htp->htpasswd = strndup(_file->data, _file->size);
	if (htp->htpasswd == NULL)
		err(1, NULL);
	char *s = htp->htpasswd;
	int i = 0;
	while (s && *s != '\0' && i < MAX_HTPASSWD_ENTRIES) {
		htp->usernames[i] = s;
		// Find a ':'
		if ((s = strchr(s, ':')) != NULL) {
			*s++ = '\0';
			htp->hashes[i++] = s;
			s = strchr(s, '\n');
			if (s != NULL)
				*s++ = '\0';
		} else {
			break;
		}
	}

	return htp;
}


struct htpasswd	*
htpasswd_init(const char *_filename)
{
	struct htpasswd *result = NULL;
	struct memmap *file = memmap_new(_filename);
	if (file) {
		result = _htpasswd_init(file);
		memmap_free(file);
	}
	return result;
}


struct htpasswd *
htpasswd_init_at(int _fd, const char *_filename)
{
	struct htpasswd *result = NULL;
	struct memmap *file = memmap_new_at(_fd, _filename);
	if (file) {
		result = _htpasswd_init(file);
		memmap_free(file);
	}
	return result;
}


void
htpasswd_free(struct htpasswd *_htpasswd)
{
	if (_htpasswd) {
		if (_htpasswd->filesize)
			freezero(_htpasswd->htpasswd, _htpasswd->filesize);
		free(_htpasswd);
	}
}


bool
htpasswd_check_password(struct htpasswd *_htp, const char *_username,
		const char *_pass)
{
	if (_htp== NULL)
		return false;

	int i = 0;
	while (_htp->usernames[i] && i < MAX_HTPASSWD_ENTRIES) {
		if (strcmp(_htp->usernames[i], _username) == 0) {
			return (crypt_checkpass(_pass, _htp->hashes[i]) == 0);
		}
		i++;
	}
	return false;
}
