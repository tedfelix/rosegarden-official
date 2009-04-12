# ########## Project setup ##########
PROJECT(parameters)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## parameters executable ##########
# Sources:
SET(parameters_executable_SRCS
    RosegardenParameterBox.cpp
    SegmentParameterBox.cpp
    AudioInstrumentParameterPanel.cpp
    TrackParameterBox.cpp
    InstrumentParameterBox.cpp
    MIDIInstrumentParameterPanel.cpp
    RosegardenParameterArea.cpp
    InstrumentParameterPanel.cpp
)

# Headers:
SET(parameters_executable_HDRS
    MIDIInstrumentParameterPanel.h
    InstrumentParameterPanel.h
    AudioInstrumentParameterPanel.h
    RosegardenParameterBox.h
    SegmentParameterBox.h
    TrackParameterBox.h
    InstrumentParameterBox.h
    RosegardenParameterArea.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${parameters_executable_SRCS})

# actual target:
ADD_EXECUTABLE(parameters ${parameters_executable_SRCS})
TARGET_LINK_LIBRARIES(parameters ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS parameters DESTINATION bin)
