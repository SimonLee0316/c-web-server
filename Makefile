CC = gcc
CFLAGS = -Wall -g
TARGET = tcp_server
SRCS = main.c system.c task.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
