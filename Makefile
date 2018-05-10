# Main Makefile

PROG=		kcms
SRCS=		kcms.c filehelper.c buffer.c sitemap.c

SUBDIR=		sitemap

LDADD+=		-lutil -lz
LDSTATIC=	${STATIC}
NOMAN=		1

# include site settings
.include "cmsconfig.mk"

.include <bsd.prog.mk>
