# Common setup

CMS_HOSTNAME?=		`hostname`
CMS_CONTENT_DIR?=	/var/www/cms/content
PREFIX?=		/var/www/cgi-bin/
BINDIR=			${DESTDIR}${PREFIX}
DAEMON=			${BINDIR}/${PROG}
CHROOT?=		/var/www

# Absolute path from within the chroot
CONTENT_DIR=		${CMS_CONTENT_DIR:S/^${CHROOT}//}

# Common CFLAGS
CFLAGS+=		-Wall -I${.CURDIR}
CFLAGS+=		-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=		-Wmissing-declarations
CFLAGS+=		-Wshadow -Wpointer-arith -Wsign-compare -Wcast-qual

.if exists(localconfig.mk)
.include <localconfig.mk>
.endif

CFLAGS+=		-DCMS_HOSTNAME=\"${CMS_HOSTNAME}\" \
			-DCMS_CONTENT_DIR=\"${CONTENT_DIR}\"

