Name: 		gnocatan
Summary: 	Playable implementation of the Settlers of Catan 
Version: 	0.8.0
Release: 	2
Group: 		Amusements/Games
License: 	GPL
Url: 		http://gnocatan.sourceforge.net/
Packager: 	Steve Langasek <vorlon@dodds.net>
Source: 	http://unm.dl.sourceforge.net/gnocatan/%{name}-%{version}.tar.gz
BuildRoot: 	%{_tmppath}/%{name}-%{version}-%{release}-root

%description 
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

%package 	client
Summary: 	Gnocatan Client
Group: 		Amusements/Games

%description 	client
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

This is the client software to play the game.

%package 	help
Summary: 	Gnocatan Help
Group: 		Amusements/Games

%description 	help
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

This package contains the help files.

%package 	ai
Summary:	Gnocatan AI Player
Group:		Amusements/Games
%description 	ai
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

This package contains a computer player that can take part in Gnocatan games.

%package 	server-console
Summary:	Gnocatan Console Server
Group:		Amusements/Games
Requires:	gnocatan-server-data
%description 	server-console
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

%package 	server-gtk
Summary:	Gnocatan GTK Server
Group:		Amusements/Games
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
Group: 		Amusements/Games

%description 	server-data
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

This package contains the data files for a game server.

%package 	meta-server
Summary:	Gnocatan Meta Server
Group:		Amusements/Games
Requires:	gnocatan-server-console
%description 	meta-server
Gnocatan is an Internet playable implementation of the Settlers of
Catan board game.  The aim is to remain as faithful to the board game
as is possible.

The meta server registers available game servers and offers them to new
players. It can also create new servers on client request.

%prep
%setup -q

%build
CFLAGS="$RPM_OPT_FLAGS" ./autogen.sh --prefix=/usr
make

%install
rm -rf $RPM_BUILD_ROOT
make install prefix=$RPM_BUILD_ROOT/usr
rm -rf $RPM_BUILD_ROOT/usr/var/scrollkeeper

%clean
rm -rf $RPM_BUILD_ROOT

%files client
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%doc /usr/man/man6/gnocatan.6.gz
%{_bindir}/gnocatan
%{_datadir}/gnome/apps/Games/gnocatan.desktop
%{_datadir}/pixmaps/gnome-gnocatan.png
%{_datadir}/pixmaps/gnocatan/*
%{_datadir}/games/gnocatan/images/*
%{_datadir}/locale/es/LC_MESSAGES/gnocatan.mo
%{_datadir}/locale/de/LC_MESSAGES/gnocatan.mo

%files help
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS
%{_datadir}/gnome/help/gnocatan/C/*.xml
%{_datadir}/gnome/help/gnocatan/C/images/*
%{_datadir}/omf/gnocatan/gnocatan-C.omf

%post help
if which scrollkeeper-update>/dev/null 2>&1; then scrollkeeper-update -q -o %{_datadir}/omf/gnocatan; fi

%postun help
if which scrollkeeper-update>/dev/null 2>&1; then scrollkeeper-update -q; fi

%files ai
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%{_bindir}/gnocatanai
%{_datadir}/games/gnocatan/computer_names

%files server-console
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%doc /usr/man/man6/gnocatan-server-console.6.gz
%{_bindir}/gnocatan-server-console

%files server-gtk
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%doc /usr/man/man6/gnocatan-server-gtk.6.gz
%{_bindir}/gnocatan-server-gtk
%{_datadir}/gnome/apps/Games/gnocatan-server.desktop

%files server-data
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%{_datadir}/games/gnocatan/*.game

%files meta-server
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%{_bindir}/gnocatan-meta-server
