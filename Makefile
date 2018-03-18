PREFIX?=	/var/www/cgi-bin/
BINDIR=		${DESTDIR}${PREFIX}
DAEMON=		${BINDIR}/${PROG}

PROG=		kcms
SRCS=		kcms.c filehelper.c sitemap.c

CFLAGS+=	-Wall -I${.CURDIR}
CFLAGS+=	-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=	-Wmissing-declarations
CFLAGS+=	-Wshadow -Wpointer-arith -Wsign-compare -Wcast-qual
CFLAGS+=	-ggdb -O0
LDADD+=		-lutil
LDSTATIC=	${STATIC}
#DPADD+=		${LIBUTIL} ${LIBCRYPTO} ${LIBSSL} ${LIBTLS}
NOMAN=		1

.include <bsd.prog.mk>
