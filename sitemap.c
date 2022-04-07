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

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "filehelper.h"
#include "buffer.h"
#include "sitemap.h"


static char			*concat_path(const char *, const char *);
static struct lang_entry	*read_language_dir(const char *, const char *,
    const char *);
static struct url_entry		*read_pages_dir(const char *, const char *,
    const char *, const char *);
static char			*format_time(time_t);
static char			*format_url(const char *, const char *,
    const char *, bool ssl);
static struct buffer_list	*create_xml_buffers(struct sitemap *);


struct url_entry *
url_entry_new(char *_url, time_t _mtime)
{
	struct url_entry *url = malloc(sizeof(struct url_entry));
	url->url = strdup(_url);
	url->mtime = _mtime;
	return url;
}


void
url_entry_free(struct url_entry *_url)
{
	if (_url) {
		free(_url->url);
		free(_url);
	}
}


struct lang_entry *
lang_entry_new(const char *_lang)
{
	struct lang_entry *lang = malloc(sizeof(struct lang_entry));
	lang->lang = strdup(_lang);
	TAILQ_INIT(&lang->pages);
	lang->dir = NULL;
	return lang;
}


void
lang_entry_free(struct lang_entry *_lang)
{
	if (_lang) {
		struct url_entry *url;
		while ((url = TAILQ_FIRST(&_lang->pages))) {
			TAILQ_REMOVE(&_lang->pages, url, entries);
			url_entry_free(url);
		}
		dir_list_free(_lang->dir);
		free(_lang->lang);
	}
}


void
sitemap_free(struct sitemap *_sitemap)
{
	if (_sitemap) {
		struct lang_entry *lang;
		while ((lang = TAILQ_FIRST(&_sitemap->languages))) {
			TAILQ_REMOVE(&_sitemap->languages, lang, entries);
			lang_entry_free(lang);
		}
		free(_sitemap->hostname);
		dir_list_free(_sitemap->dir);
		free(_sitemap);
	}
}


struct url_entry *
read_pages_dir(const char *_content_dir, const char *_lang, const char *_page,
    const char *_hostname)
{
	char *path;
	if (asprintf(&path, "%s/%s/%s", _content_dir, _lang, _page) == -1)
		err(1, NULL);
	struct dir_list *dir = get_dir_entries(path);
	if (! dir)
		err(1, NULL);
	char *url_string = format_url(_hostname, _lang, _page,
				dir_entry_exists("SSL", dir)); 
	struct url_entry *url = url_entry_new(url_string, dir->newest);
	url->dir = dir;

	free(path);
	return url;
}


struct lang_entry *
read_language_dir(const char *_content_dir, const char *_lang,
    const char *_hostname)
{
	char *lang_path = concat_path(_content_dir, _lang);
	struct dir_list *dir = get_dir_entries(lang_path);
	free(lang_path);
	if (! dir)
		err(1, NULL);
	struct lang_entry *l = lang_entry_new(_lang);
	if (!l)
		err(1, NULL);
	l->dir = dir;

	struct dir_entry *entry;
	TAILQ_FOREACH(entry, &dir->entries, entries) {
		if ((entry->sb.st_mode & S_IFDIR) == 0)
			continue;

		struct url_entry *url = read_pages_dir(_content_dir, _lang,
				entry->filename, _hostname);
		TAILQ_INSERT_TAIL(&l->pages, url, entries);
	}

	return l;
}


struct sitemap *
sitemap_new(const char *_content_dir, const char *_hostname)
{
	struct sitemap *sitemap = malloc(sizeof(struct sitemap));
	TAILQ_INIT(&sitemap->languages);
	sitemap->hostname = strdup(_hostname);

	sitemap->dir = get_dir_entries(_content_dir);
	if (sitemap->dir == NULL) {
		warn(NULL);
		goto bailout;
	}

	struct dir_entry *file;
	TAILQ_FOREACH(file, &sitemap->dir->entries, entries) {
		if ((file->sb.st_mode & S_IFDIR) == 0)
			continue;

		struct lang_entry *l = read_language_dir(_content_dir,
				file->filename, _hostname);
		TAILQ_INSERT_TAIL(&sitemap->languages, l, entries);
	}
	return sitemap;

bailout:
	free(sitemap->hostname);
	sitemap_free(sitemap);
	return NULL;
}


struct buffer_list *
create_xml_buffers(struct sitemap *s)
{
	struct buffer_list *xml = buffer_list_new();

	buffer_list_add_string(xml,
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\" "
		"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
		"xsi:schemaLocation="
		"\"http://www.sitemaps.org/schemas/sitemap/0.9\n"
		"http://www.sitemaps.org/schemas/sitemap/0.9/sitemap.xsd\">\n");
	struct lang_entry *lang;
	TAILQ_FOREACH(lang, &s->languages, entries) {
		struct url_entry *url;
		TAILQ_FOREACH(url, &lang->pages, entries) {
			buffer_list_add_string(xml, "<url><loc>");
			buffer_list_add_string(xml, url->url);
			buffer_list_add_string(xml, "</loc><lastmod>");
			char *isots = format_time(url->mtime);
			buffer_list_add_string(xml, isots);
			free(isots);
			buffer_list_add_string(xml, "</lastmod></url>");
		}
	}
	buffer_list_add_string(xml, "</urlset>");
	return xml;
}

char *
sitemap_toxml(struct sitemap *_s)
{
	struct buffer_list *xml = create_xml_buffers(_s);
	char *xml_string = buffer_list_concat_string(xml);
	buffer_list_free(xml);
	return xml_string;
}


char *
sitemap_toxmlgz(struct sitemap *_s, size_t *_size, const char *_filename,
		uint32_t _mtime)
{
	struct buffer_list *xml = create_xml_buffers(_s);
	struct buffer_list *gz = buffer_list_gzip(xml, _filename, _mtime);

	char *result = buffer_list_concat(gz);
	*_size = gz->size;

	buffer_list_free(gz);
	buffer_list_free(xml);

	return result;
}



char *
concat_path(const char *_path1, const char *_path2)
{
	char *path;
	if (asprintf(&path, "%s/%s", _path1, _path2) == -1)
		err(1, NULL);
	return path;
}


char *
format_time(time_t _datetime)
{
	char *result;
	struct tm *tm;
	if ((tm = gmtime(&_datetime)) == NULL)
		err(1, NULL);

	if (asprintf(&result, "%04d-%02d-%02dT%02d:%02d:%02dZ",
				tm->tm_year + 1900, tm->tm_mon, tm->tm_mday,
				tm->tm_hour, tm->tm_min, tm->tm_sec) == -1)
		err(1, NULL);

	return result;
}


char *
format_url(const char *_hostname, const char *_lang, const char *_page,
    bool ssl)
{
	const char *http = (ssl) ? "https" : "http";
	char *url;
	if (asprintf(&url, "%s://%s%s%s/%s.html", http, _hostname,
	    CMS_ROOT_URL, _lang, _page) == -1)
		err(1, NULL);
	return url;
}


uint32_t
sitemap_newest(struct sitemap *_sitemap, const char *_lang)
{
	uint32_t result = 0;

	struct lang_entry *lang;
	TAILQ_FOREACH(lang, &_sitemap->languages, entries) {
		if (!_lang || (strcmp(_lang, lang->lang) == 0)) {
			if (lang->dir->newest > result)
				result = lang->dir->newest;
			struct url_entry *url;
			TAILQ_FOREACH(url, &lang->pages, entries) {
				if (url->dir->newest > result)
					result = url->dir->newest;
			}
		}
	}

	return result;
}
