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

#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "filehelper.h"
#include "handler.h"
#include "helper.h"
#include "template.h"

struct _link {
	TAILQ_ENTRY(_link)	 entries;
	struct memmap		*link;
	struct memmap		*descr;
	struct memmap		*sort;
	bool			 sub;
	bool			 ssl;
	char			*linkname;
	bool			 selected;
};

struct _link_list {
	TAILQ_HEAD(, _link)	 links;
};

static void			 _link_free(struct _link *);
static struct _link		*_link_new_at(int, char *);
static struct tmpl_data		*_link_get_tmpl_data(struct _link *,
		struct request *, bool);
static int			 _link_cmp(struct _link *, struct _link *);
static void			 _link_list_insert_link(struct _link_list *,
		struct _link *);
static void			 _link_list_free(struct _link_list *);
static struct _link_list	*_link_list_new_at(int, char *);
static void			 _link_list_remove_unselected_subs(
		struct _link_list *_lst);

int
_link_cmp(struct _link *_a, struct _link *_b)
{
	size_t cmplen = (_a->sort->size <= _b->sort->size)
		? _a->sort->size : _b->sort->size;
	int result = strncmp((char *)_a->sort->data, (char *)_b->sort->data,
			cmplen);

	return result;
}


void
_link_list_insert_link(struct _link_list *_lst, struct _link *_link)
{
	struct _link *l = TAILQ_FIRST(&_lst->links);
	while (l) {
		int cmp = _link_cmp(_link, l);
		if (cmp > 0) {
			l = TAILQ_NEXT(l, entries);
		} else  {
			TAILQ_INSERT_BEFORE(l, _link, entries);
			return;
		}
	}
	TAILQ_INSERT_TAIL(&_lst->links, _link, entries);
}


void
_link_list_free(struct _link_list *_lst)
{
	struct _link *l;
	while ((l = TAILQ_FIRST(&_lst->links))) {
		TAILQ_REMOVE(&_lst->links, l, entries);
		_link_free(l);
	}
	free(_lst);
}


struct _link_list *
_link_list_new_at(int _fd, char *_cur_page)
{
	struct dirent *dirent;
	struct _link_list *lst = malloc(sizeof(struct _link_list));
	TAILQ_INIT(&lst->links);

	int dirfd = dup(_fd);
	if (dirfd == -1)
		err(1, NULL);
	DIR *dir = fdopendir(dirfd);
	if (dir) {
		while ((dirent = readdir(dir)) != NULL) {
			if (dirent->d_name[0] == '.')
				continue;
			struct _link *l = _link_new_at(_fd, dirent->d_name);
			if (l != NULL) {
				l->selected = (strcmp(l->linkname,
							_cur_page) == 0);
				_link_list_insert_link(lst, l);
			}
		}
		closedir(dir);
	} else {
		free(lst);
		lst = NULL;
	}

	return lst;
}


struct _link *
_link_new_at(int _fd, char *_dirname)
{
	struct _link *link = calloc(1, sizeof(struct _link));
	int dirfd = openat(_fd, _dirname, O_RDONLY | O_DIRECTORY);
	if (-1 == dirfd)
		err(1, NULL);

	// Try to mmap the files LINK and SORT
	link->link = memmap_new_at(dirfd, "LINK");
	if (link->link == NULL)
		goto bailout;
	link->descr = memmap_new_at(dirfd, "DESCR");
	if (link->descr == NULL)
		goto bailout;
	link->sort = memmap_new_at(dirfd, "SORT");
	if (link->sort == NULL)
		goto bailout;

	struct stat sb;
	if (fstatat(dirfd, "SUB", &sb, 0) != -1)
		link->sub = ((sb.st_mode & S_IFREG) != 0);
	if (fstatat(dirfd, "SSL", &sb, 0) != -1)
		link->ssl = ((sb.st_mode & S_IFREG) != 0);
	link->linkname = strdup(_dirname);

	close(dirfd);
	return link;

bailout:
	_link_free(link);
	close(dirfd);
	return NULL;
}


void
_link_free(struct _link *_link)
{
	memmap_free(_link->link);
	memmap_free(_link->descr);
	memmap_free(_link->sort);
	free(_link->linkname);
	free(_link);
}


