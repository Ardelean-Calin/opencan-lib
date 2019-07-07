.DEFAULT_GOAL := all

BUILD_DIR := build

# Compiler
CC=i686-w64-mingw32-gcc
CFLAGS= -std=c11 -Iinclude

# Linker
LIB=i686-w64-mingw32-ar
LIBFLAGS=rcs
LIBFILE=libopencan.a

# User defined functions
MKDIR_P = mkdir -p

SRC=src
OBJ=$(BUILD_DIR)

ifeq ($(DEBUG),1)
	CFLAGS += -g -Og
else
	CFLAGS += -O0
endif

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

all: clean directories main.exe

directories: $(BUILD_DIR)
$(BUILD_DIR):
	$(MKDIR_P) $(BUILD_DIR)

# Generate an executable which uses our static library
main.exe: lib
	$(CC) -o $@ main.c $(LIBFILE) $(CFLAGS)

# Generate the static library!
lib: $(OBJECTS)
	$(LIB) $(LIBFLAGS) $(LIBFILE) $^

# Generic object compiler
$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -Wall -c $< -o $@ $(CFLAGS)

clean:
	rm -f *.exe
	rm -f *.lib
	rm -f build/*