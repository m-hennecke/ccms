# Sitemap Makefile

.PATH:		${.CURDIR}/../

PROG=		sitemap
SRCS=		sitemap_cgi.c filehelper.c buffer.c sitemap.c

CFLAGS+=	-I..
LDADD+=		-lutil -lz
LDSTATIC=	${STATIC}
NOMAN=		1

.include <cmsconfig.mk>

.include <bsd.prog.mk>
