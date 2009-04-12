# ########## Project setup ##########
PROJECT(misc)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## misc library ##########
# Sources:
SET(misc_library_SRCS
    AppendLabel.cpp
    Debug.cpp
    Strings.cpp
)

# Headers:
SET(misc_library_HDRS
    Strings.h
    stableheaders.h
    Debug.h
    AppendLabel.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${misc_library_SRCS})

# actual target:
ADD_LIBRARY(misc ${misc_library_SRCS} ${misc_library_HDRS})
SET_TARGET_PROPERTIES(misc PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(misc ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS misc DESTINATION lib)
