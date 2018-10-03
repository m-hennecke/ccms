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
#include <sys/time.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
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

const char *supported_request_methods[3] = {
	"GET",
	"POST",
	NULL
};

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
	memmap_free(_info->login);
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
		err(1, "%s", cms_content_dir);

	req->template_dir = open(cms_template_dir, O_DIRECTORY | O_RDONLY);
	if (-1 == req->template_dir)
		err(1, "%s", cms_template_dir);
	req->page_dir = -1;
	req->lang_dir = -1;
	req->path_info = strdup(_path_info);

	req->lang = NULL;
	req->page = NULL;
	char *env_request_path = getenv("DOCUMENT_ROOT");
	req->path = strdup((env_request_path) ? env_request_path : "/");

	TAILQ_INIT(&req->headers);
	TAILQ_INIT(&req->accept_languages);
	TAILQ_INIT(&req->cookies);
	TAILQ_INIT(&req->params);

	const char *req_method = getenv("REQUEST_METHOD");
	if (req_method) {
		int i = 0;
		const char **supported = &supported_request_methods[0];
		while (*supported) {
			if (strcmp(req_method, *supported) == 0) {
				req->request_method = i;
				break;
			}
			i++;
			supported++;
		}
		if (*supported == NULL)
			errx(1, "unsupported request method '%s'", req_method);
	}

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

		struct cookie *c;
		while ((c = TAILQ_FIRST(&_req->cookies))) {
			TAILQ_REMOVE(&_req->cookies, c, entries);
			free(c);
		}

		struct param *p;
		while ((p = TAILQ_FIRST(&_req->params))) {
			TAILQ_REMOVE(&_req->params, p, entries);
			free(p);
		}

		dir_list_free(_req->avail_languages);

		session_free(_req->session);
		session_store_free(_req->session_store);

		htpasswd_free(_req->htpasswd);

		memmap_free(_req->tmpl_file);

		if (_req->session_store)
			session_store_cleanup(_req->session_store);
	}
}


