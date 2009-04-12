# ########## Project setup ##########
PROJECT(base)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## base executable ##########
# Sources:
SET(base_executable_SRCS
    RulerScale.cpp
    NotationQuantizer.cpp
    Segment.cpp
    RealTime.cpp
    MidiDevice.cpp
    SnapGrid.cpp
    CompositionTimeSliceAdapter.cpp
    AnalysisTypes.cpp
    MidiTypes.cpp
    Marker.cpp
    ControlParameter.cpp
    Instrument.cpp
    MidiProgram.cpp
    AudioDevice.cpp
    TriggerSegment.cpp
    Quantizer.cpp
    Profiler.cpp
    Property.cpp
    Equation.cpp
    XmlExportable.cpp
    AudioLevel.cpp
    Exception.cpp
    Device.cpp
    ColourMap.cpp
    Colour.cpp
    SegmentNotationHelper.cpp
    Event.cpp
    ViewElement.cpp
    Selection.cpp
    PropertyName.cpp
    LegatoQuantizer.cpp
    ScriptAPI.cpp
    Track.cpp
    Studio.cpp
    Sets.cpp
    AudioPluginInstance.cpp
    SoftSynthDevice.cpp
    BaseProperties.cpp
    Configuration.cpp
    ViewSegment.cpp
    Clipboard.cpp
    Composition.cpp
    PropertyMap.cpp
    SegmentMatrixHelper.cpp
    NotationTypes.cpp
    BasicQuantizer.cpp
    SegmentPerformanceHelper.cpp
)

# Headers:
SET(base_executable_HDRS
    AudioLevel.h
    ColourMap.h
    Device.h
    StaffExportTypes.h
    RefreshStatus.h
    Colour.h
    NotationRules.h
    MidiMetronome.h
    Sets.h
    NotationTypes.h
    Event.h
    AudioPluginInstance.h
    PropertyMap.h
    Studio.h
    SegmentPerformanceHelper.h
    BasicQuantizer.h
    Selection.h
    PropertyName.h
    NotationQuantizer.h
    Segment.h
    RealTime.h
    MidiDevice.h
    LayoutEngine.h
    SoftSynthDevice.h
    AnalysisTypes.h
    Instrument.h
    MidiProgram.h
    Track.h
    FastVector.h
    XmlExportable.h
    Quantizer.h
    Profiler.h
    Property.h
    TriggerSegment.h
    BaseProperties.h
    ViewSegment.h
    Exception.h
    Clipboard.h
    Composition.h
    Configuration.h
    SegmentNotationHelper.h
    SegmentMatrixHelper.h
    ViewElement.h
    ScriptAPI.h
    RulerScale.h
    MidiTypes.h
    CompositionTimeSliceAdapter.h
    SnapGrid.h
    LegatoQuantizer.h
    Controllable.h
    Marker.h
    AudioDevice.h
    ControlParameter.h
    Equation.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${base_executable_SRCS})

# actual target:
ADD_EXECUTABLE(base ${base_executable_SRCS})
TARGET_LINK_LIBRARIES(base ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS base DESTINATION bin)