void
_link_list_remove_unselected_subs(struct _link_list *_lst)
{
	struct _link *topic = TAILQ_FIRST(&_lst->links);
	struct _link *next;
	bool selected = false;

	while (topic) {
		selected = topic->selected;
		next = TAILQ_NEXT(topic, entries);
		if (next && next->sub) {
			struct _link *sub_start = next;
			while (next && next->sub) {
				selected |= next->selected;
				next = TAILQ_NEXT(next, entries);
			}
			if (!selected) {
				// Remove all SUB entries from sub_start to next
				struct _link *rem = sub_start;
				do {
					struct _link *next_rem 
						= TAILQ_NEXT(rem, entries);
					TAILQ_REMOVE(&_lst->links, rem,
							entries);
					_link_free(rem);

					rem = next_rem;
				} while (rem != next);
			}
		}
		topic = next;
	}
}


struct tmpl_data *
_link_get_tmpl_data(struct _link *_l, struct request *_req, bool _js)
{
	char *linkdata = strndup(_l->link->data, memmap_chomp(_l->link));
	char *aref, *jslink, *link;
	if ((asprintf(&link, "%s%s/%s.html", _req->path, _req->lang,
					_l->linkname) == -1))
		err(1, NULL);
	if ((asprintf(&aref, "<a href=\"%s%s\">%s</a>",
					(_l->ssl || _req->page_info->ssl)
					? "https://" CMS_HOSTNAME
					: "",
					link, linkdata) == -1))
		err(1, NULL);
	struct tmpl_data *data =  tmpl_data_new();
	tmpl_data_set_variablen(data, "NR", _l->sort->data,
			memmap_chomp(_l->sort));
	tmpl_data_set_variablen(data, "DESCR", _l->descr->data,
			_l->descr->size);
	tmpl_data_move_variable(data, "LINK", aref);
	if (_js) {
		if ((asprintf(&jslink, "onclick=\"javascript:location.replace"
						"('%s')\"", link) == -1))
			err(1, NULL);
		tmpl_data_move_variable(data, "JSLINK", jslink);
	}
	if (_l->sub)
		tmpl_data_set_variable(data, "SUB", "1");
	if (_l->selected)
		tmpl_data_set_variable(data, "SELECTED", "1");

	free(linkdata);
	free(link);

	return data;
}


struct tmpl_loop *
request_get_links(struct request *_req)
{
	struct _link_list *lst = _link_list_new_at(_req->lang_dir, _req->page);
	if (lst == NULL)
		return NULL;

	_link_list_remove_unselected_subs(lst);

	struct tmpl_loop *loop = tmpl_loop_new("LINK_LOOP");
	struct _link *l;
	TAILQ_FOREACH(l, &lst->links, entries) {
		struct tmpl_data *data = _link_get_tmpl_data(l, _req, true);
		tmpl_loop_add_data(loop, data);
	}

	// TODO: add logout if we have a valid login session

	_link_list_free(lst);
	return loop;
}


struct tmpl_loop *
request_get_language_links(struct request *_req)
{
	struct tmpl_loop *loop = tmpl_loop_new("LANGUAGE_LINKS");
	int contentfd = dup(_req->content_dir);
	DIR *dir = fdopendir(contentfd);
	if (dir) {
		struct dirent *dirent;
		while ((dirent = readdir(dir))) {
			if (dirent->d_name[0] == '.')
				continue;
			if (strcmp(dirent->d_name, _req->page) == 0)
				continue;
			int fd = openat(_req->content_dir, dirent->d_name,
					O_DIRECTORY | O_RDONLY);
			if (-1 == fd) {
				warn(NULL);
				continue;
			}
			if (dir_exists_at(fd, _req->page)) {
				char *lang_link;
				if ((asprintf(&lang_link,
						"<a href=\"%s%s/%s.html\">"
						"<img src=\""
						CMS_CONFIG_URL_IMAGES
						"flag_%s.png\" alt=\"%s\"/>"
						"</a>",
						_req->path, dirent->d_name,
						_req->page, dirent->d_name,
						dirent->d_name) == -1))
					err(1, NULL);
				struct tmpl_data *d = tmpl_data_new();
				tmpl_data_move_variable(d, "LANGUAGE_LINK",
						lang_link);
				tmpl_loop_add_data(loop, d);
			}

			close(fd);
		}
	}
	return loop;
}
