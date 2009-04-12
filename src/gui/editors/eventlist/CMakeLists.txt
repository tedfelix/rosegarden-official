# ########## Project setup ##########
PROJECT(eventlist)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## eventlist library ##########
# Sources:
SET(eventlist_library_SRCS
    TrivialVelocityDialog.cpp
    EventViewItem.cpp
    EventView.cpp
)

# Headers:
SET(eventlist_library_HDRS
    TrivialVelocityDialog.h
    EventViewItem.h
    EventView.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${eventlist_library_SRCS})

# actual target:
ADD_LIBRARY(eventlist ${eventlist_library_SRCS} ${eventlist_library_HDRS})
SET_TARGET_PROPERTIES(eventlist PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(eventlist ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS eventlist DESTINATION lib)
