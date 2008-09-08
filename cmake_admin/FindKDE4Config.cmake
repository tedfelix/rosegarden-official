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
# Find the kde-config program and retrieve the install dirs

# Variables:
#       HAVE_KDECONFIG
#       KDECONFIG_EXECUTABLE
#       KDE4PREFIX
#       KDE4HTMLDIR
#       KDE4DATADIR
#       KDE4ICONDIR
#       KDE4MIMEDIR
#       KDE4MENUDIR
#       KDE4EXECDIR
#       KDE4L18NDIR



# provide an ALIAS for old kde3 variable:
IF(KDE4_KDECONFIG_EXECUTABLE)
	set(KDECONFIG_EXECUTABLE "${KDE4_KDECONFIG_EXECUTABLE}")
	#
ENDIF(KDE4_KDECONFIG_EXECUTABLE)


IF(KDECONFIG_EXECUTABLE)
    SET(HAVE_KDECONFIG TRUE)
    MESSAGE("Note: KDECONFIG was already set, to: ${KDECONFIG_EXECUTABLE} ")
ELSE(KDECONFIG_EXECUTABLE)
    FIND_PROGRAM(KDECONFIG_EXECUTABLE NAMES kde4-config PATHS
    	/usr/bin
        $ENV{KDEDIR}/bin
        /opt/kde4/bin
        /opt/kde/bin
    	/usr/local/bin
        NO_DEFAULT_PATH
    )
    FIND_PROGRAM(KDECONFIG_EXECUTABLE kde4-config)
ENDIF(KDECONFIG_EXECUTABLE)


# simplified it, so it works hopfully....
#IF(NOT KDE4PREFIX)
IF(1)
    EXECUTE_PROCESS(COMMAND ${KDECONFIG_EXECUTABLE} --prefix
                    OUTPUT_VARIABLE KDE4PREFIX )
    # add slash, remove \n
    STRING(REGEX REPLACE "\n" "" KDE4PREFIX "${KDE4PREFIX}/")
#    set(KDE4PREFIX "${KDE4PREFIX}/")
    MESSAGE(STATUS "KDE4PREFIX : ${KDE4PREFIX}")
    
#    EXECUTE_PROCESS(COMMAND ${KDECONFIG_EXECUTABLE} --version
#                    OUTPUT_VARIABLE kde_config_version )
    #STRING(REGEX MATCH "KDE: .\\."  ${kde_config_version})
#    set(kde_version "KDE: 4")
#    MESSAGE(STATUS "KDE CONFIG VERSION : ${kde_config_version}")
#    IF (${kde_version} MATCHES "KDE: 4\\.")
#        EXECUTE_PROCESS(COMMAND ${KDECONFIG_EXECUTABLE} --prefix
#                	OUTPUT_VARIABLE kdedir )
#        STRING(REGEX REPLACE "\n" "" KDE4PREFIX "${kdedir}/")
#    ENDIF (${kde_version} MATCHES "KDE: 4\\.")
#ENDIF(NOT KDE4PREFIX)
ENDIF(1)

IF(NOT KDE4HTMLDIR)
    EXECUTE_PROCESS(COMMAND ${KDECONFIG_EXECUTABLE} --expandvars --install html
        OUTPUT_VARIABLE _htmlinstalldir)
    STRING(REGEX REPLACE "\n" "" _htmlinstalldir "${_htmlinstalldir}")
#    MESSAGE(STATUS "KDE4HTMLDIRx: ${_htmlinstalldir}")
#    set(KDE4HTMLDIR "${_htmlinstalldir}")
    STRING(REPLACE "${KDE4PREFIX}" "" KDE4HTMLDIR "${_htmlinstalldir}")
    MESSAGE(STATUS "KDE4HTMLDIR : ${KDE4HTMLDIR}")
ENDIF(NOT KDE4HTMLDIR)

IF(NOT KDE4DATADIR)
    EXECUTE_PROCESS(COMMAND ${KDECONFIG_EXECUTABLE} --expandvars --install data
        OUTPUT_VARIABLE _datainstalldir)
    STRING(REGEX REPLACE "\n" "" _datainstalldir "${_datainstalldir}")
    STRING(REPLACE "${KDE4PREFIX}" "" KDE4DATADIR "${_datainstalldir}")
    MESSAGE(STATUS "KDE4DATADIR : ${KDE4DATADIR}")
ENDIF(NOT KDE4DATADIR)

IF(NOT KDE4ICONDIR)
    EXECUTE_PROCESS(COMMAND ${KDECONFIG_EXECUTABLE} --expandvars --install icon
        OUTPUT_VARIABLE _iconinstalldir)
    STRING(REGEX REPLACE "\n" "" _iconinstalldir "${_iconinstalldir}")
    STRING(REPLACE "${KDE4PREFIX}" "" KDE4ICONDIR "${_iconinstalldir}")
    MESSAGE(STATUS "KDE4ICONDIR : ${KDE4ICONDIR}")
ENDIF(NOT KDE4ICONDIR)

IF(NOT KDE4MIMEDIR)
    EXECUTE_PROCESS(COMMAND ${KDECONFIG_EXECUTABLE} --expandvars --install mime
        OUTPUT_VARIABLE _mimeinstalldir)
    STRING(REGEX REPLACE "\n" "" _mimeinstalldir "${_mimeinstalldir}")
    STRING(REPLACE "${KDE4PREFIX}" "" KDE4MIMEDIR "${_mimeinstalldir}")
    MESSAGE(STATUS "KDE4MIMEDIR : ${KDE4MIMEDIR}")
ENDIF(NOT KDE4MIMEDIR)

IF(NOT KDE4MENUDIR)
    EXECUTE_PROCESS(COMMAND ${KDECONFIG_EXECUTABLE} --expandvars --install xdgdata-apps
        OUTPUT_VARIABLE _menuinstalldir)
    STRING(REGEX REPLACE "\n" "" _menuinstalldir "${_menuinstalldir}")
    STRING(REPLACE "${KDE4PREFIX}" "" KDE4MENUDIR "${_menuinstalldir}")
    MESSAGE(STATUS "KDE4MENUDIR : ${KDE4MENUDIR}")
ENDIF(NOT KDE4MENUDIR)

IF(NOT KDE4L18NDIR)
    EXECUTE_PROCESS(COMMAND ${KDECONFIG_EXECUTABLE} --expandvars --install locale
        OUTPUT_VARIABLE _l18ninstalldir)
    STRING(REGEX REPLACE "\n" "" _l18ninstalldir "${_l18ninstalldir}")
    STRING(REPLACE "${KDE4PREFIX}" "" KDE4L18NDIR "${_l18ninstalldir}")
    MESSAGE(STATUS "KDE4L18NDIR : ${KDE4L18NDIR}")
ENDIF(NOT KDE4L18NDIR)

IF(NOT KDE4EXECDIR)
    EXECUTE_PROCESS(COMMAND ${KDECONFIG_EXECUTABLE} --expandvars --install exe
        OUTPUT_VARIABLE _execinstalldir)
    STRING(REGEX REPLACE "\n" "" _execinstalldir "${_execinstalldir}")
    STRING(REPLACE "${KDE4PREFIX}" "" KDE4EXECDIR "${_execinstalldir}")
    MESSAGE(STATUS "KDE4EXECDIR : ${KDE4EXECDIR}")
ENDIF(NOT KDE4EXECDIR)
