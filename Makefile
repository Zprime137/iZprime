# Central phony registry (extended by mk/*.mk fragments)
PHONY_TARGETS :=

# ---------------------------------------------------------
# Core toolchain and defaults
# ---------------------------------------------------------
CC_BASE ?= gcc
USE_CCACHE ?= 0
CCACHE ?= ccache
CC = $(if $(filter 1,$(USE_CCACHE)),$(CCACHE) ,)$(CC_BASE)
AR ?= ar
RANLIB ?= ranlib

USE_PKG_CONFIG ?= 1
PKG_CONFIG ?= pkg-config

LOGGING ?= 1
THREAD_FLAGS ?= -pthread
ARCH_NATIVE ?= 1
TUNE_NATIVE ?= 1
ENABLE_NDEBUG ?= 1
ENABLE_STRIP_DEAD ?= 1

SRC_DIR ?= src
TEST_DIR ?= test
INCLUDE_DIR ?= include
OBJ_DIR ?= build
OUTPUT_DIR ?= output
LOG_DIR ?= logs
PROGRAM ?= izprime
CLI_ENTRY ?= $(SRC_DIR)/main.c
STDOUT_FILE ?= stdout.txt
VERSION ?= 1.0.0
SOVERSION ?= $(word 1,$(subst ., ,$(VERSION)))
BUILD_SHARED ?= 1
UNAME_S := $(shell uname -s)

OBJ_SRC_DIR = $(OBJ_DIR)/src
OBJ_TEST_DIR = $(OBJ_DIR)/test
BIN_DIR = $(OBJ_DIR)/bin
LIB_BUILD_DIR = $(OBJ_DIR)/lib
EXAMPLES_OBJ_DIR = $(OBJ_DIR)/examples

TARGET = $(BIN_DIR)/$(PROGRAM)
STATIC_LIB = $(LIB_BUILD_DIR)/libizprime.a
OUTPUT = $(OUTPUT_DIR)/$(STDOUT_FILE)

ifeq ($(UNAME_S),Darwin)
SHARED_LIB_FULL = $(LIB_BUILD_DIR)/libizprime.$(VERSION).dylib
SHARED_LIB_SONAME = $(LIB_BUILD_DIR)/libizprime.$(SOVERSION).dylib
SHARED_LIB_LINK = $(LIB_BUILD_DIR)/libizprime.dylib
SHARED_LINK_FLAGS = -dynamiclib -Wl,-install_name,@rpath/libizprime.$(SOVERSION).dylib -Wl,-compatibility_version,$(SOVERSION) -Wl,-current_version,$(VERSION)
else
SHARED_LIB_FULL = $(LIB_BUILD_DIR)/libizprime.so.$(VERSION)
SHARED_LIB_SONAME = $(LIB_BUILD_DIR)/libizprime.so.$(SOVERSION)
SHARED_LIB_LINK = $(LIB_BUILD_DIR)/libizprime.so
SHARED_LINK_FLAGS = -shared -Wl,-soname,libizprime.so.$(SOVERSION)
endif

# Optional local overrides (not committed by default)
-include config.mk

# ---------------------------------------------------------
# Dependency discovery
# ---------------------------------------------------------
HOMEBREW_PREFIX ?= $(shell /opt/homebrew/bin/brew --prefix 2>/dev/null || brew --prefix 2>/dev/null)
ifeq ($(HOMEBREW_PREFIX),)
ifneq ($(wildcard /opt/homebrew),)
HOMEBREW_PREFIX = /opt/homebrew
else ifneq ($(wildcard /usr/local),)
HOMEBREW_PREFIX = /usr/local
endif
endif

BREW_INC = $(HOMEBREW_PREFIX)/include
BREW_LIB = $(HOMEBREW_PREFIX)/lib
OPENSSL_INC = $(HOMEBREW_PREFIX)/opt/openssl@3/include
OPENSSL_LIB = $(HOMEBREW_PREFIX)/opt/openssl@3/lib

THIRD_PARTY_CFLAGS =
THIRD_PARTY_LDFLAGS =
THIRD_PARTY_LDLIBS =

