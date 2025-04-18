RG_UNIT_TESTS(
   lilypond_export_test
)

if (VERSIONS_TEST)
   add_executable(lilypond_versions_test lilypond_versions_test.cpp)
   target_link_libraries(lilypond_versions_test ${QT_QTTEST_LIBRARY} ${QT_QTGUI_LIBRARY} rosegardenprivate)
   set_target_properties(lilypond_versions_test PROPERTIES COMPILE_FLAGS -DSRCDIR="\\"${CMAKE_CURRENT_SOURCE_DIR}/\\"")
endif()
