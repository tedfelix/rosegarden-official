# ########## Project setup ##########
PROJECT(sequencer)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## sequencer library ##########
# Sources:
SET(sequencer_library_SRCS
    SequencerThread.cpp
    RosegardenSequencer.cpp
    MmappedSegment.cpp
)

# Headers:
SET(sequencer_library_HDRS
    RosegardenSequencerIface.h
    SequencerThread.h
    RosegardenSequencer.h
    MmappedSegment.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${sequencer_library_SRCS})

# actual target:
ADD_LIBRARY(sequencer ${sequencer_library}_SRCS ${sequencer_library_HDRS})
SET_TARGET_PROPERTIES(sequencer PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(sequencer ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS sequencer DESTINATION lib)
