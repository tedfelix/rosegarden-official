# ########## Project setup ##########
PROJECT(matrix_commands)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ######### General setup ##########
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

# ########## matrix_commands library ##########
# Sources:
SET(matrix_commands_library_SRCS
    MatrixInsertionCommand.cpp
    MatrixEraseCommand.cpp
    MatrixModifyCommand.cpp
    MatrixPercussionInsertionCommand.cpp
)

# Headers:
SET(matrix_commands_library_HDRS
    MatrixModifyCommand.h
    MatrixPercussionInsertionCommand.h
    MatrixInsertionCommand.h
    MatrixEraseCommand.h
)



# actual target:
ADD_LIBRARY(matrix_commands ${matrix_commands_library}_SRCS ${matrix_commands_library_HDRS})
SET_TARGET_PROPERTIES(matrix_commands PROPERTIES VERSION 0.0.0)

# add install target:
INSTALL(TARGETS matrix_commands DESTINATION lib)
