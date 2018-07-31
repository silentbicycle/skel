PROJECT= 	skel
OPTIMIZE =	-O3
WARN =		-Wall -pedantic -Wextra
CSTD +=		-std=c99 -D_POSIX_C_SOURCE=200112L
CDEFS +=
CFLAGS +=	${CSTD} -g ${WARN} ${CDEFS} ${OPTIMIZE}
LDFLAGS +=

BUILD =		build
SRC =		src
EXAMPLES = 	examples
MAN =		man

OBJS=		${BUILD}/main.o \
		${BUILD}/path.o \
		${BUILD}/sub.o \

# Basic targets

all: ${BUILD}/${PROJECT}

clean:
	rm -rf ${BUILD}

${BUILD}:
	mkdir ${BUILD}

${BUILD}/${PROJECT}: ${OBJS} | ${BUILD}
	${CC} -o $@ $^ ${LDFLAGS}

${BUILD}/%.o: ${SRC}/%.c ${SRC}/*.h | ${BUILD}
	${CC} -c -o $@ ${CFLAGS} $<

test: ${BUILD}/${PROJECT}
	test/run_tests

ci: test

${BUILD}/*.o: ${SRC}/*.h Makefile

${BUILD}/%.png: ${SRC}/%.dot | ${BUILD}
	dot -o $@ -Tpng $<

# Regenerate documentation (requires ronn)
docs: man/${PROJECT}.1 man/${PROJECT}.1.html

man/${PROJECT}.1: man/${PROJECT}.1.ronn
	ronn --roff $<

man/${PROJECT}.1.html: man/${PROJECT}.1.ronn
	ronn --html $<

# Installation
PREFIX ?=	/usr/local
INSTALL ?=	install
RM ?=		rm
MAN_DEST ?=	${PREFIX}/share/man

install:
	${INSTALL} -c ${BUILD}/${PROJECT} ${PREFIX}/bin
	${INSTALL} -c man/${PROJECT}.1 ${MAN_DEST}/man1/

uninstall:
	${RM} -f ${PREFIX}/bin/${PROJECT}
	${RM} -f ${MAN_DEST}/man1/${PROJECT}.1

.PHONY: test
