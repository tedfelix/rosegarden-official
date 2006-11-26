# - pkg-config module for CMake with version checking support
#
# Defines the following macros:
#
# PKGCONFIGV(package version includedir libdir linkflags cflags)
#
# Calling PKGCONFIGV will fill the desired information into the given arguments.
# e.g. PKGCONFIGV(alsa 1.0 ALSA_VERSION ALSA_INC_DIR ALSA_CFLAGS ALSA_LINK_DIR ALSA_LINK_FLAGS)
# if pkg-config was NOT found or the specified software package doesn't exist, the
# variable will be empty when the function returns, otherwise they will contain the respective information
#

FIND_PROGRAM(PKGCONFIG_EXECUTABLE NAMES pkg-config PATHS /usr/local/bin )

MACRO(PKGCONFIGV _package _version _fversion _include_DIR _link_DIR _link_FLAGS _cflags)
# reset the variables at the beginning
  SET(${_include_DIR})
  SET(${_link_DIR})
  SET(${_link_FLAGS})
  SET(${_cflags})
  SET(${_fversion})
  SET(_AUX_FLAGS "")

  # if pkg-config has been found
  IF(PKGCONFIG_EXECUTABLE)

    EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --atleast-version=${_version} RETURN_VALUE _return_VALUE OUTPUT_VARIABLE _pkgconfigDevNull )
    ##MESSAGE("pkg-config return value: ${_return_VALUE}")

    # and if the package of interest also exists for pkg-config, then get the information
    IF(NOT _return_VALUE)

      EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --modversion OUTPUT_VARIABLE ${_fversion} )

      EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --variable=includedir OUTPUT_VARIABLE ${_include_DIR} )

      EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --variable=libdir OUTPUT_VARIABLE ${_link_DIR} )

      EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --libs-only-l OUTPUT_VARIABLE ${_link_FLAGS} )

      EXEC_PROGRAM(${PKGCONFIG_EXECUTABLE} ARGS ${_package} --cflags-only-other OUTPUT_VARIABLE ${_cflags} )
      
    ENDIF(NOT _return_VALUE)

  ENDIF(PKGCONFIG_EXECUTABLE)

ENDMACRO(PKGCONFIGV _fversion _include_DIR _link_DIR _link_FLAGS _cflags)

MARK_AS_ADVANCED(PKGCONFIG_EXECUTABLE)
