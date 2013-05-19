Short guide to build Pioneers from source in MinGW

Prepare the build environment
=============================
* Download mingw-get-inst from http://www.mingw.org/wiki/Getting_Started
  - Install with the following settings:
    - Use pre-packaged repository catalogues (20111118)
    - Selected: C compiler, MSYS basic system, MinGW developer toolkit
* Download the all-in-one bundle from http://www.gtk.org/download/win32.php
  - Extract to C:\MinGW
* Download from http://ftp.gnome.org/pub/GNOME/binaries/win32:
  - From the folder intltool: intltool_0.40.4-1_win32.zip
  - From the folder librsvg: librsvg_2.32.1-1_win32.zip, 
    svg-gdk-pixbuf-loader_2.32.1-1_win32.zip and
    librsvg-dev_2.32.1-1_win32.zip
  - From the folder libcroco: libcroco_0.6.2-1_win32.zip
  - From the folder dependencies: libxml2_2.7.7-1_win32.zip
  - From the folder gdk-pixbuf: gdk-pixbuf_2.24.0-1_win32.zip
  - Extract all to C:\MinGW
* Download the GOB2 tarball from http://www.jirka.org/gob.html
  - ./configure --prefix=/mingw
  - make
  - make install
  
* In MSYS shell:
  - vim /MinGW/bin/intltool-*
    - Adjust all first lines to point to /bin/perl instead of
      /opt/perl/bin/perl
  - gdk-pixbuf-query-loaders --update-cache
  - cp /MinGW/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache MinGW/loaders.cache
  - vim MinGW/loaders.cache
    - Remove all directories before the filename part of the DLL

If any of the downloads cannot be found, a newer version will probably work.

Build and install Pioneers
==========================
1) Download the source tarball to your home directory
   (c:\MinGW\msys\1.0\home\%username%)
2) Start the MinGW shell
3) Expand the source tarball
   (tar xvzf pioneers-%versionnumber%.tar.gz)
4) Enter the source directory (cd pioneers-%versionnumber%)
5) Configure, build and install
     ./configure
     make
     make install
     make install-MinGW

Start Pioneers
==============
a) Start Pioneers by double clicking on the executable
    (found in C:\MinGW\msys\1.0\local)
b) or start pioneers.exe from /usr/local in the shell

Rebuild the icons
=================
a) Load the svg file at 768x768 in Gimp 2.8 (768=least common multiple(48,256))
b) Execute the Script-Fu script "Iconify2.scm"
c) Export as *.ico

Known limitations
=================
* The online help is not built
* The server and metaserver are not built

Roland Clobus
2013-03-18 Pioneers-14.1
