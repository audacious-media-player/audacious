# Shut up GNU make
.SILENT:

SUBDIRS = none
CFLAGS += -DHAVE_CONFIG_H

default: all
all: build

install:
	$(MAKE) install-prehook
	@for i in $(OBJECTIVE_DIRECTORIES); do \
		printf "%10s     %-20s\n" MKDIR $$i; \
		$(INSTALL) -d -m 755 $(DESTDIR)/$$i; \
	done
	@for i in $(OBJECTIVE_LIBS); do \
		printf "%10s     %-20s\n" INSTALL $$i; \
		$(INSTALL) $(INSTALL_OVERRIDE) $(DESTDIR)/$(LIBDIR)/$(LIB_SUFFIX)/$$i; \
	done
	@for i in $(OBJECTIVE_BINS); do \
		printf "%10s     %-20s\n" INSTALL $$i; \
		$(INSTALL) $(INSTALL_OVERRIDE) $(DESTDIR)/$(BINDIR)/$(LIB_SUFFIX)/$$i; \
	@done
	$(MAKE) install-posthook
	@echo "[all objectives installed]"

clean:
	$(MAKE) clean-prehook
	@if test "$(SUBDIRS)" != "none"; then \
		for i in $(SUBDIRS); do \
			echo "[cleaning subobjective: $$i]"; \
			(cd $$i; $(MAKE) clean; cd ..); \
		done; \
	fi
	$(MAKE) clean-posthook
	$(RM) *.o *.lo *.so *.a *.sl
	@echo "[all objectives cleaned]"

distclean: clean
	$(RM) mk/rules.mk

build:
	$(MAKE) build-prehook
	@if test "$(SUBDIRS)" != "none"; then \
		for i in $(SUBDIRS); do \
			echo "[building subobjective: $$i]"; \
			(cd $$i; $(MAKE); cd ..); \
		done; \
	fi
	@for i in $(OBJECTIVE_LIBS); do \
		echo "[building library objective: $$i]"; \
		$(MAKE) $$i; \
	done
	@for i in $(OBJECTIVE_BINS); do \
		echo "[building binary objective: $$i]"; \
		$(MAKE) $$i; \
	done
	$(MAKE) build-posthook
	@echo "[all objectives built]"

.c.o:
	printf "%10s     %-20s\n" CC $<
	$(CC) $(CFLAGS) -c $< -o $@

.cc.o:
	printf "%10s     %-20s\n" CXX $<;
	$(CXX) $(CXXFLAGS) -c $< -o $@

.cpp.o:
	printf "%10s     %-20s\n" CXX $<;
	$(CXX) $(CXXFLAGS) -c $< -o $@

.cxx.o:
	printf "%10s     %-20s\n" CXX $<;
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean-prehook:
clean-posthook:
build-prehook:
build-posthook:
install-prehook:
install-posthook:

# compatibility with automake follows
am--refresh:
