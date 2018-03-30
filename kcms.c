
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "filehelper.h"
#include "sitemap.h"

#ifndef CMS_CONTENT_DIR
#error "Need CMS_CONTENT_DIR defined to compile"
#endif
#ifndef CMS_HOSTNAME
#error "Need CMS_HOSTNAME defined to compile"
#endif


int
main(int argc, char **argv)
{
	struct sitemap *sitemap = sitemap_new(CMS_CONTENT_DIR, CMS_HOSTNAME);
#if 0
	printf("%s\n", sitemap_toxml(sitemap));
#else
	uint32_t mtime = sitemap_newest(sitemap, NULL);
	size_t size;
	char *out = sitemap_toxmlgz(sitemap, &size, "sitemap.xml", mtime);

	char *n = out;
	char *last = out + size;
	for (; n < last ; n++)
		printf("%c", *n);

	free(out);
#endif
	return 0;
}
