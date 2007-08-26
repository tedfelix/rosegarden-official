project(matrix4) # the name of your project

cmake_minimum_required(VERSION 2.4.0)

# Show full commands and messages. It generates a lot of console output.
# It is necessary for Eclipse warning/error parser, 
# but it can be enabled anyway with "make VERBOSE=1"
SET(CMAKE_VERBOSE_MAKEFILE OFF)

# Seems that relative paths break some scripts. Don't set it ON
SET(CMAKE_USE_RELATIVE_PATHS OFF)

# Rebuild the object files if the rules have changed, but not the actual source 
# files or headers (e.g. if you changed the some compiler switches)
SET(CMAKE_SKIP_RULE_DEPENDENCY OFF)

# Misc settings
SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

# Additional CMake modules for 3rd party library checks reside here
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_admin")

FIND_PACKAGE(PCHSupport)
IF(PCHSupport_FOUND)
    SET(USE_PCH TRUE)
    MESSAGE(STATUS "Enabling precompiled headers for GCC ${gcc_compiler_version}") 
ENDIF(PCHSupport_FOUND)


find_package(Qt4 REQUIRED) # find and setup Qt4 for this project

# tell cmake to process CMakeLists.txt in that subdirectory
add_subdirectory(src)
