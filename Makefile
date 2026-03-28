CC ?= gcc
PKG_CONFIG ?= pkg-config

# The application is Linux-only, so we keep the flags close to the target toolchain.
BASE_CFLAGS := -O2 -Wall -Wextra -std=c11 -D_GNU_SOURCE
GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk4 2>/dev/null)
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk4 2>/dev/null)
VTE_CFLAGS := $(shell $(PKG_CONFIG) --cflags vte-2.91-gtk4 2>/dev/null)
VTE_LIBS := $(shell $(PKG_CONFIG) --libs vte-2.91-gtk4 2>/dev/null)
CAPSTONE_CFLAGS := $(shell $(PKG_CONFIG) --cflags capstone 2>/dev/null)
CAPSTONE_LIBS := $(shell $(PKG_CONFIG) --libs capstone 2>/dev/null)

CFLAGS += $(BASE_CFLAGS) $(GTK_CFLAGS) $(VTE_CFLAGS) $(CAPSTONE_CFLAGS) -Isrc
LDLIBS += $(GTK_LIBS) $(VTE_LIBS) $(CAPSTONE_LIBS) -lm

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
APP := $(BUILD_DIR)/heaplens

# The GUI app links every C file except the standalone demo programs.
APP_SRCS := $(filter-out $(wildcard src/techniques/demos/*.c),$(shell find src -name '*.c'))
APP_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(APP_SRCS))

.PHONY: all clean dirs demos run

all: dirs $(APP) demos

dirs:
	@mkdir -p $(OBJ_DIR)/src/core $(OBJ_DIR)/src/ui $(OBJ_DIR)/src/ui/learning $(OBJ_DIR)/src/techniques $(BUILD_DIR)/demos

$(APP): $(APP_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

demos:
	$(MAKE) -f src/techniques/demos/Makefile.demos BUILD_DIR=$(abspath $(BUILD_DIR)) CC="$(CC)" BASE_CFLAGS="$(BASE_CFLAGS)"

run: all
	./$(APP)

clean:
	@rm -rf $(BUILD_DIR)
