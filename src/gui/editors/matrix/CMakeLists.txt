# ########## Project setup ##########
PROJECT(matrix)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## matrix library ##########
# Sources:
SET(matrix_library_SRCS
    MatrixParameterBox.cpp
    OldMatrixView.cpp
    MatrixSelector.cpp
    MatrixWidget.cpp
    MatrixResizer.cpp
    MatrixToolBox.cpp
    MatrixMover.cpp
    MatrixScene.cpp
    MatrixVelocity.cpp
    MatrixPainter.cpp
    PianoKeyboard.cpp
    MatrixView.cpp
    MatrixTool.cpp
    MatrixEraser.cpp
    MatrixViewSegment.cpp
    MatrixElement.cpp
    MatrixStaff.cpp
)

# Headers:
SET(matrix_library_HDRS
    MatrixView.h
    MatrixTool.h
    MatrixEraser.h
    MatrixElement.h
    MatrixParameterBox.h
    MatrixViewSegment.h
    MatrixStaff.h
    OldMatrixView.h
    MatrixSelector.h
    MatrixResizer.h
    MatrixWidget.h
    MatrixToolBox.h
    MatrixMover.h
    MatrixMouseEvent.h
    MatrixScene.h
    MatrixVelocity.h
    MatrixPainter.h
    PianoKeyboard.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${matrix_library_SRCS})

# actual target:
ADD_LIBRARY(matrix ${matrix_library}_SRCS ${matrix_library_HDRS})
SET_TARGET_PROPERTIES(matrix PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(matrix ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS matrix DESTINATION lib)