ifeq ($(USE_PKG_CONFIG),1)
PKG_CONFIG_OK := $(shell command -v $(PKG_CONFIG) >/dev/null 2>&1 && echo 1)
ifeq ($(PKG_CONFIG_OK),1)
PKG_OPENSSL_OK := $(shell $(PKG_CONFIG) --exists openssl >/dev/null 2>&1 && echo 1)
PKG_LIBSSL_OK := $(shell $(PKG_CONFIG) --exists libssl libcrypto >/dev/null 2>&1 && echo 1)
ifeq ($(PKG_OPENSSL_OK),1)
OPENSSL_PC = openssl
else ifeq ($(PKG_LIBSSL_OK),1)
OPENSSL_PC = libssl libcrypto
else
OPENSSL_PC =
endif
PKG_DEPS_OK := $(shell $(PKG_CONFIG) --exists gmp $(OPENSSL_PC) >/dev/null 2>&1 && echo 1)
ifeq ($(PKG_DEPS_OK),1)
THIRD_PARTY_CFLAGS += $(shell $(PKG_CONFIG) --cflags gmp $(OPENSSL_PC))
THIRD_PARTY_LDLIBS += $(shell $(PKG_CONFIG) --libs gmp $(OPENSSL_PC))
endif
endif
endif

ifeq ($(strip $(THIRD_PARTY_LDLIBS)),)
THIRD_PARTY_CFLAGS += $(if $(HOMEBREW_PREFIX),-I$(BREW_INC)) $(if $(HOMEBREW_PREFIX),-I$(OPENSSL_INC))
THIRD_PARTY_LDFLAGS += $(if $(HOMEBREW_PREFIX),-L$(BREW_LIB)) $(if $(HOMEBREW_PREFIX),-L$(OPENSSL_LIB))
THIRD_PARTY_LDLIBS += -lgmp -lssl -lcrypto
endif

# ---------------------------------------------------------
# Compile/link flags
# ---------------------------------------------------------
CPPFLAGS = -I$(INCLUDE_DIR) $(if $(filter 1,$(LOGGING)),-DENABLE_LOGGING,) -DIZ_VERSION=\"$(VERSION)\" $(THIRD_PARTY_CFLAGS)
COMMON_FLAGS = -Wall -Wextra -Wshadow -Wformat=2 -Wpedantic -MMD -MP
OPT_LEVEL ?= -O3
LTO_FLAGS ?= -flto
ARCH_FLAGS = $(if $(filter 1,$(ARCH_NATIVE)),-march=native,) $(if $(filter 1,$(TUNE_NATIVE)),-mtune=native,)
SECTION_FLAGS = $(if $(filter 1,$(ENABLE_STRIP_DEAD)),-ffunction-sections -fdata-sections,)
PIC_FLAGS = $(if $(filter 1,$(BUILD_SHARED)),-fPIC,)
CFLAGS_OPT ?= $(OPT_LEVEL) $(LTO_FLAGS) $(ARCH_FLAGS) $(SECTION_FLAGS) $(PIC_FLAGS)
CFLAGS_DBG ?= -g
CFLAGS = $(CFLAGS_OPT) $(COMMON_FLAGS) $(CFLAGS_DBG) $(THREAD_FLAGS)

DEAD_STRIP_LDFLAGS =
ifeq ($(ENABLE_STRIP_DEAD),1)
ifeq ($(UNAME_S),Darwin)
DEAD_STRIP_LDFLAGS += -Wl,-dead_strip
else
DEAD_STRIP_LDFLAGS += -Wl,--gc-sections
endif
endif
LDFLAGS = $(THIRD_PARTY_LDFLAGS) $(DEAD_STRIP_LDFLAGS)
LDLIBS = $(THIRD_PARTY_LDLIBS) $(THREAD_FLAGS)
LDLIBS += -lm

