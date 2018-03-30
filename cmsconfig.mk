
CMS_HOSTNAME?=		`hostname`
CMS_CONTENT_DIR?=	/var/www/cms/content

.if exists(localconfig.mk)
.include <localconfig.mk>
.endif

CFLAGS+=		-DCMS_HOSTNAME=\"${CMS_HOSTNAME}\" \
			-DCMS_CONTENT_DIR=\"${CMS_CONTENT_DIR}\"

