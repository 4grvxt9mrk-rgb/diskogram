# Makefile for spacetime - cross-platform disk space histogram tool

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -O2 -std=c99
LDFLAGS =

# Target executable
TARGET = spacetime

# Source files
SOURCES = main.c scan.c histogram.c display.c export.c
OBJECTS = $(SOURCES:.c=.o)
HEADERS = spacetime.h

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    RM = del /Q
    RMDIR = rmdir /S /Q
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        # macOS specific flags
        CFLAGS += -D_DARWIN_C_SOURCE
    endif
    ifeq ($(UNAME_S),Linux)
        # Linux specific flags
        CFLAGS += -D_DEFAULT_SOURCE
    endif
    ifeq ($(UNAME_S),FreeBSD)
        # FreeBSD specific flags
        CFLAGS += -D_BSD_SOURCE
    endif
    RM = rm -f
    RMDIR = rm -rf
endif

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	$(RM) $(OBJECTS) $(TARGET)

# Install (optional)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall (optional)
uninstall:
	$(RM) /usr/local/bin/$(TARGET)

# Phony targets
.PHONY: all clean install uninstall
