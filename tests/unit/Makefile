CC = gcc
CFLAGS = -Wall -Werror -std=c99 -g
LDFLAGS =

TARGET = test_fsm_rules
SRC = test_fsm_rules.c

all: ${TARGET}

${TARGET}: ${SRC}
	${CC} ${CFLAGS} -o $@ ${SRC} ${LDFLAGS}

run: all
	./${TARGET}

check: run

clean:
	rm -f ${TARGET} *.o
