# -orig: %define	desktop_vendor	planetccrma
%define	desktop_vendor	Rosegarden-4
%define desktop_utils   %(if which desktop-file-install 2>1 >/dev/null ; then echo "yes" ; fi)

Summary: Midi, audio and notation editor
Name: 	 rosegarden4
Version: 0.9
Release: 1_rh
URL:     http://www.all-day-breakfast.com/rosegarden/
Source0: rosegarden-4-%{version}.tar.gz
License: GPL
Group:   Applications/Multimedia
BuildRoot: %{_tmppath}/%{name}-root
Requires:  jack-audio-connection-kit >= 0.40
Obsoletes: rosegarden
Obsoletes: rosegarden-4
# -orig: Distribution: Planet CCRMA
Distribution: Rosegarden-4

%description
Rosegarden-4 is an attractive, user-friendly MIDI and audio sequencer,
notation editor, and general-purpose music composition and editing
application for Unix and Linux

%prep
%setup -q -n rosegarden-4-%{version}

%build
./configure --prefix=%{_prefix} --mandir=%{_mandir} --with-jack --with-ladspa
make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

# redhat menus
cat << EOF > %{desktop_vendor}-%{name}.desktop
[Desktop Entry]
Name=Rosegarden-4
Comment=Midi, audio and notation editor
Icon=
Exec=%{_bindir}/rosegarden
Terminal=false
Type=Application
EOF

%if "%{desktop_utils}" == "yes"
  mkdir -p %{buildroot}%{_datadir}/applications
  desktop-file-install --vendor %{desktop_vendor} \
    --dir %{buildroot}%{_datadir}/applications    \
    --add-category X-Red-Hat-Base                 \
    --add-category Application                    \
    --add-category AudioVideo                     \
    %{desktop_vendor}-%{name}.desktop
%else
  mkdir -p %{buildroot}%{_sysconfdir}/X11/applnk/System
  cp %{desktop_vendor}-%{name}.desktop \
     %{buildroot}%{_sysconfdir}/X11/applnk/System/%{desktop_vendor}-%{name}.desktop
%endif

# rh-added: add the docs
cd docs
rm -rf ./CVS ./*/CVS
cd ..
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/doc/%{name}-%{version}
cp -a docs/* $RPM_BUILD_ROOT/%{_datadir}/doc/%{name}-%{version}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/*rosegarden*
%{_libdir}/libRose*
%{_datadir}/applnk/Applications/rosegarden.desktop
%{_datadir}/apps/rosegarden
%{_datadir}/doc/HTML/en/rosegarden
%if "%{desktop_utils}" == "yes"
%{_datadir}/applications/*%{name}.desktop
%else
%{_sysconfdir}/X11/applnk/System/%{desktop_vendor}-%{name}.desktop
%endif
%{_datadir}/icons/hicolor/16x16/apps/rosegarden.xpm
%{_datadir}/icons/hicolor/32x32/apps/rosegarden.xpm
%{_datadir}/icons/locolor/16x16/apps/rosegarden.xpm
%{_datadir}/icons/locolor/32x32/apps/rosegarden.xpm

# rh-added: (declare doc dir, no %doc section)
%{_datadir}/locale/*/LC_MESSAGES/rosegarden.mo
%dir %{_datadir}/doc/%{name}-%{version}
%{_datadir}/doc/%{name}-%{version}/*

%changelog
* Wed May 07 2003 Ryurick M. Hristev <ryurick.hristev@canterbury.ac.nz>
- update spec to 0.9

* Wed Dec 18 2002 Fernando Lopez Lezcano <nando@ccrma.stanford.edu> 0.8.5-1
- changed name to rosegarden, anaconda does not like the current name
  (anaconda should be fixed, name is legal)
- update to 0.8.5
* Sun Nov 10 2002 Fernando Lopez Lezcano <nando@ccrma.stanford.edu> 0.8-2
- changed name of package to rosegarden-4 (what was I thinking?...)
- added patch to rename jack alsa ports for jack >= 0.40
- added explicit dependency to jack
- added redhat menu entry
* Fri Oct 18 2002 Fernando Lopez Lezcano <nando@ccrma.stanford.edu>
- Initial build.


