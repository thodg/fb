
PROGRAM = fbdemo
SRCDIR = ../..

CPPFLAGS =
CFLAGS = -O0 -ggdb -W -Wall -Werror

LDFLAGS =
LIBS = -lm
HEADERS = fb.h
OBJECTS = fb.o fbdemo.o

SRC = ${HEADERS} ${OBJECTS:%.o=%.c}

${PROGRAM}: ${OBJECTS}
	${CC} ${LDFLAGS} ${OBJECTS} ${LIBS} -o ${PROGRAM}

${OBJECTS}: ${HEADERS}

${SRC}: %: ${SRCDIR}/%
	if [ -f $@ ]; then chmod u+w $@; fi
	cp $< $@
	chmod a-w $@

CLEANFILES += ${PROGRAM} ${OBJECTS} ${SRC}

clean:
	rm -f ${CLEANFILES}

.PHONY: clean
