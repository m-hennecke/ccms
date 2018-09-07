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

#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "buffer.h"
#include "filehelper.h"
#include "handler.h"
#include "helper.h"

#define PAGE_URI_RX_PATTERN "/?((([a-z]{1,8}(-[a-z]{1,8})?))/)?" \
	"([_a-z0-9-]+)\\.html$"
#define PAGE_URI_MAX_GROUPS 6
#define PAGE_URI_LANG_GROUP 2
#define PAGE_URI_PAGE_GROUP 5

#define ACCEPT_LANGUAGE_RX_PATTERN "^ *([a-z]{1,8}(-[a-z]{1,8})?)" \
	" *(; *q *= *(1|0\\.[0-9]+))?$"
#define ACCEPT_LANGUAGE_MAX_GROUPS 5

struct lang_pref *
lang_pref_new(const char *_lang, const float _prio)
{
	struct lang_pref *lp = malloc(sizeof(struct lang_pref));
	if (lp == NULL)
		err(1, NULL);

	strlcpy(lp->lang, _lang, sizeof(lp->lang));
	lp->priority = _prio;
	strlcpy(lp->short_lang, _lang, sizeof(lp->short_lang));
	strtok(lp->short_lang, "-");
	return lp;
}


void
request_add_lang_pref(struct request *_req, struct lang_pref *_lang_pref)
{
	struct lang_pref *l = TAILQ_FIRST(&_req->accept_languages);
	while (l) {
		if (_lang_pref->priority < l->priority) {
			l = TAILQ_NEXT(l, entries);
			continue;
		}

		TAILQ_INSERT_BEFORE(l, _lang_pref, entries);
		return;
	}
	TAILQ_INSERT_TAIL(&_req->accept_languages, _lang_pref, entries);
}



void
request_parse_lang_pref(struct request *_req)
{
	regex_t rx;
	regmatch_t pmatch[ACCEPT_LANGUAGE_MAX_GROUPS];

	char *env_accept_lang = getenv("HTTP_ACCEPT_LANGUAGE");
	if (env_accept_lang == NULL)
		return;

	char *accept_lang = strdup(env_accept_lang);

	// Split the string and feed the bits to request_add_lang_pref
	size_t len = strlen(accept_lang);
	char *end = accept_lang + len;
	char *it = accept_lang;

	int rc = regcomp(&rx, ACCEPT_LANGUAGE_RX_PATTERN,
			REG_EXTENDED | REG_ICASE);
	if (rc != 0) {
		warnx("regcomp: %s", rx_get_errormsg(rc, &rx));
		return;
	}

	while (it < end) {
		float prio = 1;
		size_t match_len;
		char *comma = strchr(it, ',');
		char *lang;

		if (comma == NULL)
			comma = end;
		else
			*comma = '\0';
		rc = regexec(&rx, it, ACCEPT_LANGUAGE_MAX_GROUPS, pmatch, 0);
		switch (rc) {
		case REG_NOMATCH:
			warnx("'%s' did not match language rx\n", it);
			goto cleanup_lang;
		case 0:
			if (pmatch[4].rm_so != -1)
				prio = atof(it + pmatch[4].rm_so);
			match_len = pmatch[1].rm_eo - pmatch[1].rm_so;
			lang = strndup(it + pmatch[1].rm_so, match_len);
			struct lang_pref *lp = lang_pref_new(lang, prio);
			// Only store available languages
			bool store = false;
			if (dir_entry_exists(lp->lang, _req->avail_languages)) {
				store = true;
			} else {
				lp->lang[0] = '\0';
			}
			if (dir_entry_exists(lp->short_lang,
						_req->avail_languages)) {
				store = true;
			} else {
				lp->short_lang[0] = '\0';
			}
			if (store) {
				request_add_lang_pref(_req, lp);
			} else {
				free(lp);
			}
			free(lang);

			break;
		default:
			warnx("regexec: %s", rx_get_errormsg(rc, &rx));
			break;
		}
		it = comma + 1;
	}

cleanup_lang:
	free(accept_lang);
	regfree(&rx);
}


