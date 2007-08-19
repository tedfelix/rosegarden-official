# the next line sets up include and link directories and defines some variables that we will use.
# you can modify the behavior by setting some variables, e.g.
#   set(QT_USE_OPENGL TRUE)
# -> this will cause cmake to include and link against the OpenGL module
include(${QT_USE_FILE})

# the variable "qtproject_SRCS" contains all .cpp files of this project
set(matrix4_SRCS
    main.cpp
    gvtest.cpp
    GraphicsItemPin.cpp
    FadeItem.cpp
)

# tell cmake to create .moc files for all files in the variable qtproject_SRCS that require such a file.
# note: this assumes that you use #include "header.moc" in your files
qt4_automoc(${matrix4_SRCS})

# create an executable file named "matrix4" from the source files in the variable "matrix4_SRCS".
add_executable(matrix4 ${matrix4_SRCS})

# link the "matrix4" target against the Qt libraries. which libraries exactly, is defined by the "include(${QT_USE_FILE})" line above, which sets up this variable.
target_link_libraries(matrix4 ${QT_LIBRARIES})
