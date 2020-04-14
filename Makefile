# Main Makefile

PROG=		cms
SRCS=		cms.c filehelper.c buffer.c sitemap.c template.c \
		tmpl_parser.c helper.c handler.c linklist.c session.c \
		htpasswd.c

SUBDIR=		sitemap cgienv

CFLAGS+=	-I/usr/local/include
LDFLAGS+=	-L/usr/local/lib
LDADD+=		-lutil -lz -llowdown -lm
LDSTATIC=	${STATIC}
NOMAN=		1

# include site settings
.include "cmsconfig.mk"

afterinstall:
	${INSTALL} -d -o ${WWW_USER} -g ${WWW_GROUP} -m 700 \
		${DESTDIR}${CHROOT}${SESSION_DIR}

.include <bsd.prog.mk>
