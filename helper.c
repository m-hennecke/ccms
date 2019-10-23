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
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "helper.h"

static struct memmap	*_map_fd(int);
static char		 decode_hexdigit(const char *);
static bool		 ishexdigit(const int);



const char *
rx_get_errormsg(int _rc, regex_t *_rx)
{
	static char errmsg[512];
	regerror(_rc, _rx, errmsg, sizeof(errmsg));
	return errmsg;
}


struct memmap *
_map_fd(int _fd)
{
	struct stat sb;
	if (-1 == fstat(_fd, &sb))
		err(1, NULL);

	struct memmap *mm = malloc(sizeof(struct memmap));
	if (NULL == mm)
		err(1, NULL);
	mm->size = sb.st_size;

	mm->data = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, _fd, 0);
	close(_fd);
	return mm;
}


struct memmap *
memmap_new(const char *_filename)
{
	int fd = open(_filename, O_RDONLY);
	if (-1 == fd) {
		warn(NULL);
		return NULL;
	}

	return _map_fd(fd);
}


struct memmap *
memmap_new_at(int _fd, const char *_filename)
{
	int fd = openat(_fd, _filename, O_RDONLY);
	if (-1 == fd) {
		warn(NULL);
		return NULL;
	}

	return _map_fd(fd);
}


void
memmap_free(struct memmap *_map)
{
	if (_map && _map->data)
		munmap(_map->data, _map->size);
	free(_map);
}


size_t
memmap_chomp(struct memmap *_m)
{
	size_t s = _m->size;
	if (_m && _m->data) {
		while ((s > 0) && isspace(((char *)_m->data)[s-1]))
			s--;
	}
	return s;
}


bool
ishexdigit(const int _c)
{
	int c = tolower(_c);
	return (isdigit(c) || ((c >= 'a') && (c <= 'f')));
}


char
decode_hexdigit(const char *_s)
{
	_s++;
	int highnibble = tolower(*_s++);
	if (isdigit(highnibble))
		highnibble -= '0';
	else
		highnibble -= 'a';
	int lownibble = tolower(*_s);
	if (isdigit(lownibble))
		lownibble -= '0';
	else
		lownibble -= 'a';
	return ((highnibble << 4) | (lownibble));
}


void
decode_string(char *_s)
{
	if (_s == NULL)
		return;

	char *s = _s;
	char *d = s;
	while (*s != '\0') {
		if (*s == '+') {
			*d = ' ';
		} else {
			if (*s == '%' && s[1] != '\0' && ishexdigit(s[1])
					&& s[2] != '\0' && ishexdigit(s[2])) {
				*d = decode_hexdigit(s);
				s += 2;
			} else {
				*d = *s;
			}
		}
		s++;
		d++;
	}
	*d = *s;
}
