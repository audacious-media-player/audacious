default: all
all: build

.SUFFIXES: .cxx .cc

install: build
	$(MAKE) install-prehook
	@for i in $(BINDIR) $(LIBDIR) $(INCLUDEDIR); do \
		if [ ! -d $(DESTDIR)/$$i ]; then \
			$(INSTALL) -d -m 755 $(DESTDIR)/$$i; \
		fi; \
	done;
	@if [ "x$(SUBDIRS)" != "x" ]; then \
		for i in $(SUBDIRS); do \
			if [ $(VERBOSITY) -gt 0 ]; then \
				echo "[installing subobjective: $$i]"; \
			fi; \
			(cd $$i; $(MAKE) install || exit; cd ..); \
		done; \
	fi
	@if [ "x$(OBJECTIVE_DIRECTORIES)" != "x" ]; then \
		for i in $(OBJECTIVE_DIRECTORIES); do \
			printf "%10s     %-20s\n" MKDIR $$i; \
			$(INSTALL) -d -m 755 $(DESTDIR)/$$i; \
		done; \
	fi
	@if [ "x$(HEADERS)" != "x" ]; then \
		for i in $(HEADERS); do \
			printf "%10s     %-20s\n" INSTALL $$i; \
			$(INSTALL_DATA) $(INSTALL_OVERRIDE) $$i $(DESTDIR)/$(INCLUDEDIR)/$$i; \
		done; \
	fi
	@if [ "x$(OBJECTIVE_LIBS)" != "x" ]; then \
		for i in $(OBJECTIVE_LIBS); do \
			printf "%10s     %-20s\n" INSTALL $$i; \
			$(INSTALL) $(INSTALL_OVERRIDE) $$i $(DESTDIR)/$(LIBDIR)/$$i; \
		done; \
	fi
	@if [ "x$(OBJECTIVE_BINS)" != "x" ]; then \
		for i in $(OBJECTIVE_BINS); do \
			printf "%10s     %-20s\n" INSTALL $$i; \
			$(INSTALL) $(INSTALL_OVERRIDE) $$i $(DESTDIR)/$(BINDIR)/$$i; \
		done; \
	fi;
	@if [ "x$(OBJECTIVE_DATA)" != "x" ]; then \
		for i in $(OBJECTIVE_DATA); do \
			source=`echo $$i | cut -d ":" -f1`; \
			destination=`echo $$i | cut -d ":" -f2`; \
			if [ ! -d $(DESTDIR)/$$destination ]; then \
				$(INSTALL) -d -m 755 $(DESTDIR)/$$destination; \
			fi; \
			printf "%10s     %-20s\n" INSTALL $$source; \
			$(INSTALL_DATA) $(INSTALL_OVERRIDE) $$source $(DESTDIR)/$$destination; \
		done; \
	fi
	$(MAKE) install-posthook
	@if [ $(VERBOSITY) -gt 0 ]; then \
		echo "[all objectives installed]"; \
	fi

clean:
	$(MAKE) clean-prehook
	@if [ "x$(SUBDIRS)" != "x" ]; then \
		for i in $(SUBDIRS); do \
			if [ $(VERBOSITY) -gt 0 ]; then \
				echo "[cleaning subobjective: $$i]"; \
			fi; \
			(cd $$i; $(MAKE) clean || exit; cd ..); \
		done; \
	fi
	$(MAKE) clean-posthook
	rm -f *.o *.lo *.so *.a *.sl
	@if [ "x$(OBJECTIVE_BINS)" != "x" ]; then \
		for i in $(OBJECTIVE_BINS); do \
			rm -f $$i; \
		done; \
	fi
	@if [ "x$(OBJECTIVE_LIBS)" != "x" ]; then \
		for i in $(OBJECTIVE_LIBS); do \
			rm -f $$i; \
		done; \
	fi
	@if [ "x$(OBJECTIVE_LIBS_NOINST)" != "x" ]; then \
		for i in $(OBJECTIVE_LIBS_NOINST); do \
			rm -f $$i; \
		done; \
	fi
	@if [ $(VERBOSITY) -gt 0 ]; then \
		echo "[all objectives cleaned]"; \
	fi

distclean: clean
	@if [ "x$(SUBDIRS)" != "x" ]; then \
		for i in $(SUBDIRS); do \
			if [ $(VERBOSITY) -gt 0 ]; then \
				echo "[distcleaning subobjective: $$i]"; \
			fi; \
			(cd $$i; $(MAKE) distclean || exit; cd ..); \
		done; \
	fi
	@if [ -f Makefile.in ]; then \
		rm -f Makefile; \
	fi
	@if [ -f mk/rules.mk ]; then \
		rm -f mk/rules.mk; \
	fi

