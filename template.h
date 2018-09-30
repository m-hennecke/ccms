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

#ifndef __TEMPLATE_H__
#define __TEMPLATE_H__

#include <sys/queue.h>
#include <stdbool.h>

#include "buffer.h"

struct tmpl_name {
	char					*name;
};

struct tmpl_var {
	TAILQ_ENTRY(tmpl_var)			 entry;
	struct tmpl_name			 name;
	char					*value;
};

struct tmpl_data;
struct tmpl_loop {
	TAILQ_ENTRY(tmpl_loop)			 entry;
	struct tmpl_name			 name;
	TAILQ_HEAD(tmpl_data_array, tmpl_data)	 data;
};

struct tmpl_data {
	TAILQ_HEAD(tmpl_vars, tmpl_var)		 variables;
	TAILQ_HEAD(tmpl_loops, tmpl_loop)	 loops;
	TAILQ_ENTRY(tmpl_data)			 entry;
};

void			 tmpl_name_free(struct tmpl_name *);
void			 tmpl_var_free(struct tmpl_var *);
struct tmpl_var		*tmpl_var_new(const char *);
void			 tmpl_var_set(struct tmpl_var *, const char *);
void			 tmpl_var_setn(struct tmpl_var *, const char *, size_t);
void			 tmpl_data_free(struct tmpl_data *);
struct tmpl_data	*tmpl_data_new(void);
struct tmpl_var		*tmpl_data_get_variable(struct tmpl_data *,
				const char *);
void			 tmpl_data_set_variable(struct tmpl_data *,
				const char *, const char *);
void			 tmpl_data_set_variablen(struct tmpl_data *,
				const char *, const char *, size_t);
void			 tmpl_data_move_variable(struct tmpl_data *,
				const char *, char *);
struct tmpl_loop	*tmpl_data_get_loop(struct tmpl_data *, const char *);
struct tmpl_loop	*tmpl_data_add_loop(struct tmpl_data *, const char *);
void			 tmpl_data_set_loop(struct tmpl_data *, const char *,
				struct tmpl_loop *);
void			 tmpl_loop_free(struct tmpl_loop *);
struct tmpl_loop	*tmpl_loop_new(const char *);
bool			 tmpl_loop_isempty(struct tmpl_loop *);
void			 tmpl_loop_add_data(struct tmpl_loop *,
				struct tmpl_data *);

struct buffer_list	*tmpl_parse(const char *, size_t _len,
				struct tmpl_data *);
struct buffer_list	*tmpl_parse_file(const char *, struct tmpl_data *);

#endif // __TEMPLATE_H__
