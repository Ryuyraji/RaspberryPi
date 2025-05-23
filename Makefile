# Makefile for Raspberry Pi server program with wiringPi

TARGET = songsong
CC = gcc
CFLAGS = -Wall -pthread
LIBS = -lwiringPi -lpthread -lrt

SRC = songsong.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

