#ifndef _SITEMAP_H_
#define _SITEMAP_H_

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

struct lang_entry	*lang_entry_new(char *);
struct url_entry	*url_entry_new(char *, time_t);
void			 lang_entry_free(struct lang_entry *);
void			 url_entry_free(struct url_entry *);
struct sitemap		*sitemap_new(char *_content_dir, char *_hostname);
void			 sitemap_free(struct sitemap *);
char			*sitemap_toxml(struct sitemap *);


#endif // _SITEMAP_H_
