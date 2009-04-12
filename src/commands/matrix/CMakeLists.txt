# ########## Project setup ##########
PROJECT(matrix)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ######### General setup ##########
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

# ########## matrix library ##########
# Sources:
SET(matrix_library_SRCS
    MatrixInsertionCommand.cpp
    MatrixEraseCommand.cpp
    MatrixModifyCommand.cpp
    MatrixPercussionInsertionCommand.cpp
)

# Headers:
SET(matrix_library_HDRS
    MatrixModifyCommand.h
    MatrixPercussionInsertionCommand.h
    MatrixInsertionCommand.h
    MatrixEraseCommand.h
)



# actual target:
ADD_LIBRARY(matrix ${matrix_library}_SCRS ${matrix_library_HDRS})
SET_TARGET_PROPERTIES(matrix PROPERTIES VERSION 0.0.0)

# add install target:
INSTALL(TARGETS matrix DESTINATION lib)
