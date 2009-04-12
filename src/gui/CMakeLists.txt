# ########## Project setup ##########
PROJECT(gui)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ######### General setup ##########
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

# ########## gui library ##########
# Sources:
SET(gui_library_SRCS
)

# Headers:
SET(gui_library_HDRS
)



# actual target:
ADD_LIBRARY(gui ${gui_library}_SRCS ${gui_library_HDRS})
SET_TARGET_PROPERTIES(gui PROPERTIES VERSION 0.0.0)

# add install target:
INSTALL(TARGETS gui DESTINATION lib)
