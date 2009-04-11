# Rosegarden
# A MIDI and audio sequencer and musical notation editor.
#
# This program is Copyright 2000-2008
#     Guillaume Laurent   <glaurent@telegraph-road.org>,
#     Chris Cannam        <cannam@all-day-breakfast.com>,
#     Richard Bown        <richard.bown@ferventsoftware.com>
#
# The moral rights of Guillaume Laurent, Chris Cannam, and Richard
# Bown to claim authorship of this work have been asserted.
#
# This file is Copyright 2006-2008
#     Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>
#
# Other copyrights also apply to some parts of this work.  Please
# see the AUTHORS file and individual file headers for details.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.  See the file
# COPYING included with this distribution for more information.

# - Try to find LADSPA header
# Once done this will define:
#
#  LADSPA_FOUND - system has LADSPA
#  LADSPA_INCLUDE_DIR - LADSPA header path

IF(LADSPA_INCLUDE_DIR)
   SET(LADSPA_FIND_QUIETLY TRUE)
ENDIF(LADSPA_INCLUDE_DIR)
   
FIND_PATH(LADSPA_INCLUDE_DIR "ladspa.h"
   /usr/include
   /usr/local/include
)

IF(LADSPA_INCLUDE_DIR)
   SET(LADSPA_FOUND TRUE)
ELSE(LADSPA_INCLUDE_DIR)
   SET(LADSPA_FOUND FALSE)
   SET(LADSPA_INCLUDE_DIR "")
ENDIF(LADSPA_INCLUDE_DIR)

IF(LADSPA_FOUND)
    IF(NOT LADSPA_FIND_QUIETLY)
        MESSAGE(STATUS "Found LADSPA: ${LADSPA_INCLUDE_DIR}")
    ENDIF(NOT LADSPA_FIND_QUIETLY)
ELSE(LADSPA_FOUND)
    IF(LADSPA_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find LADSPA")
    ENDIF(LADSPA_FIND_REQUIRED)
ENDIF(LADSPA_FOUND)

MARK_AS_ADVANCED(LADSPA_INCLUDE_DIR)
