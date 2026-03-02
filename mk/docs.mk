# ---------------------------------------------------------
# Documentation (Doxygen + LaTeX PDF)
# ---------------------------------------------------------
DOXYFILE ?= Doxyfile
DOCS_DIR = docs
DOCS_PDF = $(DOCS_DIR)/userManual.pdf
DOCS_LOG_DIR = $(DOCS_DIR)/logs
DOCS_DOXY_LOG = $(DOCS_LOG_DIR)/doxygen.log
DOCS_LATEX_LOG = $(DOCS_LOG_DIR)/latex.log
PSEUDOCODE_SRC_DIR = $(DOCS_DIR)/latex_src
PSEUDOCODE_TEX = $(PSEUDOCODE_SRC_DIR)/pseudocode.tex
PSEUDOCODE_BUILD_PDF = $(PSEUDOCODE_SRC_DIR)/pseudocode.pdf
PSEUDOCODE_PDF = $(DOCS_DIR)/pseudocode.pdf
PSEUDOCODE_LOG = $(DOCS_LOG_DIR)/pseudocode.log

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

pseudocode:
	@mkdir -p $(DOCS_LOG_DIR)
	@test -f $(PSEUDOCODE_TEX) || { echo "Missing $(PSEUDOCODE_TEX)"; exit 1; }
	@if command -v latexmk >/dev/null 2>&1; then \
		echo "Building pseudocode PDF with latexmk (log: $(PSEUDOCODE_LOG))..."; \
		cd $(PSEUDOCODE_SRC_DIR) && latexmk -pdf -interaction=nonstopmode -halt-on-error pseudocode.tex > $(abspath $(PSEUDOCODE_LOG)) 2>&1; \
	else \
		echo "latexmk not found. Falling back to pdflatex (log: $(PSEUDOCODE_LOG))..."; \
		cd $(PSEUDOCODE_SRC_DIR) && pdflatex -interaction=nonstopmode -halt-on-error pseudocode.tex > $(abspath $(PSEUDOCODE_LOG)) 2>&1; \
		cd $(PSEUDOCODE_SRC_DIR) && pdflatex -interaction=nonstopmode -halt-on-error pseudocode.tex >> $(abspath $(PSEUDOCODE_LOG)) 2>&1; \
	fi
	@cp $(PSEUDOCODE_BUILD_PDF) $(PSEUDOCODE_PDF)
	@echo "Generated $(PSEUDOCODE_PDF)"

PHONY_TARGETS += userManual docs pseudocode
