PROG=		diskinfo
WARNINGS=	yes
BINDIR?=	/usr/local/sbin
MANDIR?=	/usr/local/man/man
DPADD=		${LIBUTIL}
LDADD?=		-lutil

VERSION=	0.1

.include <bsd.prog.mk>
