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

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <err.h>
#include <fcntl.h>
#include <regex.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "buffer.h"
#include "helper.h"
#include "template.h"


typedef enum {
	ELSE = 0,
	IF, INCL, INCLUDE, LOOP, UNLESS, VAR,
	MAX__TAG
} tag_type_t;

struct tag_info {
	TAILQ_ENTRY(tag_info)	entry;
	tag_type_t		 type;
	char			*start;		// Start of the tag
	char			*end;		// End of the tag
	struct tag_info		*else_tag;	// ELSE from an IF block
	struct tag_info		*closing_tag;	// Pointer to a closing tag
	bool			 close;		// true if itself is close tag
	char			*name;		// The name attribute
};


struct parser_state {
	char				*input;
	size_t				 size;
	struct buffer_list		*output;
	struct tmpl_data		*data;
	TAILQ_HEAD(tag_head, tag_info)	 tags;
};


struct tag_strings {
	tag_type_t	 type;
	const char	*id;
	size_t		 len;
	char		*(*handle_func)(struct parser_state *,
				struct tag_info *);
};

#define TMPL_RX_PATTERN "(<(TMPL_"					\
	"(ELSE|"							\
	"(IF|INCL|INCLUDE|LOOP|VAR|UNLESS)( +name=\"([^\"]+)\"))?)"	\
	" *>)|(</TMPL_(IF|LOOP|VAR|UNLESS) *>)"
#define TMPL_RX_MAX_GROUPS 10
static regex_t _rx;
static bool _rx_initialized = false;
static const size_t _rx_max_groups = TMPL_RX_MAX_GROUPS;
static regmatch_t _rx_group_array[TMPL_RX_MAX_GROUPS];




static struct parser_state	*parser_state_new(const char *, size_t,
		struct buffer_list *);
static void			 parser_state_free(struct parser_state *);
static regmatch_t		*parser_find_tmpl_tag(char *);
static void			 parser_init(void);
static void			 parser_cleanup(void);
static int			 parser_enumerate_tags(struct parser_state *);
static struct tag_info		*parser_find_close_tag(struct tag_info *);
static struct tag_info		*tag_info_new(char *, regmatch_t *);
static void			 tag_info_free(struct tag_info *);

// Tag handler functions:
static char	*tmpl_handle_else(struct parser_state *, struct tag_info *);
static char	*tmpl_handle_if(struct parser_state *, struct tag_info *);
static char	*tmpl_handle_incl(struct parser_state *, struct tag_info *);
static char	*tmpl_handle_loop(struct parser_state *, struct tag_info *);
static char	*tmpl_handle_var(struct parser_state *, struct tag_info *);



static struct tag_strings tags[MAX__TAG] = {
	{ ELSE,    "TMPL_ELSE",     9, tmpl_handle_else },
	{ IF,      "TMPL_IF",       7, tmpl_handle_if   },
	{ INCL,    "TMPL_INCL",     9, tmpl_handle_incl },
	{ INCLUDE, "TMPL_INCLUDE", 12, tmpl_handle_incl },
	{ LOOP,    "TMPL_LOOP",     9, tmpl_handle_loop },
	{ UNLESS,  "TMPL_UNLESS",  11, tmpl_handle_if   },
	{ VAR,     "TMPL_VAR",      8, tmpl_handle_var  },
};


void
tag_info_free(struct tag_info *_info)
{
	if (_info) {
		free(_info->name);
		free(_info);
	}
}


struct tag_info *
tag_info_new(char *_s, regmatch_t *_tag)
{
	regmatch_t *name_attr = &_tag[6]; // 6th group holds the attr value
	size_t name_attr_val_len = name_attr->rm_eo - name_attr->rm_so;
	char *name_attr_val = (name_attr_val_len > 0)
		? _s + name_attr->rm_so
		: NULL;
	struct tag_info *info = NULL;
	char *s = _s + _tag->rm_so;
	if (*s++ != '<')
		return NULL;

	bool is_close = *s == '/';
	if (is_close)
		s++;

	for (int i = 0; i < MAX__TAG; ++i) {
		int r = strncmp(s, tags[i].id, tags[i].len);
		if (r < 0) {
			break;
		} else if (r == 0) {
			info = malloc(sizeof(struct tag_info));
			if (NULL == info)
				err(1, NULL);
			info->type = tags[i].type;
			info->start = _s + _tag->rm_so;
			info->end   = _s + _tag->rm_eo;
			info->else_tag = NULL;
			info->closing_tag = NULL;
			info->close = is_close;
			if (name_attr_val)
				info->name = strndup(name_attr_val,
						name_attr_val_len);
			else
				info->name = NULL;
			break;
		}
	}

#if defined(DEBUG)
	if (info && info->name)
		dprintf(STDERR_FILENO, "%s has name = %s\n",
				tags[info->type].id, info->name);
#endif

	return info;
}



