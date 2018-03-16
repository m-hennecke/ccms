
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "filehelper.h"

struct dir_list *
get_dir_entries(const char *_directory)
{
	DIR *dir = opendir(_directory);
	if (dir) {
		struct dir_list *list = malloc(sizeof(struct dir_list));
		if (list == NULL)
			err(1, NULL);
		TAILQ_INIT(&list->entries);
		list->newest = 0;
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
	if (stat(_filename, &entry->sb) != 0)
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


void
dir_list_free(struct dir_list *_list)
{
	struct dir_entry *entry;
	while ((entry = TAILQ_FIRST(&_list->entries))) {
		TAILQ_REMOVE(&_list->entries, entry, entries);
		dir_entry_free(entry);
	}
}
