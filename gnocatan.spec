Summary: 	Playable implementation of the Settlers of Catan 
Name: 		gnocatan
Version: 	0.7.1
Release: 	1
Group: 		X11/Games
Copyright: 	GPL
Url: 		http://gnocatan.sourceforge.net/
Packager: 	Steve Langasek <vorlon@dodds.net>
Source: 	http://download.sourceforge.net/gnocatan/%{name}-%{version}.tar.gz
BuildRoot: 	/var/tmp/%{name}_root


%description 
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

%package 	client
Summary: 	Gnocatan Client
Group: 		X11/Games

%description 	client
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

This is the client software to play the game.

%package 	help
Summary: 	Gnocatan Help
Group: 		X11/Games

%description 	help
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

This package contains the help files.

%package 	ai
Summary:	Gnocatan AI Player
Group:		X11/Games
%description 	ai
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

This package contains a computer player that can take part in Gnocatan games.

%package 	server-console
Summary:	Gnocatan Console Server
Group:		X11/Games
Requires:	gnocatan-server-data
%description 	server-console
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

%package 	server-gtk
Summary:	Gnocatan GTK Server
Group:		X11/Games
Requires:	gnocatan-server-data
%description 	server-gtk
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

The server has a user interface in which you can customise the game
parameters.  Customisation is fairly limited at the moment, but this
should change in later versions.  Once you are happy with the game
parameters, press the Start Server button, and the server will start
listening for client connections.

%package 	server-data
Summary: 	Gnocatan Data
Group: 		X11/Games

%description 	server-data
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

This package contains the data files for a game server.

%package 	meta-server
Summary:	Gnocatan Meta Server
Group:		X11/Games
Requires:	gnocatan-server-console
%description 	meta-server
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

The meta server registers available game servers and offers them to new
players. It can also create new servers on client request.


%prep
%setup

%build
CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh --prefix=/usr
make

%install
rm -rf $RPM_BUILD_ROOT
make install prefix=$RPM_BUILD_ROOT/usr

%files client
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
/usr/bin/gnocatan
/usr/share/gnome/apps/Games/gnocatan.desktop
/usr/share/pixmaps/gnome-gnocatan.png
/usr/share/pixmaps/gnocatan/*

%files help
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%dir /usr/share/gnome/help/gnocatan/
%dir /usr/share/gnome/help/gnocatan/C
%dir /usr/share/gnome/help/gnocatan/C/images
/usr/share/gnome/help/gnocatan/C/topic.dat
/usr/share/gnome/help/gnocatan/C/*.html
/usr/share/gnome/help/gnocatan/C/images/*

%files ai
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
/usr/bin/gnocatanai
/usr/share/gnocatan/computer_names

%files server-console
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
/usr/bin/gnocatan-server-console

%files server-gtk
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
/usr/bin/gnocatan-server-gtk
/usr/share/gnome/apps/Games/gnocatan-server.desktop

%files server-data
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
/usr/share/gnocatan/*.game

%files meta-server
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
/usr/bin/gnocatan-meta-server

%changelog

* Sun May 19 2002 Roman Hodek <roman@hodek.net>
- 0.6.99 as beta for 0.7.0

* Sun Aug 27 2000 Steve Langasek <vorlon@dodds.net>
- 0.6.1 released

* Tue Jun 20 2000 Steve Langasek <vorlon@dodds.net>
- updated version number

* Thu Jun 01 2000 Steve Langasek <vorlon@dodds.net>
- Updated to behave more like the filesystem standard tells us to (and
  more like configure expects us to)

* Sun May 07 2000 Dave Cole <adve@dccs.com.au>
- Removed ship building development card

* Mon May 01 2000 Andy Heroff <aheroff@mediaone.net>
- SourceForge release version 0.5.0

* Fri Sep 03 1999 Dave Cole <dave@dccs.com.au>
- Modifications to build 0.4.0

* Sun May 23 1999 Preben Randhol <randhol@pvv.org>
- Building version 0.31

* Wed May 12 1999 Preben Randhol <randhol@pvv.org>
- First try at making the packages
