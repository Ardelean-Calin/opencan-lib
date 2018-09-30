.DEFAULT_GOAL := main.exe

CC=gcc
CFLAGS=
LIB="C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin\lib.exe"

ifeq ($(DEBUG),1)
	CFLAGS += -g -Og
else
	CFLAGS += -O0
endif

all: clean main.exe lib

main.exe: main.c OpenCAN_api.c
	$(CC) -o $@ main.c OpenCAN_api.c cobs.c -I. $(CFLAGS)

%.o: %.c
	$(CC) -m32 -Wall -c $^

# Generate the static library!
lib: OpenCAN_api.c OpenCAN_api.o cobs.o
	$(LIB) /out:opencan.lib $(word 2,$^) $(word 3,$^)

clean:
	rm -f *.exe
	rm -f *.o
	rm -f *.lib