# ---------------------------------------------------------
# Source sets
# ---------------------------------------------------------
CORE_SOURCES = $(filter-out $(SRC_DIR)/main.c $(SRC_DIR)/playground.c,$(wildcard $(SRC_DIR)/*.c))
TOOLKIT_SOURCES = $(wildcard $(SRC_DIR)/toolkit/*.c)
LIB_SOURCES ?= $(CORE_SOURCES) $(TOOLKIT_SOURCES)
CLI_SOURCES = $(wildcard $(CLI_ENTRY))
CLI_MODULE_SOURCES = $(wildcard $(SRC_DIR)/cli/*.c)
HAS_CLI = $(if $(strip $(CLI_SOURCES)),1,0)

LIB_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_SRC_DIR)/%.o,$(LIB_SOURCES))
CLI_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_SRC_DIR)/%.o,$(CLI_SOURCES) $(CLI_MODULE_SOURCES))

EXAMPLES_DIR = examples
EXAMPLE_SOURCES = $(wildcard $(EXAMPLES_DIR)/*.c)
EXAMPLE_BINS = $(patsubst $(EXAMPLES_DIR)/%.c,$(EXAMPLES_OBJ_DIR)/%,$(EXAMPLE_SOURCES))

# ---------------------------------------------------------
# Main targets
# ---------------------------------------------------------
all: directories $(TARGET) run

LIB_ARTIFACTS = $(STATIC_LIB)
ifeq ($(BUILD_SHARED),1)
LIB_ARTIFACTS += $(SHARED_LIB_LINK)
endif

lib: directories $(LIB_ARTIFACTS)

cli: directories $(TARGET)

directories:
	@mkdir -p $(OBJ_SRC_DIR) $(OBJ_TEST_DIR) $(BIN_DIR) $(LIB_BUILD_DIR) $(EXAMPLES_OBJ_DIR) $(OUTPUT_DIR) $(LOG_DIR)

$(STATIC_LIB): $(LIB_OBJECTS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $(LIB_OBJECTS)
	@$(RANLIB) $@ >/dev/null 2>&1 || true

$(SHARED_LIB_FULL): $(LIB_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SHARED_LINK_FLAGS) -o $(SHARED_LIB_FULL) $(LIB_OBJECTS) $(LDFLAGS) $(LDLIBS)

$(SHARED_LIB_SONAME): $(SHARED_LIB_FULL)
	@ln -sf $(notdir $(SHARED_LIB_FULL)) $(SHARED_LIB_SONAME)

$(SHARED_LIB_LINK): $(SHARED_LIB_SONAME)
	@ln -sf $(notdir $(SHARED_LIB_SONAME)) $(SHARED_LIB_LINK)

ifeq ($(HAS_CLI),1)
$(TARGET): $(STATIC_LIB) $(CLI_OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $(TARGET) $(CLI_OBJECTS) $(STATIC_LIB) $(LDFLAGS) $(LDLIBS)
else
$(TARGET):
	@echo "No CLI entry configured. Set CLI_ENTRY=<path to .c with main()>."
	@false
endif

run: $(TARGET)
	@mkdir -p $(OUTPUT_DIR)
	./$(TARGET) >> $(OUTPUT) 2>&1

# ---------------------------------------------------------
# Examples
# ---------------------------------------------------------
examples: directories $(EXAMPLE_BINS)

$(EXAMPLES_OBJ_DIR)/%: $(EXAMPLES_DIR)/%.c $(STATIC_LIB)
	@mkdir -p $(EXAMPLES_OBJ_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(STATIC_LIB) $(LDFLAGS) $(LDLIBS)

EX ?=
ARGS ?=
run-example: examples
	@test -n "$(EX)" || (echo "Set EX=<example_name> (without path)" && exit 2)
	./$(EXAMPLES_OBJ_DIR)/$(EX) $(ARGS)

# ---------------------------------------------------------
# Pattern rules
# ---------------------------------------------------------
$(OBJ_SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_TEST_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# ---------------------------------------------------------
# Split make fragments
# ---------------------------------------------------------
include mk/docs.mk
include mk/tests.mk
include mk/modes.mk
include mk/install.mk
include mk/package.mk
include mk/help.mk

# Include generated dependencies after all source sets are declared
-include $(shell find $(OBJ_DIR) -name "*.d" 2>/dev/null)

PHONY_TARGETS += all run directories lib cli examples run-example
.PHONY: $(PHONY_TARGETS)

.DELETE_ON_ERROR:
