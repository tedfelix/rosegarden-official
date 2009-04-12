# ########## Project setup ##########
PROJECT(dialogs)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## dialogs executable ##########
# Sources:
SET(dialogs_executable_SRCS
    DocumentConfigureDialog.cpp
    SplitByPitchDialog.cpp
    RescaleDialog.cpp
    MakeOrnamentDialog.cpp
    CountdownBar.cpp
    TextEventDialog.cpp
    EventFilterDialog.cpp
    UseOrnamentDialog.cpp
    ManageMetronomeDialog.cpp
    PitchPickerDialog.cpp
    BeatsBarsDialog.cpp
    TupletDialog.cpp
    ShowSequencerStatusDialog.cpp
    IdentifyTextCodecDialog.cpp
    TimeSignatureDialog.cpp
    ConfigureDialog.cpp
    MarkerModifyDialog.cpp
    TimeDialog.cpp
    FileMergeDialog.cpp
    AddTracksDialog.cpp
    SimpleEventEditDialog.cpp
    FileLocateDialog.cpp
    PitchDialog.cpp
    ClefDialog.cpp
    CountdownDialog.cpp
    SplitByRecordingSrcDialog.cpp
    IntervalDialog.cpp
    EventEditDialog.cpp
    LyricEditDialog.cpp
    PasteNotationDialog.cpp
    ImportDeviceDialog.cpp
    FloatEdit.cpp
    TempoDialog.cpp
    UnusedAudioSelectionDialog.cpp
    AudioManagerDialog.cpp
    TransportDialog.cpp
    QuantizeDialog.cpp
    ExportDeviceDialog.cpp
    CompositionLengthDialog.cpp
    LilyPondOptionsDialog.cpp
    AudioSplitDialog.cpp
    AudioPluginDialog.cpp
    EventParameterDialog.cpp
    AudioPlayingDialog.cpp
    TriggerSegmentDialog.cpp
    KeySignatureDialog.cpp
    InterpretDialog.cpp
    ConfigureDialogBase.cpp
)

# Headers:
SET(dialogs_executable_HDRS
    ShowSequencerStatusDialog.h
    TupletDialog.h
    AudioPluginDialog.h
    ConfigureDialog.h
    EventParameterDialog.h
    TempoDialog.h
    TriggerSegmentDialog.h
    MarkerModifyDialog.h
    FileMergeDialog.h
    SimpleEventEditDialog.h
    PitchDialog.h
    FileLocateDialog.h
    ClefDialog.h
    KeySignatureDialog.h
    InterpretDialog.h
    ConfigureDialogBase.h
    SplitByPitchDialog.h
    RescaleDialog.h
    DocumentConfigureDialog.h
    TextEventDialog.h
    EventEditDialog.h
    ManageMetronomeDialog.h
    UnusedAudioSelectionDialog.h
    SplitByRecordingSrcDialog.h
    FloatEdit.h
    UseOrnamentDialog.h
    ExportDeviceDialog.h
    BeatsBarsDialog.h
    CompositionLengthDialog.h
    AudioSplitDialog.h
    IdentifyTextCodecDialog.h
    TimeSignatureDialog.h
    AudioPlayingDialog.h
    TimeDialog.h
    AddTracksDialog.h
    CountdownDialog.h
    MakeOrnamentDialog.h
    IntervalDialog.h
    CountdownBar.h
    EventFilterDialog.h
    PasteNotationDialog.h
    LyricEditDialog.h
    ImportDeviceDialog.h
    AudioManagerDialog.h
    TransportDialog.h
    QuantizeDialog.h
    PitchPickerDialog.h
    LilyPondOptionsDialog.h
    RosegardenTransportUi.h
)

# UI files:
SET(dialogs_executable_UIS
    RosegardenTransportUi.ui
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${dialogs_executable_SRCS})

# actual target:
ADD_EXECUTABLE(dialogs ${dialogs_executable_SRCS})
TARGET_LINK_LIBRARIES(dialogs ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS dialogs DESTINATION bin)
