# ########## Project setup ##########
PROJECT(application)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## application executable ##########
# Sources:
SET(application_executable_SRCS
    main.cpp
    RosegardenApplication.cpp
    SetWaitCursor.cpp
    LircCommander.cpp
    RosegardenMainViewWidget.cpp
    RosegardenMainWindow.cpp
    LircClient.cpp
    StartupTester.cpp
)

# Headers:
SET(application_executable_HDRS
    TransportStatus.h
    StartupTester.h
    LircClient.h
    RosegardenMainViewWidget.h
    RosegardenApplication.h
    SetWaitCursor.h
    LircCommander.h
    RosegardenMainWindow.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${application_executable_SRCS})

# actual target:
ADD_EXECUTABLE(application ${application_executable_SRCS})
TARGET_LINK_LIBRARIES(application ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS application DESTINATION bin)
