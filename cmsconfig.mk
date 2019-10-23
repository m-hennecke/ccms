# Common setup

CMS_HOSTNAME?=		`hostname`
CMS_ROOT_DIR?=		/var/www/cms
CONTENT_DIR=		${ROOT_DIR}/content
TEMPLATE_DIR=		${ROOT_DIR}/templates
SESSION_DIR=		${ROOT_DIR}/session
CMS_DEFAULT_LANGUAGE?=	en
CMS_DEFAULT_TEMPLATE?=	page.tmpl
CMS_CONFIG_URL_IMAGES?=	/images/
CMS_ROOT_URL?=		/
BINDIR=			/var/www/cgi-bin
DAEMON=			${BINDIR}/${PROG}
CHROOT?=		/var/www
WWW_USER?=		www
WWW_GROUP?=		www

# Absolute path from within the chroot
ROOT_DIR=		${CMS_ROOT_DIR:S/^${CHROOT}//}

# Common CFLAGS
CFLAGS+=		-Wall -I${.CURDIR} -Werror
CFLAGS+=		-Wstrict-prototypes -Wmissing-prototypes
CFLAGS+=		-Wmissing-declarations
CFLAGS+=		-Wshadow -Wpointer-arith -Wsign-compare -Wcast-qual
CFLAGS+=		-fdata-sections -ffunction-sections
LDFLAGS+=		-Wl,--gc-sections

.if exists(localconfig.mk)
.include <localconfig.mk>
.endif

CFLAGS+=		-DCMS_HOSTNAME=\"${CMS_HOSTNAME}\" \
			-DCMS_DEFAULT_LANGUAGE=\"${CMS_DEFAULT_LANGUAGE}\" \
			-DCMS_CONTENT_DIR=\"${CONTENT_DIR}\" \
			-DCMS_TEMPLATE_DIR=\"${TEMPLATE_DIR}\" \
			-DCMS_SESSION_DIR=\"${SESSION_DIR}\" \
			-DCMS_DEFAULT_TEMPLATE=\"${CMS_DEFAULT_TEMPLATE}\" \
			-DCMS_CONFIG_URL_IMAGES=\"${CMS_CONFIG_URL_IMAGES}\" \
			-DCMS_ROOT_URL=\"${CMS_ROOT_URL}\" \
			-DCMS_CHROOT=\"${CHROOT}\"

