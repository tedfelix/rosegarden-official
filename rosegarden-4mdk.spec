%define version 0.8
%define release 2
# libtoolize --copy --force breaks the libtool script on MDK9.0
%define __libtoolize :

Summary:   MIDI and audio sequencer and notation editor
Name:	   rosegarden-4
Version:   %{version}
Release:   %{release}mdk
Copyright: GPL
Group:     Applications/Sound
Source:    http://prdownloads.sourceforge.net/rosegarden/rosegarden-4-%{version}.tar.gz
URL:       http://www.all-day-breakfast.com/rosegarden/
Packager:  Hans Kieserman <hkieserman@mail.com>

# Please update these if you know better
# unfortunately various rpm based distros may have different naming conventions
# and not all set the 'Provides' field properly
Requires: kdelibs zlib1
Requires: alsa-driver >= 0.9.0beta12
Requires: ladspa
Requires: jack-audio-connection-kit

BuildRequires: kdemultimedia arts libarts-devel kdelibs-devel libalsa2-devel
BuildRequires: zlib1-devel XFree86-devel libaudiofile0-devel libpng3-devel
BuildRequires: jack-audio-connection-kit-devel
BuildRoot:     %{_tmppath}/%{name}-buildroot

%description
Rosegarden-4 is an attractive, user-friendly MIDI and audio sequencer, notation
editor, and general-purpose music composition and editing application for Unix
and Linux.

#---------------------------------------------------------------------
%prep

%setup

# prepare docs for packaging: cleanup CVS stuff
# BUG: find returns a !=0 err code: Why ?
#      do it in a subshell to 'exit 0'
( find docs -name CVS -exec rm -rf '{}' ';' > /dev/null 2>&1 ) 

#---------------------------------------------------------------------
%build

%configure
# warning: '%make' is not portable, do not use
make %{_smp_mflags}

#---------------------------------------------------------------------
%install

# BUG: 'make install' reruns configure: Why ?
%makeinstall

#---------------------------------------------------------------------
%clean

# be paranoid
[ "${RPM_BUILD_ROOT}" != "/" ] && rm -rf ${RPM_BUILD_ROOT}

#---------------------------------------------------------------------
%files

%{_bindir}/*
%{_libdir}/*
%{_datadir}/applnk/Applications/*
%dir %{_datadir}/apps/rosegarden
%{_datadir}/apps/rosegarden/*
%dir %{_datadir}/doc/HTML/en/rosegarden
%{_datadir}/doc/HTML/en/rosegarden/*
%doc AUTHORS ChangeLog INSTALL NEWS README TODO
%doc docs/* 

#---------------------------------------------------------------------
%changelog
* Sun Oct 27 2002 Hans Kieserman <hkieserman@mail.com>
- build for Mandrake 9.0
- based on rosegarden-4.spec by ryurick.hristev@canterbury.ac.nz
