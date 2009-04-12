# ########## Project setup ##########
PROJECT(widgets)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## widgets library ##########
# Sources:
SET(widgets_library_SRCS
    LedButton.cpp
    AudioRouteMenu.cpp
    SpinBox.cpp
    DiatonicPitchChooser.cpp
    BaseTextFloat.cpp
    TmpStatusMsg.cpp
    AudioFaderBox.cpp
    QDeferScrollView.cpp
    CurrentProgressDialog.cpp
    ProgressBar.cpp
    DeferScrollArea.cpp
    PluginControl.cpp
    Panned.cpp
    Led.cpp
    Rotary.cpp
    CollapsingFrame.cpp
    StartupLogo.cpp
    Panner.cpp
    VUMeter.cpp
    CanvasGroupableItem.cpp
    MidiFaderWidget.cpp
    PitchChooser.cpp
    PluginPushButton.cpp
    TimeWidget.cpp
    TextFloat.cpp
    Label.cpp
    Fader.cpp
    ProgressDialog.cpp
    IconButton.cpp
    SqueezedLabel.cpp
    AudioListView.cpp
    PitchDragLabel.cpp
    FontRequester.cpp
    TristateCheckBox.cpp
    ScrollBox.cpp
    AudioVUMeter.cpp
    ScrollBoxDialog.cpp
    ColourTable.cpp
    InputDialog.cpp
    QuantizeParameters.cpp
    ZoomSlider.cpp
    WheelyButton.cpp
    ColourTableItem.cpp
    IconStackedWidget.cpp
    LineEdit.cpp
)

# Headers:
SET(widgets_library_HDRS
    DeferScrollArea.h
    ProgressBar.h
    PluginControl.h
    Panned.h
    RosegardenPopupMenu.h
    CanvasGroupableItem.h
    TristateCheckBox.h
    FontRequester.h
    BigArrowButton.h
    Panner.h
    StartupLogo.h
    QuantizeParameters.h
    VUMeter.h
    ColourTableItem.h
    AudioFaderBox.h
    LedButton.h
    QDeferScrollView.h
    Label.h
    Fader.h
    SqueezedLabel.h
    SpinBox.h
    CollapsingFrame.h
    Led.h
    PitchDragLabel.h
    AudioVUMeter.h
    ScrollBox.h
    ColourTable.h
    AudioListItem.h
    InputDialog.h
    DiatonicPitchChooser.h
    Rotary.h
    ZoomSlider.h
    ScrollBoxDialog.h
    MidiFaderWidget.h
    WheelyButton.h
    PluginPushButton.h
    PitchChooser.h
    IconStackedWidget.h
    LineEdit.h
    TimeWidget.h
    BaseTextFloat.h
    AudioRouteMenu.h
    TextFloat.h
    TmpStatusMsg.h
    ProgressDialog.h
    IconButton.h
    CurrentProgressDialog.h
    AudioListView.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${widgets_library_SRCS})

# actual target:
ADD_LIBRARY(widgets ${widgets_library}_SCRS ${widgets_library_HDRS})
SET_TARGET_PROPERTIES(widgets PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(widgets ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS widgets DESTINATION lib)
