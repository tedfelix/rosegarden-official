Rosegarden
==========

Rosegarden is a MIDI sequencer and musical notation editor for Linux.

  https://www.rosegardenmusic.com/

Please keep an eye on the FAQ for known problems and workarounds:

  https://rosegardenmusic.com/wiki/frequently_asked_questions

When you find bugs, first check whether a newer version of Rosegarden
has been released yet; if not, please continue on to:

  https://rosegardenmusic.com/tutorials/bug-guidelines.html

Release notes can be found in the CHANGELOG.


Build requirements
==================

For a complete list of build requirements, please see:

  https://rosegardenmusic.com/wiki/dev:get_dependencies


Installation instructions (v15.12 and up)
=========================================

The simplest way to build Rosegarden is to run the following commands from
within the directory where you found this README file:

    mkdir build
    cd build
    cmake ..
    make
    sudo make install

Assuming all goes well, that will build and install Rosegarden to:

    /usr/local/bin/rosegarden

You can always run Rosegarden directly from the build directory:

    ./rosegarden

If you run into trouble with cmake, you will likely have to hunt down
missing dependencies.

"make" can take a very long time.  You might want to consider adding the
following line to your .bashrc to tell make it can use multiple threads.
This will significantly speed up the build:

    export MAKEFLAGS="-j `nproc`"

You will need to start a new terminal for a change to .bashrc to take effect.

Rosegarden builds with Qt version 5 by default.  To use Qt version 6:

    cmake .. -DUSE_QT6=ON

The default install location might not be ideal.  You can override this
when you run cmake.  E.g. to install Rosegarden to /usr/bin:

    cmake .. -DCMAKE_INSTALL_PREFIX=/usr

This will likely wipe out any existing Rosegarden installation from your
package manager.

For a debug build, use the CMAKE_BUILD_TYPE variable:

    cmake .. -DCMAKE_BUILD_TYPE=Debug

This will build Rosegarden so that it is useful for debugging, which can
greatly improve our ability to find and correct bugs by allowing Rosegarden
to produce useful stack traces when it crashes.  See "Running the Unit Tests"
below for information on doing a debug build and running the tests.

A debug build results in the use of shared libs and the building of unit
tests.  Use -DBUILD_TESTING=OFF to disable the shared library and unit
tests in a Debug build.

If you are a developer, it is recommended that you build with warnings as
errors and Address Sanitizer (ASan) turned on:

    cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-Werror -fsanitize=address -fno-omit-frame-pointer"

This will catch many potential C++ issues.

Flags for the C++ compiler can be added via CMAKE_CXX_FLAGS:

    -DCMAKE_CXX_FLAGS="-Werror -Wpedantic"

Support for LIRC (Linux Infrared Remote Control) can be disabled using:

    -DDISABLE_LIRC=1

New starting with 10.02, most of the application data files are bundled in the
rosegarden binary.  The install process will only copy a few files to various
directories under CMAKE_INSTALL_PREFIX ([PREFIX]):

    [PREFIX]/bin                                   application binary
    [PREFIX]/share/icons/hicolor/.../mimetypes     MIME type icons
    [PREFIX]/share/mime/packages                   MIME type configuration
    [PREFIX]/share/applications                    .desktop file
    [PREFIX]/share/icons/hicolor/32x32/apps        application icon


Runtime requirements
====================

In order to be fully functional and provide the optimal user experience,
Rosegarden requires the following external applications.

  - General MIDI soft synth (TiMidity + Freepats or better)
  - LilyPond
  - Okular, Evince, Acroread, MuPDF, ePDFView, or other PDF viewer reachable
    through xdg-open
  - lpr or lp
  - QjackCtl (JACK Audio Connection Kit - Qt GUI Interface)
  - FLAC
  - WavPack
  - DSSI plugins (any your distro carries)
  - LADSPA plugins (any your distro carries)


Running the Unit Tests
======================

To run the unit tests, build as follows:

    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
    make

