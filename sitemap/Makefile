# Sitemap Makefile

.PATH:		${.CURDIR}/../

PROG=		sitemap
SRCS=		sitemap_cgi.c filehelper.c buffer.c sitemap.c

CFLAGS+=	-I.. -I/usr/local/include
LDADD+=		-L/usr/local/lib
LDADD+=		-lutil -lkcgi -lz
LDSTATIC=	${STATIC}
NOMAN=		1

.include <cmsconfig.mk>

.include <bsd.prog.mk>
