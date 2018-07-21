
#include <sys/mman.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "buffer.h"
#include "filehelper.h"
#include "handler.h"
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

	fprintf(stderr, "usage: %s URI\n", __progname);
	exit(1);
}


int
main(int argc, char **argv)
{
	if (argc > 2)
		usage();
	if (argc == 2) {
		if (argv[1][0] == '\0')
			errx(1, "Require absolute path as argument");
		// XXX Substitude PATH_INFO env variable
		setenv("PATH_INFO", argv[1], 1);
	}
	char *path_info = getenv("PATH_INFO");
	if (path_info == NULL)
		path_info = "/home_" CMS_DEFAULT_LANGUAGE ".html";
	struct request *r = request_new(path_info);
	request_parse_lang_pref(r);

	struct page_info *page = fetch_page(r);
	r->page_info = page;

	if (NULL == page)
		errx(1, "Unable to fetch page %s", path_info);

	struct tmpl_data *d = tmpl_data_new();
	if (page->content)
		tmpl_data_set_variablen(d, "CONTENT", page->content->data,
				page->content->size);
	else
		errx(1, "no CONTENT found");
	if (page->descr)
		tmpl_data_set_variablen(d, "DESCR", page->descr->data,
				page->descr->size);

	struct tmpl_loop *links = get_links(r);
	struct tmpl_loop *lang_links = get_language_links(r);
	if (links)
		tmpl_data_set_loop(d, "LINK_LOOP", links);
	if (lang_links)
		tmpl_data_set_loop(d, "LANGUAGE_LINKS", lang_links);
	struct memmap *tmpl_file = memmap_new_at(r->template_dir,
			CMS_DEFAULT_TEMPLATE);
	if (tmpl_file == NULL)
		errx(1, "Unable to load template file %s",
				CMS_DEFAULT_TEMPLATE);
	struct buffer_list *out = tmpl_parse((char *)tmpl_file->data,
			tmpl_file->size, d);

	char *result = buffer_list_concat_string(out);
	// TODO: Print headers from struct request
	printf("Content-type: application/xhtml+xml\r\n\r\n%s", result);
	tmpl_data_free(d);
	memmap_free(tmpl_file);

	return 0;
}
