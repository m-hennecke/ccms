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

#ifndef __HTPASSWD_H__
#define __HTPASSWD_H__

#include <stdbool.h>


#define MAX_HTPASSWD_ENTRIES	128

struct htpasswd {
	size_t		 filesize;
	char		*htpasswd;
	char		*usernames[MAX_HTPASSWD_ENTRIES];
	char		*hashes[MAX_HTPASSWD_ENTRIES];
};



struct htpasswd	*htpasswd_init(const char *);
struct htpasswd	*htpasswd_init_at(int, const char *);
void		 htpasswd_free(struct htpasswd *);
bool		 htpasswd_check_password(struct htpasswd *, const char *,
		const char *);

#endif // __HTPASSWD_H__
