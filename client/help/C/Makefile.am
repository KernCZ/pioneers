helpdir = $(datadir)/gnome/help/gnocatan/C

help_DATA = \
	index.html \
	topic.dat

EXTRA_DIST = gnocatan.sgml $(help_DATA)

all: index.html

# No-op if db2html failed
index.html: gnocatan/index.html
	-cd $(srcdir) && cp gnocatan/index.html index.html

gnocatan/index.html: gnocatan.sgml
	-cd $(srcdir) && db2html gnocatan.sgml

dist-hook:
	-mkdir $(distdir)/gnocatan
	cp $(srcdir)/gnocatan/*.html $(distdir)/gnocatan

# No-op if db2html failed
install-data-local: index.html
	$(mkinstalldirs) $(helpdir)/images
	files=`echo $(srcdir)/gnocatan/*.html`; \
	if test "$$files" != '$(srcdir)/gnocatan/*.html'; then \
	  for file in $$files; do \
	    basefile=`echo $$file | sed -e 's,^.*/,,'`; \
	    $(INSTALL_DATA) $$file $(helpdir)/$$basefile; \
	  done; \
	fi
	-for file in $(srcdir)/images/*.gif; do \
	$(INSTALL_DATA) $(srcdir)/$$file $(helpdir)/images;\
	done

gnocatan.ps: gnocatan.sgml
	db2ps $<

gnocatan.rtf: gnocatan.sgml
	db2rtf $<
