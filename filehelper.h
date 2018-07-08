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

#ifndef _FILEHELPER_H_
#define _FILEHELPER_H_

#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdbool.h>

struct dir_entry {
	TAILQ_ENTRY(dir_entry)	 entries;
	struct stat		 sb;
	u_int8_t		 type;
	char			*filename;
};

struct dir_list {
	TAILQ_HEAD(, dir_entry)	 entries;
	time_t			 newest;
	char			*path;
};

struct dir_entry	*dir_entry_new(const char *);
void	 		 dir_entry_free(struct dir_entry *);
bool			 dir_entry_exists(const char *, struct dir_list *);
struct dir_list		*get_dir_entries(const char *);
struct dir_list		*get_dir_entries_fd(int);
struct dir_list		*get_dir_entries_at(int, const char *);
void			 dir_list_free(struct dir_list *);

bool			 file_exists(const char *_filename);
bool			 dir_exists(const char *_dirname);
bool			 dir_exists_at(int, const char *_dirname);

#endif // _FILEHELPER_H_
