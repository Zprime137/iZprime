# Clean generated files
clean:
	rm -rf $(OBJ_DIR) $(OUTPUT_DIR) $(LOG_DIR)

# Build with debug flags
debug: LOGGING = 1
debug: CFLAGS_OPT = -O0
debug: CFLAGS_DBG = -g
debug: $(TARGET)

# Build optimized release version without logging
release: LOGGING = 0
release: CFLAGS_OPT = $(OPT_LEVEL) $(LTO_FLAGS) $(ARCH_FLAGS) $(SECTION_FLAGS)
release: CFLAGS_DBG =
release: CPPFLAGS += $(if $(filter 1,$(ENABLE_NDEBUG)),-DNDEBUG,)
release: $(TARGET)

# PGO two-step build flow
PGO_DIR ?= $(OBJ_DIR)/pgo

pgo-generate: LOGGING = 0
pgo-generate: CFLAGS_OPT = $(OPT_LEVEL) $(LTO_FLAGS) $(ARCH_FLAGS) $(SECTION_FLAGS) -fprofile-generate=$(PGO_DIR)
pgo-generate: CFLAGS_DBG =
pgo-generate: CPPFLAGS += $(if $(filter 1,$(ENABLE_NDEBUG)),-DNDEBUG,)
pgo-generate: LDFLAGS += -fprofile-generate=$(PGO_DIR)
pgo-generate: clean $(TARGET)

pgo-use: LOGGING = 0
pgo-use: CFLAGS_OPT = $(OPT_LEVEL) $(LTO_FLAGS) $(ARCH_FLAGS) $(SECTION_FLAGS) -fprofile-use=$(PGO_DIR) -fprofile-correction
pgo-use: CFLAGS_DBG =
pgo-use: CPPFLAGS += $(if $(filter 1,$(ENABLE_NDEBUG)),-DNDEBUG,)
pgo-use: LDFLAGS += -fprofile-use=$(PGO_DIR)
pgo-use: $(TARGET)

PHONY_TARGETS += clean debug release pgo-generate pgo-use
