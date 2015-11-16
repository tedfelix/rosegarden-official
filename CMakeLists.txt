# CMake version required. This must be the very first line, because it sets default policies affecting everything else
cmake_minimum_required(VERSION 2.8.12)

# Project name
project(rosegarden)

# Version
set(ROSEGARDEN_VERSION "15.12")
set(ROSEGARDEN_CODENAME "Peace")
#set(ROSEGARDEN_CODENAME "Querida")
#set(ROSEGARDEN_CODENAME "Rhapsody in Blue")
#set(ROSEGARDEN_CODENAME "Seagull")
#set(ROSEGARDEN_CODENAME "Twice in a Blue Moon")
#set(ROSEGARDEN_CODENAME "Ultimate Pleasure")
#set(ROSEGARDEN_CODENAME "Vesper")
#set(ROSEGARDEN_CODENAME "Worcestershire")
#set(ROSEGARDEN_CODENAME "Xerxes")
#set(ROSEGARDEN_CODENAME "Yesterday")
#set(ROSEGARDEN_CODENAME "Zepherine Drouhin")

# Make the build type default to "Release"
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Make it possible to use cmake files from the cmake subdir
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Enable the really useful automoc feature from cmake
set(CMAKE_AUTOMOC TRUE)

# Find headers in "." as well
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Used to set installation paths
# This takes care of installing into "lib64" on distros that use that, for instance,
# by setting CMAKE_INSTALL_FULL_LIBDIR.
include(GNUInstallDirs)

# Tweak RPATH handling so that installed debug builds work out of the box
include(RPathHandling)

# Activate unit tests (this enables `make test`)
enable_testing()
# and this defines the BUILD_TESTING option (and enabled CDash integration)
include(CTest)

# Check whether we explicitely select the Qt version to be used or autodetect
if(NOT USE_QT4 AND NOT USE_QT5)
   # Autodetect, prefering Qt 4
   message(STATUS "Autodetecting Qt version to use")
   find_package(Qt4 QUIET)
   if(Qt4_FOUND)
      set(USE_QT4 TRUE)
   endif()
endif()

if(USE_QT4)
   set(QT_MIN_VERSION "4.8.0")

   # Locate Qt4 on the system.
   find_package(Qt4 REQUIRED QtCore QtGui QtXml QtNetwork QtTest)

   # properly set up compile flags (QT_DEBUG/QT_NO_DEBUG, ...)
   include(${QT_USE_FILE})

   set(QT_QTCORE_LIBRARY Qt4::QtCore)
   set(QT_QTGUI_LIBRARY Qt4::QtGui)
   set(QT_QTXML_LIBRARY Qt4::QtXml)
   set(QT_QTNETWORK_LIBRARY Qt4::QtNetwork)
   set(QT_QTTEST_LIBRARY Qt4::QtTest)

else()

   # Locate Qt5 on the system.
   find_package(Qt5 "5.1.0" REQUIRED COMPONENTS Core Gui Widgets Xml Network PrintSupport Test)

   set(QT_QTCORE_LIBRARY Qt5::Core)
   set(QT_QTGUI_LIBRARY Qt5::Widgets Qt5::PrintSupport)
   set(QT_QTXML_LIBRARY Qt5::Xml)
   set(QT_QTNETWORK_LIBRARY Qt5::Network)
   set(QT_QTTEST_LIBRARY Qt5::Test)

endif()

# Compiler defines

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG -DBUILD_DEBUG -DWANT_TIMING)
else()
    add_definitions(-DNDEBUG -DBUILD_RELEASE -DNO_TIMING)
endif()

add_definitions(-DUNSTABLE) # this is changed to STABLE by the release script

# Compiler flags

if(CMAKE_COMPILER_IS_GNUCXX)
   if(CMAKE_SYSTEM_NAME MATCHES Linux OR CMAKE_SYSTEM_NAME STREQUAL GNU)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Woverloaded-virtual -Wextra -Wformat-nonliteral -Wformat-security -Winit-self -Wswitch-enum -Wnon-virtual-dtor -Wno-long-long -ansi -Wundef -Wcast-align -Wchar-subscripts -W -Wpointer-arith -fno-check-new -fno-common")
      add_definitions(-DQT_NO_EXCEPTIONS)
   endif()
