.DEFAULT_GOAL := all

BUILD_DIR := build

CC=gcc
CFLAGS= -std=c11 -Iinclude -m32
LIB="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\lib.exe"

SRC=src
OBJ=$(BUILD_DIR)

ifeq ($(DEBUG),1)
	CFLAGS += -g -Og
else
	CFLAGS += -O0
endif

SOURCES := $(wildcard $(SRC)/*.c)
OBJECTS := $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SOURCES))

all: clean main.exe lib

main.exe: $(OBJECTS)
	$(CC) -o $@ main.c $^ $(CFLAGS)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) -Wall -c $< -o $@ $(CFLAGS)

# Generate the static library!
lib: $(OBJECTS)
	$(LIB) /out:opencan.lib $^

clean:
	rm -f *.exe
	rm -f *.lib
	rm -f build/*