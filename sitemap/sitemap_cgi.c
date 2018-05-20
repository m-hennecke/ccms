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
#include <kcgi.h>

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
	"sitemap", /* PAGE_SITEMAP */
	"sitemap.xml.gz", /* PAGE_SITEMAP_GZ */
};


int
main(void)
{
	size_t size;
	struct kreq r;
	char *out;

	if (KCGI_OK != khttp_parse(&r, NULL, 0, pages, PAGE__MAX, PAGE_SITEMAP))
		return EXIT_FAILURE;

	struct sitemap *sitemap = sitemap_new(CMS_CONTENT_DIR, CMS_HOSTNAME);
	uint32_t mtime = sitemap_newest(sitemap, NULL);

	switch (r.page) {
	case PAGE_SITEMAP:
		out = sitemap_toxml(sitemap);
		size = strlen(out);
		break;
	case PAGE_SITEMAP_GZ:
	default:
		out = sitemap_toxmlgz(sitemap, &size, "sitemap.xml", mtime);
		break;
	}

	khttp_head(&r, kresps[KRESP_STATUS], "%s", khttps[KHTTP_200]);
	khttp_head(&r, kresps[KRESP_CONTENT_TYPE], "%s", "application/xml");
	if (r.page == PAGE_SITEMAP_GZ)
		khttp_head(&r, kresps[KRESP_CONTENT_ENCODING], "%s", "x-gzip");

	khttp_body(&r);
	khttp_write(&r, out, size);
	khttp_free(&r);

	free(out);
	return EXIT_SUCCESS;
}
