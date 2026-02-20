# ---------------------------------------------------------
# Install / uninstall / pkg-config metadata
# ---------------------------------------------------------
PREFIX ?= /usr/local
DESTDIR ?=
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig
CLI_INSTALL_NAME ?= izprime
LIB_INSTALL_NAME ?= libizprime.a
PKGCONFIG_NAME ?= izprime.pc
INSTALL ?= install

PUBLIC_HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)
PKGCONFIG_TEMPLATE ?= packaging/izprime.pc.in
PKGCONFIG_BUILD_DIR = $(OBJ_DIR)/pkgconfig
PKGCONFIG_FILE = $(PKGCONFIG_BUILD_DIR)/$(PKGCONFIG_NAME)
INSTALL_LIB_DEPS = $(STATIC_LIB)
ifeq ($(BUILD_SHARED),1)
INSTALL_LIB_DEPS += $(SHARED_LIB_LINK)
endif

$(PKGCONFIG_FILE): $(PKGCONFIG_TEMPLATE)
	@mkdir -p $(PKGCONFIG_BUILD_DIR)
	@sed \
		-e 's|@PREFIX@|$(PREFIX)|g' \
		-e 's|@EXEC_PREFIX@|$${prefix}|g' \
		-e 's|@LIBDIR@|$${exec_prefix}/lib|g' \
		-e 's|@INCLUDEDIR@|$${prefix}/include|g' \
		-e 's|@VERSION@|$(VERSION)|g' \
		-e 's|@LIBS@|-lizprime $(THIRD_PARTY_LDLIBS)|g' \
		-e 's|@CFLAGS@|-I$${includedir}|g' \
		$(PKGCONFIG_TEMPLATE) > $(PKGCONFIG_FILE)

install: $(INSTALL_LIB_DEPS) $(TARGET) $(PKGCONFIG_FILE)
	$(INSTALL) -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCLUDEDIR) $(DESTDIR)$(PKGCONFIGDIR)
	$(INSTALL) -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(CLI_INSTALL_NAME)
	$(INSTALL) -m 644 $(STATIC_LIB) $(DESTDIR)$(LIBDIR)/$(LIB_INSTALL_NAME)
	@if [ "$(BUILD_SHARED)" = "1" ]; then \
		$(INSTALL) -m 755 $(SHARED_LIB_FULL) $(DESTDIR)$(LIBDIR)/$(notdir $(SHARED_LIB_FULL)); \
		ln -sf $(notdir $(SHARED_LIB_FULL)) $(DESTDIR)$(LIBDIR)/$(notdir $(SHARED_LIB_SONAME)); \
		ln -sf $(notdir $(SHARED_LIB_SONAME)) $(DESTDIR)$(LIBDIR)/$(notdir $(SHARED_LIB_LINK)); \
	fi
	$(INSTALL) -m 644 $(PUBLIC_HEADERS) $(DESTDIR)$(INCLUDEDIR)/
	$(INSTALL) -m 644 $(PKGCONFIG_FILE) $(DESTDIR)$(PKGCONFIGDIR)/$(PKGCONFIG_NAME)

install-lib: $(INSTALL_LIB_DEPS) $(PKGCONFIG_FILE)
	$(INSTALL) -d $(DESTDIR)$(LIBDIR) $(DESTDIR)$(INCLUDEDIR) $(DESTDIR)$(PKGCONFIGDIR)
	$(INSTALL) -m 644 $(STATIC_LIB) $(DESTDIR)$(LIBDIR)/$(LIB_INSTALL_NAME)
	@if [ "$(BUILD_SHARED)" = "1" ]; then \
		$(INSTALL) -m 755 $(SHARED_LIB_FULL) $(DESTDIR)$(LIBDIR)/$(notdir $(SHARED_LIB_FULL)); \
		ln -sf $(notdir $(SHARED_LIB_FULL)) $(DESTDIR)$(LIBDIR)/$(notdir $(SHARED_LIB_SONAME)); \
		ln -sf $(notdir $(SHARED_LIB_SONAME)) $(DESTDIR)$(LIBDIR)/$(notdir $(SHARED_LIB_LINK)); \
	fi
	$(INSTALL) -m 644 $(PUBLIC_HEADERS) $(DESTDIR)$(INCLUDEDIR)/
	$(INSTALL) -m 644 $(PKGCONFIG_FILE) $(DESTDIR)$(PKGCONFIGDIR)/$(PKGCONFIG_NAME)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(CLI_INSTALL_NAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB_INSTALL_NAME)
	rm -f $(DESTDIR)$(LIBDIR)/$(notdir $(SHARED_LIB_LINK))
	rm -f $(DESTDIR)$(LIBDIR)/$(notdir $(SHARED_LIB_SONAME))
	rm -f $(DESTDIR)$(LIBDIR)/$(notdir $(SHARED_LIB_FULL))
	rm -f $(DESTDIR)$(PKGCONFIGDIR)/$(PKGCONFIG_NAME)
	rm -f $(addprefix $(DESTDIR)$(INCLUDEDIR)/,$(notdir $(PUBLIC_HEADERS)))

PHONY_TARGETS += install install-lib uninstall
