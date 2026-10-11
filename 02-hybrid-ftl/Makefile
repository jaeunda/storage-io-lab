CC = gcc
CFLAGS ?= -Wall -Wextra -O0 -g
TARGET = ../build/hybrid-ftl
SRCS = src/main.c src/fdevicedriver.c src/ftlmgr.c
HEADERS = src/hybridmapping.h

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(SRCS) $(HEADERS)
	mkdir -p ../build
	$(CC) $(CFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(TARGET)
