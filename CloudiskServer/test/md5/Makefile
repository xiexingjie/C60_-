SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c, %, $(SRCS))
CC:=gcc
FLAGS:=-g
LIBS:=-lcrypto

ALL:$(OBJS)

%:%.c
	$(CC)  $^ -o $@ $(FLAGS) $(LIBS)

clean:
	rm -rf $(OBJS)
