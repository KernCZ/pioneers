SUBDIRS = po intl common client server meta-server ai docs

EXTRA_DIST = autogen.sh gnocatan.spec xmldocs.make omf.make

dist-hook:
	mkdir $(distdir)/debian
	set -e; for f in control changelog rules copyright dirs menu; do \
		cp -p $(srcdir)/debian/$$f $(distdir)/debian; \
	done
	cp -p $(srcdir)/debian/*.files $(distdir)/debian
	cp -p $(srcdir)/debian/*.menu $(distdir)/debian
	cp -p $(srcdir)/debian/*.undocumented $(distdir)/debian
	cp -p $(srcdir)/debian/*.init $(distdir)/debian

distclean-local:
	rm -f *~
	rm -rf autom4te.cache
