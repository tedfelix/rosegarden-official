Summary:   MIDI and audio sequencer and notation editor
Name:	   rosegarden-4
Version:   0.8
Release:   2
Copyright: GPL
Group:     Applications/Sound
Source:    http://prdownloads.sourceforge.net/rosegarden/rosegarden-4-0.8.tar.gz
URL:       http://www.all-day-breakfast.com/rosegarden/
Packager:  Ryurick M. Hristev <ryurick.hristev@canterbury.ac.nz>

# unfortunately various rpm based distros may have different naming conventions
# and not all set the 'Provides' field properly
Requires: alsa-driver >= 0.9.0beta12
Requires: ladspa
Requires: jack-audio-connection-kit

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
* Sun Oct 20 2002 Ryurick M. Hristev <ryurick.hristev@canterbury.ac.nz>
- rebuild wit ladsa & jack-audio-connection-kit

* Sat Oct 19 2002 Ryurick M. Hristev <ryurick.hristev@canterbury.ac.nz>
- first spec
- tested under RedHat 8.0

