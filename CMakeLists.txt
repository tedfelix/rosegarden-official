# CMake version required. This must be the very first line, because it sets default policies affecting everything else
cmake_minimum_required(VERSION 3.1)

# Project name
project(rosegarden)

# Version
set(ROSEGARDEN_VERSION "20.06")
set(ROSEGARDEN_CODENAME "Zepherine Drouhin")
#set(ROSEGARDEN_CODENAME "Altissimo")
#set(ROSEGARDEN_CODENAME "Bouquet d'Or")
#set(ROSEGARDEN_CODENAME "Cloud Nine")
#set(ROSEGARDEN_CODENAME "Distant Drums")
#set(ROSEGARDEN_CODENAME "Enchantress")
#set(ROSEGARDEN_CODENAME "Flutterbye")
#set(ROSEGARDEN_CODENAME "Grace Note")
#set(ROSEGARDEN_CODENAME "Hermosa")
#set(ROSEGARDEN_CODENAME "Incredible")
#set(ROSEGARDEN_CODENAME "Joyfulness")
#set(ROSEGARDEN_CODENAME "Keepsake")
#set(ROSEGARDEN_CODENAME "Let Freedom Ring")
#set(ROSEGARDEN_CODENAME "Mountain Music")
#set(ROSEGARDEN_CODENAME "Night Owl")
#set(ROSEGARDEN_CODENAME "Orange Flame")
#set(ROSEGARDEN_CODENAME "Purple Rain")
#set(ROSEGARDEN_CODENAME "Quietness")
#set(ROSEGARDEN_CODENAME "Raphaela")
#set(ROSEGARDEN_CODENAME "Sagesse de Lune")
#set(ROSEGARDEN_CODENAME "Tangerine Dream")
#set(ROSEGARDEN_CODENAME "Uncle Buck")
#set(ROSEGARDEN_CODENAME "Van Gogh")
#set(ROSEGARDEN_CODENAME "Waterlily")
#set(ROSEGARDEN_CODENAME "Xenophon")
#set(ROSEGARDEN_CODENAME "Yellow Brick Road")
#set(ROSEGARDEN_CODENAME "Zaïd")

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
# then define the BUILD_TESTING option, with the default value that we want
# We'll set it to OFF in release mode, and let users (developers) choose in debug mode.
# include(CTest) would make it ON by default in all cases, so we don't use that

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    option(BUILD_TESTING "Build the testing tree." OFF)
else()
    option(BUILD_TESTING "Build the testing tree." ON)
endif()

# Compiler defines

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DNDEBUG -DBUILD_RELEASE -DNO_TIMING)
else()
    add_definitions(-DDEBUG -DBUILD_DEBUG -DWANT_TIMING)
endif()

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
add_definitions(-DUNSTABLE) # this is changed to STABLE by the release script

# Compiler flags

set(CMAKE_CXX_STANDARD 11) # Enable C++11

if(CMAKE_COMPILER_IS_GNUCXX)
   if(CMAKE_SYSTEM_NAME MATCHES Linux OR CMAKE_SYSTEM_NAME STREQUAL GNU)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Woverloaded-virtual -Wextra -Wformat-nonliteral -Wformat-security -Winit-self -Wswitch-enum -Wnon-virtual-dtor -Wno-long-long -ansi -Wundef -Wcast-align -Wchar-subscripts -W -Wpointer-arith -fno-check-new -fno-common -Werror=return-type")
      add_definitions(-DQT_NO_EXCEPTIONS)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
      if (CMAKE_COMPILER_IS_GNUCC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.7))
          # Too many left, someone needs to finish this...
          #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wzero-as-null-pointer-constant") # suggest using nullptr where applicable
      endif()
      if (CMAKE_COMPILER_IS_GNUCC AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 5.1))
         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsuggest-override")
      endif()
   endif()
endif()

# Looking for dependencies

find_package(PkgConfig REQUIRED)
include(FeatureSummary)

# Required dependencies ---------------------------------------

# Qt5 is required.
find_package(Qt5 "5.1.0" REQUIRED COMPONENTS Core Gui Widgets Xml Network PrintSupport Test)

set(QT_QTCORE_LIBRARY Qt5::Core)
set(QT_QTGUI_LIBRARY Qt5::Widgets Qt5::PrintSupport)
set(QT_QTXML_LIBRARY Qt5::Xml)
set(QT_QTNETWORK_LIBRARY Qt5::Network)
set(QT_QTTEST_LIBRARY Qt5::Test)

set(QT_RCC_EXECUTABLE ${Qt5Core_RCC_EXECUTABLE})


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

# Build with LIRC support if LIRC detected, unless DISABLE_LIRC is set
if(NOT DISABLE_LIRC)
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
endif()
add_feature_info(LIRCCLIENT LIRCCLIENT_FOUND "The LIRC client library, for remote control support")

# --------------------------------------------------------

# Print out a summary of what was found and what is missing
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# --------------------------------------------------------


# Unit tests disabled: use a static lib
# Unit tests enabled: use shared libs, to speed up linking
if(BUILD_TESTING)
    set(RG_LIBRARY_TYPE SHARED)
    message(STATUS "Building unit tests, using shared libraries")
else()
    set(RG_LIBRARY_TYPE STATIC)
    set(ROSEGARDENPRIVATE_STATIC_BUILD TRUE) # for generating rosegardenprivate-export.h
    message(STATUS "Not building unit tests, using a static library for rosegarden")
endif()


# --------------------------------------------------------

add_subdirectory(src)
add_subdirectory(data)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
