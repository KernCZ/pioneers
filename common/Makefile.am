noinst_LIBRARIES = libgnocatan.a libgnocatan_gui.a

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
        -I$(includedir) $(GNOME_INCLUDEDIR)

libgnocatan_a_SOURCES = \
	buildrec.c \
	buildrec.h \
	cards.c \
	cards.h \
	cost.c \
	cost.h \
	game.c \
	game.h \
	log.c \
	log.h \
	map.c \
	map.h \
	map_query.c \
	meta.h \
	network.c \
	network.h \
	quoteinfo.c \
	quoteinfo.h \
	state.c \
	state.h

libgnocatan_gui_a_SOURCES = \
	common_gui.c \
	common_gui.h
