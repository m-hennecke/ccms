# cgienv Makefile

.PATH:		${.CURDIR}/../

PROG=		cgienv
SRCS=		cgienv.c template.c tmpl_parser.c buffer.c helper.c

CFLAGS+=	-I.. -I/usr/local/include
CFLAGS+=	-Wall
CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=	-Wmissing-declarations
CFLAGS+=	-Wshadow -Wpointer-arith -Wsign-compare -Wcast-qual
CFLAGS+=	-fdata-sections -ffunction-sections
LDFLAGS+=	-Wl,--gc-sections

LDADD+=		-lutil
LDSTATIC=	${STATIC}
NOMAN=		1

BINDIR=		/var/www/cgi-bin
DAEMON=		${BINDIR}/${PROG}

tmpl.h:		${.CURDIR}/env.template
	(echo -n "const char *env_tmpl = "; \
		sed 's/"/\\"/g;s/^/"/;s/$$/\\n"/' ${.CURDIR}/env.template ; \
		echo ";") > $@

.include <bsd.prog.mk>
