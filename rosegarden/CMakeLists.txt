# The name of our project is "ROSEGARDEN". CMakeLists files in this project can
# refer to the root source directory of the project as ${ROSEGARDEN_SOURCE_DIR} 
# and to the root binary directory of the project as ${ROSEGARDEN_BINARY_DIR}
PROJECT(ROSEGARDEN)

# CMake supports KDE checks since this version number
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

# Show full commands and messages. 
# Necessary for Eclipse warning/error parser
SET(CMAKE_VERBOSE_MAKEFILE ON)

# Seems that relative paths break the link scripts. Don't set it ON
SET(CMAKE_USE_RELATIVE_PATHS OFF)

# Additional CMake modules for 3rd party library checks reside here
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_admin")

# Rosegarden version string
SET (VERSION_MAJOR "1")
SET (VERSION_MINOR "5")
SET (VERSION_PATCH "0")
SET (VERSION_SUFFIX "reorg")
SET (VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_SUFFIX})
ADD_DEFINITIONS(-D'VERSION="${VERSION}"')
MESSAGE(STATUS "Rosegarden ${VERSION} will be built for install into ${CMAKE_INSTALL_PREFIX}")

# BUILD SETUP: None, Debug, Release, .. or custom ones
SET(WANT_DEBUG ON)
IF(WANT_DEBUG)
  SET(CMAKE_BUILD_TYPE DEBUG)
ELSE(WANT_DEBUG)
  SET(CMAKE_BUILD_TYPE RELEASE)
ENDIF(WANT_DEBUG)

# Check for KDE3
FIND_PACKAGE(KDE3 REQUIRED)
ADD_DEFINITIONS(${QT_DEFINITIONS} ${KDE3_DEFINITIONS})

# Check for ALSA 1.0
FIND_PACKAGE(ALSA REQUIRED)
IF(ALSA_FOUND)
  SET(HAVE_ALSA TRUE)
  ADD_DEFINITIONS(-DHAVE_ALSA)
ENDIF(ALSA_FOUND)

# TODO: Other checks, assumed true for now,
# These lines will be replaced with real checks ASAP
ADD_DEFINITIONS(-DHAVE_LIBJACK)
ADD_DEFINITIONS(-DHAVE_DSSI)
ADD_DEFINITIONS(-DHAVE_LADSPA)
ADD_DEFINITIONS(-DHAVE_LIBLO)
ADD_DEFINITIONS(-DHAVE_LIBLRDF)
ADD_DEFINITIONS(-DHAVE_XFT)
SET(OTHER_INCLUDES /usr/local/include /usr/include/freetype2)
SET(OTHER_LIBS "-ljack -llo -llrdf -lraptor -lmad -llirc_client")
SET(OTHER_FLAGS "-fexceptions -iquote ${ROSEGARDEN_SOURCE_DIR}/src")
# end other checks

SET(CMAKE_CXX_FLAGS "${ALSA_CFLAGS} ${CMAKE_CXX_FLAGS} ${OTHER_FLAGS}")
SET(CMAKE_CXX_LINK_FLAGS "${ALSA_LIBS} ${OTHER_LIBS} ${CMAKE_CXX_LINK_FLAGS}")

# These directories will be overriden later. 
LINK_DIRECTORIES(${KDE3_LIB_DIR} ${ALSA_LIB_DIR})
INCLUDE_DIRECTORIES( 
   ${KDE3_INCLUDE_DIR} 
   ${QT_INCLUDE_DIR} 
   ${ALSA_INCLUDE_DIR} 
   ${OTHER_INCLUDES}
)

# Recurse into the "src" subdirectory. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
ADD_SUBDIRECTORY(src)

#TODO: we should install some files...
