## Process this file with automake to produce Makefile.in

if HAVE_LIBVISUAL

lib_LTLIBRARIES = libvisual_proxy.la

endif

libdir = $(plugindir)/$(VISUALIZATION_PLUGIN_DIR)

INCLUDES = $(GTK_CFLAGS) $(LIBVISUAL_CFLAGS) $(SDL_CFLAGS) $(all_includes)

libvisual_proxy_la_LDFLAGS = $(PLUGIN_LDFLAGS) $(GTK_LDFLAGS) $(LIBVISUAL_LDFLAGS) $(SDL_LDFLAGS)
libvisual_proxy_la_LIBADD = $(GTK_LIBS) $(LIBVISUAL_LIBS) $(SDL_LIBS)

libvisual_proxy_la_CFLAGS = $(PLUGIN_CFLAGS)

libvisual_proxy_la_SOURCES = main.c \
			    lv_bmp_config.c lv_bmp_config.h \
			    config_gui.c config_gui.h \
			    about.c about.h

