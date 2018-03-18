
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
#include "sitemap.h"


static char			*concat_path(const char *, const char *);
static struct lang_entry	*read_language_dir(char *_content_dir, char *,
		char *);
static struct url_entry		*read_pages_dir(char *, char *, char *, char *);
static char			*format_time(time_t _datetime);
static char			*format_url(char *_hostname, char *_lang,
		char *_page, bool ssl);

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
lang_entry_new(char *_lang)
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
read_pages_dir(char *_content_dir, char *_lang, char *_page, char *_hostname)
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
read_language_dir(char *_content_dir, char *_lang, char *_hostname)
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
sitemap_new(char *_content_dir, char *_hostname)
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


struct xml_buffer {
	SIMPLEQ_ENTRY(xml_buffer)	entries;
	size_t			size;
	char			data[1];
};

static struct xml_buffer *xml_buffer_new(char *_data);
struct xml_buffer *
xml_buffer_new(char *_data)
{
	size_t data_len = strlen(_data);
	struct xml_buffer *buf = malloc(sizeof(struct xml_buffer) + data_len);
	buf->size = data_len;
	strlcpy(buf->data, _data, data_len + 1);
	return buf;
}


char *
sitemap_toxml(struct sitemap *s)
{
	SIMPLEQ_HEAD(, xml_buffer) buffers;
	SIMPLEQ_INIT(&buffers);

	struct xml_buffer *buf = xml_buffer_new(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		"<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\" "
		"xmlns:xsi=\"http://www.w3.org/2001/XMSchema-instance\" "
		"xsi:schemaLocation="
		"\"http://www.sitemaps.org/schemas/sitemap/0.9\n"
		"http://www.sitemaps.org/schemas/sitemap/0.9/sitemap.xsd\">\n");
	SIMPLEQ_INSERT_TAIL(&buffers, buf, entries);
	struct lang_entry *lang;
	TAILQ_FOREACH(lang, &s->languages, entries) {
		struct url_entry *url;
		TAILQ_FOREACH(url, &lang->pages, entries) {
			buf  = xml_buffer_new("<url><loc>");
			SIMPLEQ_INSERT_TAIL(&buffers, buf, entries);
			buf = xml_buffer_new(url->url);
			SIMPLEQ_INSERT_TAIL(&buffers, buf, entries);
			buf = xml_buffer_new("</loc><lastmod>");
			SIMPLEQ_INSERT_TAIL(&buffers, buf, entries);
			buf = xml_buffer_new(format_time(url->mtime));
			SIMPLEQ_INSERT_TAIL(&buffers, buf, entries);
			buf = xml_buffer_new("</lastmod></url>");
			SIMPLEQ_INSERT_TAIL(&buffers, buf, entries);
		}
	}
	buf = xml_buffer_new("</urlset>");
	SIMPLEQ_INSERT_TAIL(&buffers, buf, entries);

	size_t len = 0;
	SIMPLEQ_FOREACH(buf, &buffers, entries) {
		len += buf->size;
	}

	char *xml = malloc(len + 1);
	xml[0] = '\0';
	while (! SIMPLEQ_EMPTY(&buffers)) {
		buf = SIMPLEQ_FIRST(&buffers);
		strlcat(xml, buf->data, len + 1);
		SIMPLEQ_REMOVE_HEAD(&buffers, entries);
	}

	return xml;
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
format_url(char *_hostname, char *_lang, char *_page, bool ssl)
{
	char *http = (ssl) ? "https" : "http";
	char *url;
	if (asprintf(&url, "%s://%s/%s_%s.html", http, _hostname, _page,
				_lang) == -1)
		err(1, NULL);
	return url;
}
