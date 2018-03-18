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

struct dir_entry	*dir_entry_new(const char *_filename);
void	 		 dir_entry_free(struct dir_entry *);
bool			 dir_entry_exists(const char *, struct dir_list *);
struct dir_list		*get_dir_entries(const char *_directory);
void			 dir_list_free(struct dir_list *);

bool			 file_exists(const char *_filename);

#endif // _FILEHELPER_H_
