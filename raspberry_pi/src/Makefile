TARGET = "zoxnoxiousd"
INCLUDE = -I../include
LIB_PATH ?= ../build_lib
LIBS = -lpigpio -lconfig -lasound -lpthread -lzlog -ldl -latomic -L$(LIB_PATH) -lzdk
CC = gcc
CFLAGS = -g -O2 -Wall
#CFLAGS = -g -Wall

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
