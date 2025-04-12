Build Status
============

[![CI Ubuntu](https://github.com/tedfelix/rosegarden-official/actions/workflows/ci.yml/badge.svg)](https://github.com/tedfelix/rosegarden-official/actions/workflows/ci.yml)
[![CI Windows](https://github.com/tedfelix/rosegarden-official/actions/workflows/ci-windows.yml/badge.svg)](https://github.com/tedfelix/rosegarden-official/actions/workflows/ci-windows.yml)

Introduction
============

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

The default install location might not be ideal.  You can override this
when you run cmake.  E.g. to install Rosegarden to /usr/bin:

    cmake .. -DCMAKE_INSTALL_PREFIX=/usr

This will likely wipe out any existing Rosegarden installation from your
package manager.

For a debug build, use the CMAKE_BUILD_TYPE variable:

    cmake .. -DCMAKE_BUILD_TYPE=Debug

This will build Rosegarden so that it is useful for debugging, which can
greatly improve our ability to find and correct bugs by allowing Rosegarden
to produce useful stack traces when it crashes.  The debug binary will be
somewhat large: about 200MB as of July 2019.  See "Running the Unit Tests"
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


Authors and Copyright
=====================

Rosegarden is Copyright 2000-2024 The Rosegarden Development Team

See http://rosegardenmusic.com/resources/authors/ for a complete list of
developers past and present, and to learn something about the history of our
project.

Rosegarden is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.  See the file COPYING for more details.

