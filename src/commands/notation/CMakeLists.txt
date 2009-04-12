# ########## Project setup ##########
PROJECT(notation)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## notation library ##########
# Sources:
SET(notation_library_SRCS
    EraseEventCommand.cpp
    IncrementDisplacementsCommand.cpp
    ChangeStemsCommand.cpp
    SetVisibilityCommand.cpp
    ClefInsertionCommand.cpp
    RespellCommand.cpp
    GuitarChordInsertionCommand.cpp
    RestoreTiesCommand.cpp
    ChangeTiePositionCommand.cpp
    TupletCommand.cpp
    CollapseRestsCommand.cpp
    RemoveFingeringMarksCommand.cpp
    BreakCommand.cpp
    UnGraceCommand.cpp
    TieNotesCommand.cpp
    MakeChordCommand.cpp
    RestoreStemsCommand.cpp
    DeCounterpointCommand.cpp
    BeamCommand.cpp
    MakeAccidentalsCautionaryCommand.cpp
    AddTextMarkCommand.cpp
    MakeNotesViableCommand.cpp
    AddSlashesCommand.cpp
    RestoreSlursCommand.cpp
    TextInsertionCommand.cpp
    GraceCommand.cpp
    NormalizeRestsCommand.cpp
    AddIndicationCommand.cpp
    RemoveMarksCommand.cpp
    UntieNotesCommand.cpp
    AutoBeamCommand.cpp
    ChangeStyleCommand.cpp
    RestInsertionCommand.cpp
    InterpretCommand.cpp
    UnTupletCommand.cpp
    KeyInsertionCommand.cpp
    RemoveNotationQuantizeCommand.cpp
    TextChangeCommand.cpp
    MultiKeyInsertionCommand.cpp
    NoteInsertionCommand.cpp
    AddMarkCommand.cpp
    MakeRegionViableCommand.cpp
    ChangeSlurPositionCommand.cpp
    SustainInsertionCommand.cpp
    FixNotationQuantizeCommand.cpp
    AddFingeringMarkCommand.cpp
    ResetDisplacementsCommand.cpp
)

# Headers:
SET(notation_library_HDRS
    TieNotesCommand.h
    DeCounterpointCommand.h
    AddMarkCommand.h
    BeamCommand.h
    AddTextMarkCommand.h
    RestoreSlursCommand.h
    SustainInsertionCommand.h
    FixNotationQuantizeCommand.h
    AddIndicationCommand.h
    ChangeStemsCommand.h
    EraseEventCommand.h
    SetVisibilityCommand.h
    ChangeStyleCommand.h
    RestInsertionCommand.h
    InterpretCommand.h
    TupletCommand.h
    UnTupletCommand.h
    CollapseRestsCommand.h
    TextChangeCommand.h
    RemoveNotationQuantizeCommand.h
    MakeChordCommand.h
    MultiKeyInsertionCommand.h
    RestoreStemsCommand.h
    NoteInsertionCommand.h
    MakeRegionViableCommand.h
    MakeAccidentalsCautionaryCommand.h
    MakeNotesViableCommand.h
    AddSlashesCommand.h
    TextInsertionCommand.h
    ChangeSlurPositionCommand.h
    NormalizeRestsCommand.h
    GraceCommand.h
    RemoveMarksCommand.h
    AddFingeringMarkCommand.h
    AutoBeamCommand.h
    UntieNotesCommand.h
    ResetDisplacementsCommand.h
    IncrementDisplacementsCommand.h
    ClefInsertionCommand.h
    GuitarChordInsertionCommand.h
    RespellCommand.h
    RestoreTiesCommand.h
    ChangeTiePositionCommand.h
    KeyInsertionCommand.h
    RemoveFingeringMarksCommand.h
    BreakCommand.h
    UnGraceCommand.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${notation_library_SRCS})

# actual target:
ADD_LIBRARY(notation ${notation_library}_SCRS ${notation_library_HDRS})
SET_TARGET_PROPERTIES(notation PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(notation ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS notation DESTINATION lib)