struct page_info *
request_fetch_page(struct request *_req)
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
		} else if (strcmp("LOGIN", e->filename) == 0) {
			p->login = memmap_new_at(cwd, e->filename);
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

	_req->page_info = p;
	_req->content = p->content;
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


struct tmpl_data *
request_init_tmpl_data(struct request *_req)
{
	_req->data = tmpl_data_new();
	tmpl_data_set_variable(_req->data, "CURRENT_PAGE", _req->path_info);
	if (_req->page_info->descr)
		tmpl_data_set_variablen(_req->data, "DESCR",
				_req->page_info->descr->data,
				_req->page_info->descr->size);

	return _req->data;
}


bool
request_handle_login(struct request *_req)
{
	if (! _req->page_info->login)
		return true;

	request_parse_cookies(_req);
	_req->session_store = session_store_new(cms_session_db);

	struct cookie *c = request_cookie_get(_req, "sid");
	if (c != NULL)
		_req->session = session_load(c->value, _req->session_store);
	const bool new_session = (_req->session == NULL);
	const time_t now = time(NULL);
	const bool timeout = (_req->session 
			&& (_req->session->data.timeout < now));
	if (new_session || timeout) {
		struct session *session = session_new();
		if (timeout) {
			session_free(_req->session);
		}
		_req->session = session;
		request_cookie_remove(_req, c);
		cookie_free(c);
		c = cookie_new("sid", _req->session->sid);
		c->path = strdup("/");
	}

	request_cookie_set(_req, c);
	_req->session->data.timeout = now + (60 * 30);
	session_save(_req->session, _req->session_store, new_session);

	if (_req->request_method == POST) {
		request_parse_params(_req, getenv("QUERY_STRING"));
		char *content_type = getenv("CONTENT_TYPE");
		if (content_type
				&& (strcmp(content_type,
					"application/x-www-form-urlencoded")
					== 0)) {
			if (request_read_post_body(_req))
				request_parse_params(_req, _req->req_body);
		}
		char *username = NULL;
		char *password = NULL;
		struct param *p = request_get_param(_req, "username", NULL);
		if (p)
			username = p->value;
		p = request_get_param(_req, "password", NULL);
		if (p)
			password = p->value;
		if (username && password) {
			if (_req->htpasswd == NULL)
				_req->htpasswd
					= htpasswd_init(cms_session_htpasswd);
			_req->session->data.loggedin
				= htpasswd_check_password(_req->htpasswd,
						username, password);
			session_save(_req->session, _req->session_store, false);
		}
	}
	if (!_req->session->data.loggedin) {
		_req->content = _req->page_info->login;
	}

	return (_req->content == _req->page_info->content);
}


struct buffer_list *
request_render_page(struct request *_req, const char *_tmpl_filename)
{
	struct buffer_list *cb;
	struct buffer_list *result = NULL;

	if (_req->content) {
		cb = tmpl_parse(_req->content->data, _req->content->size,
				_req->data);
		tmpl_data_set_variable(_req->data, "CONTENT",
				buffer_list_concat_string(cb));
	} else {
		_error("404 Not Found", NULL);
	}

	struct tmpl_loop *links = request_get_links(_req);
	struct tmpl_loop *lang_links = request_get_language_links(_req);
	if (links)
		tmpl_data_set_loop(_req->data, "LINK_LOOP", links);
	if (lang_links)
		tmpl_data_set_loop(_req->data, "LANGUAGE_LINKS", lang_links);

	_req->tmpl_file = memmap_new_at(_req->template_dir, _tmpl_filename);
	if (_req->tmpl_file == NULL)
		_error("500 Internal Server Error", NULL);
	result = tmpl_parse(_req->tmpl_file->data, _req->tmpl_file->size,
			_req->data);

	buffer_list_free(cb);

	return result;
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


void
request_cookie_mark_delete(struct request *_req, struct cookie *_cookie)
{
	free(_cookie->value);
	_cookie->value = strdup("deleted");
	_cookie->expires = 1;
	request_cookie_set(_req, _cookie);
}


void
request_cookie_set(struct request *_req, struct cookie *_cookie)
{
	char expires[38] = { 0 };
	if (_cookie->expires != 0) {
		struct tm *tm = gmtime(&_cookie->expires);
		strftime(expires, sizeof(expires), "%a, %d %b %Y %T GMT", tm);
	}
	char *value;
	if (asprintf(&value, "%s=%s%s%s%s%s%s",
				_cookie->name, _cookie->value,
				(_cookie->path) ? "; path="    : "",
				(_cookie->path) ? _cookie->path : "",
				(_cookie->domain) ? "; domain="    : "",
				(_cookie->domain) ? _cookie->domain : "",
				(expires[0]) ? expires : "") == -1)
		err(1, NULL);
	request_add_header(_req, "Set-Cookie", value);
	free(value);
}


struct cookie *
request_cookie_get(struct request *_req, const char *_name)
{
	struct cookie *c;
	TAILQ_FOREACH(c, &_req->cookies, entries) {
		if (strcmp(c->name, _name) == 0)
			return c;
	}
	return NULL;
}


bool
request_cookie_remove(struct request *_req, struct cookie *_c)
{
	struct cookie *c = TAILQ_FIRST(&_req->cookies);
	while (c) {
		if (_c == c) {
			TAILQ_REMOVE(&_req->cookies, c, entries);
			return true;
		}
		c = TAILQ_NEXT(c, entries);
	}
	return false;
}


struct cookie *
cookie_new(const char *_name, const char *_value)
{
	struct cookie *c = calloc(1, sizeof(struct cookie));
	if (c == NULL)
		err(1, NULL);
	c->name = strdup(_name);
	c->value = strdup(_value);

	return c;
}


void
cookie_free(struct cookie *_cookie)
{
	if (_cookie) {
		free(_cookie->name);
		free(_cookie->value);
		free(_cookie->path);
		free(_cookie->domain);
		free(_cookie);
	}
}


void
request_parse_cookies(struct request *_req)
{
	const char *env_http_cookie = getenv("HTTP_COOKIE");
	if (env_http_cookie == NULL)
		return;

	char *cookies = strdup(env_http_cookie);
	if (cookies == NULL)
		err(1, NULL);

	char *key, *value;
	char *s = cookies;

	while (s && *s != '\0') { 
		// Skip leading whitespace
		while (*s == ' ')
			s++;

		// Find a '='
		key = s;
		if ((s = strchr(s, '=')) != NULL) {
			*s++ = '\0';
			value = s;
			s = strchr(s, ';');
			if (s != NULL) {
				*s++ = '\0';
			}
		} else {
			// Try to find a ';', and continue after
			s = strchr(key, ';');
			if (s != NULL)
				s++;
			continue;
		}

	        struct cookie * cookie = cookie_new(key, value);
		TAILQ_INSERT_TAIL(&_req->cookies, cookie, entries);
	}
	free(cookies);
}


struct param *
param_new(const char *_name, const char *_value)
{
	struct param *p = calloc(1, sizeof(struct param));
	if (_name)
		p->name = strdup(_name);
	if (_value)
		p->value = strdup(_value);
	return p;
}


void
param_decode(struct param *_param)
{
	decode_string(_param->value);
}


void
param_free(struct param *_param)
{
	if (_param) {
		free(_param->name);
		free(_param->value);
	}
}


void
request_parse_params(struct request *_req, const char *_params)
{
	if (_params == NULL)
		return;

	char *param_string = strdup(_params);
	if (param_string == NULL)
		err(1, NULL);
	char *s = param_string;
	char *name, *value;

	while (s && *s != '\0') {
		name = s;
		value = NULL;
		s = strchr(s, '=');
		if (s != NULL) {
			*s++ = '\0';
			value = s;
			s = strchr(s, '&');
			if (s != NULL) {
				*s++ = '\0';
			}
		}
		struct param *p = param_new(name, value);
		TAILQ_INSERT_TAIL(&_req->params, p, entries);
	}

	free(param_string);
}


struct param *
request_get_param(struct request *_req, const char *_name, struct param *_p)
{
	struct param *p;
	TAILQ_FOREACH(p, &_req->params, entries) {
		if (strcmp(_name, p->name) == 0) {
			if (p != _p)
				break;
		}
	}
	return p;
}


bool
request_read_post_body(struct request *_req)
{
	if (_req->request_method == POST) {
		const char *content_length_env = getenv("CONTENT_LENGTH");
		if (content_length_env == NULL)
			return false;
		errno = 0;
		char *ep;
		unsigned long content_length
			= strtoul(content_length_env, &ep, 10);
		if (content_length_env[0] == '\0' || *ep != '\0') {
			warnx("CONTENT_LENGTH is not a number");
			return false;
		}
		if (errno == ERANGE && content_length == ULONG_MAX) {
			warnx("CONTENT_LENGTH out of range");
			return false;
		}

		_req->req_body_size = content_length;
		_req->req_body = malloc(content_length + 1);
		unsigned char *buf = _req->req_body;

		ssize_t todo = content_length;
		off_t offset = 0;
		while (todo > 0) {
			ssize_t n = read(STDIN_FILENO, buf + offset,
					todo - offset);
			if (n == -1)
				err(1, NULL);
			todo -= n;
			offset += n;
		}
		buf[content_length] = '\0';
		return (todo == 0);
	}
	return false;
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
