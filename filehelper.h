#ifndef _FILEHELPER_H_
#define _FILEHELPER_H_

#include <sys/queue.h>
#include <sys/stat.h>
#include <sys/types.h>

struct dir_entry {
	TAILQ_ENTRY(dir_entry)	 entries;
	struct stat		 sb;
	u_int8_t		 type;
	char			*filename;
};

struct dir_list {
	TAILQ_HEAD(, dir_entry)	entries;
	time_t			newest;
};

struct dir_entry	*dir_entry_new(const char *_filename);
void	 		 dir_entry_free(struct dir_entry *);
struct dir_list		*get_dir_entries(const char *_directory);
void			 dir_list_free(struct dir_list *);

#endif // _FILEHELPER_H_
