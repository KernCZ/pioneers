# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /usr
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/gnocatan
pkglibdir = $(libdir)/gnocatan
pkgincludedir = $(includedir)/gnocatan

top_builddir = ..

ACLOCAL = aclocal -I macros
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = i686-pc-linux
host_triplet = i686-pc-linux-gnu
AS = @AS@
CC = gcc
CFLAGS = -g -O2 -Wall -Wno-unused 
CPP = gcc -E
CPPFLAGS = 
DLLTOOL = @DLLTOOL@
GLIB_CFLAGS = -I/usr/lib/glib/include
GLIB_CONFIG = /usr/bin/glib-config
GLIB_LIBS = -L/usr/lib -lglib
GNOMEGNORBA_LIBS = -rdynamic -lgnorba -lORBitCosNaming -lORBit -lIIOP -lORBitutil -lnsl -lgnomeui -lart_lgpl -lgdk_imlib -lSM -lICE -lgtk -lgdk -lgmodule -lXi -lXext -lX11 -lgnome -lgnomesupport -lesd -laudiofile -lm -ldb1 -lglib -ldl
GNOMEUI_LIBS = -rdynamic -lgnomeui -lart_lgpl -lgdk_imlib -lSM -lICE -lgtk -lgdk -lgmodule -lXi -lXext -lX11 -lgnome -lgnomesupport -lesd -laudiofile -lm -ldb1 -lglib -ldl
GNOME_APPLETS_LIBS = 
GNOME_CAPPLET_LIBS = 
GNOME_CONFIG = /usr/bin/gnome-config
GNOME_INCLUDEDIR = -I/usr/include -DNEED_GNOMESUPPORT_H -I/usr/lib/gnome-libs/include -I/usr/lib/glib/include -I/usr/X11R6/include
GNOME_LIBDIR = -rdynamic -L/usr/lib -L/usr/X11R6/lib
GNOME_LIBS = -lgnome -lgnomesupport -lesd -laudiofile -lm -ldb1 -lglib -ldl
GNORBA_CFLAGS = -I/usr/include -DNEED_GNOMESUPPORT_H -I/usr/lib/gnome-libs/include -I/usr/lib/glib/include -I/usr/X11R6/include
GNORBA_LIBS = -rdynamic -L/usr/lib -L/usr/X11R6/lib -lgnorba -lORBitCosNaming -lORBit -lIIOP -lORBitutil -lnsl -lgnomeui -lart_lgpl -lgdk_imlib -lSM -lICE -lgtk -lgdk -lgmodule -lXi -lXext -lX11 -lgnome -lgnomesupport -lesd -laudiofile -lm -ldb1 -lglib -ldl
GTKXMHTML_LIBS = -rdynamic -lgtkxmhtml -lXpm -ljpeg -lpng -lz -lSM -lICE -lgtk -lgdk -lgmodule -lglib -ldl -lXi -lXext -lX11 -lm
LDFLAGS = 
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LN_S = ln -s
MAINT = #
MAKEINFO = makeinfo
MT_RAND_OBJ = mt_rand.o
OBJDUMP = @OBJDUMP@
ORBIT_CFLAGS = -I/usr/lib/glib/include -I/usr/include
ORBIT_CONFIG = /usr/bin/orbit-config
ORBIT_IDL = /usr/bin/orbit-idl
ORBIT_LIBS = -L/usr/lib -lORBitCosNaming -lORBit -lIIOP -lORBitutil -lglib -lnsl -lm
PACKAGE = gnocatan
RANLIB = ranlib
VERSION = 0.6.1
ZVT_LIBS = -rdynamic -lzvt -lutil -lSM -lICE -lgtk -lgdk -lgmodule -lglib -ldl -lXi -lXext -lX11 -lm
cflags_set = yes
cxxflags_set = @cxxflags_set@

configdir = $(datadir)/gnocatan

bin_PROGRAMS = gnocatanai

gnocatanai_DEPENDENCIES = ../common/libgnocatan.a ../common/libgnocatan_gtk.a
#ADMIN_GTK = -DADMIN_GTK
ADMIN_GTK = 
#ADMIN_GTK_FILES_ACTIVE = admin-gtk.c
ADMIN_GTK_FILES_ACTIVE = 
#ADMIN_GTK_FILES_INACTIVE = 
ADMIN_GTK_FILES_INACTIVE = admin-gtk.c

INCLUDES =  	-I$(srcdir)/../common -I. 	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" 	$(ADMIN_GTK)         -I$(includedir) $(GNOME_INCLUDEDIR)


EXTRA_gnocatanai_SOURCES = $(ADMIN_GTK_FILES_INACTIVE)

gnocatanai_SOURCES =  	ai.c 	client.c 	client.h 	greedy.c 	greedy.h 	computer.h 	computer.c 	develop.c 	discard.c 	monopoly.c 	player.c 	player.h 	plenty.c 	resource.c 	robber.c 	stock.c 	trade.c 	turn.c


gnocatanai_LDADD =  	../common/libgnocatan.a 	../common/libgnocatan_gtk.a 	$(GNOME_LIBDIR) 	$(GNOMEUI_LIBS)


config_DATA =  	computer_names


EXTRA_DIST =  	$(config_DATA)

mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
PROGRAMS =  $(bin_PROGRAMS)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I..
LIBS = 
gnocatanai_OBJECTS =  ai.o client.o greedy.o computer.o develop.o \
discard.o monopoly.o player.o plenty.o resource.o robber.o stock.o \
trade.o turn.o
gnocatanai_LDFLAGS = 
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
DATA =  $(config_DATA)

DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = gtar
GZIP_ENV = --best
DEP_FILES =  .deps/admin-gtk.P .deps/ai.P .deps/client.P \
.deps/computer.P .deps/develop.P .deps/discard.P .deps/greedy.P \
.deps/monopoly.P .deps/player.P .deps/plenty.P .deps/resource.P \
.deps/robber.P .deps/stock.P .deps/trade.P .deps/turn.P
SOURCES = $(gnocatanai_SOURCES) $(EXTRA_gnocatanai_SOURCES)
OBJECTS = $(gnocatanai_OBJECTS)

all: all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .lo .o .s
$(srcdir)/Makefile.in: # Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu ai/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	@$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(bindir)/`echo $$p|sed 's/$(EXEEXT)$$//'|sed '$(transform)'|sed 's/$$/$(EXEEXT)/'`; \
	done

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

.s.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

.S.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs _libs

distclean-libtool:

maintainer-clean-libtool:

gnocatanai: $(gnocatanai_OBJECTS) $(gnocatanai_DEPENDENCIES)
	@rm -f gnocatanai
	$(LINK) $(gnocatanai_LDFLAGS) $(gnocatanai_OBJECTS) $(gnocatanai_LDADD) $(LIBS)

install-configDATA: $(config_DATA)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(configdir)
	@list='$(config_DATA)'; for p in $$list; do \
	  if test -f $(srcdir)/$$p; then \
	    echo " $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(configdir)/$$p"; \
	    $(INSTALL_DATA) $(srcdir)/$$p $(DESTDIR)$(configdir)/$$p; \
	  else if test -f $$p; then \
	    echo " $(INSTALL_DATA) $$p $(DESTDIR)$(configdir)/$$p"; \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(configdir)/$$p; \
	  fi; fi; \
	done

uninstall-configDATA:
	@$(NORMAL_UNINSTALL)
	list='$(config_DATA)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(configdir)/$$p; \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = ai

distdir: $(DISTFILES)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(top_distdir) && pwd`; \
	distdir=`cd $(distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --gnu ai/Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$d/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)

-include $(DEP_FILES)

mostlyclean-depend:

clean-depend:

distclean-depend:
	-rm -rf .deps

maintainer-clean-depend:

%.o: %.c
	@echo '$(COMPILE) -c $<'; \
	$(COMPILE) -Wp,-MD,.deps/$(*F).pp -c $<
	@-cp .deps/$(*F).pp .deps/$(*F).P; \
	tr ' ' '\012' < .deps/$(*F).pp \
	  | sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	    >> .deps/$(*F).P; \
	rm .deps/$(*F).pp

%.lo: %.c
	@echo '$(LTCOMPILE) -c $<'; \
	$(LTCOMPILE) -Wp,-MD,.deps/$(*F).pp -c $<
	@-sed -e 's/^\([^:]*\)\.o[ 	]*:/\1.lo \1.o :/' \
	  < .deps/$(*F).pp > .deps/$(*F).P; \
	tr ' ' '\012' < .deps/$(*F).pp \
	  | sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	    >> .deps/$(*F).P; \
	rm -f .deps/$(*F).pp
info-am:
info: info-am
dvi-am:
dvi: dvi-am
check-am: all-am
check: check-am
installcheck-am:
installcheck: installcheck-am
install-exec-am: install-binPROGRAMS
install-exec: install-exec-am

install-data-am: install-configDATA
install-data: install-data-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-am
uninstall-am: uninstall-binPROGRAMS uninstall-configDATA
uninstall: uninstall-am
all-am: Makefile $(PROGRAMS) $(DATA)
all-redirect: all-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs:
	$(mkinstalldirs)  $(DESTDIR)$(bindir) $(DESTDIR)$(configdir)


mostlyclean-generic:

clean-generic:

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-binPROGRAMS mostlyclean-compile \
		mostlyclean-libtool mostlyclean-tags mostlyclean-depend \
		mostlyclean-generic

mostlyclean: mostlyclean-am

clean-am:  clean-binPROGRAMS clean-compile clean-libtool clean-tags \
		clean-depend clean-generic mostlyclean-am

clean: clean-am

distclean-am:  distclean-binPROGRAMS distclean-compile distclean-libtool \
		distclean-tags distclean-depend distclean-generic \
		clean-am
	-rm -f libtool

distclean: distclean-am

maintainer-clean-am:  maintainer-clean-binPROGRAMS \
		maintainer-clean-compile maintainer-clean-libtool \
		maintainer-clean-tags maintainer-clean-depend \
		maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-am

.PHONY: mostlyclean-binPROGRAMS distclean-binPROGRAMS clean-binPROGRAMS \
maintainer-clean-binPROGRAMS uninstall-binPROGRAMS install-binPROGRAMS \
mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile mostlyclean-libtool distclean-libtool \
clean-libtool maintainer-clean-libtool uninstall-configDATA \
install-configDATA tags mostlyclean-tags distclean-tags clean-tags \
maintainer-clean-tags distdir mostlyclean-depend distclean-depend \
clean-depend maintainer-clean-depend info-am info dvi-am dvi check \
check-am installcheck-am installcheck install-exec-am install-exec \
install-data-am install-data install-am install uninstall-am uninstall \
all-redirect all-am all installdirs mostlyclean-generic \
distclean-generic clean-generic maintainer-clean-generic clean \
mostlyclean distclean maintainer-clean


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