build:
	$(MAKE) build-prehook
	@if [ "x$(SUBDIRS)" != "x" ]; then \
		for i in $(SUBDIRS); do \
			if [ $(VERBOSITY) -gt 0 ]; then \
				echo "[building subobjective: $$i]"; \
			fi; \
			cd $$i; $(MAKE) || exit; cd ..; \
			if [ $(VERBOSITY) -gt 0 ]; then \
				echo "[finished subobjective: $$i]"; \
			fi; \
		done; \
	fi
	@if [ "x$(OBJECTIVE_LIBS)" != "x" ]; then \
		for i in $(OBJECTIVE_LIBS); do \
			if [ $(VERBOSITY) -gt 0 ]; then \
				echo "[building library objective: $$i]"; \
			fi; \
			$(MAKE) $$i || exit; \
			if [ $(VERBOSITY) -gt 0 ]; then \
				echo "[finished library objective: $$i]"; \
			fi; \
		done; \
	fi
	@if [ "x$(OBJECTIVE_LIBS_NOINST)" != "x" ]; then \
		for i in $(OBJECTIVE_LIBS_NOINST); do \
			if [ $(VERBOSITY) -gt 0 ]; then \
				echo "[building library dependency: $$i]"; \
			fi; \
			$(MAKE) $$i || exit; \
			if [ $(VERBOSITY) -gt 0 ]; then \
				echo "[finished library dependency: $$i]"; \
			fi; \
		done; \
	fi
	@if test "x$(OBJECTIVE_BINS)" != "x"; then \
		for i in $(OBJECTIVE_BINS); do \
			if [ $(VERBOSITY) -gt 0 ]; then \
				echo "[building binary objective: $$i]"; \
			fi; \
			$(MAKE) $$i || exit; \
			if [ $(VERBOSITY) -gt 0 ]; then \
				echo "[finished binary objective: $$i]"; \
			fi; \
		done; \
	fi
	$(MAKE) build-posthook
	@if [ $(VERBOSITY) -gt 0 ]; then \
		echo "[all objectives built]"; \
	fi

.c.o:
	@if [ $(SHOW_CFLAGS) -eq 1 ]; then	\
		printf "%10s     %-20s (%s)\n" CC $< "${CFLAGS}";	\
	else \
		printf "%10s     %-20s\n" CC $<;	\
	fi;
	$(CC) $(CFLAGS) -c $< -o $@

.cc.o .cxx.o:
	@if [ $(SHOW_CFLAGS) -eq 1 ]; then	\
		printf "%10s     %-20s (%s)\n" CXX $< "${CXXFLAGS}";	\
	else \
		printf "%10s     %-20s\n" CXX $<;	\
	fi;
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJECTIVE_LIBS): $(OBJECTS)
	if [ "x$(OBJECTS)" != "x" ]; then \
		$(MAKE) $(OBJECTS) || exit;		\
		printf "%10s     %-20s\n" LINK $@; \
		(if [ "x$(SHARED_SUFFIX)" = "xso" ]; then \
			(if [ "x$(OBJECTIVE_SONAME_SUFFIX)" != "x" ]; then \
				$(CC) $(PICLDFLAGS) -o $@ -Wl,-soname=$@.$(OBJECTIVE_SONAME_SUFFIX) $(OBJECTS) $(LDFLAGS) $(LIBADD); \
			else \
				$(CC) $(PICLDFLAGS) -o $@ -Wl,-soname=$@ $(OBJECTS) $(LDFLAGS) $(LIBADD); \
			fi;) \
		 else \
			$(CC) $(PICLDFLAGS) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBADD); \
		 fi;) \
	fi

%.a: $(OBJECTS)
	if [ "x$(OBJECTS)" != "x" ]; then \
		$(MAKE) $(OBJECTS) || exit;		\
		printf "%10s     %-20s\n" LINK $@; \
		$(AR) cq $@ $(OBJECTS); \
	fi

$(OBJECTIVE_BINS): $(OBJECTS)
	if [ "x$(OBJECTS)" != "x" ]; then \
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

mk/rules.mk:
	@if [ -f "configure" ]; then \
		echo "[building rules.mk for posix target, run configure manually if you do not want this]"; \
		sh configure $(CONFIG_OPTS); \
		echo "[complete]"; \
	fi

