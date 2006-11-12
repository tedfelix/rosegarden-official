# The name of our project is "ROSEGARDEN". CMakeLists files in this project can
# refer to the root source directory of the project as ${ROSEGARDEN_SOURCE_DIR} 
# and to the root binary directory of the project as ${ROSEGARDEN_BINARY_DIR}
PROJECT(ROSEGARDEN)

# CMake supports KDE checks since this version number
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.2)

# Show full commands and messages. It generates a lot of console output.
# It is necessary for Eclipse warning/error parser, 
# but it can be enabled anyway with "make VERBOSE=1"
SET(CMAKE_VERBOSE_MAKEFILE OFF)

# Seems that relative paths break the link scripts. Don't set it ON
SET(CMAKE_USE_RELATIVE_PATHS OFF)

# Misc settings
SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
SET(CMAKE_SKIP_RULE_DEPENDENCY ON)

# Additional CMake modules for 3rd party library checks reside here
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_admin")

# Rosegarden version string
SET (VERSION_MAJOR "1")
SET (VERSION_MINOR "5")
SET (VERSION_PATCH "0")
SET (VERSION_SUFFIX "reorg")
SET (VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_SUFFIX})
ADD_DEFINITIONS(-D'VERSION="${VERSION}"')

SET(BASE_NAME "rosegarden")
SET(VERSIONED_NAME "${BASE_NAME}-${VERSION}")
SET(DOCDIR "share/docs/${VERSIONED_NAME}")

# User options
OPTION(WANT_DEBUG "Include debug support" OFF)
OPTION(WANT_SOUND "Include Sound support" ON)
OPTION(WANT_ALSA  "Include ALSA support" ON)
OPTION(WANT_JACK  "Include Jack support" ON)
OPTION(WANT_DSSI  "Include DSSI support" ON)
OPTION(WANT_LIRC  "Include LIRC support" ON)

IF(NOT WANT_SOUND)
    SET(WANT_ALSA OFF)
    SET(WANT_JACK OFF)
    SET(WANT_DSSI OFF)
    ADD_DEFINITIONS(-DNO_SOUND)
ENDIF(NOT WANT_SOUND)

# Check for KDE3
FIND_PACKAGE(KDE3 REQUIRED QUIET)
IF(QT_FOUND)
    ADD_DEFINITIONS(${QT_DEFINITIONS})
    MESSAGE(STATUS "Found Qt3 (version ${qt_version_str})")
    #MESSAGE("QT_LIBRARIES: ${QT_LIBRARIES}")
ENDIF(QT_FOUND)
IF(KDE3_FOUND)
    ADD_DEFINITIONS(${KDE3_DEFINITIONS})
    MESSAGE(STATUS "Found KDE3")
    #MESSAGE("QT_AND_KDECORE_LIBS: ${QT_AND_KDECORE_LIBS}")
ENDIF(KDE3_FOUND)

# This FindKDE3 module sets stupid flags, and forgets to set KDE install prefix
SET(CMAKE_CXX_FLAGS "")
INCLUDE(KDE3Prefix)
MESSAGE(STATUS "Rosegarden ${VERSION} will be built for install into ${CMAKE_INSTALL_PREFIX}")
    
# Check for Xft
FIND_PACKAGE(XFT)
IF(XFT_FOUND)
    ADD_DEFINITIONS(-DHAVE_XFT)
    SET(HAVE_XFT TRUE)
    MESSAGE(STATUS "Found XFT (version ${XFT_VERSION})")
    #MESSAGE("XFT_LIBRARY: ${XFT_LIBRARY}")
    #MESSAGE("XFT_LIBS: ${XFT_LIBS}")
    #MESSAGE("XFT_LIB_DIR: ${XFT_LIB_DIR}")
    #MESSAGE("XFT_INC_DIR: ${XFT_INC_DIR}")    
ENDIF(XFT_FOUND)

