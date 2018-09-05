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

#include <sys/types.h>
#include <err.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include "filehelper.h"
#include "sitemap.h"

#ifndef CMS_CONTENT_DIR
#error "Need CMS_CONTENT_DIR defined to compile"
#endif
#ifndef CMS_HOSTNAME
#error "Need CMS_HOSTNAME defined to compile"
#endif

enum page {
	PAGE_SITEMAP,
	PAGE_SITEMAP_GZ,
	PAGE__MAX
};


const char *const pages[PAGE__MAX] = {
	"/sitemap.xml",    /* PAGE_SITEMAP */
	"/sitemap.xml.gz", /* PAGE_SITEMAP_GZ */
};

static __dead void	usage(void);

__dead void
usage(void)
{
	extern char *__progname;

	dprintf(STDERR_FILENO, "usage: %s CMS_ROOT\n", __progname);
	exit(1);
}


int
main(int argc, char **argv)
{
	size_t size;
	char *out;
	size_t page = PAGE__MAX;
	int fd = STDOUT_FILENO;
	char *cms_root = CMS_CONTENT_DIR;

	if (argc > 2)
		usage();
	if (argc == 2) {
		if (argv[1][0] == '\0')
			errx(1, "Require abolute path as argument");
		cms_root = argv[1];
	}

	char *path_info = getenv("PATH_INFO");
	if (path_info == NULL || strlen(path_info) == 0) {
		page = 0;
	} else {
		for (page = 0; page < PAGE__MAX; ++page)
			if (strcmp(pages[page], path_info) == 0)
				break;
	}

	struct sitemap *sitemap = sitemap_new(cms_root, CMS_HOSTNAME);
	uint32_t mtime = sitemap_newest(sitemap, NULL);

	switch (page) {
	default:
	case PAGE_SITEMAP:
		out = sitemap_toxml(sitemap);
		size = strlen(out);
		break;
	case PAGE_SITEMAP_GZ:
		out = sitemap_toxmlgz(sitemap, &size, "sitemap.xml", mtime);
		break;
	}

	dprintf(fd, "Status: 200 OK\r\n");
	dprintf(fd, "Content-length: %zu\r\n", size);
	dprintf(fd, "Content-type: application/xml\r\n");
	if (page == PAGE_SITEMAP_GZ)
		dprintf(fd, "Content-encoding: x-gzip\r\n");
	dprintf(fd, "\r\n");
	write(fd, out, size);

	free(out);
	return EXIT_SUCCESS;
}
