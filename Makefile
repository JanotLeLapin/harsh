CC = gcc
LDFLAGS = -lm -lsndfile
CFLAGS = -Wall -Wextra -O3

SRCS = main.c oscillators.c synthesizers.c processors.c shapers.c audio.c
OBJS = $(SRCS:.c=.o)

TARGET = harsh

all: $(TARGET)

$(TARGET): main.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) harsh.o $(TARGET)

.PHONY: all clean
