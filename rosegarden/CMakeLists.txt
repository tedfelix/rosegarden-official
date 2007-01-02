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

# Seems that relative paths break some scripts. Don't set it ON
SET(CMAKE_USE_RELATIVE_PATHS OFF)

# Rebuild the object files if the rules have changed, but not the actual source 
# files or headers (e.g. if you changed the some compiler switches)
SET(CMAKE_SKIP_RULE_DEPENDENCY OFF)

# Misc settings
SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

# Additional CMake modules for 3rd party library checks reside here
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_admin")

# Rosegarden version string
SET (VERSION_MAJOR "1")
SET (VERSION_MINOR "5")
SET (VERSION_PATCH "0")
SET (VERSION_SUFFIX "svn")
SET (VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_SUFFIX})
ADD_DEFINITIONS(-D'VERSION="${VERSION}"')

# User options
OPTION(WANT_DEBUG   "Include debug support" OFF)
OPTION(WANT_FULLDBG "Full debug support (BIG executables!)" OFF)
OPTION(WANT_SOUND   "Include Sound support" ON)
OPTION(WANT_JACK    "Include Jack (Jack Audio Connection Kit) support" ON)
OPTION(WANT_DSSI    "Include DSSI support" ON)
OPTION(WANT_LIRC    "Include LIRC (Linux Infrared Remote Control) support" ON)
OPTION(WANT_PCH     "Use precompiled headers" OFF)

IF(NOT WANT_SOUND)
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
ELSE(QT_FOUND)
    MESSAGE(FATAL_ERROR "Qt3 package not found")
ENDIF(QT_FOUND)
IF(KDE3_FOUND)
    ADD_DEFINITIONS(${KDE3_DEFINITIONS})
    #MESSAGE("KDE3PREFIX: ${KDE3PREFIX}")
    KDE3_PRINT_RESULTS()
    #MESSAGE("QT_AND_KDECORE_LIBS: ${QT_AND_KDECORE_LIBS}")
ELSE(KDE3_FOUND)    
    MESSAGE(FATAL_ERROR "KDE3 Not found")
ENDIF(KDE3_FOUND)

# Clear the flags set by FindKDE3 
SET(CMAKE_CXX_FLAGS "")

# Provide the KDE3 prefix as a default prefix (if the user has CMake 2.4.4)
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX ${KDE3PREFIX} CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) 

# Check for PKG-CONFIG
FIND_PACKAGE(PkgConfig REQUIRED)
IF(PKG_CONFIG_FOUND)
    MESSAGE(STATUS "Program pkg-config found (${PKG_CONFIG_EXECUTABLE})")
ELSE(PKG_CONFIG_FOUND)
    MESSAGE(FATAL_ERROR "Program pkg-config not found")
ENDIF(PKG_CONFIG_FOUND)

# Initialize some variables
SET(HAVE_XFT FALSE)
SET(HAVE_ALSA FALSE)
SET(HAVE_JACK FALSE)
SET(HAVE_DSSI FALSE)
SET(HAVE_LIBLO FALSE)
SET(HAVE_LIBLRDF FALSE)
SET(HAVE_LADSPA FALSE)
SET(HAVE_LIRC FALSE)
SET(HAVE_FFTW3F FALSE)

# Check for Xft
PKG_CHECK_MODULES(XFT xft>=2.1.0)
IF(XFT_FOUND)
    ADD_DEFINITIONS(-DHAVE_XFT)
    SET(HAVE_XFT TRUE)
    SET(XFT_LIBS ${XFT_LIBRARIES})
    #MESSAGE("XFT_LIBS: ${XFT_LIBS}")
    LIST(APPEND XFT_LIB_DIR ${XFT_LIBRARY_DIRS} ${XFT_LIBDIR})
    #MESSAGE("XFT_LIB_DIR: ${XFT_LIB_DIR}")
    LIST(APPEND XFT_INC_DIR ${XFT_INCLUDE_DIRS} ${XFT_INCLUDEDIR})
    #MESSAGE("XFT_INC_DIR: ${XFT_INC_DIR}")    
ENDIF(XFT_FOUND)

