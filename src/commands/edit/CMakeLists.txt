# ########## Project setup ##########
PROJECT(edit)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## edit library ##########
# Sources:
SET(edit_library_SRCS
    InvertCommand.cpp
    EraseCommand.cpp
    PasteEventsCommand.cpp
    EventUnquantizeCommand.cpp
    MoveCommand.cpp
    EventInsertionCommand.cpp
    CopyCommand.cpp
    MoveAcrossSegmentsCommand.cpp
    InsertTriggerNoteCommand.cpp
    RescaleCommand.cpp
    SetTriggerCommand.cpp
    SetLyricsCommand.cpp
    PasteSegmentsCommand.cpp
    RetrogradeInvertCommand.cpp
    ModifyMarkerCommand.cpp
    RemoveMarkerCommand.cpp
    AddDotCommand.cpp
    EventQuantizeCommand.cpp
    ChangeVelocityCommand.cpp
    CutAndCloseCommand.cpp
    CollapseNotesCommand.cpp
    TransposeCommand.cpp
    SetNoteTypeCommand.cpp
    EventEditCommand.cpp
    SelectionPropertyCommand.cpp
    AddMarkerCommand.cpp
    CutCommand.cpp
    ClearTriggersCommand.cpp
    RetrogradeCommand.cpp
)

# Headers:
SET(edit_library_HDRS
    AddDotCommand.h
    RemoveMarkerCommand.h
    EventQuantizeCommand.h
    ChangeVelocityCommand.h
    CutAndCloseCommand.h
    CollapseNotesCommand.h
    TransposeCommand.h
    SetNoteTypeCommand.h
    MoveAcrossSegmentsCommand.h
    EventEditCommand.h
    SelectionPropertyCommand.h
    AddMarkerCommand.h
    CutCommand.h
    ClearTriggersCommand.h
    RetrogradeCommand.h
    EraseCommand.h
    PasteEventsCommand.h
    EventUnquantizeCommand.h
    MoveCommand.h
    InvertCommand.h
    CopyCommand.h
    EventInsertionCommand.h
    InsertTriggerNoteCommand.h
    RescaleCommand.h
    SetTriggerCommand.h
    SetLyricsCommand.h
    PasteSegmentsCommand.h
    RetrogradeInvertCommand.h
    ModifyMarkerCommand.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${edit_library_SRCS})

# actual target:
ADD_LIBRARY(edit ${edit_library}_SCRS ${edit_library_HDRS})
SET_TARGET_PROPERTIES(edit PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(edit ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS edit DESTINATION lib)