endif()

# Looking for dependencies

find_package(PkgConfig REQUIRED)
include(FeatureSummary)

# Required dependencies ---------------------------------------

find_path(LADSPA_INCLUDE_DIR ladspa.h)
if(NOT LADSPA_INCLUDE_DIR)
    message(FATAL_ERROR "Failed to find required LADSPA header ladspa.h")
endif()

pkg_check_modules(ALSA alsa>=0.9)
add_feature_info(ALSA ALSA_FOUND "Alsa library (Advanced Linux Sound Architecture), used for MIDI support")
if(NOT ALSA_FOUND)
    # This little bit of juggling is to enable DSSI to be included even on
    # systems without ALSA (dssi.h includes an ALSA header, but we can
    # fill in the gap if ALSA itself is not present)
    message(WARNING "No ALSA library found: MIDI will be unavailable!")
    message(STATUS "Using compatibility code for DSSI event types")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/sound/dssi-compatibility)
endif()

find_path(DSSI_INCLUDE_DIRS dssi.h)
if(NOT DSSI_INCLUDE_DIRS)
    message(FATAL_ERROR "Failed to find required DSSI header dssi.h")
endif()

find_package(ZLIB REQUIRED)

find_package(X11 REQUIRED) # for XSetErrorHandler & XGetErrorText

pkg_check_modules(LIBLO REQUIRED liblo>=0.7)

pkg_check_modules(LRDF REQUIRED lrdf>=0.2)

pkg_check_modules(FFTW3F REQUIRED fftw3f>=3.0.0)

pkg_check_modules(SAMPLERATE REQUIRED samplerate>=0.1.2)

# Optional dependencies ---------------------------------------

pkg_check_modules(SNDFILE sndfile>=1.0.16)
add_feature_info(SNDFILE SNDFILE_FOUND "Better support for WAV files")

pkg_check_modules(JACK jack)
if(JACK_FOUND)
    add_feature_info(JACK JACK_FOUND "Library for accessing the JACK server (http://jackaudio.org).")
else()
    add_feature_info(JACK JACK_FOUND "WARNING: the library for accessing the JACK server (http://jackaudio.org) was not found. Audio will be unavailable!")
endif()

find_path(LIRCCLIENT_INCLUDE_DIR lirc/lirc_client.h)
if(NOT LIRCCLIENT_INCLUDE_DIR)
    message(WARNING "Failed to find LIRC header lirc/lirc_client.h")
endif()
find_library(LIRCCLIENT_LIBRARY lirc_client)
if(NOT LIRCCLIENT_LIBRARY)
    message(WARNING "Failed to find LIRC library lirc_client")
endif()
if(LIRCCLIENT_INCLUDE_DIR AND LIRCCLIENT_LIBRARY)
    set(LIRCCLIENT_FOUND TRUE)
endif()
add_feature_info(LIRCCLIENT LIRCCLIENT_FOUND "The LIRC client library, for remote control support")

# --------------------------------------------------------

# Print out a summary of what was found and what is missing
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# --------------------------------------------------------

# BUILD_TESTING is a builtin option in cmake, which defaults to ON.
# We'll set it to OFF in release mode, and let users (developers) choose in debug mode.

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BUILD_TESTING OFF)
endif()

# Unit tests disabled: use a static lib
# Unit tests enabled: use shared libs, to speed up linking
if(BUILD_TESTING)
    set(RG_LIBRARY_TYPE SHARED)
    message(STATUS "Building unit tests, using shared libraries")
else()
    set(RG_LIBRARY_TYPE STATIC)
    message(STATUS "Not building unit tests, using a static library for rosegarden")
endif()


# --------------------------------------------------------

add_subdirectory(src)
add_subdirectory(data)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
