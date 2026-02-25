# ---------------------------------------------------------
# Documentation (Doxygen + LaTeX PDF)
# ---------------------------------------------------------
DOXYFILE ?= Doxyfile
DOCS_DIR = docs
DOCS_PDF = $(DOCS_DIR)/userManual.pdf
DOCS_LOG_DIR = $(DOCS_DIR)/logs
DOCS_DOXY_LOG = $(DOCS_LOG_DIR)/doxygen.log
DOCS_LATEX_LOG = $(DOCS_LOG_DIR)/latex.log

userManual:
	@mkdir -p $(DOCS_LOG_DIR)
	@tmp_docs_dir=$$(mktemp -d /tmp/izprime-docs.XXXXXX); \
	tmp_doxy_cfg="$$tmp_docs_dir/Doxyfile.tmp"; \
	trap 'rm -rf "$$tmp_docs_dir"' EXIT INT TERM; \
	printf '@INCLUDE = %s/%s\nOUTPUT_DIRECTORY = %s\nFULL_PATH_NAMES = NO\n' "$(CURDIR)" "$(DOXYFILE)" "$$tmp_docs_dir" > "$$tmp_doxy_cfg"; \
	echo "Running Doxygen (log: $(DOCS_DOXY_LOG))..."; \
	doxygen "$$tmp_doxy_cfg" > $(DOCS_DOXY_LOG) 2>&1; \
	echo "Building LaTeX manual (log: $(DOCS_LATEX_LOG))..."; \
	$(MAKE) -C "$$tmp_docs_dir/latex" clean > $(DOCS_LATEX_LOG) 2>&1; \
	$(MAKE) -C "$$tmp_docs_dir/latex" >> $(DOCS_LATEX_LOG) 2>&1; \
	cp "$$tmp_docs_dir/latex/refman.pdf" $(DOCS_PDF)
	@echo "Generated $(DOCS_PDF)"

# Backward-compatible alias. Prefer `make userManual`.
docs: userManual

PHONY_TARGETS += userManual docs
