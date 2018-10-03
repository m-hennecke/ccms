# Main Makefile

PROG=		kcms
SRCS=		kcms.c filehelper.c buffer.c sitemap.c template.c \
		tmpl_parser.c helper.c handler.c linklist.c session.c \
		htpasswd.c

SUBDIR=		sitemap cgienv

LDADD+=		-lutil -lz
LDSTATIC=	${STATIC}
NOMAN=		1

# include site settings
.include "cmsconfig.mk"

.include <bsd.prog.mk>
