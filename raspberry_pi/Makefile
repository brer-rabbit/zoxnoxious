# Top-level Makefile

# Environment variable for installation prefix
INSTALL_PREFIX ?= /usr/local/zoxnoxious

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

# Library directories
LIB_DIRS = lib/src/zdk lib/src/poledancer lib/src/z3372 lib/src/z3340 lib/src/audio_out lib/src/z5524

# Library target prefix and suffix
LIB_TARGET_PREFIX = lib/
PLUGIN_LIB_EXTENSION = .plugin.so
LIB_TARGET_SUFFIX = .so

APP_EXE = zoxnoxiousd

BUILD_LIB_DIR = build_lib

PLUGIN_LIB_TARGETS = $(addprefix $(BUILD_LIB_DIR)/, $(addsuffix $(PLUGIN_LIB_EXTENSION), $(filter-out zdk, $(notdir $(LIB_DIRS)))))

LIB_ZDK_TARGET = $(BUILD_LIB_DIR)/libzdk$(LIB_TARGET_SUFFIX)

all: $(APP_EXE) $(LIB_ZDK_TARGET)

$(APP_EXE): $(LIB_ZDK_TARGET)
	$(MAKE) -C src LIB_PATH=../$(BUILD_LIB_DIR)

$(BUILD_LIB_DIR)/libzdk$(LIB_TARGET_SUFFIX): lib/src/zdk/Makefile
	$(MAKE) -C lib/src/zdk OUTPUT_DIR=../../../$(BUILD_LIB_DIR) TARGET=libzdk$(LIB_TARGET_SUFFIX)

$(BUILD_LIB_DIR)/%: lib/src/%/Makefile
	$(MAKE) -C lib/src/$* OUTPUT_DIR=$(BUILD_LIB_DIR) TARGET=$(notdir $@)

clean:
	$(MAKE) -C src clean
	$(foreach dir, $(LIB_DIRS), $(MAKE) -C $(dir) clean OUTPUT_DIR=$(BUILD_LIB_DIR);)
	rm -rf etc/*.generated $(BUILD_LIB_DIR)

etc_install_files := $(shell cat etc/.install 2>/dev/null)

install: $(APP_EXE)
	mkdir -p $(INSTALL_PREFIX)/bin
	mkdir -p $(INSTALL_PREFIX)/lib
	mkdir -p $(INSTALL_PREFIX)/etc
	install -m 755 src/$(APP_EXE) $(INSTALL_PREFIX)/bin
	install -m 755 $(LIB_ZDK_TARGET) $(INSTALL_PREFIX)/lib
	$(foreach dir, $(LIB_DIRS), $(MAKE) -C $(dir) install INSTALL_PREFIX=$(INSTALL_PREFIX);) # library install
	install -m 644 $(addprefix etc/,$(etc_install_files)) $(INSTALL_PREFIX)/etc/

uninstall:
	rm -rf $(INSTALL_PREFIX)

.PHONY: all clean install uninstall
