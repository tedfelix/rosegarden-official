# ########## Project setup ##########
PROJECT(commands)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ######### General setup ##########
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

# ########## commands library ##########
# Sources:
SET(commands_library_SRCS
)

# Headers:
SET(commands_library_HDRS
)



# actual target:
ADD_LIBRARY(commands ${commands_library}_SCRS ${commands_library_HDRS})
SET_TARGET_PROPERTIES(commands PROPERTIES VERSION 0.0.0)

# add install target:
INSTALL(TARGETS commands DESTINATION lib)
