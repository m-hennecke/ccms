
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "filehelper.h"
#include "sitemap.h"

int
main(int argc, char **argv)
{
	struct sitemap *sitemap = sitemap_new("/var/www/cms/content",
			"www.markus-hennecke.de");
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
