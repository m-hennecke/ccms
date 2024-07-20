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

#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <sys/queue.h>
#include <stdbool.h>

#include "helper.h"
#include "htpasswd.h"
#include "session.h"
#include "template.h"

extern char *cms_content_dir;
extern char *cms_template_dir;
extern char *cms_session_db;
extern char *cms_session_htpasswd;

struct page_info {
	char		*path;

	struct md_mmap	*content;
	struct md_mmap	*descr;
	struct memmap	*link;
	struct md_mmap	*login;
	struct memmap	*sort;
	struct memmap	*style;
	struct memmap	*script;
	struct memmap	*title;
	bool		 ssl;
	bool		 sub;
};


struct lang_pref {
	TAILQ_ENTRY(lang_pref)	entries;
	float			priority;
	char			lang[18];
	char			short_lang[9];
};


struct header {
	TAILQ_ENTRY(header)	 entries;
	char			*key;
	char			*value;
};


struct param {
	TAILQ_ENTRY(param)	 entries;
	char			*name;
	char			*value;
};


struct cookie {
	TAILQ_ENTRY(cookie)	 entries;
	char			*name;
	char			*value;
	char			*path;
	char			*domain;
	time_t			 expires;
};


enum request_method {
	GET = 0, POST, INVALID
};


struct request {
	int			 content_dir;
	int			 lang_dir;
	int			 page_dir;
	int			 template_dir;

	char			*path_info;
	char			*page;
	char			*lang;
	char			*path;

	struct page_info	*page_info;
	struct tmpl_data	*data;
	struct md_mmap		*content;

	struct memmap		*tmpl_file;

	int			 status_code;
	int			 request_method;
	char			*status;

	TAILQ_HEAD(, header)	 headers;
	TAILQ_HEAD(, lang_pref)	 accept_languages;
	TAILQ_HEAD(, cookie)	 cookies;
	TAILQ_HEAD(, param)	 params;

	struct dir_list		*avail_languages;

	struct session		*session;
	struct session_store	*session_store;

	struct htpasswd		*htpasswd;

	void			*req_body;
	size_t			 req_body_size;
};


struct lang_pref	*lang_pref_new(const char *_lang, const float _prio);
void			 lang_pref_free(struct lang_pref *);


struct request		*request_new(char *_page_uri);
void			 request_free(struct request *);


struct page_info	*page_info_new(char *_path);
void			 page_info_free(struct page_info *);


struct page_info	*request_fetch_page(struct request *);
struct tmpl_loop	*fetch_language_links(struct request *);
struct tmpl_loop	*fetch_links(struct request *);

char			*set_language(struct request *);
void			 request_add_lang_pref(struct request *,
		struct lang_pref *);
void			 request_parse_lang_pref(struct request *);

struct header		*header_new(const char *, const char *);
void			 header_free(struct header *);
void			 request_add_header(struct request *, const char *,
		const char *);
struct buffer_list *	 request_output_headers(struct request *);

struct cookie		*cookie_new(const char *, const char *);
void			 cookie_free(struct cookie *);
void			 request_cookie_mark_delete(struct request *,
		struct cookie *);
bool			 request_cookie_remove(struct request *,
		struct cookie *);
void			 request_cookie_set(struct request *, struct cookie *);
struct cookie *		 request_cookie_get(struct request *, const char *);

void			 request_parse_cookies(struct request *);

struct tmpl_data	*request_init_tmpl_data(struct request *);
struct tmpl_loop	*request_get_language_links(struct request *);
struct tmpl_loop	*request_get_links(struct request *);

struct param		*param_new(const char *, const char *);
void			 param_free(struct param *);
void			 param_decode(struct param *);

void			 request_parse_params(struct request *, const char *);
struct param		*request_get_param(struct request *, const char *,
		struct param *);
bool			 request_read_post_body(struct request *);
bool			 request_handle_login(struct request *);
struct buffer_list	*request_render_page(struct request *, const char *);

__dead void		 _error(const char *, const char *);

#endif // __HANDLER_H__