void
parser_init(void)
{
	if (_rx_initialized)
		return;
	int rc = regcomp(&_rx, TMPL_RX_PATTERN, REG_EXTENDED | REG_ICASE);
	if (rc != 0)
		errx(1, "regcomp: %s", rx_get_errormsg(rc, &_rx));
	_rx_initialized = true;
}


void
parser_cleanup(void)
{
	if (_rx_initialized) {
		regfree(&_rx);
		_rx_initialized = false;
	}
}


struct tag_info *
parser_find_close_tag(struct tag_info *_open_tag)
{
	int num_opened = 1;
	struct tag_info *next = _open_tag;
	while ((num_opened > 0) && (next = TAILQ_NEXT(next, entry))) {
		if (next->type == _open_tag->type) {
			if (next->close) {
				--num_opened;
			} else {
				num_opened++;
			}
		}
		if ((_open_tag->type == IF) && (num_opened == 1)
				&& (next->type == ELSE)) {
			_open_tag->else_tag = next;
		}
	}
	if (next)
		_open_tag->closing_tag = next;
	return next;
}


struct parser_state *
parser_state_new(const char *_input, size_t _size, struct buffer_list *_out)
{
	struct parser_state *state = malloc(sizeof(struct parser_state));
	if (NULL == state)
		err(1, NULL);
	state->input  = malloc(_size + 1);
	if (NULL == state->input)
		err(1, NULL);
	memcpy(state->input, _input, _size);
	state->input[_size] = '\0';
	state->size   = _size;
	state->output = _out;
	state->data   = NULL;
	TAILQ_INIT(&state->tags);

	return state;
}


void
parser_state_free(struct parser_state *_state)
{
	if (_state) {
		struct tag_info *info;
		while ((info = TAILQ_FIRST(&_state->tags))) {
			TAILQ_REMOVE(&_state->tags, info, entry);
			tag_info_free(info);
		}
		free(_state->input);
		free(_state);
	}
}


/*
 * Searches for one of the <TMPL_xxxx> tags and returns the position of that
 * tag if found.
 * If no such tag is found NULL is returned.
 */
regmatch_t *
parser_find_tmpl_tag(char *_start)
{
	if ('\0' != *_start) {
		int rc = regexec(&_rx, _start, _rx_max_groups,
				&_rx_group_array[0], 0);
		switch (rc) {
		case REG_NOMATCH:
			return NULL;
		case 0:
			return _rx_group_array;
			break;
		default:
			warnx("regexec: %s", rx_get_errormsg(rc, &_rx));
			break;
		}
	}
	return NULL;
}


int
parser_enumerate_tags(struct parser_state *_state)
{
	int n = 0;
	char *pos = _state->input;
	regmatch_t *loc;

	while ((loc = parser_find_tmpl_tag(pos) ) != NULL) {
		struct tag_info *info = tag_info_new(pos, loc);
		if (info) {
			TAILQ_INSERT_TAIL(&_state->tags, info, entry);
			n++;
		}
		pos += loc->rm_eo;
	}

	return n;
}


char *
tmpl_handle_else(struct parser_state *_parser, struct tag_info *_info)
{
	// An ELSE tag should never be handled alone, so abort here
	errx(1, "Got ELSE tag without IF or UNLESS, aborting");
}


char *
tmpl_handle_if(struct parser_state *_p, struct tag_info *_info)
{
	bool cond = false;
	struct tmpl_var *var = tmpl_data_get_variable(_p->data, _info->name);
	if (var) {
		if ((var->value) && (strlen(var->value) > 0))
			cond = (strcmp(var->value, "0") != 0);
	} else {
		cond = !tmpl_loop_isempty(
				tmpl_data_get_loop(_p->data, _info->name)
			);
	}
	cond ^= (_info->type == UNLESS);

	char *block_start;
	char *block_end;
	struct tag_info *else_tag = _info->else_tag;
	if (cond) {
		block_start = _info->end;
		if (else_tag)
			block_end = else_tag->start;
		else
			block_end = _info->closing_tag->start;
	} else {
		if (else_tag) {
			block_start = else_tag->end + 1;
			block_end = _info->closing_tag->start;
		} else
			return _info->closing_tag->end;
	}
	struct buffer_list *block = tmpl_parse(block_start,
			block_end - block_start, _p->data);
	buffer_list_add_list(_p->output, block);
	buffer_list_free(block);
	return _info->closing_tag->end;
}


