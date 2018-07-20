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

#ifndef _hander_h_
#define _hander_h_

#include <sys/queue.h>
#include <stdbool.h>

#include "helper.h"
#include "template.h"

struct page_info {
	char		*path;

	struct memmap	*content;
	struct memmap	*descr;
	struct memmap	*link;
	struct memmap	*sort;
	struct memmap	*style;
	struct memmap	*script;
	struct memmap	*title;
	bool		 ssl;
	bool		 sub;
};


struct lang_pref {
	TAILQ_ENTRY(lang_pref)	entries;
	char			lang[18];
	float			priority;
};


struct header {
	TAILQ_ENTRY(header)	 entries;
	char			*key;
	char			*value;
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

	int			 status_code;
	char			*status;

	TAILQ_HEAD(, header)	 headers;
	TAILQ_HEAD(, lang_pref)	 accept_languages;
};


struct lang_pref	*lang_pref_new(const char *_lang, const float _prio);
void			 lang_pref_free(struct lang_pref *);


struct request		*request_new(char *_page_uri);
void			 request_free(struct request *);


struct page_info	*page_info_new(char *_path);
void			 page_info_free(struct page_info *);


struct page_info	*fetch_page(struct request *);
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

struct tmpl_loop	*get_language_links(struct request *);
struct tmpl_loop	*get_links(struct request *);

#endif
