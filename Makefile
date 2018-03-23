CC=gcc
CFLAGS=-Wall
DEBUG= -ggdb  #-DDEBUG
LDFLAGS=
LIBS=-lm
SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.o)
TARGET=tp1

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

%.o:%.c
	$(CC) -c $^ $(CFLAGS) $(DEBUG)

clean:
	rm -f $(TARGET) $(OBJS)
