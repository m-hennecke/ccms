PREFIX?=	/var/www/cgi-bin/
BINDIR=		${DESTDIR}${PREFIX}
DAEMON=		${BINDIR}/${PROG}

PROG=		kcms
SRCS=		kcms.c filehelper.c buffer.c sitemap.c

CFLAGS+=	-Wall -I${.CURDIR}
CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=	-Wmissing-declarations
CFLAGS+=	-Wshadow -Wpointer-arith -Wsign-compare -Wcast-qual
LDADD+=		-lutil -lz
LDSTATIC=	${STATIC}
#DPADD+=		${LIBUTIL} ${LIBCRYPTO} ${LIBSSL} ${LIBTLS}
NOMAN=		1

# include site settings
.include "cmsconfig.mk"

.include <bsd.prog.mk>
