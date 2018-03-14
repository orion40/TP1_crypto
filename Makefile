CC=gcc
CFLAGS=-ggdb -std=c99
LDFLAGS=
LIBS=
SRC=$(wildcard *.c)
OBJS=$(SRC:.c=.o)
TARGET=tp1

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

%.o:%.c
	$(CC) -c $^ $(CFLAGS)

clean:
	rm -r $(TARGET) $(OBJS)
