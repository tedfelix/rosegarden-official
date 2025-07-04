# CMake version required. This must be the very first line, because it sets
# default policies affecting everything else.
cmake_minimum_required(VERSION 3.5)

# Project name
project(rosegarden)

# Version
set(ROSEGARDEN_VERSION "25.12")
set(ROSEGARDEN_CODENAME "Keepsake")
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
#set(ROSEGARDEN_CODENAME "Amadeus")
#set(ROSEGARDEN_CODENAME "Blue Moon")
#set(ROSEGARDEN_CODENAME "Cabaret")
#set(ROSEGARDEN_CODENAME "Duet")
#set(ROSEGARDEN_CODENAME "Etude")
#set(ROSEGARDEN_CODENAME "Foxtrot")
#set(ROSEGARDEN_CODENAME "Gloire du Midi")
#set(ROSEGARDEN_CODENAME "Harmonie")
#set(ROSEGARDEN_CODENAME "Intermezzo")
#set(ROSEGARDEN_CODENAME "Jazz")
#set(ROSEGARDEN_CODENAME "Koto")
#set(ROSEGARDEN_CODENAME "Lyric")
#set(ROSEGARDEN_CODENAME "Mozart")
#set(ROSEGARDEN_CODENAME "Nocturne")
#set(ROSEGARDEN_CODENAME "Operetta")
#set(ROSEGARDEN_CODENAME "Piccolo")
#set(ROSEGARDEN_CODENAME "Quintet")
#set(ROSEGARDEN_CODENAME "Rumba")
#set(ROSEGARDEN_CODENAME "Symphony")
#set(ROSEGARDEN_CODENAME "Tenor")
#set(ROSEGARDEN_CODENAME "Unforgettable")
#set(ROSEGARDEN_CODENAME "Violina")
#set(ROSEGARDEN_CODENAME "Wind Chimes")
#set(ROSEGARDEN_CODENAME "Xanadu")
#set(ROSEGARDEN_CODENAME "Yodel")
#set(ROSEGARDEN_CODENAME "Ziggy Stardust")

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

set(CMAKE_CXX_STANDARD 14) # Enable C++14

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

option(USE_QT6 "Compile with Qt6." OFF)

if (USE_QT6)
  # Qt6 is required.
  find_package(Qt6 "6.0.0" REQUIRED COMPONENTS CoreTools Core GuiTools Gui WidgetsTools Widgets Xml Network PrintSupport Test Core5Compat)

  set(QT_QTCORE_LIBRARY Qt6::Core)
  set(QT_QTCORE5COMPAT_LIBRARY Qt6::Core5Compat)
  set(QT_QTGUI_LIBRARY Qt6::Widgets Qt6::PrintSupport)
  set(QT_QTXML_LIBRARY Qt6::Xml)
  set(QT_QTNETWORK_LIBRARY Qt6::Network)
  set(QT_QTTEST_LIBRARY Qt6::Test)

  set(QT_RCC_EXECUTABLE Qt::rcc)
else()
  # Qt5 is required.
  find_package(Qt5 "5.1.0" REQUIRED COMPONENTS Core Gui Widgets Xml Network PrintSupport Test)

  set(QT_QTCORE_LIBRARY Qt5::Core)
  set(QT_QTGUI_LIBRARY Qt5::Widgets Qt5::PrintSupport)
  set(QT_QTXML_LIBRARY Qt5::Xml)
  set(QT_QTNETWORK_LIBRARY Qt5::Network)
  set(QT_QTTEST_LIBRARY Qt5::Test)

  set(QT_RCC_EXECUTABLE ${Qt5Core_RCC_EXECUTABLE})
endif()

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

pkg_check_modules(PC_JACK jack)
# Use find_library to turn it into a full path
find_library(JACK_LIBRARIES NAMES ${PC_JACK_LIBRARIES} jack PATHS ${PC_JACK_LIBRARY_DIRS})
if(JACK_LIBRARIES)
    set(JACK_FOUND TRUE)
endif()
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

# lilv for lv2 plugins
pkg_check_modules(LILV lilv-0)
if(NOT LILV_INCLUDE_DIRS)
  message(WARNING "Failed to find Lilv header lilv/lilv.h for lv2 plugins")
endif()

pkg_check_modules(LV2 lv2)
if(LILV_INCLUDE_DIRS AND LILV_LIBRARIES AND LV2_INCLUDE_DIRS)
  set(LILV_FOUND TRUE)
endif()
add_feature_info(LILV LILV_FOUND "The Lilv library, for lv2 plugins")

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

#########################
# target for cppcheck

set(CPPCHECK_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cppcheck.out)

# dummy_file will never exist so "make cppcheck" always runs cppcheck

#add_custom_command(OUTPUT ${CPPCHECK_OUTPUT} dummy_file
#  COMMAND cppcheck -I${CMAKE_CURRENT_SOURCE_DIR}/src --force --enable=all --inconclusive --std=c++14 --library=qt --inline-suppr --quiet --suppress=missingInclude --suppress=missingIncludeSystem ${CMAKE_CURRENT_SOURCE_DIR}/src >& ${CPPCHECK_OUTPUT})
add_custom_command(OUTPUT ${CPPCHECK_OUTPUT} dummy_file
  COMMAND $(CMAKE_SOURCE_DIR)/scripts/cppcheck-rg $(CMAKE_SOURCE_DIR)/src >& ${CPPCHECK_OUTPUT})

add_custom_target(cppcheck DEPENDS ${CPPCHECK_OUTPUT} dummy_file)
#########################


# --------------------------------------------------------

add_subdirectory(src)
add_subdirectory(data)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
