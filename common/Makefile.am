noinst_LIBRARIES = libgnocatan.a libgnocatan_gtk.a

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
        -I$(includedir) $(GNOME2_CFLAGS)

libgnocatan_a_SOURCES = \
	buildrec.c \
	buildrec.h \
	cards.c \
	cards.h \
	common_glib.c \
	common_glib.h \
	cost.c \
	cost.h \
	driver.c \
	driver.h \
	game.c \
	game.h \
	gnocatan-path.h.in \
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

EXTRA_libgnocatan_a_SOURCES = \
	mt_rand.c \
	mt_rand.h

libgnocatan_a_LIBADD = \
	$(MT_RAND_OBJ)

libgnocatan_a_DEPENDENCIES = \
	$(MT_RAND_OBJ) gnocatan-path.h

libgnocatan_gtk_a_SOURCES = \
	common_gtk.c \
	common_gtk.h \
	config-gnome.c \
	config-gnome.h

gnocatan-path.h: gnocatan-path.h.in
	rm -f $(<D)/$(@F)
	ddir="`echo $(datadir)`"; \
	bdir="`echo $(bindir)`"; \
	sed -e "s,@datadir\@,$$ddir,g" -e "s,@bindir\@,$$bdir,g" $< >$(<D)/$(@F)

