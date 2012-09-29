PROJECT = skel
CFLAGS += -std=c99 -Wall -pedantic -g -Os

${PROJECT}: skel.c

test: ${PROJECT}
	./test_skel

clean:
	rm -f ${PROJECT} *.o *.core
