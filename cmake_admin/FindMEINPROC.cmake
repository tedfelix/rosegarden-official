# Find the MEINPROC program
#
# Defined variables:
#  MEINPROC_FOUND
#  MEINPROC_EXECUTABLE
#
# Macro:
#  ADD_DOCS

IF(MEINPROC_EXECUTABLE)
    SET(MEINPROC_FOUND TRUE)
ELSE(MEINPROC_EXECUTABLE)
    FIND_PROGRAM(MEINPROC_EXECUTABLE
        NAME meinproc 
	PATHS ${KDE3_BIN_INSTALL_DIR}
         $ENV{KDEDIR}/bin
	 /usr/bin
	 /usr/local/bin
	 /opt/kde/bin
         /opt/kde3/bin )
    IF(MEINPROC_EXECUTABLE)
        SET(MEINPROC_FOUND TRUE)
    ELSE(MEINPROC_EXECUTABLE)
	IF(NOT MEINPROC_FIND_QUIETLY)
	    IF(MEINPROC_FIND_REQUIRED)
		MESSAGE(FATAL_ERROR "Program meinproc couldn't be found")
	    ENDIF(MEINPROC_FIND_REQUIRED)
	ENDIF(NOT MEINPROC_FIND_QUIETLY)
    ENDIF(MEINPROC_EXECUTABLE)
    MARK_AS_ADVANCED(MEINPROC_EXECUTABLE)
ENDIF (MEINPROC_EXECUTABLE)

MACRO(ADD_DOCS _baseName)
    SET(_outputs)
    FOREACH(_dir ${ARGN})
	SET(_out "${CMAKE_CURRENT_BINARY_DIR}/${_dir}_index.cache.bz2")
	SET(_in  "${CMAKE_CURRENT_SOURCE_DIR}/${_dir}/index.docbook")
	FILE(GLOB _images ${_dir}/*.png)
	ADD_CUSTOM_COMMAND(OUTPUT ${_out}
	    COMMAND ${MEINPROC_EXECUTABLE}
	    ARGS --check --cache ${_out} ${_in}
    	    DEPENDS ${_in} )
	INSTALL(FILES ${_out}
    	    DESTINATION share/doc/HTML/${_dir}/${_baseName}
	    RENAME index.cache.bz2)
	INSTALL(FILES ${_in} ${_images}
    	    DESTINATION share/doc/HTML/${_dir}/${_baseName})
	SET(_outputs ${_outputs} ${_out})
    ENDFOREACH(_dir)
    ADD_CUSTOM_TARGET(documentation ALL DEPENDS ${_outputs})
ENDMACRO(ADD_DOCS)
