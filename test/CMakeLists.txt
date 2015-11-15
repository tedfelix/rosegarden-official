include_directories(../src)

# Code factorization: this macro defines unit tests
macro(RG_UNIT_TESTS)
   foreach(_testname ${ARGN})
      add_executable(${_testname} ${_testname}.cpp)
      add_test(${_testname} ${CMAKE_CURRENT_BINARY_DIR}/${_testname})
      target_link_libraries(${_testname} ${QT_QTTEST_LIBRARY} ${QT_QTGUI_LIBRARY} rosegardenprivate)
      set_target_properties(${_testname} PROPERTIES COMPILE_FLAGS -DSRCDIR="\\"${CMAKE_CURRENT_SOURCE_DIR}/\\"")
   endforeach(_testname)
endmacro()

# Each line here defines a unit test (the executable name matches the .cpp filename)
RG_UNIT_TESTS(
   accidentals
   segmenttransposecommand
   transpose
)

add_subdirectory(lilypond)
