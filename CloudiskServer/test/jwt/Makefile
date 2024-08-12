SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c, %, $(SRCS))
CC:=gcc
FLAGS:=-g
LIBS:= -ll8w8jwt -lmbedtls -lmbedx509 -lmbedcrypto

ALL:$(OBJS)

%:%.c
	$(CC)  $^ -o $@ $(FLAGS) $(LIBS)

clean:
	rm -rf $(OBJS)
