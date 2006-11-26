# - Try to find JACK 0.77.0
# Once done this will define:
#
#  JACK_FOUND - system has JACK
#  JACK_LIBRARY - JACK library name
#  JACK_CFLAGS - Compiler switches required for using JACK
#  JACK_LIBS - Linker flags
#  JACK_LIB_DIR - Library directory
#  JACK_INC_DIR - Include diretory
#  JACK_VERSION - JACK version found

SET(CMAKE_INCLUDE_PATH ".")
INCLUDE(PkgConfigV)

PKGCONFIGV(jack 0.77.0 _JACKVersion _JACKIncDir _JACKLinkDir _JACKLinkFlags _JACKCflags)

SET(JACK_CFLAGS ${_JACKCflags})
SET(JACK_LIBS ${_JACKLinkFlags})
SET(JACK_LIB_DIR ${_JACKLinkDir})
SET(JACK_INC_DIR ${_JACKIncDir})
SET(JACK_VERSION ${_JACKVersion})

SEPARATE_ARGUMENTS(JACK_LIBS)

FIND_LIBRARY(JACK_LIBRARY
  NAMES jack
  PATHS ${_JACKLinkDir} /usr/lib /usr/local/lib
)

IF(JACK_LIBRARY)
    SET(JACK_FOUND TRUE)
ENDIF(JACK_LIBRARY)

IF(NOT JACK_FOUND)
    IF(JACK_FIND_REQUIRED)
	MESSAGE(FATAL_ERROR "Could not find JACK >= 0.77.0")
    ENDIF(JACK_FIND_REQUIRED)
ENDIF(NOT JACK_FOUND)

MARK_AS_ADVANCED(JACK_LIBRARY JACK_CFLAGS JACK_LIBS)
