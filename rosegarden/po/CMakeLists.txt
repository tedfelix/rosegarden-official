# .po to .mo

FILE(GLOB _pofiles *.po)

FOREACH(_file ${_pofiles})
	GET_FILENAME_COMPONENT(_file_we ${_file} NAME_WE)
	SET(_out "${CMAKE_CURRENT_BINARY_DIR}/${_file_we}.gmo")
	SET(_in  "${CMAKE_CURRENT_SOURCE_DIR}/${_file_we}.po")
	ADD_CUSTOM_COMMAND(
		OUTPUT ${_out}
		COMMAND ${MSGFMT_EXECUTABLE} -o ${_out} ${_in}
		DEPENDS ${_in}
	)
	INSTALL(FILES ${_out}
		DESTINATION share/locale/${_file_we}/LC_MESSAGES/
		RENAME ${BASE_NAME}.mo
	)
	SET(_outputs ${_outputs} ${_out})
ENDFOREACH(_file)

ADD_CUSTOM_TARGET(
	pofiles ALL
	DEPENDS ${_outputs}
)
