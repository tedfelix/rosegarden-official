# - Try to find LADSPA header
# Once done this will define:
#
#  LADSPA_FOUND - system has LADSPA
#  LADSPA_INC_DIR - LADSPA header path

INCLUDE(CheckIncludeFile)

CHECK_INCLUDE_FILE("ladspa.h" HAVE_LADSPA_H)

IF(HAVE_LADSPA_H)
	FIND_PATH(LADSPA_INC_DIR "ladspa.h" "")
    SET(LADSPA_FOUND TRUE)
ENDIF(HAVE_LADSPA_H)
