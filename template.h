#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

#include <sys/queue.h>
#include <stdbool.h>

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
void			 tmpl_data_free(struct tmpl_data *);
struct tmpl_data	*tmpl_data_new(void);
struct tmpl_var		*tmpl_data_get_variable(struct tmpl_data *,
				const char *);
void			 tmpl_data_set_variable(struct tmpl_data *,
				const char *, const char *);
struct tmpl_loop	*tmpl_data_get_loop(struct tmpl_data *, const char *);
struct tmpl_loop	*tmpl_data_add_loop(struct tmpl_data *, const char *);
void			 tmpl_loop_free(struct tmpl_loop *);
struct tmpl_loop	*tmpl_loop_new(const char *);
bool			 tmpl_loop_isempty(struct tmpl_loop *);
void			 tmpl_loop_add_data(struct tmpl_loop *,
				struct tmpl_data *);

#endif // _TEMPLATE_H_
