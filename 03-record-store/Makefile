CC = gcc
CFLAGS ?= -Wall -Wextra -O0 -g
TARGET = ../build/record-store
SRCS = src/student.c
HEADERS = src/student.h

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	mkdir -p ../build
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(TARGET)
