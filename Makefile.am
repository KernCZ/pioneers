SUBDIRS = macros common client server meta-server ai

## to automatically rebuild aclocal.m4 if any of the macros in
## `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep

EXTRA_DIST = autogen.sh gnocatan.spec

dist-hook:
	mkdir $(distdir)/debian
	set -e; for f in control changelog rules copyright dirs menu; do \
		cp -p $(srcdir)/debian/$$f $(distdir)/debian; \
	done
	cp -p $(srcdir)/debian/*.files $(distdir)/debian
	cp -p $(srcdir)/debian/*.menu $(distdir)/debian
	cp -p $(srcdir)/debian/*.undocumented $(distdir)/debian
	cp -p $(srcdir)/debian/*.init $(distdir)/debian
