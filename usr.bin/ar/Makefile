CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2
PROG = ar
SRCS = append.c ar.c archive.c contents.c delete.c extract.c misc.c \
       move.c print.c replace.c strmode.c
OBJS = $(SRCS:.c=.o)

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lutil

clean:
	rm -f $(PROG) $(OBJS)

.PHONY: all clean
