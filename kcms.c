
#include <err.h>
#include <stdio.h>
#include "filehelper.h"

int
main(int argc, char **argv)
{
	struct dir_list *dir = get_dir_entries(".");
	if (! dir)
		errx(1, "Unable to read \".\" directory\n");
	struct dir_entry *entry;
	TAILQ_FOREACH(entry, &dir->entries, entries) {
		printf("%s\n", entry->filename);
	}
	printf("Newest: %lld\n", dir->newest);
	dir_list_free(dir);
	return 0;
}