IF(WANT_SOUND)

    # Check for ALSA 1.0
    PKG_CHECK_MODULES(ALSA REQUIRED alsa>=1.0)
    IF(ALSA_FOUND)
        SET(HAVE_ALSA TRUE)
        ADD_DEFINITIONS(-DHAVE_ALSA)
        SET(ALSA_LIBS ${ALSA_LIBRARIES})
        #MESSAGE("ALSA_LIBS: ${ALSA_LIBS}")
        LIST(APPEND ALSA_LIB_DIR ${ALSA_LIBRARY_DIRS} ${ALSA_LIBDIR} )
        #MESSAGE("ALSA_LIB_DIR: ${ALSA_LIB_DIR}")
        LIST(APPEND ALSA_INC_DIR ${ALSA_INCLUDE_DIRS} ${ALSA_INCLUDEDIR})
        #MESSAGE("ALSA_INC_DIR: ${ALSA_INC_DIR}")
    ELSE(ALSA_FOUND)
        MESSAGE(FATAL_ERROR "Please install the required package. Aborting")
    ENDIF(ALSA_FOUND)
    
    IF(WANT_JACK)
        PKG_CHECK_MODULES(JACK jack>=0.77)
        IF(JACK_FOUND)
            ADD_DEFINITIONS(-DHAVE_LIBJACK)
            SET(HAVE_JACK TRUE)
            SET(JACK_LIBS ${JACK_LIBRARIES})
            #MESSAGE("JACK_LIBS: ${JACK_LIBS}")
            LIST(APPEND JACK_LIB_DIR ${JACK_LIBDIR} ${JACK_LIBRARY_DIRS})
            #MESSAGE("JACK_LIB_DIR: ${JACK_LIB_DIR}")
            LIST(APPEND JACK_INC_DIR ${JACK_INCLUDEDIR} ${JACK_INCLUDE_DIRS})
            #MESSAGE("JACK_INC_DIR: ${JACK_INC_DIR}")
        ENDIF(JACK_FOUND)
    ENDIF(WANT_JACK)
    
    IF(WANT_DSSI)
        PKG_CHECK_MODULES(DSSI dssi>=0.4)
        IF(DSSI_FOUND)
            ADD_DEFINITIONS(-DHAVE_DSSI)
            SET(HAVE_DSSI TRUE)
            LIST(APPEND DSSI_INC_DIR ${DSSI_INCLUDE_DIRS} ${DSSI_INCLUDEDIR})
            #MESSAGE("DSSI_INC_DIR: ${DSSI_INC_DIR}")
        ENDIF(DSSI_FOUND)
    ENDIF(WANT_DSSI)
    
    PKG_CHECK_MODULES(LIBLO liblo>=0.7)
    IF(LIBLO_FOUND)
        ADD_DEFINITIONS(-DHAVE_LIBLO)
        SET(HAVE_LIBLO TRUE)
        SET(LIBLO_LIBS ${LIBLO_LIBRARIES})
        #MESSAGE("LIBLO_LIBS: ${LIBLO_LIBS}")
        LIST(APPEND LIBLO_LIB_DIR ${LIBLO_LIBDIR} ${LIBLO_LIBRARY_DIRS})
        #MESSAGE("LIBLO_LIB_DIR: ${LIBLO_LIB_DIR}")
        LIST(APPEND LIBLO_INC_DIR ${LIBLO_INCLUDEDIR} ${LIBLO_INCLUDE_DIRS})
        #MESSAGE("LIBLO_INC_DIR: ${LIBLO_INC_DIR}")
    ENDIF(LIBLO_FOUND)
    
    PKG_CHECK_MODULES(LRDF lrdf>=0.3)
    IF(LRDF_FOUND)
        SET(HAVE_LIBLRDF TRUE)
        ADD_DEFINITIONS(-DHAVE_LIBLRDF)
        SET(LRDF_LIBS ${LRDF_LIBRARIES})
        #MESSAGE("LRDF_LIBS: ${LRDF_LIBS}")
        LIST(APPEND LRDF_LIB_DIR ${LRDF_LIBDIR} ${LRDF_LIBRARY_DIRS})
        #MESSAGE("LRDF_LIB_DIR: ${LRDF_LIB_DIR}")
        LIST(APPEND LRDF_INC_DIR ${LRDF_INCLUDEDIR} ${LRDF_INCLUDE_DIRS})
        #MESSAGE("LRDF_INC_DIR: ${LRDF_INC_DIR}")
    ENDIF(LRDF_FOUND)

    FIND_PACKAGE(LADSPA QUIET)
    IF(LADSPA_FOUND)
        SET(HAVE_LADSPA TRUE)
        ADD_DEFINITIONS(-DHAVE_LADSPA)
        SET(LADSPA_INC_DIR ${LADSPA_INCLUDE_DIR})
        MESSAGE(STATUS "Found LADSPA (${LADSPA_INC_DIR})")
    ENDIF(LADSPA_FOUND)
    
