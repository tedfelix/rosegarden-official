RG_UNIT_TESTS(
   lilypond_export_test
)

if (MANUAL_TEST)
   add_executable(manual_test lilypond_manual_test.cpp)
   target_link_libraries(manual_test ${QT_QTTEST_LIBRARY} ${QT_QTGUI_LIBRARY} rosegardenprivate)
   set_target_properties(manual_test PROPERTIES COMPILE_FLAGS -DSRCDIR="\\"${CMAKE_CURRENT_SOURCE_DIR}/\\"")
endif()
