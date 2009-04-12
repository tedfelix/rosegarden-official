# ########## Project setup ##########
PROJECT(general)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## general library ##########
# Sources:
SET(general_library_SRCS
    FileSource.cpp
    DecoyAction.cpp
    ClefIndex.cpp
    RecentFiles.cpp
    CanvasItemGC.cpp
    BaseTool.cpp
    ResourceFinder.cpp
    CanvasCursor.cpp
    EditViewBase.cpp
    PresetHandlerDialog.cpp
    AutoSaveFinder.cpp
    IconLoader.cpp
    CategoryElement.cpp
    RosegardenScrollView.cpp
    TempDirectory.cpp
    ActionFileParser.cpp
    GUIPalette.cpp
    ActionCommandRegistry.cpp
    MidiPitchLabel.cpp
    ActionFileClient.cpp
    ListEditView.cpp
    PresetElement.cpp
    PresetGroup.cpp
    HZoomable.cpp
    EditView.cpp
    RosegardenCanvasView.cpp
    BaseToolBox.cpp
    PixmapFunctions.cpp
    ProgressReporter.cpp
    Spline.cpp
)

# Headers:
SET(general_library_HDRS
    RosegardenScrollView.h
    ActionFileParser.h
    TempDirectory.h
    GUIPalette.h
    ActionCommandRegistry.h
    MidiPitchLabel.h
    ActionFileClient.h
    PresetElement.h
    ListEditView.h
    PresetGroup.h
    HZoomable.h
    Spline.h
    RosegardenCanvasView.h
    EditView.h
    BaseToolBox.h
    ProgressReporter.h
    PixmapFunctions.h
    DecoyAction.h
    FileSource.h
    ClefIndex.h
    EditViewTimeSigNotifier.h
    RecentFiles.h
    CanvasItemGC.h
    BaseTool.h
    CanvasCursor.h
    EditViewBase.h
    AutoSaveFinder.h
    IconLoader.h
    ResourceFinder.h
    PresetHandlerDialog.h
    SelectionManager.h
    CategoryElement.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${general_library_SRCS})

# actual target:
ADD_LIBRARY(general ${general_library_SRCS} ${general_library_HDRS})
SET_TARGET_PROPERTIES(general PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(general ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS general DESTINATION lib)
