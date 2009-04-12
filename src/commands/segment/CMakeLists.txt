# ########## Project setup ##########
PROJECT(segment_commands)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## segment_commands library ##########
# Sources:
SET(segment_commands_library_SRCS
    SegmentTransposeCommand.cpp
    SegmentSyncCommand.cpp
    AddTracksCommand.cpp
    SegmentRepeatToCopyCommand.cpp
    SegmentRecordCommand.cpp
    SegmentSplitTwiceCommand.cpp
    AudioSegmentResizeFromStartCommand.cpp
    DeleteTracksCommand.cpp
    SegmentEraseCommand.cpp
    SegmentJoinCommand.cpp
    AudioSegmentRescaleCommand.cpp
    PasteToTriggerSegmentCommand.cpp
    RenameTrackCommand.cpp
    SetTriggerSegmentDefaultRetuneCommand.cpp
    AddTimeSignatureAndNormalizeCommand.cpp
    SegmentChangePlayableRangeCommand.cpp
    AudioSegmentSplitCommand.cpp
    SegmentLabelCommand.cpp
    AddTempoChangeCommand.cpp
    OpenOrCloseRangeCommand.cpp
    SegmentAutoSplitCommand.cpp
    SegmentChangeTransposeCommand.cpp
    RemoveTempoChangeCommand.cpp
    AudioSegmentInsertCommand.cpp
    ChangeCompositionLengthCommand.cpp
    CreateTempoMapFromSegmentCommand.cpp
    AudioSegmentDistributeCommand.cpp
    SegmentColourMapCommand.cpp
    SegmentInsertCommand.cpp
    AddTimeSignatureCommand.cpp
    SegmentQuickCopyCommand.cpp
    SegmentSplitByPitchCommand.cpp
    ModifyDefaultTempoCommand.cpp
    SegmentSingleRepeatToCopyCommand.cpp
    CutRangeCommand.cpp
    SetTriggerSegmentBaseVelocityCommand.cpp
    SegmentSplitByRecordingSrcCommand.cpp
    SegmentChangeQuantizationCommand.cpp
    DeleteRangeCommand.cpp
    EraseSegmentsStartingInRangeCommand.cpp
    SegmentColourCommand.cpp
    SegmentResizeFromStartCommand.cpp
    PasteRangeCommand.cpp
    AudioSegmentAutoSplitCommand.cpp
    DeleteTriggerSegmentCommand.cpp
    AddTriggerSegmentCommand.cpp
    SegmentRescaleCommand.cpp
    SegmentCommand.cpp
    SegmentCommandRepeat.cpp
    SetTriggerSegmentDefaultTimeAdjustCommand.cpp
    SetTriggerSegmentBasePitchCommand.cpp
    MoveTracksCommand.cpp
    SegmentSplitCommand.cpp
    InsertRangeCommand.cpp
    PasteConductorDataCommand.cpp
    SegmentSyncClefCommand.cpp
    SegmentReconfigureCommand.cpp
    RemoveTimeSignatureCommand.cpp
)

# Headers:
SET(segment_commands_library_HDRS
    SetTriggerSegmentBasePitchCommand.h
    RenameTrackCommand.h
    AddTriggerSegmentCommand.h
    AudioSegmentSplitCommand.h
    SegmentRescaleCommand.h
    SegmentCommand.h
    SetTriggerSegmentDefaultTimeAdjustCommand.h
    SegmentLabelCommand.h
    SegmentCommandRepeat.h
    RemoveTempoChangeCommand.h
    DeleteTriggerSegmentCommand.h
    InsertRangeCommand.h
    SegmentColourMapCommand.h
    PasteConductorDataCommand.h
    SegmentSyncClefCommand.h
    SegmentQuickCopyCommand.h
    SegmentReconfigureCommand.h
    SegmentSyncCommand.h
    SegmentTransposeCommand.h
    SegmentRepeatToCopyCommand.h
    SegmentRecordCommand.h
    CutRangeCommand.h
    SetTriggerSegmentBaseVelocityCommand.h
    SegmentSplitByRecordingSrcCommand.h
    AudioSegmentRescaleCommand.h
    SegmentResizeFromStartCommand.h
    SegmentColourCommand.h
    SetTriggerSegmentDefaultRetuneCommand.h
    AddTimeSignatureAndNormalizeCommand.h
    SegmentChangePlayableRangeCommand.h
    AddTempoChangeCommand.h
    OpenOrCloseRangeCommand.h
    SegmentChangeTransposeCommand.h
    SegmentAutoSplitCommand.h
    AudioSegmentInsertCommand.h
    ChangeCompositionLengthCommand.h
    CreateTempoMapFromSegmentCommand.h
    MoveTracksCommand.h
    SegmentSplitCommand.h
    AudioSegmentDistributeCommand.h
    AddTimeSignatureCommand.h
    SegmentInsertCommand.h
    RemoveTimeSignatureCommand.h
    AddTracksCommand.h
    ModifyDefaultTempoCommand.h
    SegmentSplitByPitchCommand.h
    SegmentSingleRepeatToCopyCommand.h
    AudioSegmentResizeFromStartCommand.h
    DeleteTracksCommand.h
    SegmentSplitTwiceCommand.h
    SegmentChangeQuantizationCommand.h
    DeleteRangeCommand.h
    SegmentEraseCommand.h
    SegmentJoinCommand.h
    PasteToTriggerSegmentCommand.h
    EraseSegmentsStartingInRangeCommand.h
    PasteRangeCommand.h
    AudioSegmentAutoSplitCommand.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${segment_commands_library_SRCS})

# actual target:
ADD_LIBRARY(segment_commands ${segment_commands_library}_SRCS ${segment_commands_library_HDRS})
#SET_TARGET_PROPERTIES(segment_commands PROPERTIES VERSION 0.0.0)
#TARGET_LINK_LIBRARIES(segment_commands ${QT_LIBRARIES})

# add install target:
#INSTALL(TARGETS segment_commands DESTINATION lib)
