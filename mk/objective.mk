# Shut up GNU make
.SILENT:

OBJECTIVE_DIRECTORIES = none
OBJECTIVE_LIBS = none
OBJECTIVE_LIBS_NOINST = none
OBJECTIVE_BINS = none
OBJECTIVE_DATA = none
SUBDIRS = none
HEADERS = none
VERBOSITY = 0
SHOW_CFLAGS = 0

ifeq ($(SHOW_CFLAGS),0)
ifeq ($(V),1)
SHOW_CFLAGS = 1
endif
ifeq ($(VERBOSE),1)
SHOW_CFLAGS = 1
endif
endif

LIBDIR = $(libdir)
BINDIR = $(bindir)
INCLUDEDIR = $(pkgincludedir)
CFLAGS += -DHAVE_CONFIG_H -I/usr/pkg/include -I/usr/pkg/xorg/include
CXXFLAGS += -DHAVE_CONFIG_H -I/usr/pkg/include -I/usr/pkg/xorg/include

default: all
all: build

install: build
	$(MAKE) install-prehook
	@for i in $(BINDIR) $(LIBDIR) $(INCLUDEDIR); do \
		if test ! -d $(DESTDIR)/$$i; then \
			$(INSTALL) -d -m 755 $(DESTDIR)/$$i; \
		fi; \
	done;
	@if test "$(SUBDIRS)" != "none"; then \
		for i in $(SUBDIRS); do \
			if test $(VERBOSITY) -gt 0; then \
				echo "[installing subobjective: $$i]"; \
			fi; \
			(cd $$i; $(MAKE) install || exit; cd ..); \
		done; \
	fi
	@if test "$(OBJECTIVE_DIRECTORIES)" != "none"; then \
		for i in $(OBJECTIVE_DIRECTORIES); do \
			printf "%10s     %-20s\n" MKDIR $$i; \
			$(INSTALL) -d -m 755 $(DESTDIR)/$$i; \
		done; \
	fi
	@if test "$(HEADERS)" != "none"; then \
		for i in $(HEADERS); do \
			printf "%10s     %-20s\n" INSTALL $$i; \
			$(INSTALL_DATA) $(INSTALL_OVERRIDE) $$i $(DESTDIR)/$(INCLUDEDIR)/$$i; \
		done; \
	fi
	@if test "$(OBJECTIVE_LIBS)" != "none"; then \
		for i in $(OBJECTIVE_LIBS); do \
			printf "%10s     %-20s\n" INSTALL $$i; \
			$(INSTALL) $(INSTALL_OVERRIDE) $$i $(DESTDIR)/$(LIBDIR)/$$i; \
		done; \
	fi
	@if test "$(OBJECTIVE_BINS)" != "none"; then \
		for i in $(OBJECTIVE_BINS); do \
			printf "%10s     %-20s\n" INSTALL $$i; \
			$(INSTALL) $(INSTALL_OVERRIDE) $$i $(DESTDIR)/$(BINDIR)/$$i; \
		done; \
	fi;
	@if test "$(OBJECTIVE_DATA)" != "none"; then \
		for i in $(OBJECTIVE_DATA); do \
			source=`echo $$i | cut -d ":" -f1`; \
			destination=`echo $$i | cut -d ":" -f2`; \
			if test ! -d $(DESTDIR)/$$destination; then \
				$(INSTALL) -d -m 755 $(DESTDIR)/$$destination; \
			fi; \
			printf "%10s     %-20s\n" INSTALL $$source; \
			$(INSTALL_DATA) $(INSTALL_OVERRIDE) $$source $(DESTDIR)/$$destination; \
		done; \
	fi
	$(MAKE) install-posthook
	@if test $(VERBOSITY) -gt 0; then \
		echo "[all objectives installed]"; \
	fi

clean:
	$(MAKE) clean-prehook
	@if test "$(SUBDIRS)" != "none"; then \
		for i in $(SUBDIRS); do \
			if test $(VERBOSITY) -gt 0; then \
				echo "[cleaning subobjective: $$i]"; \
			fi; \
			(cd $$i; $(MAKE) clean || exit; cd ..); \
		done; \
	fi
	$(MAKE) clean-posthook
	$(RM) *.o *.lo *.so *.a *.sl
	@if test "$(OBJECTIVE_BINS)" != "none"; then \
		for i in $(OBJECTIVE_BINS); do \
			$(RM) $$i; \
		done; \
	fi
	@if test "$(OBJECTIVE_LIBS)" != "none"; then \
		for i in $(OBJECTIVE_LIBS); do \
			$(RM) $$i; \
		done; \
	fi
	@if test "$(OBJECTIVE_LIBS_NOINST)" != "none"; then \
		for i in $(OBJECTIVE_LIBS_NOINST); do \
			$(RM) $$i; \
		done; \
	fi
	@if test $(VERBOSITY) -gt 0; then \
		echo "[all objectives cleaned]"; \
	fi

