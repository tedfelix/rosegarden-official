
FIND_PROGRAM( KDE3_KDECONFIG_EXECUTABLE 
    NAMES kde-config 
    PATHS
	$ENV{KDEDIR}/bin
	/opt/kde/bin
	/opt/kde3/bin )
	
EXEC_PROGRAM( ${KDE3_KDECONFIG_EXECUTABLE}
    ARGS --prefix
    OUTPUT_VARIABLE KDE3_PREFIX )

SET( CMAKE_INSTALL_PREFIX ${KDE3_PREFIX} CACHE PATH
     "Install path prefix, prepended onto install directories." 
     FORCE )
