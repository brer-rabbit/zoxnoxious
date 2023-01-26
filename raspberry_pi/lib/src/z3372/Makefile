TARGET = "z3372.plugin.so"
INCLUDE = -I../../../include
LIBS = -shared -lpigpio -lzlog -ldl
#config -lasound -lpthread -lzlog
CC = gcc
CFLAGS = -g -O2 -Wall -shared -fPIC
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
