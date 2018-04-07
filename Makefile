CC=g++
CFLAGS=-Wall -std=c++11 -O3
DEBUG= -ggdb  #-DDEBUG
LDFLAGS=
LIBS=
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
