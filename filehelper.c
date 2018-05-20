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

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "filehelper.h"

struct dir_list *
get_dir_entries(const char *_directory)
{
	DIR *dir = opendir(_directory);
	if (dir) {
		struct dir_list *list = malloc(sizeof(struct dir_list));
		if (list == NULL)
			err(1, NULL);
		int cwd = open(".", O_DIRECTORY | O_RDONLY);
		if (cwd == -1)
			err(1, NULL);
		if (chdir(_directory) == -1)
			err(1, NULL);
		TAILQ_INIT(&list->entries);
		list->newest = 0;
		list->path = strdup(_directory);
		struct dirent *dirent;
		while ((dirent = readdir(dir)) != NULL) {
			struct dir_entry *entry;

			if (dirent->d_name[0] == '.')
				continue;

			entry = dir_entry_new(dirent->d_name);
			TAILQ_INSERT_TAIL(&list->entries, entry, entries);
			if (entry->sb.st_mtim.tv_sec > list->newest)
				list->newest = entry->sb.st_mtim.tv_sec;
		}
		closedir(dir);
		fchdir(cwd);
		return list;
	}
	return NULL;
}


struct dir_entry *
dir_entry_new(const char *_filename)
{
	struct dir_entry *entry = calloc(1,
			sizeof(struct dir_entry));
	if (entry == NULL)
		err(1, NULL);
	entry->filename = strdup(_filename);
	if (lstat(_filename, &entry->sb) != 0)
		err(1, "stat error on file %s", _filename);
	return entry;
}


void
dir_entry_free(struct dir_entry *_entry)
{
	if (_entry) {
		free(_entry->filename);
		free(_entry);
	}
}


bool
dir_entry_exists(const char *_name, struct dir_list *_dir)
{
	struct dir_entry *entry;
	TAILQ_FOREACH(entry, &_dir->entries, entries) {
		if (strcmp(entry->filename, _name) == 0)
			return true;
	}
	return false;
}


void
dir_list_free(struct dir_list *_list)
{
	struct dir_entry *entry;
	while ((entry = TAILQ_FIRST(&_list->entries))) {
		TAILQ_REMOVE(&_list->entries, entry, entries);
		dir_entry_free(entry);
	}
	free(_list->path);
}


bool
file_exists(const char *_filename)
{
	struct stat sb;
	if (lstat(_filename, &sb) == -1)
		return false;
	return true;
}
