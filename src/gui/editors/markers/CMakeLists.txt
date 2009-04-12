# ########## Project setup ##########
PROJECT(markers)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ######### General setup ##########
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

# ########## markers library ##########
# Sources:
SET(markers_library_SRCS
)

# Headers:
SET(markers_library_HDRS
)



# actual target:
ADD_LIBRARY(markers ${markers_library}_SRCS ${markers_library_HDRS})
SET_TARGET_PROPERTIES(markers PROPERTIES VERSION 0.0.0)

# add install target:
INSTALL(TARGETS markers DESTINATION lib)
