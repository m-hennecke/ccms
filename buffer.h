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

struct buffer		*buffer_new(const char *_data);
struct buffer		*buffer_bin_new(const void *, size_t);
struct buffer		*buffer_empty_new(size_t);
ssize_t			 buffer_write(struct buffer *, int);
char			*buffer_list_concat_string(struct buffer_list *);
char			*buffer_list_concat(struct buffer_list *);
struct buffer_list	*buffer_list_new(void);
void			 buffer_list_free(struct buffer_list *);
void			 buffer_list_add(struct buffer_list *, const void *,
		size_t);
void			 buffer_list_add_string(struct buffer_list *,
		const char *);
void			 buffer_list_add_stringn(struct buffer_list *,
		const char *, size_t);
void			 buffer_list_add_buffer(struct buffer_list *,
		struct buffer *);
void			buffer_list_add_list(struct buffer_list *,
		struct buffer_list *);
struct buffer		*buffer_list_rem_head(struct buffer_list *);
struct buffer		*buffer_list_rem_tail(struct buffer_list *);
struct buffer		*buffer_list_rem(struct buffer_list *, struct buffer *);
struct buffer_list	*buffer_list_gzip(struct buffer_list *,
		const char *_filename, uint32_t _mtime);



#endif // _BUFFER_H_
