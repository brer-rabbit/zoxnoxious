INSTALL_PREFIX ?= /usr/local/zoxnoxious

TARGET = "poledancer.plugin.so"
INCLUDE = -I../../../include
LIBS = -shared -lpigpio -lzlog -ldl
#config -lasound -lpthread -lzlog
CC = gcc
#CFLAGS = -g -Wall -shared -fPIC
CFLAGS = -g -O2 -Wall -shared -fPIC

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

$(OUTPUT_DIR)/$(TARGET): $(OBJECTS)
	mkdir -p $(OUTPUT_DIR)
	$(CC) $(OBJECTS) -Wall -shared $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)

install: $(TARGET)
	mkdir -p $(INSTALL_PREFIX)/lib
	install -m 755 $(TARGET) $(INSTALL_PREFIX)/lib/
