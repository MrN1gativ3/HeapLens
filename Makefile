CC ?= gcc
PKG_CONFIG ?= pkg-config
CARGO ?= cargo

BASE_CFLAGS := -O2 -Wall -Wextra -std=c11 -D_GNU_SOURCE -fPIC
GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk4 2>/dev/null)
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk4 2>/dev/null)
VTE_CFLAGS := $(shell $(PKG_CONFIG) --cflags vte-2.91-gtk4 2>/dev/null)
VTE_LIBS := $(shell $(PKG_CONFIG) --libs vte-2.91-gtk4 2>/dev/null)
CAPSTONE_CFLAGS := $(shell $(PKG_CONFIG) --cflags capstone 2>/dev/null)
CAPSTONE_LIBS := $(shell $(PKG_CONFIG) --libs capstone 2>/dev/null)

CFLAGS_SHARED := $(BASE_CFLAGS) $(GTK_CFLAGS) $(VTE_CFLAGS) $(CAPSTONE_CFLAGS) -Isrc
LDLIBS_SHARED := $(GTK_LIBS) $(VTE_LIBS) $(CAPSTONE_LIBS) -lm

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
LIB := $(BUILD_DIR)/libheaplens.so
APP := $(BUILD_DIR)/heaplens
RUST_TARGET_DIR := $(BUILD_DIR)/cargo-target
PROFILE ?= debug

ifeq ($(PROFILE),release)
CARGO_PROFILE_FLAG := --release
RUST_BIN := $(RUST_TARGET_DIR)/release/heaplens
else
CARGO_PROFILE_FLAG :=
RUST_BIN := $(RUST_TARGET_DIR)/debug/heaplens
endif

LIB_SRCS := $(sort \
	src/core/disasm_engine.c \
	src/core/heap_parser.c \
	src/core/memory_reader.c \
	src/core/process_manager.c \
	src/core/ptrace_backend.c \
	src/core/symbol_resolver.c \
	src/core/syscall_table.c \
	src/core/tcache_parser.c \
	src/techniques/technique_registry.c)
LIB_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
RUST_SRCS := $(shell find src -name '*.rs' 2>/dev/null)

.PHONY: all clean demos run

all: $(LIB) $(APP) demos

$(LIB): $(LIB_OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) -shared -Wl,-soname,libheaplens.so -o $@ $^ $(LDLIBS_SHARED)

$(APP): $(LIB) Cargo.toml build.rs $(RUST_SRCS)
	@mkdir -p $(BUILD_DIR)
	HEAPLENS_LIB_DIR="$(abspath $(BUILD_DIR))" $(CARGO) build $(CARGO_PROFILE_FLAG) --target-dir $(RUST_TARGET_DIR)
	cp $(RUST_BIN) $@

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS_SHARED) -c $< -o $@

demos:
	$(MAKE) -f src/techniques/demos/Makefile.demos \
		BUILD_DIR=$(abspath $(BUILD_DIR)) \
		CC="$(CC)" \
		BASE_CFLAGS="-O2 -Wall -Wextra -std=c11 -D_GNU_SOURCE"

run: all
	LD_LIBRARY_PATH="$(abspath $(BUILD_DIR)):$$LD_LIBRARY_PATH" ./$(APP)

clean:
	rm -rf $(BUILD_DIR)
