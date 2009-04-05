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

# - Try to find LIRC
# Once done this will define:
#
#  LIRC_FOUND - system has LIRC
#  LIRC_LIBRARY - LIRC library
#  LIRC_INCLUDE_DIR - LIRC include path
#  LIRC_LIBRARY_DIR - LIRC library path

IF(LIRC_INCLUDE_DIR)
    SET(LIRC_FIND_QUIETLY TRUE)
ENDIF(LIRC_INCLUDE_DIR)

FIND_PATH(LIRC_INCLUDE_DIR "lirc/lirc_client.h"
    /usr/include
    /usr/local/include
)

FIND_LIBRARY(LIRC_LIBRARY 
    NAMES lirc_client
    PATHS /usr/lib /usr/local/lib
)

IF(LIRC_INCLUDE_DIR AND LIRC_LIBRARY)
    SET(LIRC_FOUND TRUE)
    GET_FILENAME_COMPONENT(LIRC_LIBRARY_DIR ${LIRC_LIBRARY} PATH)
ELSE(LIRC_INCLUDE_DIR AND LIRC_LIBRARY)
    SET(LIRC_FOUND FALSE)
    SET(LIRC_LIBRARY_DIR)
ENDIF(LIRC_INCLUDE_DIR AND LIRC_LIBRARY)

IF(LIRC_FOUND)
    IF(NOT LIRC_FIND_QUIETLY)
        MESSAGE(STATUS "Found LIRC: ${LIRC_LIBRARY}")
    ENDIF(NOT LIRC_FIND_QUIETLY)
ELSE(LIRC_FOUND)
    IF(LIRC_FIND_REQUIRED)
        MESSAGE(FATAL_ERROR "Could not find LIRC library")
    ENDIF(LIRC_FIND_REQUIRED)
ENDIF(LIRC_FOUND)

MARK_AS_ADVANCED(LIRC_LIBRARY LIRC_INCLUDE_DIR)