char *
tmpl_handle_incl(struct parser_state *_p, struct tag_info *_info)
{
	struct buffer_list *block = tmpl_parse_file(_info->name, _p->data);
	if (block) {
		buffer_list_add_list(_p->output, block);
		buffer_list_free(block);
	}
	return _info->end;
}


char *
tmpl_handle_loop(struct parser_state *_p, struct tag_info *_info)
{
	struct tmpl_loop *loop = tmpl_data_get_loop(_p->data, _info->name);
	bool cond = !tmpl_loop_isempty(loop);

#if defined(DEBUG)
	dprintf(STDERR_FILENO, "loop %s is %sempty\n", _info->name,
			cond ? "not " : "");
#endif
	struct tag_info *closing_tag = _info->closing_tag;

	if (!cond)
		return closing_tag->end;

	const size_t len = closing_tag->start - _info->end;
	struct tmpl_data *data;
	TAILQ_FOREACH(data, &loop->data, entry) {
		struct buffer_list *block = tmpl_parse(_info->end,
				len + 1, data);
		buffer_list_add_list(_p->output, block);
		buffer_list_free(block);
	}
	return closing_tag->end;
}


char *
tmpl_handle_var(struct parser_state *_parser, struct tag_info *_info)
{
	const struct tmpl_var *var = tmpl_data_get_variable(
			_parser->data, _info->name
		);
	if (var) {
		buffer_list_add_string(_parser->output, var->value);
	}
	return _info->end;
}


struct buffer_list *
tmpl_parse(const char *_tmpl, size_t _len, struct tmpl_data *_data)
{
	struct tag_info *info;

	parser_init();
	struct buffer_list *out = buffer_list_new();
	struct parser_state *state = parser_state_new(_tmpl, _len, out);
	state->data = _data;

	// Get all tags from the current template
	parser_enumerate_tags(state);

	// Try to find all closing tags for tags defining a block and
	// put the information about the closing tag into the tag_info
	// for the opening tag
	TAILQ_FOREACH(info, &state->tags, entry) {
		struct tag_info *closer;
#if defined(DEBUG)
		dprintf(STDERR_FILENO, "%s%s tag at %td\n",
				(info->close) ? "/" : "",
				tags[info->type].id,
				(info->start - state->input));
#endif

		if (info->close)
			continue;
		switch (info->type) {
		case ELSE:
		case VAR:
		case INCL:
		case INCLUDE:
			break;
		default:
			closer = parser_find_close_tag(info);
			if (NULL == closer) {
				errx(1, "template: %s for %s",
						"Unable to find closing tag",
						tags[info->type].id);
			}
			break;
		}

	}

	// Output each block with data and handle all the tags
	char *s = state->input;
	TAILQ_FOREACH(info, &state->tags, entry) {
		if (s > info->start)
			continue;
		if (s < info->start || info->close) {
			// Copy the data block from s to the tags start
			buffer_list_add_stringn(out, s, info->start - s + 1);
			s = info->end;
		}
		s = (*tags[info->type].handle_func)(state, info);
	}
	char *end = state->input + state->size;
	if (s < end)
		buffer_list_add_stringn(out, s, end - s); 

	parser_state_free(state);
	parser_cleanup();
	return out;
}


struct buffer_list *
tmpl_parse_file(const char *_filename, struct tmpl_data *_data)
{
	struct stat sb;
	int fd = open(_filename, O_RDONLY);
	if (-1 == fd) {
		warn("%s", _filename);
		return NULL;
	}
	if (-1 == fstat(fd, &sb)) {
		warn("%s", _filename);
		return NULL;
	}
	if (sb.st_size == 0)
		return NULL;
	void *tmpl = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);
	if (NULL == tmpl)
		err(1, NULL);

	struct buffer_list *out = tmpl_parse((char *)tmpl, sb.st_size, _data);
	munmap(tmpl, sb.st_size);

	return out;
}
