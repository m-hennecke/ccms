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

#ifndef __SITEMAP_H__
#define __SITEMAP_H__

#include <sys/queue.h>

struct url_entry {
	TAILQ_ENTRY(url_entry)	 entries;
	struct dir_list		*dir;
	char			*url;
	time_t			 mtime;
};

struct lang_entry {
	TAILQ_ENTRY(lang_entry)	 entries;
	TAILQ_HEAD(, url_entry)	 pages;
	struct dir_list		*dir;
	char			*lang;
};

struct sitemap {
	TAILQ_HEAD(, lang_entry)	 languages;
	struct dir_list			*dir;
	char				*hostname;
};

struct lang_entry	*lang_entry_new(const char *);
struct url_entry	*url_entry_new(char *, time_t);
void			 lang_entry_free(struct lang_entry *);
void			 url_entry_free(struct url_entry *);
struct sitemap		*sitemap_new(const char *, const char *);
void			 sitemap_free(struct sitemap *);
char			*sitemap_toxml(struct sitemap *);
char			*sitemap_toxmlgz(struct sitemap *, size_t *,
    const char *, uint32_t);
uint32_t		 sitemap_newest(struct sitemap *, const char *);


#endif //__SITEMAP_H__
