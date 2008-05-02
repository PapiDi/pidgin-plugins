plugindir = $(libdir)/pidgin

hotkeys_la_LDFLAGS = -module -avoid-version
hotkeys_la_LIBADD = $(PIDGIN_LIBS) $(GTK_LIBS)

plugin_LTLIBRARIES = hotkeys.la

hotkeys_la_common_sources = \
	hotkeys.h \
	hotkeys.c

if WITH_X11
hotkeys_la_SOURCES = $(hotkeys_la_common_sources) hotkeys_x11.c
endif

if WITH_WIN32
hotkeys_la_SOURCES = $(hotkeys_la_common_sources) hotkeys_win32.c
hotkeys_la_LDFLAGS += -no-undefined
hotkeys_la_LIBADD += -lpidgin
endif

AM_CPPFLAGS = \
	-DHOTKEYS_DATADIR=\"$(datadir)\" \
	$(GTK_CFLAGS) \
	$(PIDGIN_CFLAGS)

EXTRA_DIST = \
	hotkeys_x11.c \
	hotkeys_win32.c
