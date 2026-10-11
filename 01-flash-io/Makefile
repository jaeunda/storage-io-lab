CC = gcc
CFLAGS ?= -Wall -Wextra -O0 -g
TARGET = ../build/flash-io
SRCS = src/fdevicedriver.c src/ftlmgr.c
HEADERS = src/flash.h

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	mkdir -p ../build
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(TARGET)
