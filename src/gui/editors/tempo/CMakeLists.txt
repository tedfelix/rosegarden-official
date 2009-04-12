# ########## Project setup ##########
PROJECT(tempo)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## tempo library ##########
# Sources:
SET(tempo_library_SRCS
    TempoListItem.cpp
    TempoView.cpp
)

# Headers:
SET(tempo_library_HDRS
    TempoListItem.h
    TempoView.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${tempo_library_SRCS})

# actual target:
ADD_LIBRARY(tempo ${tempo_library}_SRCS ${tempo_library_HDRS})
SET_TARGET_PROPERTIES(tempo PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(tempo ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS tempo DESTINATION lib)
