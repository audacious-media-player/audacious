include mk/rules.mk

# Shut up GNU make
.SILENT:

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

build:
	$(MAKE) build-prehook
	@for i in $(SUBDIRS); do \
		cd $$i; $(MAKE) build; cd .. \
	done
	@for i in $(OBJECTIVE_LIBS); do \
		$(MAKE) $$i; \
	done
	@for i in $(OBJECTIVE_BINS); do \
		$(MAKE) $$i; \
	done
	$(MAKE) build-posthook

.c.o:
	printf "%10s     %-20s\n" CC $$i;
	$(CC) $(CFLAGS) -c $< -o $@

.cc.o:
	printf "%10s     %-20s\n" CXX $$i;
	$(CXX) $(CXXFLAGS) -c $< -o $@

.cpp.o:
	printf "%10s     %-20s\n" CXX $$i;
	$(CXX) $(CXXFLAGS) -c $< -o $@

.cxx.o:
	printf "%10s     %-20s\n" CXX $$i;
	$(CXX) $(CXXFLAGS) -c $< -o $@

