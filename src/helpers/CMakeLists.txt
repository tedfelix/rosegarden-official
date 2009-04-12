# ########## Project setup ##########
PROJECT(helpers)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ######### General setup ##########
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

# ########## helpers library ##########
# Sources:
SET(helpers_library_SRCS
)

# Headers:
SET(helpers_library_HDRS
)



# actual target:
ADD_LIBRARY(helpers ${helpers_library_SRCS} ${helpers_library_HDRS})
SET_TARGET_PROPERTIES(helpers PROPERTIES VERSION 0.0.0)

# add install target:
INSTALL(TARGETS helpers DESTINATION lib)