distclean: clean
	@if test "$(SUBDIRS)" != "none"; then \
		for i in $(SUBDIRS); do \
			if test $(VERBOSITY) -gt 0; then \
				echo "[distcleaning subobjective: $$i]"; \
			fi; \
			(cd $$i; $(MAKE) distclean || exit; cd ..); \
		done; \
	fi
	@if test -f Makefile.in; then \
		$(RM) -f Makefile; \
	fi
	@if test -f mk/rules.mk; then \
		$(RM) -f mk/rules.mk; \
	fi

build:
	$(MAKE) build-prehook
	@if test "$(SUBDIRS)" != "none"; then \
		for i in $(SUBDIRS); do \
			if test $(VERBOSITY) -gt 0; then \
				echo "[building subobjective: $$i]"; \
			fi; \
			cd $$i; $(MAKE) || exit; cd ..; \
			if test $(VERBOSITY) -gt 0; then \
				echo "[finished subobjective: $$i]"; \
			fi; \
		done; \
	fi
	@if test "$(OBJECTIVE_LIBS)" != "none"; then \
		for i in $(OBJECTIVE_LIBS); do \
			if test $(VERBOSITY) -gt 0; then \
				echo "[building library objective: $$i]"; \
			fi; \
			$(MAKE) $$i || exit; \
			if test $(VERBOSITY) -gt 0; then \
				echo "[finished library objective: $$i]"; \
			fi; \
		done; \
	fi
	@if test "$(OBJECTIVE_LIBS_NOINST)" != "none"; then \
		for i in $(OBJECTIVE_LIBS_NOINST); do \
			if test $(VERBOSITY) -gt 0; then \
				echo "[building library dependency: $$i]"; \
			fi; \
			$(MAKE) $$i || exit; \
			if test $(VERBOSITY) -gt 0; then \
				echo "[finished library dependency: $$i]"; \
			fi; \
		done; \
	fi
	@if test "$(OBJECTIVE_BINS)" != "none"; then \
		for i in $(OBJECTIVE_BINS); do \
			if test $(VERBOSITY) -gt 0; then \
				echo "[building binary objective: $$i]"; \
			fi; \
			$(MAKE) $$i || exit; \
			if test $(VERBOSITY) -gt 0; then \
				echo "[finished binary objective: $$i]"; \
			fi; \
		done; \
	fi
	$(MAKE) build-posthook
	@if test $(VERBOSITY) -gt 0; then \
		echo "[all objectives built]"; \
	fi

.c.o:
	if test $(SHOW_CFLAGS) -eq 1; then	\
		printf "%10s     %-20s (%s)\n" CC $< "${CFLAGS}";	\
	else \
		printf "%10s     %-20s\n" CC $<;	\
	fi;
	$(CC) $(CFLAGS) -c $< -o $@

.cc.o .cpp.o .cxx.o:
	if test $(SHOW_CFLAGS) -eq 1; then	\
		printf "%10s     %-20s (%s)\n" CXX $< "${CXXFLAGS}";	\
	else \
		printf "%10s     %-20s\n" CXX $<;	\
	fi;
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.so: $(OBJECTS)
	if test "x$(OBJECTS)" != "x"; then \
		$(MAKE) $(OBJECTS) || exit;		\
		printf "%10s     %-20s\n" LINK $@; \
		(if test "x$(OBJECTIVE_SONAME_SUFFIX)" != "x"; then \
			$(CC) -fPIC -DPIC -shared -o $@ -Wl,-soname=$@.$(OBJECTIVE_SONAME_SUFFIX) $(OBJECTS) $(LDFLAGS) $(LIBADD); \
		else \
			$(CC) -fPIC -DPIC -shared -o $@ -Wl,-soname=$@ $(OBJECTS) $(LDFLAGS) $(LIBADD); \
		fi;) \
	fi

%.a: $(OBJECTS)
	if test "x$(OBJECTS)" != "x"; then \
		$(MAKE) $(OBJECTS) || exit;		\
		printf "%10s     %-20s\n" LINK $@; \
		$(AR) cq $@ $(OBJECTS); \
	fi

$(OBJECTIVE_BINS): $(OBJECTS)
	if test "x$(OBJECTS)" != "x"; then \
		$(MAKE) $(OBJECTS) || exit;		\
		printf "%10s     %-20s\n" LINK $@; \
		$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBADD); \
	fi

clean-prehook:
clean-posthook:
build-prehook:
build-posthook:
install-prehook:
install-posthook:

# compatibility with automake follows
am--refresh:
