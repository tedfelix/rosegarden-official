# - Try to find liblo 0.7
# Once done this will define:
#
#  LO_FOUND - system has LO
#  LO_LIBRARY - LO library name
#  LO_CFLAGS - Compiler switches required for using LO
#  LO_LIBS - Linker flags
#  LO_LIB_DIR - Library directory
#  LO_INC_DIR - Include diretory
#  LO_VERSION - LO version found

SET(CMAKE_INCLUDE_PATH ".")
INCLUDE(PkgConfigV)

PKGCONFIGV(liblo 0.7 _LOVersion _LOIncDir _LOLinkDir _LOLinkFlags _LOCflags)

SET(LO_CFLAGS ${_LOCflags})
SET(LO_LIBS ${_LOLinkFlags})
SET(LO_LIB_DIR ${_LOLinkDir})
SET(LO_INC_DIR ${_LOIncDir})
SET(LO_VERSION ${_LOVersion})

SEPARATE_ARGUMENTS(LO_LIBS)

FIND_LIBRARY(LO_LIBRARY
  NAMES lo
  PATHS ${_LOLinkDir} /usr/lib /usr/local/lib
)

IF(LO_LIBRARY)
    SET(LO_FOUND TRUE)
ENDIF(LO_LIBRARY)

IF(NOT LO_FOUND)
    IF(LO_FIND_REQUIRED)
	MESSAGE(FATAL_ERROR "Could not find liblo >= 0.7")
    ENDIF(LO_FIND_REQUIRED)
ENDIF(NOT LO_FOUND)

MARK_AS_ADVANCED(LO_LIBRARY LO_CFLAGS LO_LIBS)
