
#include <sys/mman.h>
#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static __dead void	usage(void);

__dead void
usage(void)
{
	extern char *__progname;

	dprintf(STDERR_FILENO, "usage: %s URI\n", __progname);
	exit(1);
}


int
main(int argc, char **argv)
{
	char *result = "";
	struct buffer_list *hb, *out;
	struct request *r;
	struct tmpl_data *d = NULL;
	struct memmap *tmpl_file = NULL;

	if (argc > 2)
		usage();
	if (argc == 2) {
		if (argv[1][0] == '\0')
			errx(1, "Require absolute path as argument");
		// XXX Substitude PATH_INFO env variable
		setenv("PATH_INFO", argv[1], 1);
	}
	char *path_info = getenv("PATH_INFO");
	if (path_info == NULL || strlen(path_info) == 0
			|| strcmp(path_info, "index.html") == 0
			|| strcmp(path_info, "/index.html") == 0)
		path_info = "/home.html";
	r = request_new(path_info);
	if (r == NULL)
		_error("404 Not Found", NULL);
	request_parse_lang_pref(r);

	struct page_info *page = fetch_page(r);
	if (page == NULL)
		_error("404 Not Found", NULL);
	r->page_info = page;

	d = tmpl_data_new();
	if (page->descr)
		tmpl_data_set_variablen(d, "DESCR", page->descr->data,
				page->descr->size);

	if (page->content) {
		struct buffer_list *content = tmpl_parse(page->content->data,
				page->content->size, d);
		tmpl_data_set_variable(d, "CONTENT",
				buffer_list_concat_string(content));
	} else {
		request_add_header(r, "Status", "404 Not Found");
		goto not_found;
	}
	struct tmpl_loop *links = get_links(r);
	struct tmpl_loop *lang_links = get_language_links(r);
	if (links)
		tmpl_data_set_loop(d, "LINK_LOOP", links);
	if (lang_links)
		tmpl_data_set_loop(d, "LANGUAGE_LINKS", lang_links);
	tmpl_file = memmap_new_at(r->template_dir, CMS_DEFAULT_TEMPLATE);
	if (tmpl_file == NULL) {
		request_add_header(r, "Status", "500 Internal Server Error");
		result = "Unable to load template file " CMS_DEFAULT_TEMPLATE;
		goto not_found;
	}
	out = tmpl_parse((char *)tmpl_file->data, tmpl_file->size, d);

	result = buffer_list_concat_string(out);
	request_add_header(r, "Content-type", "application/xhtml+xml");
	request_add_header(r, "Status", "200 Ok");

not_found:
	hb = request_output_headers(r);
	char *header = buffer_list_concat_string(hb);
	dprintf(STDOUT_FILENO, "%s\r\n%s", header, result);
	tmpl_data_free(d);
	memmap_free(tmpl_file);

	return 0;
}
