# ########## Project setup ##########
PROJECT(studio)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## studio library ##########
# Sources:
SET(studio_library_SRCS
    MidiMixerVUMeter.cpp
    BankEditorDialog.cpp
    NameSetEditor.cpp
    ChangeRecordDeviceCommand.cpp
    AudioPluginManager.cpp
    RemapInstrumentDialog.cpp
    AudioPlugin.cpp
    MidiKeyMapTreeWidgetItem.cpp
    AudioMixerWindow.cpp
    SynthPluginManagerDialog.cpp
    AudioPluginClipboard.cpp
    MidiDeviceTreeWidgetItem.cpp
    MidiProgramsEditor.cpp
    MidiKeyMappingEditor.cpp
    StudioControl.cpp
    MixerWindow.cpp
    DeviceManagerDialog.cpp
    MidiMixerWindow.cpp
    MidiBankTreeWidgetItem.cpp
    AudioPluginOSCGUIManager.cpp
    TimerCallbackAssistant.cpp
    OSCMessage.cpp
    AudioPluginOSCGUI.cpp
)

# Headers:
SET(studio_library_HDRS
    AudioPluginClipboard.h
    MidiKeyMapTreeWidgetItem.h
    AudioMixerWindow.h
    SynthPluginManagerDialog.h
    ChangeRecordDeviceCommand.h
    MidiProgramsEditor.h
    MidiDeviceTreeWidgetItem.h
    StudioControl.h
    MixerWindow.h
    MidiKeyMappingEditor.h
    MidiMixerWindow.h
    MidiBankTreeWidgetItem.h
    AudioPluginOSCGUIManager.h
    TimerCallbackAssistant.h
    OSCMessage.h
    AudioPluginOSCGUI.h
    MidiMixerVUMeter.h
    BankEditorDialog.h
    NameSetEditor.h
    RemapInstrumentDialog.h
    AudioPluginManager.h
    AudioPlugin.h
    DeviceManagerDialog.h
    DeviceManagerDialogUi.h
    NoPortDialogUi.h
)

# UI files:
SET(studio_library_UIS
    DeviceManagerDialogUi.ui
    NoPortDialogUi.ui
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${studio_library_SRCS})

# actual target:
ADD_LIBRARY(studio ${studio_library_SRCS} ${studio_library_HDRS})
#ADD_EXECUTABLE(studio ${studio_library_SRCS})
#TARGET_LINK_LIBRARIES(studio ${QT_LIBRARIES})

# add install target:
#INSTALL(TARGETS studio DESTINATION bin)