ENDIF(WANT_SOUND)

PKG_CHECK_MODULES(FFTW3F REQUIRED fftw3f>=3.0.0)
IF(FFTW3F_FOUND)
    ADD_DEFINITIONS(-DHAVE_FFTW3F)
    SET(HAVE_FFTW3F TRUE)
    SET(FFTW3F_LIBS ${FFTW3F_LIBRARIES})
    #MESSAGE("FFTW3F_LIBS: ${FFTW3F_LIBS}")
    LIST(APPEND FFTW3F_LIB_DIR ${FFTW3F_LIBDIR} ${FFTW3F_LIBRARY_DIRS})
    #MESSAGE("FFTW3F_LIB_DIR: ${FFTW3F_LIB_DIR}")
    LIST(APPEND FFTW3F_INC_DIR ${FFTW3F_INCLUDEDIR} ${FFTW3F_INCLUDE_DIRS})
    #MESSAGE("FFTW3F_INC_DIR: ${FFTW3F_INC_DIR}")
ELSE(FFTW3F_FOUND)
    MESSAGE(FATAL_ERROR "Please install the required package. Aborting")
ENDIF(FFTW3F_FOUND)

IF(WANT_LIRC)
    FIND_PACKAGE(LIRC QUIET)
    IF(LIRC_FOUND)
        SET(HAVE_LIRC TRUE)
        ADD_DEFINITIONS(-DHAVE_LIRC)
        MESSAGE(STATUS "Found LIRC (${LIRC_LIBRARY})")
        SET(LIRC_LIBS ${LIRC_LIBRARY})
        SET(LIRC_INC_DIR ${LIRC_INCLUDE_DIR})
        SET(LIRC_LIB_DIR ${LIRC_LIBRARY_DIR})
        #MESSAGE("LIRC_LIBS: ${LIRC_LIBS}")
        #MESSAGE("LIRC_LIB_DIR: ${LIRC_LIB_DIR}")
        #MESSAGE("LIRC_INC_DIR: ${LIRC_INC_DIR}")    
    ENDIF(LIRC_FOUND)
ENDIF(WANT_LIRC)

FIND_PACKAGE(MSGFMT REQUIRED)
IF(MSGFMT_FOUND)
    MESSAGE(STATUS "Program msgfmt found (${MSGFMT_EXECUTABLE})")
ENDIF(MSGFMT_FOUND)

FIND_PACKAGE(MEINPROC REQUIRED)
IF(MEINPROC_FOUND)
    MESSAGE(STATUS "Program meinproc found (${MEINPROC_EXECUTABLE})")
ENDIF(MEINPROC_FOUND)

# Check GCC for PCH support
SET(USE_PCH FALSE)
IF(WANT_PCH)
    FIND_PACKAGE(PCHSupport)
    IF(PCHSupport_FOUND)
        SET(USE_PCH TRUE)
        MESSAGE(STATUS "Enabling precompiled headers for GCC ${gcc_compiler_version}") 
    ENDIF(PCHSupport_FOUND)
ENDIF(WANT_PCH)

SET(CMAKE_CXX_FLAGS_RELEASE         "-O2 -w -fexceptions -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO  "-O2 -g -Wall -fexceptions -DDEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG           "-O0 -g3 -Wall -fexceptions -DDEBUG")

