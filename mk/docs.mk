# ---------------------------------------------------------
# Documentation (Doxygen + LaTeX PDF)
# ---------------------------------------------------------
DOXYFILE ?= Doxyfile
DOCS_DIR = docs
DOCS_LATEX_DIR = $(DOCS_DIR)/api/latex
DOCS_PDF = $(DOCS_DIR)/userManual.pdf
DOCS_LOG_DIR = $(DOCS_DIR)/logs
DOCS_DOXY_LOG = $(DOCS_LOG_DIR)/doxygen.log
DOCS_LATEX_LOG = $(DOCS_LOG_DIR)/latex.log

userManual:
	@mkdir -p $(DOCS_LOG_DIR)
	@echo "Running Doxygen (log: $(DOCS_DOXY_LOG))..."
	@doxygen $(DOXYFILE) > $(DOCS_DOXY_LOG) 2>&1
	@echo "Building LaTeX manual (log: $(DOCS_LATEX_LOG))..."
	@$(MAKE) -C $(DOCS_LATEX_DIR) clean > $(DOCS_LATEX_LOG) 2>&1
	@$(MAKE) -C $(DOCS_LATEX_DIR) >> $(DOCS_LATEX_LOG) 2>&1
	@cp $(DOCS_LATEX_DIR)/refman.pdf $(DOCS_PDF)
	@echo "Generated $(DOCS_PDF)"

# Backward-compatible alias. Prefer `make userManual`.
docs: userManual

PHONY_TARGETS += userManual docs
