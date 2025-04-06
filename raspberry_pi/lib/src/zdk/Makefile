INSTALL_PREFIX ?= /usr/local/zoxnoxious

TARGET ?= libzdk.so
INCLUDE = -I../../../include
LIBS = -shared -lpigpio -lzlog -ldl
#config -lasound -lpthread -lzlog
CC = gcc
CFLAGS = -g -O2 -Wall -shared -fPIC
#CFLAGS = -g -Wall
OUTPUT_DIR = ../../../build_lib

.PHONY: default all clean

default: $(OUTPUT_DIR)/$(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(OUTPUT_DIR)/$(TARGET): $(OBJECTS)
	mkdir -p $(OUTPUT_DIR)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)

install: $(TARGET)
	mkdir -p $(INSTALL_PREFIX)/lib
	install -m 755 $(TARGET) $(INSTALL_PREFIX)/lib/
