
MACRO(SEARCH_HEADERS _hprefix _hdir _output)
	SET(_regex  "^[^/]*#include +<${_hprefix}")
    EXECUTE_PROCESS(
    	COMMAND find . -type f -name *.h -or -name *.cpp
    	COMMAND xargs egrep -h ${_regex}
    	COMMAND sort -u
    	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE _search_RESULT
        ERROR_VARIABLE _search_ERROR
		OUTPUT_VARIABLE _search_OUTPUT 
		OUTPUT_STRIP_TRAILING_WHITESPACE )
	#MESSAGE("_search_RESULT: ${_search_RESULT}")
	#MESSAGE("_search_ERROR: ${_search_ERROR}")
	#MESSAGE("_search_OUTPUT: ${_search_OUTPUT}")
	IF(NOT _search_RESULT)
		STRING(REGEX REPLACE "\n" ";" _search_OUTPUT ${_search_OUTPUT})
		SET(_wrklist)
		FOREACH(_srcline ${_search_OUTPUT})
		    STRING(REGEX REPLACE "#include +<(.*)>" "\\1" _hfile ${_srcline})
		    SET(_hfile "${_hdir}/${_hfile}")
		    LIST(APPEND _wrklist ${_hfile})
		ENDFOREACH(_srcline)
		SET(${_output} ${_wrklist})
    ELSE(NOT _search_RESULT)
        MESSAGE(FATAL_ERROR "Error in SEARCH_HEADERS rc=${_search_RESULT} msg=${_search_ERROR}")
    ENDIF(NOT _search_RESULT)
ENDMACRO(SEARCH_HEADERS)

MACRO(ADD_QTKDE_PRECOMPILED_HEADERS _output_dir)
	SEARCH_HEADERS("q" ${QT_INCLUDE_DIR} QT_HEADERS)
    #MESSAGE("Qt Headers: ${QT_HEADERS}")
	SEARCH_HEADERS("k" ${KDE3_INCLUDE_DIR} KDE_HEADERS)
    #MESSAGE("KDE Headers: ${KDE_HEADERS}")
    SET(SOURCE_HEADERS)
    LIST(APPEND SOURCE_HEADERS ${QT_HEADERS})
    LIST(APPEND SOURCE_HEADERS ${KDE_HEADERS})
    #MESSAGE("SOURCE Headers: ${SOURCE_HEADERS}")
    SET(_flags_var_name)
    STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
    #MESSAGE("_flags_var_name: ${_flags_var_name}")
    SET(_compiler_FLAGS ${${_flags_var_name}})
    SEPARATE_ARGUMENTS(_compiler_FLAGS)
    #MESSAGE("_compiler_FLAGS: ${_compiler_FLAGS}")
    MAKE_DIRECTORY(${_output_dir}/${CMAKE_BUILD_TYPE})
    SET(_outputs)
    FOREACH(_file ${SOURCE_HEADERS})
		GET_FILENAME_COMPONENT(_name ${_file} NAME)
		SET(_out "${_output_dir}/${_name}.gch")
		ADD_CUSTOM_COMMAND(
		    OUTPUT ${_out}
		    COMMAND ${CMAKE_CXX_COMPILER}
		       ${_compiler_FLAGS}
		       -I${QT_INCLUDE_DIR}
		       -I${KDE3_INCLUDE_DIR}
		       ${QT_DEFINITIONS}
		       ${KDE3_DEFINITIONS}
		       -x c++-header
               -o ${_out} ${_file}
		    DEPENDS ${_file} )
		SET(_outputs ${_outputs} ${_out})
    ENDFOREACH(_file)
    ADD_CUSTOM_TARGET(precompiled_headers ALL DEPENDS ${_outputs})
	INCLUDE_DIRECTORIES(BEFORE ${_output_dir}/${CMAKE_BUILD_TYPE})
	SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Winvalid-pch")
ENDMACRO(ADD_QTKDE_PRECOMPILED_HEADERS)
