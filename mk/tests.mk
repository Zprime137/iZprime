# ---------------------------------------------------------
# Test runner API
# ---------------------------------------------------------
# Usage examples:
#   make test-all
#   make test-unit verbose
#   make -- test-unit --verbose
#   make test-integration TEST_OUTPUT=output/integration.txt
#   make benchmark-p_sieve save-results
#   make benchmark-p_gen plot
#   make -- benchmark-p_sieve --save-results --plot

NEW_TEST_DIR = $(TEST_DIR)
# Include test_all.c and individual test files, but exclude any legacy tests.c
NEW_TEST_SOURCES = $(shell find $(NEW_TEST_DIR) -name "*.c" ! -name "tests.c")
NEW_TEST_OBJECTS = $(patsubst $(NEW_TEST_DIR)/%.c, $(OBJ_DIR)/$(NEW_TEST_DIR)/%.o, $(NEW_TEST_SOURCES))

# Legacy test sources under test/*.c (kept for backward compatibility).
LEGACY_TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
LEGACY_TEST_OBJECTS = $(patsubst $(TEST_DIR)/%.c, $(OBJ_TEST_DIR)/%.o, $(LEGACY_TEST_SOURCES))

VERBOSE ?= 0
SAVE_RESULTS ?= 0
PLOT ?= 0
TEST_OUTPUT ?=
TEST_ARGS ?=
PYTHON ?= python3
PLOT_ENTRYPOINT = py_tools/plot_results.py

CLI_FLAG_TARGETS = verbose save-results save_results plot --verbose --save-results --save_results --plot

ifneq (,$(filter verbose --verbose,$(MAKECMDGOALS)))
VERBOSE := 1
endif

ifneq (,$(filter save-results save_results --save-results --save_results,$(MAKECMDGOALS)))
SAVE_RESULTS := 1
endif

ifneq (,$(filter plot --plot,$(MAKECMDGOALS)))
PLOT := 1
SAVE_RESULTS := 1
endif

TEST_RUNNER = $(OBJ_DIR)/$(NEW_TEST_DIR)/test_runner
TEST_ARGS_COMMON = $(if $(filter 1,$(VERBOSE)),--verbose,) \
			   $(if $(filter 1,$(SAVE_RESULTS)),--save-results,) \
			   $(TEST_ARGS)

define RUN_TEST
	@echo "Running: $(TEST_RUNNER) $(1) $(TEST_ARGS_COMMON)"
	$(if $(strip $(TEST_OUTPUT)),./$(TEST_RUNNER) $(1) $(TEST_ARGS_COMMON) > $(TEST_OUTPUT) 2>&1,./$(TEST_RUNNER) $(1) $(TEST_ARGS_COMMON))
endef

define RUN_BENCHMARK
	@echo "Running: $(TEST_RUNNER) $(1) $(TEST_ARGS_COMMON)"
	@tmp_file=$$(mktemp); \
	status_file=$$(mktemp); \
	( ./$(TEST_RUNNER) $(1) $(TEST_ARGS_COMMON); echo $$? > "$$status_file" ) 2>&1 | tee "$$tmp_file"; \
	status=$$(cat "$$status_file"); \
	rm -f "$$status_file"; \
	if [ -n "$(strip $(TEST_OUTPUT))" ]; then \
		cp "$$tmp_file" "$(TEST_OUTPUT)"; \
	fi; \
	if [ $$status -ne 0 ]; then \
		rm -f "$$tmp_file"; \
		exit $$status; \
	fi; \
	if [ "$(PLOT)" = "1" ]; then \
		result_paths=$$(grep '^RESULTS_FILE:' "$$tmp_file" | sed 's/^RESULTS_FILE:[[:space:]]*//'); \
		if [ -z "$$result_paths" ]; then \
			echo "No results files detected in benchmark output; skipping plot generation."; \
		else \
			for result_path in $$result_paths; do \
				echo "Plotting $$result_path"; \
				$(PYTHON) $(PLOT_ENTRYPOINT) "$$result_path" --save --no-show || exit $$?; \
			done; \
		fi; \
	fi; \
	rm -f "$$tmp_file"
endef

# Compile new test files
$(OBJ_DIR)/$(NEW_TEST_DIR)/%.o: $(NEW_TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# CLI-flag pseudo-targets (usable as: make test-all verbose)
$(CLI_FLAG_TARGETS):
	@:

# Build test runner once and reuse for all test/benchmark targets
$(TEST_RUNNER): directories $(STATIC_LIB) $(NEW_TEST_OBJECTS)
	@mkdir -p $(OBJ_DIR)/$(NEW_TEST_DIR)
	@echo "Building test runner..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $(TEST_RUNNER) $(NEW_TEST_OBJECTS) $(STATIC_LIB) $(LDFLAGS) $(LDLIBS)

# Build and run new test framework
test-all: $(TEST_RUNNER)
	@echo "Running tests..."
	$(call RUN_TEST,--all)

test-unit: $(TEST_RUNNER)
	@echo "Running unit tests..."
	$(call RUN_TEST,--unit)

test-integration: $(TEST_RUNNER)
	@echo "Running integration tests..."
	$(call RUN_TEST,--integration)

# Dedicated benchmark drivers
benchmark-p_sieve: $(TEST_RUNNER)
	@echo "Running prime sieve model benchmarks..."
	@echo "Warning: Benchmarks may take time and could have memory issues"
	$(call RUN_BENCHMARK,--benchmark-p-sieve)

benchmark-p_gen: $(TEST_RUNNER)
	@echo "Running random prime generation benchmarks..."
	@echo "Warning: Benchmarks may take time and could have memory issues"
	$(call RUN_BENCHMARK,--benchmark-p-gen)

benchmark-SiZ_count: $(TEST_RUNNER)
	@echo "Running SiZ_count benchmarks..."
	@echo "Warning: Benchmark may take time"
	$(call RUN_BENCHMARK,--benchmark-siz-count)

# Hyphen aliases
benchmark-p-sieve: benchmark-p_sieve
benchmark-p-gen: benchmark-p_gen
benchmark-siz-count: benchmark-SiZ_count
benchmark-SiZ-count: benchmark-SiZ_count

# Legacy test support (backward compatibility)
test: directories $(STATIC_LIB) $(LEGACY_TEST_OBJECTS)
	@mkdir -p $(OBJ_TEST_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $(OBJ_TEST_DIR)/test_runner $(LEGACY_TEST_OBJECTS) $(STATIC_LIB) $(LDFLAGS) $(LDLIBS)
	./$(OBJ_TEST_DIR)/test_runner

PHONY_TARGETS += test test-all test-unit test-integration benchmark-p_sieve benchmark-p_gen benchmark-SiZ_count benchmark-p-sieve benchmark-p-gen benchmark-siz-count benchmark-SiZ-count $(CLI_FLAG_TARGETS)
