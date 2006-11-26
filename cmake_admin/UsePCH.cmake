MACRO(ADD_PRECOMPILED_HEADER _input)
	GET_FILENAME_COMPONENT(_name ${_input} NAME)
    SET(_source "${CMAKE_CURRENT_SOURCE_DIR}/${_input}")
	SET(_outdir "${CMAKE_CURRENT_BINARY_DIR}/${_name}.gch")
    MAKE_DIRECTORY(${_outdir})
    SET(_output "${_outdir}/${CMAKE_BUILD_TYPE}.c++")
    STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
    SET(_compiler_FLAGS ${${_flags_var_name}})
    SEPARATE_ARGUMENTS(_compiler_FLAGS)
    #MESSAGE("_compiler_FLAGS: ${_compiler_FLAGS}")
	ADD_CUSTOM_COMMAND(
	    OUTPUT ${_output}
	    COMMAND ${CMAKE_CXX_COMPILER}
	       ${_compiler_FLAGS}
	       -I${QT_INCLUDE_DIR}
	       -I${KDE3_INCLUDE_DIR}
	       ${QT_DEFINITIONS}
	       ${KDE3_DEFINITIONS}
	       -x c++-header
           -o ${_output} ${_source}
	    DEPENDS ${_source} )
    ADD_CUSTOM_TARGET(precompiled_headers DEPENDS ${_output})
    #SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-include ${_name} -Winvalid-pch -H")
    SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-include ${_name} -Winvalid-pch")
ENDMACRO(ADD_PRECOMPILED_HEADER)
