
#include <err.h>
#include <stdio.h>
#include "filehelper.h"
#include "sitemap.h"

void sitemap_handler(const char *content_dir);

int
main(int argc, char **argv)
{
	struct sitemap *sitemap = sitemap_new("/var/www/cms/content",
			"www.markus-hennecke.de");
	printf("%s\n", sitemap_toxml(sitemap));
	return 0;
}
