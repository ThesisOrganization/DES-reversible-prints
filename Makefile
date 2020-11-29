CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = test

WRAPPERS=wrappers.c
SRCS =
DEPS=

WRAPPERS_OBJ=$(WRAPPERS:.c=.o)

OBJS = $(SRCS:.c=.o)

DEPS= wrappers.h

.PHONY: clean

all: $(TARGET)

$(TARGET): $(OBJS) $(WRAPPERS_OBJ) $(DEPS)
	ld -g -r --wrap puts --wrap fwrite --wrap fclose $(WRAPPERS_OBJ) --whole-archive $(OBJS) -o application_wrapped.o
	$(CC) $(CFLAGS) application_wrapped.o -o $(TARGET)

%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -I Simulators/NeuRome-bin/include -c -o $@ $<

clean:
	$(RM) $(SRCS:.c=.o) $(SRCS:.c=.o.rs) $(TARGET)
