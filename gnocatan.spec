Summary: 	Playable implementation of the Settlers of Catan 
Name: 		gnocatan
Version: 	0.5.6
Release: 	1
Group: 		X11/Games
Copyright: 	GPL
Url: 		http://www.gnome.org/gnocatan/
Packager: 	Preben Randhol <randhol@pvv.org>
Source: 	http://www.gnome.org/gnocatan/%{name}-%{version}.tar.gz
BuildRoot: 	/var/tmp/%{name}_root


%description 
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

%package 	server
Summary:	Gnocatan Server
Group:		X11/Games
%description 	server
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

The server has a user interface in which you can customise the game
parameters.  Customisation is fairly limited at the moment, but this
should change in later versions.  Once you are happy with the game
parameters, press the Start Server button, and the server will start
listening for client connections.

%package 	client
Summary: 	Gnocatan Client
Group: 		X11/Games
Requires:	gnocatan-help, gnocatan-data

%description 	client
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

This is the client software to play the game.

%package 	data
Summary: 	Gnocatan Data
Group: 		X11/Games

%description 	data
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

This package contains the data files.

%package 	help
Summary: 	Gnocatan Help
Group: 		X11/Games

%description 	help
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

This package contains the help files.


%prep
%setup

%build
./autogen.sh --prefix=/usr
make 

%install
rm -rf $RPM_BUILD_ROOT
make install prefix=$RPM_BUILD_ROOT/usr

%files server
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
/usr/bin/gnocatan-server-gtk
/usr/bin/gnocatan-server-console
/usr/bin/gnocatan-meta-server
/usr/share/gnocatan/default.game
/usr/share/gnocatan/four-islands.game
/usr/share/gnocatan/seafarers.game
/usr/share/gnocatan/small.game

%files client
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
/usr/bin/gnocatan
/usr/share/gnome/apps/Games/gnocatan.desktop
/usr/share/pixmaps/gnome-gnocatan.png

%files data
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
/usr/share/pixmaps/gnocatan/city.png
/usr/share/pixmaps/gnocatan/cross.png
/usr/share/pixmaps/gnocatan/bridge.png
/usr/share/pixmaps/gnocatan/board.png
/usr/share/pixmaps/gnocatan/desert.png
/usr/share/pixmaps/gnocatan/develop.png
/usr/share/pixmaps/gnocatan/dice.png
/usr/share/pixmaps/gnocatan/field.png
/usr/share/pixmaps/gnocatan/finish.png
/usr/share/pixmaps/gnocatan/forest.png
/usr/share/pixmaps/gnocatan/hill.png
/usr/share/pixmaps/gnocatan/mountain.png
/usr/share/pixmaps/gnocatan/pasture.png
/usr/share/pixmaps/gnocatan/plain.png
/usr/share/pixmaps/gnocatan/road.png
/usr/share/pixmaps/gnocatan/sea.png
/usr/share/pixmaps/gnocatan/settlement.png
/usr/share/pixmaps/gnocatan/ship.png
/usr/share/pixmaps/gnocatan/tick.png
/usr/share/pixmaps/gnocatan/trade.png

%files help
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%dir /usr/share/gnome/help/gnocatan/
%dir /usr/share/gnome/help/gnocatan/C
%dir /usr/share/gnome/help/gnocatan/C/images
/usr/share/gnome/help/gnocatan/C/topic.dat
/usr/share/gnome/help/gnocatan/C/images/*

%changelog

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