struct page_info *
page_info_new(char *_path)
{
	struct page_info *info = calloc(1, sizeof(struct page_info));
	if (NULL == info)
		err(1, NULL);
	info->path = _path;
	return info;
}


void
page_info_free(struct page_info *_info)
{
	if (NULL == _info)
		return;

	memmap_free(_info->content);
	memmap_free(_info->descr);
	memmap_free(_info->link);
	memmap_free(_info->sort);
	memmap_free(_info->style);
	memmap_free(_info->script);
	memmap_free(_info->title);

	free(_info->path);

	free(_info);
}


struct request *
request_new(char *_path_info)
{
	size_t match_len;
	regmatch_t *rxmatch;

	struct request *req = calloc(1, sizeof(struct request));
	if (req == NULL)
		err(1, NULL);

	req->content_dir = open(cms_content_dir, O_DIRECTORY | O_RDONLY);
	if (-1 == req->content_dir)
		err(1, NULL);

	req->template_dir = open(cms_template_dir, O_DIRECTORY | O_RDONLY);
	if (-1 == req->template_dir)
		err(1, NULL);
	req->page_dir = -1;
	req->lang_dir = -1;
	req->path_info = strdup(_path_info);

	req->lang = NULL;
	req->page = NULL;
	char *env_request_path = getenv("DOCUMENT_ROOT");
	req->path = strdup((env_request_path) ? env_request_path : "/");

	TAILQ_INIT(&req->headers);
	TAILQ_INIT(&req->accept_languages);

	regex_t rx;
	int rc = regcomp(&rx, PAGE_URI_RX_PATTERN, REG_EXTENDED);
	if (rc != 0) {
		warnx("regcomp: %s", rx_get_errormsg(rc, &rx));
		request_free(req);
		return NULL;
	}

	regmatch_t rx_group_array[PAGE_URI_MAX_GROUPS];
	rc = regexec(&rx, _path_info, PAGE_URI_MAX_GROUPS,
			rx_group_array, 0);
	switch (rc) {
	case REG_NOMATCH:
		request_free(req);
		return NULL;
	case 0:
		rxmatch = &rx_group_array[PAGE_URI_PAGE_GROUP];
		match_len = rxmatch->rm_eo - rxmatch->rm_so;
		req->page = strndup(_path_info + rxmatch->rm_so, match_len);
		rxmatch = &rx_group_array[PAGE_URI_LANG_GROUP];
		match_len = rxmatch->rm_eo - rxmatch->rm_so;
		if (match_len > 0) {
			req->lang = strndup(_path_info + rxmatch->rm_so,
					match_len);
		}
		break;
	default:
		warnx("regexec: %s", rx_get_errormsg(rc, &rx));
		break;
	}

	regfree(&rx);

	req->avail_languages = get_dir_entries(cms_content_dir);
	request_parse_lang_pref(req);

	if (NULL == req->lang) {
		struct lang_pref *lang = TAILQ_FIRST(&req->accept_languages);
		if (lang == NULL) {
			req->lang = strndup(CMS_DEFAULT_LANGUAGE,
					sizeof(req->lang) - 1);
		} else {
			if (lang->lang[0] != '\0')
				req->lang = strdup(lang->lang);
			else
				req->lang = strdup(lang->short_lang);
		}
	}
	req->lang_dir = openat(req->content_dir, req->lang,
			O_DIRECTORY | O_RDONLY);

	if (req->lang_dir == -1) {
		request_free(req);
		_error("404 Not Found", NULL);
	}

	return req;
}


void
request_free(struct request *_req)
{
	if (_req) {
		if (-1 != _req->content_dir)
			close(_req->content_dir);
		if (-1 != _req->template_dir)
			close(_req->template_dir);
		if (-1 != _req->lang_dir)
			close(_req->lang_dir);
		if (-1 != _req->page_dir)
			close(_req->page_dir);

		page_info_free(_req->page_info);
		tmpl_data_free(_req->data);

		free(_req->path_info);
		free(_req->page);
		free(_req->lang);
		free(_req->path);
		free(_req->status);

		struct header *h;
		while ((h = TAILQ_FIRST(&_req->headers))) {
			TAILQ_REMOVE(&_req->headers, h, entries);
			header_free(h);
		}

		struct lang_pref *lp;
		while ((lp = TAILQ_FIRST(&_req->accept_languages))) {
			TAILQ_REMOVE(&_req->accept_languages, lp, entries);
			free(lp);
		}
		dir_list_free(_req->avail_languages);
	}
}


