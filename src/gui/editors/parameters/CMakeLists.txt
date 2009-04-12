# ########## Project setup ##########
PROJECT(parameters)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## parameters library ##########
# Sources:
SET(parameters_library_SRCS
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
SET(parameters_library_HDRS
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
QT4_AUTOMOC(${parameters_library_SRCS})

# actual target:
ADD_LIBRARY(parameters ${parameters_library_SRCS} ${parameters_library_HDRS})
#ADD_EXECUTABLE(parameters ${parameters_library_SRCS})
#TARGET_LINK_LIBRARIES(parameters ${QT_LIBRARIES})

# add install target:
#INSTALL(TARGETS parameters DESTINATION bin)
