#!/bin/sh
#
# This script will list the most current version of Pioneers in various
# distributions.
#
# The output is comma delimited:
#  Column 1: the name of the distribution
#  Column 2: the most recent version
#
# When adding new distributions, keep the alphabetic order
# (but source code first)
#
# Copyright 2006 Roland Clobus
#
# This script is distributed with the same license as the Pioneers package

# Source code
wget -o /dev/null -O - "http://sourceforge.net/project/showfiles.php?group_id=5095&package_id=5139" | gawk  '$0 ~ /pioneers-.+.tar.gz/ { gsub("<[^>]+>", ""); gsub("pioneers-", ""); gsub(".tar.gz", ""); print "Source," $1 }' | tail -1

# Debian
wget -o /dev/null -O - "http://packages.qa.debian.org/p/pioneers.html" | gawk '$0 ~ "Latest version" { getline a; gsub("<[^>]+>", "", a); print "Debian," a }' 

# Fink
wget -o /dev/null -O - "http://pdb.finkproject.org/pdb/package.php/pioneers" | gawk '$0 ~ "10.4/powerpc" { gsub("<[^>]+>", " "); n = split($0, b, " "); print "Fink," b[n] }' | head -1

# FreeBSD
wget -o /dev/null -O - --user-agent="" "http://www.freebsd.org/cgi/ports.cgi?query=pioneers&stype=all" | gawk '$0 ~ "A NAME" { gsub("<[^>]+>", ""); gsub("pioneers-", ""); print "FreeBSD," $1 }' 

# Gentoo
wget -o /dev/null -O - --user-agent="" "http://packages.gentoo.org/search/?sstring=pioneers" | gawk '$0 ~ "<th class=\"releases\">" { gsub("<[^>]+>", ""); print $1 }' | head -1 | gawk '{ print "Gentoo," $1 }'

# Mandriva
wget -o /dev/null -O - "http://rpms.mandrivaclub.com/search.php?query=pioneers&submit=Search+...&arch=i586" | gawk ' $0 ~ "<tr bgcolor" { gsub("<[^>]+>", " "); a = $1; gsub("pioneers-", "", a); gsub(".i586.html", "", a); print "Mandriva," a }'

# Microsoft Windows
wget -o /dev/null -O - "http://sourceforge.net/project/showfiles.php?group_id=5095&package_id=180259" | gawk '$0 ~ "-setup.exe" { gsub("<[^>]+>", ""); a = $1; gsub("Pioneers-", "", a); gsub("-setup.exe", "", a); print "Microsoft Windows," a }' | tail -1

# NetBSD
wget -o /dev/null -O - "ftp://ftp.netbsd.org/pub/pkgsrc/current/pkgsrc/games/pioneers/README.html" | gawk '$0 ~ "The current source" { getline; gsub("\"pioneers-", ""); gsub("\".", ""); print "NetBSD," $1 }'

# OpenBSD [Still only Gnocatan!]
wget -o /dev/null -O - "http://www.openbsd.org/cgi-bin/cvsweb/~checkout~/ports/games/gnocatan/distinfo?rev=1.2" | gawk '$0 ~ "MD5" { a = $2; gsub(".gnocatan-", "", a); gsub(".tar.gz.", "", a); print "OpenBSD," a }'

# Redhat FC4
wget -o /dev/null -O - "http://sourceforge.net/project/showfiles.php?group_id=5095&package_id=181682" | gawk  '$0 ~ /pioneers-client.+.rpm/ { gsub("<[^>]+>", ""); gsub("pioneers-client-", ""); gsub(".i386.rpm", ""); print "Redhat FC4," $1 }' | head -1

# Ubuntu
wget -o /dev/null -O - "https://launchpad.net/distros/ubuntu/+source/pioneers" | gawk '$0 ~ "Current release" { getline; gsub("<[^>]+>", ""); print "Ubuntu," $1 }' 
