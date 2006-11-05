# - Try to find ALSA 1.0
# Once done this will define:
#
#  ALSA_FOUND - system has ALSA
#  ALSA_LIBRARY - ALSA library name
#  ALSA_CFLAGS - Compiler switches required for using ALSA
#  ALSA_LIBS - Linker flags
#  ALSA_LIB_DIR - Library directory
#  ALSA_INC_DIR - Include diretory
#

SET(CMAKE_INCLUDE_PATH ".")
INCLUDE(PkgConfigV)

PKGCONFIGV(alsa 1.0 _ALSAIncDir _ALSALinkDir _ALSALinkFlags _ALSACflags)

SET(ALSA_CFLAGS ${_ALSACflags})
SET(ALSA_LIBS ${_ALSALinkFlags})
SET(ALSA_LIB_DIR ${_ALSALinkDir})
SET(ALSA_INC_DIR ${_ALSAIncDir})

FIND_LIBRARY(ALSA_LIBRARY
  NAMES asound
  PATHS ${_ALSALinkDir} /usr/lib /usr/local/lib
)

IF(ALSA_LIBRARY)
   SET(ALSA_FOUND TRUE)
ENDIF(ALSA_LIBRARY)

IF(ALSA_FOUND)
  IF(NOT ALSA_FIND_QUIETLY)
    MESSAGE(STATUS "Found ALSA 1.0: ${ALSA_LIBRARY}")
  ENDIF(NOT ALSA_FIND_QUIETLY)
ELSE(ALSA_FOUND)
  IF(ALSA_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find ALSA 1.0")
  ENDIF(ALSA_FIND_REQUIRED)
ENDIF(ALSA_FOUND)

# show the ALSA_INCLUDE_DIR and ALSA_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(ALSA_LIBRARY ALSA_CFLAGS ALSA_LIBS)
