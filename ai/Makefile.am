configdir=$(datadir)/gnocatan

bin_PROGRAMS = gnocatanai

gnocatanai_DEPENDENCIES = ../common/libgnocatan.a ../common/libgnocatan_gtk.a

# if anyone knows a cleaner way to do this, be my guest.  Automake screamed
#   at me when I tried to do it more directly.
if ADMIN_GTK_SUPPORT
ADMIN_GTK = -DADMIN_GTK
ADMIN_GTK_FILES_ACTIVE = admin-gtk.c
ADMIN_GTK_FILES_INACTIVE =
else
ADMIN_GTK =
ADMIN_GTK_FILES_ACTIVE =
ADMIN_GTK_FILES_INACTIVE = admin-gtk.c
endif

INCLUDES = \
	-I$(srcdir)/../common -I. \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(ADMIN_GTK) \
        -I$(includedir) $(GNOME_INCLUDEDIR)

EXTRA_gnocatanai_SOURCES = $(ADMIN_GTK_FILES_INACTIVE)

gnocatanai_SOURCES = \
	ai.c \
	client.c \
	client.h \
	greedy.c \
	greedy.h \
	computer.h \
	computer.c \
	develop.c \
	discard.c \
	monopoly.c \
	player.c \
	player.h \
	plenty.c \
	resource.c \
	robber.c \
	stock.c \
	trade.c \
	turn.c

gnocatanai_LDADD = \
	../common/libgnocatan.a \
	../common/libgnocatan_gtk.a \
	$(GNOME_LIBDIR) \
	$(GNOMEUI_LIBS)


config_DATA = \
	computer_names

EXTRA_DIST = \
	$(config_DATA)