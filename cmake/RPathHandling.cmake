# Taken from extra-cmake-modules KDECMakeSettings.cmake

#=============================================================================
# Copyright 2014      Alex Merry <alex.merry@kde.org>
# Copyright 2013      Aleix Pol <aleixpol@kde.org>
# Copyright 2012-2013 Stephen Kelly <steveire@gmail.com>
# Copyright 2007      Matthias Kretz <kretz@kde.org>
# Copyright 2006-2007 Laurent Montel <montel@kde.org>
# Copyright 2006-2013 Alex Neundorf <neundorf@kde.org>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file COPYING-CMAKE-SCRIPTS for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================

#
# Runtime Paths
# ~~~~~~~~~~~~~
#
# The default runtime path (used on Unix systems to search for
# dynamically-linked libraries) is set to include the location that libraries
# will be installed to (as set in CMAKE_INSTALL_LIBDIR), and also the linker search
# path.
#
# Note that ``CMAKE_INSTALL_LIBDIR`` needs to be set before including this module.
# Typically, this is done by including GNUInstallDirs.cmake
#
# This section can be disabled by setting ``KDE_SKIP_RPATH_SETTINGS`` to TRUE
# before including this module.
#

################# RPATH handling ##################################

if(NOT KDE_SKIP_RPATH_SETTINGS)

   # Set the default RPATH to point to useful locations, namely where the
   # libraries will be installed and the linker search path

   if(NOT CMAKE_INSTALL_LIBDIR)
      message(FATAL_ERROR "CMAKE_INSTALL_LIBDIR not set. This is necessary for using the RPATH settings.")
   endif()

   set(_abs_LIB_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")
   if (NOT IS_ABSOLUTE "${_abs_LIB_INSTALL_DIR}")
      set(_abs_LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
   endif()

   if (UNIX)
      # for mac os: add install name dir in addition
      # check: is the rpath stuff below really required on mac os? at least it seems so to use a stock qt from qt.io
      if (APPLE)
         set(CMAKE_INSTALL_NAME_DIR ${_abs_LIB_INSTALL_DIR})
      endif ()

      # add our LIB_INSTALL_DIR to the RPATH (but only when it is not one of
      # the standard system link directories - such as /usr/lib on UNIX)
      list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${_abs_LIB_INSTALL_DIR}" _isSystemLibDir)
      list(FIND CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES      "${_abs_LIB_INSTALL_DIR}" _isSystemCxxLibDir)
      list(FIND CMAKE_C_IMPLICIT_LINK_DIRECTORIES        "${_abs_LIB_INSTALL_DIR}" _isSystemCLibDir)
      if("${_isSystemLibDir}" STREQUAL "-1"  AND  "${_isSystemCxxLibDir}" STREQUAL "-1"  AND  "${_isSystemCLibDir}" STREQUAL "-1")
         set(CMAKE_INSTALL_RPATH "${_abs_LIB_INSTALL_DIR}")
      endif()

      # Append directories in the linker search path (but outside the project)
      set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
   endif (UNIX)

endif()

