#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <sys/queue.h>

struct buffer {
	TAILQ_ENTRY(buffer)	entries;
	size_t			size;
	char			data[1];
};

struct buffer_list {
	TAILQ_HEAD(buffer_list_head, buffer)	buffers;
	size_t					size;
};

struct buffer		*buffer_new(char *_data);
struct buffer		*buffer_bin_new(void *, size_t);
ssize_t			 buffer_write(struct buffer *, int);
char			*buffer_list_concat_string(struct buffer_list *);
struct buffer_list	*buffer_list_new(void);
void			 buffer_list_free(struct buffer_list *);
void			 buffer_list_add(struct buffer_list *, void *, size_t);
void			 buffer_list_add_string(struct buffer_list *, char *);
struct buffer		*buffer_list_rem_head(struct buffer_list *);
struct buffer		*buffer_list_rem_tail(struct buffer_list *);
struct buffer		*buffer_list_rem(struct buffer_list *, struct buffer *);



#endif // _BUFFER_H_
