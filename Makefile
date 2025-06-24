CC = gcc
LDFLAGS = -lm -lsndfile
CFLAGS = -Wall -Wextra -g

SRCS = harsh.c graph.c dsl.c hm.c vec.c
OBJS = $(SRCS:.c=.o)

TARGET = harsh

all: $(TARGET)

$(TARGET): harsh.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) harsh.o $(TARGET)

.PHONY: all clean
