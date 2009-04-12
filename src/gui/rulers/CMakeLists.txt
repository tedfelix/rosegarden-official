# ########## Project setup ##########
PROJECT(rulers)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## rulers library ##########
# Sources:
SET(rulers_library_SRCS
    TempoRuler.cpp
    TextRuler.cpp
    StandardRuler.cpp
    TempoColour.cpp
    VelocityColour.cpp
    ControlRulerEventInsertCommand.cpp
    DefaultVelocityColour.cpp
    ControlChangeCommand.cpp
    RawNoteRuler.cpp
    ControllerEventsRuler.cpp
    PropertyViewRuler.cpp
    MarkerRuler.cpp
    PropertyControlRuler.cpp
    ControlItem.cpp
    ChordNameRuler.cpp
    ControlRulerEventEraseCommand.cpp
    PitchRuler.cpp
    ControlSelector.cpp
    LoopRuler.cpp
    ViewElementAdapter.cpp
    ControllerEventAdapter.cpp
    ControlRuler.cpp
    PropertyBox.cpp
    PercussionPitchRuler.cpp
)

# Headers:
SET(rulers_library_HDRS
    PropertyControlRuler.h
    PropertyViewRuler.h
    RawNoteRuler.h
    MarkerRuler.h
    ControlItem.h
    ChordNameRuler.h
    ControlRulerEventEraseCommand.h
    PitchRuler.h
    ControlSelector.h
    ControllerEventsRuler.h
    LoopRuler.h
    ViewElementAdapter.h
    ControllerEventAdapter.h
    ControlRuler.h
    ElementAdapter.h
    PercussionPitchRuler.h
    PropertyBox.h
    TextRuler.h
    TempoRuler.h
    TempoColour.h
    StandardRuler.h
    ControlTool.h
    VelocityColour.h
    ControlRulerEventInsertCommand.h
    ControlChangeCommand.h
    DefaultVelocityColour.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${rulers_library_SRCS})

# actual target:
ADD_LIBRARY(rulers ${rulers_library_SRCS} ${rulers_library_HDRS})
SET_TARGET_PROPERTIES(rulers PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(rulers ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS rulers DESTINATION lib)
