# ########## Project setup ##########
PROJECT(editors)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ######### General setup ##########
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

# ########## editors library ##########
# Sources:
SET(editors_library_SRCS
)

# Headers:
SET(editors_library_HDRS
)



# actual target:
ADD_LIBRARY(editors ${editors_library}_SRCS ${editors_library_HDRS})
SET_TARGET_PROPERTIES(editors PROPERTIES VERSION 0.0.0)

# add install target:
INSTALL(TARGETS editors DESTINATION lib)
