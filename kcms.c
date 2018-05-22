
#include <sys/mman.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "buffer.h"
#include "filehelper.h"
#include "template.h"

#ifndef CMS_CONTENT_DIR
#error "Need CMS_CONTENT_DIR defined to compile"
#endif
#ifndef CMS_HOSTNAME
#error "Need CMS_HOSTNAME defined to compile"
#endif

static __dead void usage(void);

__dead void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s -f file\n", __progname);
	exit(1);
}


int
main(int argc, char **argv)
{
	int ch, fd = -1;

	while ((ch = getopt(argc, argv, "f:h")) != -1) {
		switch (ch) {
		case 'f':
			if ((fd = open(optarg, O_RDONLY, 0)) == -1)
				err(1, "%s", optarg);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	struct tmpl_data *data = tmpl_data_new();

	if (-1 == fd)
		usage();

	struct stat sb;
	if (-1 == fstat(fd, &sb))
		err(1, NULL);
	void *tmpl = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (NULL == tmpl)
		err(1, NULL);

	tmpl_data_set_variable(data, "LANGUAGE", "de");
	tmpl_data_set_variable(data, "TITLE", "Template-Test");
	struct tmpl_loop *language_links = tmpl_data_add_loop(
			data, "LANGUAGE_LINKS"
		);

	struct tmpl_data *d = tmpl_data_new();
	tmpl_data_set_variable(d, "LANGUAGE_LINK",
			"<a href=\"de-page.html\">de</a>");
	tmpl_loop_add_data(language_links, d);
	d = tmpl_data_new();
	tmpl_data_set_variable(d, "LANGUAGE_LINK",
			"<a href=\"en-page.html\">en</a>");
	tmpl_loop_add_data(language_links, d);

	struct buffer_list *out = tmpl_parse((char *)tmpl, sb.st_size, data);
	tmpl_data_free(data);
	close(fd);

	char *result = buffer_list_concat_string(out);
	printf("%s", result);

	return 0;
}
