bin_PROGRAMS = gnocatan-meta-server

INCLUDES = \
        -I$(includedir) -I$(srcdir)/../common $(GNOME_INCLUDEDIR)

gnocatan_meta_server_SOURCES = \
	gnocatan-meta-server.c

gnocatan_meta_server_LDADD = \
	$(GLIB_LIBS)

# We don't need this, do we?  -- egnor
# install-data-local:
#	-chgrp games $(bindir)/gnocatan-meta-server && chmod 2111 $(bindir)/gnocatan-meta-server
