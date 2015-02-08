PROJECT = skel
CFLAGS += -std=c99 -Wall -pedantic -g -Os

OBJS=	main.o \
	path.o \
	sub.o \

${PROJECT}: ${OBJS}
	${CC} -o $@ ${LDFLAGS} $^

test: ${PROJECT}
	./test_skel

clean:
	rm -f ${PROJECT} *.o *.core

*.o: *.h Makefile

sm.png: sm.dot

%.png: %.dot
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
	${INSTALL} -c ${PROJECT} ${PREFIX}/bin
	${INSTALL} -c man/${PROJECT}.1 ${MAN_DEST}/man1/

uninstall:
	${RM} -f ${PREFIX}/bin/${PROJECT}
	${RM} -f ${MAN_DEST}/man1/${PROJECT}.1
