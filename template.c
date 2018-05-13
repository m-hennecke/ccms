#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "template.h"


void
tmpl_name_free(struct tmpl_name *_entry)
{
	free(_entry->name);
}


void
tmpl_var_free(struct tmpl_var *_var)
{
	tmpl_name_free(&_var->name);
	free(_var->value);
	free(_var);
}



struct tmpl_var *
tmpl_var_new(const char *_name)
{
	struct tmpl_var *var = malloc(sizeof(struct tmpl_var));
	if (NULL == var)
		err(1, NULL);
	var->name.name = strdup(_name);
	var->value = NULL;

	return var;
}


void
tmpl_var_set(struct tmpl_var *_var, const char *_value)
{
	free(_var->value);
	_var->value = strdup(_value);
}


struct tmpl_data *
tmpl_data_new(void)
{
	struct tmpl_data *data;

	data = malloc(sizeof(struct tmpl_data));
	if (NULL == data)
		err(1, NULL);
	TAILQ_INIT(&data->variables);
	TAILQ_INIT(&data->loops);
	return data;
}


void
tmpl_data_free(struct tmpl_data *_data)
{
	struct tmpl_var *var;
	struct tmpl_loop *loop;

	while ((var = TAILQ_FIRST(&_data->variables))) {
		TAILQ_REMOVE(&_data->variables, var, entry);
		tmpl_var_free(var);
	}
	while ((loop = TAILQ_FIRST(&_data->loops))) {
		TAILQ_REMOVE(&_data->loops, loop, entry);
		tmpl_var_free(var);
	}
	free(_data);
}


struct tmpl_var *
tmpl_data_get_variable(struct tmpl_data *_data, const char *_name)
{
	struct tmpl_var *var;
	TAILQ_FOREACH(var, &_data->variables, entry) {
		if (strcmp(_name, var->name.name))
			return var;
	}
	return NULL;
}


void
tmpl_data_set_variable(struct tmpl_data *_data, const char *_name,
		const char *_value)
{
	struct tmpl_var *var = tmpl_data_get_variable(_data, _name);
	if (var == NULL) {
		var = tmpl_var_new(_name);
		TAILQ_INSERT_TAIL(&_data->variables, var, entry);
	}
	tmpl_var_set(var, _value);
}


struct tmpl_loop *
tmpl_data_get_loop(struct tmpl_data *_data, const char *_name)
{
	struct tmpl_loop *loop;
	TAILQ_FOREACH(loop, &_data->loops, entry) {
		if (strcmp(_name, loop->name.name))
			return loop;
	}
	return NULL;
}


struct tmpl_loop *
tmpl_data_add_loop(struct tmpl_data *_data, const char *_name)
{
	struct tmpl_loop *loop = tmpl_data_get_loop(_data, _name);
	if (loop)
		return loop;
	loop = tmpl_loop_new(_name);
	TAILQ_INSERT_TAIL(&_data->loops, loop, entry);
	return loop;
}


void
tmpl_loop_free(struct tmpl_loop *_loop)
{
	struct tmpl_data *data;
	while ((data = TAILQ_FIRST(&_loop->data))) {
		TAILQ_REMOVE(&_loop->data, data, entry);
		tmpl_data_free(data);
	}
	tmpl_name_free(&_loop->name);
	free(_loop);
}


struct tmpl_loop *
tmpl_loop_new(const char *_name)
{
	struct tmpl_loop *loop = malloc(sizeof(struct tmpl_loop));
	if (NULL == loop)
		err(1, NULL);

	loop->name.name = strdup(_name);
	TAILQ_INIT(&loop->data);
	return loop;
}