(-DBUILD_TESTING=ON is assumed for a Debug build so it isn't needed.)

Then run the tests with "make test":

    make test


Testing the LilyPond export against several LilyPond versions
=============================================================

In addition to the unit tests, a specific test tool is build with:

    cmake .. -DCMAKE_BUILD_TYPE=Debug -DLILY_VERSIONS_TEST=1

This tool, which is independent of the unit tests and is run separately, is
only useful if several versions of LilyPond are available on the same system.

Please, see the README file in the test/lilypond directory for more details.


User documentation
==================

Please see rosegardenmusic.com


SPECIAL NOTES FOR PACKAGE MAINTAINERS
=====================================

As of 10.02 all formerly optional dependencies are now mandatory dependencies.
We've taken this step because it is a real support hassle working through a
set of problems to finally uncover the reason something has broken is due to
the user's distro package being built without a particular feature turned on.

Most major distros already carry everything we depend on, so we don't
anticipate that the build requirements will be a serious irritation for
package maintainers.

Thank you for your cooperation, and thank you for making Rosegarden available
to our users!  If you need to patch Rosegarden for one reason or another in
the course of packaging it, please do keep upstream in the loop at

    rosegarden-devel@lists.sourceforge.net

Thanks!

An older version of the appdata file is kept with the current version in
data/appdata.  It is named rosegarden.appdata-old.xml.


Old README.DEVELOPERS
=====================

### Unit tests

* They are compiled by default in debug mode (-DBUILD_TESTING=OFF to disable
  that - not recommended).
* Run them all with `make test`.
* They trigger the switch from static libs to shared libs for rosegarden's
  own code, to speed up linking.

### Shared libs

Note that you can still run rosegarden uninstalled, even with shared libs,
because cmake takes care of setting the RUNPATH in the rosegarden executable.
So your uninstalled shared lib will be preferred over the installed one -
unless you set $LD_LIBRARY_PATH to point to the installed one.

### Scripts

A number of scripts are available for maintenance during development work.

#### make-ts

Update translation files.  Run before editing one of the .ts
files so that it is up to date before you begin work.  Run
after a string freeze when making a call to translators to
begin work in preparation for a release.  (Runs menu-ts
instruments-ts and autoload-ts automatically to extract
strings from autoload.rg, the menus, and presets.xml for
translation.)

When NOOBSOLETE=1 is set, make-ts drops obsolete translations.
Run to clean up any old translations after making big string
changes in the course of development.  (Probably a good idea
to run this before a call to translators.)

#### make-lrelease

Release all translations.  Run after editing any of the .ts
files or applying a patch from a translator.  Be sure to run
this before doing a release, because the .ts files don't get
used directly.  They have to be converted into .qm files first
by this script.

#### rebuild-qrc

Rebuild data/data.qrc to include all .pfa .png .qm .rc
.rg .rgd .xml .xpm files under the data/ directory that have
been added to the Subversion repository.  Warns you if any
files exist which have not been added to the respository, and
does not add these files.  (Files must be added before they
can be built into the resource bundle so as to make it
difficult to commit a version of data.qrc that refers to files
that only exist in the developer's local working copy, which
breaks the repository for everyone else.)

### Compiling large files

data/data.cpp is quite large (25 megabytes at this writing) and may
fail to compile on smaller systems.  It can't easily be split because
it's auto-generated.

One workaround under gcc is to add the parameters "--param
ggc-min-expand=0 --param ggc-min-heapsize=4096" to that command line
(the line that `make' echoes and fails on).  This sets gcc's garbage
collector to a much more aggressive setting.  It will take much longer
but it will compile on smaller systems.

The command will look something like:

g++ --param ggc-min-expand=0 --param ggc-min-heapsize=4096 -c -I/usr/include/qt4/Qt3Support -I/usr/include/qt4/QtGui -I/usr/include/qt4/QtXml -I/usr/include/qt4/QtNetwork -I/usr/include/qt4/QtCore -I/usr/include/qt4 -DQT3_SUPPORT -DLITTLE_ENDIAN=1 -g0 -O0 data/data.cpp -o data/data.o

With data/data.o existing, `make' should now succeed without problems.

NB, these parameter values are not tuned.

### Developing with emacs

Developers who use emacs may want to use the auto-insert templates.

 * Arrange for the two elisp files in
   /home/tehom/projects/rosegarden/rosegarden/templates to be loaded.
   auto-insert-choose.el is the general package while
   auto-insert-rosegarden-templates.el holds templates specific to
   Rosegarden.

 * Customize auto-insert-alist according to the instructions in
   auto-insert-rosegarden-templates.el.  The instructions in
   auto-insert-choose.el will work too.


Build Status
============

We don't really use this.  Just keeping it here in case we ever decide to in
the future.

[![CI Ubuntu](https://github.com/tedfelix/rosegarden-official/actions/workflows/ci.yml/badge.svg)](https://github.com/tedfelix/rosegarden-official/actions/workflows/ci.yml)
[![CI Windows](https://github.com/tedfelix/rosegarden-official/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/tedfelix/rosegarden-official/actions/workflows/ci-windows.yml)


Authors and Copyright
=====================

Rosegarden is Copyright 2000-2026 The Rosegarden Development Team

See http://rosegardenmusic.com/resources/authors/ for a complete list of
developers past and present, and to learn something about the history of our
project.

Rosegarden is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.  See the file COPYING for more details.

