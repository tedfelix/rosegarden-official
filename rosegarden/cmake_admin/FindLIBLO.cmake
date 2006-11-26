# - Try to find liblo 0.7
#
# Once done this will define:
#
#  LIBLO_FOUND - system has LO
#  LIBLO_LIBRARY - liblo library name
#  LIBLO_CFLAGS - Compiler switches required for using liblo
#  LIBLO_LIBS - Linker flags
#  LIBLO_LIB_DIR - Library directory
#  LIBLO_INC_DIR - Include diretory
#  LIBLO_VERSION - LO version found

SET(CMAKE_INCLUDE_PATH ".")
INCLUDE(PkgConfigV)

PKGCONFIGV(liblo 0.7 _LOVersion _LOIncDir _LOLinkDir _LOLinkFlags _LOCflags)

SET(LIBLO_CFLAGS ${_LOCflags})
SET(LIBLO_LIBS ${_LOLinkFlags})
SET(LIBLO_LIB_DIR ${_LOLinkDir})
SET(LIBLO_INC_DIR ${_LOIncDir})
SET(LIBLO_VERSION ${_LOVersion})

SEPARATE_ARGUMENTS(LIBLO_LIBS)

FIND_LIBRARY(LIBLO_LIBRARY
  NAMES lo
  PATHS ${_LOLinkDir} /usr/lib /usr/local/lib
)

IF(LIBLO_LIBRARY)
    SET(LIBLO_FOUND TRUE)
ENDIF(LIBLO_LIBRARY)

IF(NOT LIBLO_FOUND)
    IF(LIBLO_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Could not find liblo >= 0.7")
    ENDIF(LIBLO_FIND_REQUIRED)
ENDIF(NOT LIBLO_FOUND)

MARK_AS_ADVANCED(LIBLO_LIBRARY LIBLO_CFLAGS LIBLO_LIBS)
