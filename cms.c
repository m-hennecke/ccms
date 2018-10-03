
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
#ifndef CMS_SESSION_DIR
#error "Need CMS_SESSION_Dir defined to compile"
#endif

char *cms_content_dir  = CMS_CONTENT_DIR;
char *cms_template_dir = CMS_TEMPLATE_DIR;
char *cms_session_db  = CMS_SESSION_DIR "/session.db";
char *cms_session_htpasswd = CMS_SESSION_DIR "/htpasswd";

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

	if (argc > 2)
		usage();
	if (argc >= 2) {
		cms_content_dir  = CMS_CHROOT CMS_CONTENT_DIR;
		cms_template_dir = CMS_CHROOT CMS_TEMPLATE_DIR;
		cms_session_db  = CMS_CHROOT CMS_SESSION_DIR "/session.db";
		cms_session_htpasswd = CMS_CHROOT CMS_SESSION_DIR "/htpasswd";
		if (argv[1][0] != '/')
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

	struct page_info *page = request_fetch_page(r);
	if (page == NULL)
		_error("404 Not Found", NULL);

	request_init_tmpl_data(r);
	request_handle_login(r);
	out = request_render_page(r, CMS_DEFAULT_TEMPLATE);
	result = buffer_list_concat_string(out);
	request_add_header(r, "Content-type", "application/xhtml+xml");
	request_add_header(r, "Status", "200 Ok");

	hb = request_output_headers(r);
	char *header = buffer_list_concat_string(hb);
	dprintf(STDOUT_FILENO, "%s\r\n%s", header, result);

	request_free(r);
	buffer_list_free(hb);
	buffer_list_free(out);
	return 0;
}
