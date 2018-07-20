
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
#include "template.h"

#include "tmpl.h"

const char *env_vars[] = {
	"SERVER_SOFTWARE",
	"SERVER_NAME",
	"SERVER_PROTOCOL",
	"SERVER_PORT",
	"DOCUMENT_ROOT",
	"SERVER_ADMIN",
	"GATEWAY_INTERFACE",
	"REQUEST_METHOD",
	"SCRIPT_FILENAME",
	"SCRIPT_NAME",
	"CONTENT_TYPE",
	"CONTENT_LENGTH",
	"QUERY_STRING",
	"REQUEST_URI",
	"PATH_INFO",
	"PATH_TRANSLATED",
	"AUTH_TYPE",
	"REMOTE_HOST",
	"REMOTE_ADDR",
	"REMOTE_USER",
	"HTTP_USER_AGENT",
	"HTTP_HOST",
	"HTTP_ACCEPT",
	"HTTP_ACCEPT_CHARSET",
	"HTTP_ACCEPT_ENCODING",
	"HTTP_ACCEPT_LANGUAGE",
	"HTTP_CONNECTION",
	"HTTP_REFERER",
	NULL
};

static void
fill_env_data(struct tmpl_loop *_loop)
{
	const char **v = env_vars;
	while (*v) {
		char *value = getenv(*v);
		struct tmpl_data *data = tmpl_data_new();
		tmpl_data_set_variable(data, "NAME", *v);
		tmpl_data_set_variable(data, "VALUE", (value) ? value : "");
		tmpl_loop_add_data(_loop, data);
		v++;
	}
}


int
main(int argc, char **argv)
{
	struct tmpl_data *data = tmpl_data_new();

	struct tmpl_loop *env_variables = tmpl_data_add_loop(
			data, "ENV"
		);
	fill_env_data(env_variables);
	tmpl_data_set_variable(data, "TITLE", "Environment Variables");


	struct buffer_list *out = tmpl_parse(env_tmpl, strlen(env_tmpl), data);
	tmpl_data_free(data);

	char *result = buffer_list_concat_string(out);
	printf("Content-type: application/xhtml+xml\r\n\r\n%s", result);

	return 0;
}
