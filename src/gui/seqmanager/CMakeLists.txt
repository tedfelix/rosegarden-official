# ########## Project setup ##########
PROJECT(seqmanager)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## seqmanager library ##########
# Sources:
SET(seqmanager_library_SRCS
    TimeSigSegmentMmapper.cpp
    SequenceManager.cpp
    SegmentMmapperFactory.cpp
    AudioSegmentMmapper.cpp
    MidiFilterDialog.cpp
    CompositionMmapper.cpp
    SegmentMmapper.cpp
    SpecialSegmentMmapper.cpp
    MetronomeMmapper.cpp
    TempoSegmentMmapper.cpp
)

# Headers:
SET(seqmanager_library_HDRS
    TempoSegmentMmapper.h
    SequenceManager.h
    TimeSigSegmentMmapper.h
    AudioSegmentMmapper.h
    SegmentMmapperFactory.h
    MidiFilterDialog.h
    SegmentMmapper.h
    SpecialSegmentMmapper.h
    CompositionMmapper.h
    MetronomeMmapper.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${seqmanager_library_SRCS})

# actual target:
ADD_LIBRARY(seqmanager ${seqmanager_library}_SCRS ${seqmanager_library_HDRS})
SET_TARGET_PROPERTIES(seqmanager PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(seqmanager ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS seqmanager DESTINATION lib)
