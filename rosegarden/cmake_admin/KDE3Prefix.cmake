FIND_PROGRAM(KDE3_KDECONFIG_EXECUTABLE 
    NAMES kde-config 
    PATHS
    $ENV{KDEDIR}/bin
    /opt/kde/bin
    /opt/kde3/bin )

# Provide KDE prefix as a default but overridable prefix
# Note: this only works in cmake 2.4.4
IF(KDE3_KDECONFIG_EXECUTABLE)
  IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    EXEC_PROGRAM( ${KDE3_KDECONFIG_EXECUTABLE}
      ARGS --prefix
      OUTPUT_VARIABLE KDE3_PREFIX )
    SET(CMAKE_INSTALL_PREFIX ${KDE3_PREFIX} CACHE PATH
        "Install path prefix, prepended onto install directories." FORCE)
  ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) 
ENDIF(KDE3_KDECONFIG_EXECUTABLE)
