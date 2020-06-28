CC=gcc
CFLAGS=-Wall -Werror -pg -g
LDFLAGS=
LDLIBS=-lm # you must decribed this in your report
TARGET=a.out
OBJS=card.o \
	 index.o \
	 node.o \
	 rect.o \
	 gammavol.o \
	 split_l.o \
	 test.o \

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDLIBS) -o $(TARGET)

clean:
	rm -f *.o
	rm -f $(TARGET)