IF(WANT_SOUND)
    # Check for ALSA 1.0
    IF(WANT_ALSA)
        FIND_PACKAGE(ALSA REQUIRED)
        IF(ALSA_FOUND)
            SET(HAVE_ALSA TRUE)
            ADD_DEFINITIONS(-DHAVE_ALSA)
            MESSAGE(STATUS "Found ALSA (version ${ALSA_VERSION})")
            #MESSAGE("ALSA_LIBRARY: ${ALSA_LIBRARY}")
            #MESSAGE("ALSA_CFLAGS: ${ALSA_CFLAGS}")
            #MESSAGE("ALSA_LIBS: ${ALSA_LIBS}")
            #MESSAGE("ALSA_LIB_DIR: ${ALSA_LIB_DIR}")
            #MESSAGE("ALSA_INC_DIR: ${ALSA_INC_DIR}")
        ENDIF(ALSA_FOUND)
    ENDIF(WANT_ALSA)
    
    IF(WANT_JACK)
        FIND_PACKAGE(JACK)
        IF(JACK_FOUND)
            ADD_DEFINITIONS(-DHAVE_LIBJACK)
            SET(HAVE_JACK TRUE)
            #MESSAGE(STATUS "Found JACK (version ${JACK_VERSION})")
            #MESSAGE("JACK_LIBRARY: ${JACK_LIBRARY}")
            #MESSAGE("JACK_LIBS: ${JACK_LIBS}")
            #MESSAGE("JACK_LIB_DIR: ${JACK_LIB_DIR}")
            #MESSAGE("JACK_INC_DIR: ${JACK_INC_DIR}")
        ENDIF(JACK_FOUND)
    ENDIF(WANT_JACK)
    
    IF(WANT_DSSI)
        FIND_PACKAGE(DSSI)
        IF(DSSI_FOUND)
            ADD_DEFINITIONS(-DHAVE_DSSI)
            SET(HAVE_DSSI TRUE)
            MESSAGE(STATUS "Found DSSI (version ${DSSI_VERSION})")
            #MESSAGE("DSSI_INC_DIR: ${DSSI_INC_DIR}")
        ENDIF(DSSI_FOUND)
    ENDIF(WANT_DSSI)
    
    FIND_PACKAGE(LO)
    IF(LO_FOUND)
        ADD_DEFINITIONS(-DHAVE_LIBLO)
        SET(HAVE_LIBLO TRUE)
        MESSAGE(STATUS "Found liblo (version ${LO_VERSION})")
        #MESSAGE("LO_LIBRARY: ${LO_LIBRARY}")
        #MESSAGE("LO_LIBS: ${LO_LIBS}")
        #MESSAGE("LO_LIB_DIR: ${LO_LIB_DIR}")
        #MESSAGE("LO_INC_DIR: ${LO_INC_DIR}")
    ENDIF(LO_FOUND)
    
    FIND_PACKAGE(LADSPA)
    IF(LADSPA_FOUND)
        SET(HAVE_LADSPA TRUE)
        ADD_DEFINITIONS(-DHAVE_LADSPA)
        MESSAGE(STATUS "Found LADSPA")
        #MESSAGE("LADSPA_INC_DIR: ${LADSPA_INC_DIR}")    
    ENDIF(LADSPA_FOUND)
    
    FIND_PACKAGE(LRDF)
    IF(LRDF_FOUND)
        SET(HAVE_LRDF TRUE)
        ADD_DEFINITIONS(-DHAVE_LRDF)
        MESSAGE(STATUS "Found LRDF")
        #MESSAGE("LRDF_LIBS: ${LRDF_LIBS}")
        #MESSAGE("LRDF_LIBRARY: ${LRDF_LIBRARY}")
        #MESSAGE("LRDF_INC_DIR: ${LRDF_INC_DIR}")
    ENDIF(LRDF_FOUND)
ENDIF(WANT_SOUND)

IF(WANT_LIRC)
    FIND_PACKAGE(LIRC)
    IF(LIRC_FOUND)
        SET(HAVE_LIRC TRUE)
        ADD_DEFINITIONS(-DHAVE_LIRC)
        MESSAGE(STATUS "Found LIRC")
        #MESSAGE("LIRC_LIBS: ${LIRC_LIBS}")
        #MESSAGE("LIRC_LIBRARY: ${LIRC_LIBRARY}")
        #MESSAGE("LIRC_INC_DIR: ${LIRC_INC_DIR}")    
    ENDIF(LIRC_FOUND)
ENDIF(WANT_LIRC)

FIND_PACKAGE(Msgfmt)
IF(MSGFMT_FOUND)
    MESSAGE(STATUS "Found msgfmt")
ELSE(MSGFMT_FOUND)
    MESSAGE(STATUS "WARNING: msgfmt program not found. PO files processing disabled.")
ENDIF(MSGFMT_FOUND)

SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -w -fexceptions")
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3 -Wall -fexceptions")

IF(WANT_DEBUG)
    MESSAGE(STATUS "Configured to compile including debug information in the executables")
    SET(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Choose the type of build, options are: Debug Release" FORCE)
    ADD_DEFINITIONS(-DDEBUG)
ELSE(WANT_DEBUG)
    MESSAGE(STATUS "Configured to compile for release without debug information")
    SET(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: Debug Release" FORCE)
    ADD_DEFINITIONS(-DNDEBUG)
ENDIF(WANT_DEBUG)

# Warn the user if important libraries are missing
INCLUDE(UserWarnings)

# Custom target trying to clean the CMake generated files
ADD_CUSTOM_TARGET(distclean
    COMMAND sh scripts/distclean.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
ADD_DEPENDENCIES(distclean clean)

# uninstall custom target
CONFIGURE_FILE(
    "${CMAKE_SOURCE_DIR}/cmake_admin/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Recurse into the "src" subdirectory. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
# Output directory will be "RGbuild/"
ADD_SUBDIRECTORY(src RGbuild)

# Build and install the translations directory
IF(MSGFMT_FOUND)
    ADD_SUBDIRECTORY(po RGbuild/po)
ENDIF(MSGFMT_FOUND)

# More directories. Some need a build process, and all have an INSTALL target.
#ADD_SUBDIRECTORY(docs)
#ADD_SUBDIRECTORY(data)

# install some documents...
INSTALL( FILES AUTHORS COPYING README TRANSLATORS
         DESTINATION ${DOCDIR} )
