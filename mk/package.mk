# ---------------------------------------------------------
# Packaging and release helpers
# ---------------------------------------------------------
DIST_DIR ?= dist
DIST_NAME ?= izprime-$(VERSION)
DIST_ARCHIVE ?= $(DIST_DIR)/$(DIST_NAME).tar.gz
DIST_SHA256 ?= $(DIST_ARCHIVE).sha256

HAVE_SHASUM := $(shell command -v shasum >/dev/null 2>&1 && echo 1)
ifeq ($(HAVE_SHASUM),1)
SHA256_CMD = shasum -a 256
else
SHA256_CMD = sha256sum
endif

doctor:
	@echo "Checking build dependencies..."
	@command -v $(CC_BASE) >/dev/null 2>&1 || (echo "Missing compiler: $(CC_BASE)" && exit 1)
	@if [ "$(USE_PKG_CONFIG)" = "1" ]; then \
		if command -v $(PKG_CONFIG) >/dev/null 2>&1; then \
			$(PKG_CONFIG) --exists gmp || (echo "Missing GMP development files."; exit 1); \
			($(PKG_CONFIG) --exists openssl || $(PKG_CONFIG) --exists libssl libcrypto) || (echo "Missing OpenSSL development files."; exit 1); \
		else \
			echo "pkg-config not found: using fallback library discovery."; \
		fi; \
	fi
	@tmp_src=$$(mktemp /tmp/izprime_doctor_XXXX.c); \
	tmp_bin=$$(mktemp /tmp/izprime_doctor_XXXX); \
	printf '%s\n' '#include <gmp.h>' '#include <openssl/sha.h>' 'int main(void) { return 0; }' > "$$tmp_src"; \
	$(CC_BASE) $(THREAD_FLAGS) $(THIRD_PARTY_CFLAGS) "$$tmp_src" -o "$$tmp_bin" $(THIRD_PARTY_LDFLAGS) $(THIRD_PARTY_LDLIBS) >/dev/null 2>&1 || { \
		echo "Compiler smoke test failed: GMP/OpenSSL headers or libs are not usable."; \
		rm -f "$$tmp_src" "$$tmp_bin"; \
		exit 1; \
	}; \
	rm -f "$$tmp_src" "$$tmp_bin"
	@echo "Dependency check passed."
	@echo "Tips:"
	@echo "  macOS:  brew install gcc make pkg-config gmp openssl@3"
	@echo "  Ubuntu: sudo apt-get install build-essential make pkg-config libgmp-dev libssl-dev"

dist:
	@mkdir -p $(DIST_DIR)
	@tmp_root=$$(mktemp -d /tmp/izprime_dist_XXXXXX); \
	stage_dir="$$tmp_root/$(DIST_NAME)"; \
	mkdir -p "$$stage_dir"; \
	rsync -a \
		--exclude='.git/' \
		--exclude='build/' \
		--exclude='dist/' \
		--exclude='logs/' \
		--exclude='output/' \
		--exclude='docs/api/' \
		--exclude='py_tools/__pycache__/' \
		--exclude='.vscode/' \
		--exclude='.DS_Store' \
		./ "$$stage_dir/"; \
	tar -czf $(DIST_ARCHIVE) -C "$$tmp_root" "$(DIST_NAME)"; \
	rm -rf "$$tmp_root"
	@$(SHA256_CMD) $(DIST_ARCHIVE) > $(DIST_SHA256)
	@echo "Created: $(DIST_ARCHIVE)"
	@echo "SHA256:  $(DIST_SHA256)"

PHONY_TARGETS += doctor dist