MESSAGE(STATUS "Rosegarden ${VERSION} will be built for install into ${CMAKE_INSTALL_PREFIX}")

IF(WANT_DEBUG)
    IF(WANT_FULLDBG)
        MESSAGE(STATUS "Configured to compile including full debug information in the executables")
        SET(CMAKE_BUILD_TYPE Debug CACHE STRING
            "Choose the type of build, options are: Debug Release RelWithDebInfo" FORCE)
    ELSE(WANT_FULLDBG)
        MESSAGE(STATUS "Configured to compile including debug information in the executables")
        SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
            "Choose the type of build, options are: Debug Release RelWithDebInfo" FORCE)
    ENDIF(WANT_FULLDBG)
ELSE(WANT_DEBUG)
    MESSAGE(STATUS "Configured to compile for release without debug information")
    SET(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: Debug Release RelWithDebInfo" FORCE)
ENDIF(WANT_DEBUG)

# Warn the user if important libraries are missing
INCLUDE(UserWarnings)

# Recurse into the "src" subdirectory. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
# Output directory will be "RGbuild/"
ADD_SUBDIRECTORY(src RGbuild)

# Build and install the translations directory
ADD_SUBDIRECTORY(po RGbuild/po)

# Build and install the documentation directory
ADD_SUBDIRECTORY(docs RGbuild/docs)

# Install data files
ADD_SUBDIRECTORY(data RGbuild/data)

# install some documents...
# INSTALL( FILES AUTHORS COPYING README TRANSLATORS INSTALL rosegarden.lsm
#         DESTINATION share/doc/rosegarden-${VERSION} )

# create and install the version file
FILE(WRITE version.txt "${VERSION}\n")
INSTALL( FILES version.txt
         DESTINATION share/apps/rosegarden )

# uninstall custom target
CONFIGURE_FILE(
    "${CMAKE_SOURCE_DIR}/cmake_admin/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Custom target trying to clean the CMake generated files
ADD_CUSTOM_TARGET ( distclean
    COMMAND rm -f  {,src/,po/,docs/,data/}CMakeCache.txt
    COMMAND rm -f  {,src/,po/,docs/,data/}cmake_install.cmake
    COMMAND rm -f  {,src/,po/,docs/,data/}cmake_uninstall.cmake
    COMMAND rm -f  {,src/,po/,docs/,data/}CPackConfig.cmake
    COMMAND rm -f  {,src/,po/,docs/,data/}CPackSourceConfig.cmake
    COMMAND rm -f  {,src/,po/,docs/,data/}install_manifest.txt
    COMMAND rm -f  {,src/,po/,docs/,data/}progress.make
    COMMAND rm -f  {,src/,po/,docs/,data/}rosegarden.spec
    COMMAND rm -f  {,src/,po/,docs/,data/}Makefile
    COMMAND rm -rf {,src/,po/,docs/,data/}CMakeFiles
    COMMAND rm -rf {,src/,po/,docs/,data/}_CPack_Packages
    COMMAND rm -rf RGbuild/*
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
ADD_DEPENDENCIES(distclean clean)

# configure the RPM spec
CONFIGURE_FILE(
    "${CMAKE_SOURCE_DIR}/rosegarden.spec.in"
    "${CMAKE_SOURCE_DIR}/rosegarden.spec"
    IMMEDIATE @ONLY)

# tarball target
ADD_CUSTOM_TARGET ( tarball
    COMMAND mkdir -p rosegarden-${VERSION}
    COMMAND cp -r cmake_admin rosegarden-${VERSION}
    COMMAND cp -r data rosegarden-${VERSION}
    COMMAND cp -r docs rosegarden-${VERSION}
    COMMAND cp -r po rosegarden-${VERSION}
    COMMAND cp -r src rosegarden-${VERSION}
    COMMAND cp CMakeLists.txt AUTHORS COPYING Doxyfile INSTALL README rosegarden.* TRANSLATORS rosegarden-${VERSION}
    COMMAND tar -cj --exclude .svn --exclude gettext-0.10.35-kde -f rosegarden-${VERSION}.tar.bz2 rosegarden-${VERSION}
    COMMAND rm -rf rosegarden-${VERSION}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
