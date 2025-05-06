RG_UNIT_TESTS(
   lilypond_export_test
)

if (LILY_VERSIONS_TEST)
   add_executable(lilypond_versions_test lilypond_versions_test.cpp)
   target_link_libraries(lilypond_versions_test ${QT_QTTEST_LIBRARY} ${QT_QTGUI_LIBRARY} rosegardenprivate)
   set_target_properties(lilypond_versions_test PROPERTIES COMPILE_FLAGS -DSRCDIR="\\"${CMAKE_CURRENT_SOURCE_DIR}/\\"")

   set(LILYPOND_VERSIONS_TEST_SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/lilypond_versions_test_scripts)
   add_custom_command(
      OUTPUT versions_test_run_lilypond.sh versions_test_check_results.sh versions_test_synthesis.raku versions_test_config.example
      COMMAND ${CMAKE_COMMAND} -E create_symlink ${LILYPOND_VERSIONS_TEST_SCRIPTS}/versions_test_run_lilypond.sh versions_test_run_lilypond.sh
      COMMAND ${CMAKE_COMMAND} -E create_symlink ${LILYPOND_VERSIONS_TEST_SCRIPTS}/versions_test_check_results.sh versions_test_check_results.sh
      COMMAND ${CMAKE_COMMAND} -E create_symlink ${LILYPOND_VERSIONS_TEST_SCRIPTS}/versions_test_synthesis.raku versions_test_synthesis.raku
      COMMAND ${CMAKE_COMMAND} -E copy ${LILYPOND_VERSIONS_TEST_SCRIPTS}/versions_test_config.example versions_test_config.example
   )
   add_custom_target(
      versions_scripts ALL DEPENDS
               versions_test_run_lilypond.sh
               versions_test_check_results.sh
               versions_test_synthesis.raku
               versions_test_config.example
   )
   add_dependencies(lilypond_versions_test versions_scripts)
endif()