struct page_info *
fetch_page(struct request *_req)
{
	struct dir_list *files = NULL;
	struct dir_entry *e;
	struct page_info *p = NULL;
	int cwd = -1;
	char *path;

	if (-1 == asprintf(&path, "%s/%s/%s", cms_content_dir, _req->lang,
				_req->page))
		err(1, NULL);
	cwd = open(path, O_DIRECTORY | O_RDONLY);
	if (-1 == cwd) {
		warn("%s", path);
		goto error_out;
	}

	p = page_info_new(path);
	files = get_dir_entries(path);
	if (files == NULL)
		goto error_out;

	TAILQ_FOREACH(e, &files->entries, entries) {
		if (strcmp("CONTENT", e->filename) == 0) {
			p->content = memmap_new_at(cwd, e->filename);
		} else if (strcmp("DESCR", e->filename) == 0) {
			p->descr = memmap_new_at(cwd, e->filename);
		} else if (strcmp("LINK", e->filename) == 0) {
			p->link = memmap_new_at(cwd, e->filename);
		} else if (strcmp("SORT", e->filename) == 0) {
			p->sort = memmap_new_at(cwd, e->filename);
		} else if (strcmp("SCRIPT", e->filename) == 0) {
			p->script = memmap_new_at(cwd, e->filename);
		} else if (strcmp("SSL", e->filename) == 0) {
			p->ssl = true;
		} else if (strcmp("STYLE", e->filename) == 0) {
			p->style = memmap_new_at(cwd, e->filename);
		} else if (strcmp("SUB", e->filename) == 0) {
			p->sub = true;
		} else if (strcmp("TITLE", e->filename) == 0) {
			p->title = memmap_new_at(cwd, e->filename);
		}
	}
	char last_modified[30];
	strftime(last_modified, sizeof(last_modified),
			"%a, %d %b %y %H:%M:%S GMT", gmtime(&files->newest));
	request_add_header(_req, "Last-Modified", last_modified);

	// Test for all required fields, if not here we error out
	if (!(p->content && p->link && p->sort && p->title && p->descr)) 
		goto error_out;

cleanup:
	if (files)
		dir_list_free(files);
	if (cwd != -1) {
		fchdir(cwd);
		close(cwd);
	}
	return p;

error_out:
	page_info_free(p);
	p = NULL;
	goto cleanup;
}


void
header_free(struct header *_header)
{
	if (_header) {
		free(_header->key);
		free(_header->value);
	}
}


struct header *
header_new(const char *_key, const char *_value)
{
	struct header *h = malloc(sizeof(struct header));
	if (h == NULL)
		err(1, NULL);

	h->key = strdup(_key);
	h->value = strdup(_value);
	return h;
}


void
request_add_header(struct request *_req, const char *_key, const char *_value)
{
	struct header *h = header_new(_key, _value);
	TAILQ_INSERT_TAIL(&_req->headers, h, entries);
}


struct buffer_list *
request_output_headers(struct request *_req)
{
	struct header *h;
	struct buffer_list *bl = buffer_list_new();
	TAILQ_FOREACH(h, &_req->headers, entries) {
		char *header;
		if ((asprintf(&header, "%s: %s\n", h->key, h->value) == -1))
			err(1, NULL);
		buffer_list_add_string(bl, header);
		free(header);
	}
	return bl;
}


__dead void
_error(const char *_status, const char *_content)
{
	dprintf(STDOUT_FILENO, "Status: %s\r\n", _status);
	dprintf(STDOUT_FILENO, "Content-Length: %zd\r\n\r\n",
			_content ? strlen(_content) : 0);
	if (_content)
		dprintf(STDOUT_FILENO, "%s", _content);
	exit(0);
}
