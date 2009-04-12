# ########## Project setup ##########
PROJECT(segment)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## segment executable ##########
# Sources:
SET(segment_executable_SRCS
    TrackLabel.cpp
    TrackEditor.cpp
    TrackHeader.cpp
    ControlParameterEditDialog.cpp
    TriggerSegmentManager.cpp
    MarkerEditor.cpp
    PlayListView.cpp
    ControlParameterItem.cpp
    PlayList.cpp
    MarkerEditorViewItem.cpp
    ControlEditorDialog.cpp
    PlayListViewItem.cpp
    TrackVUMeter.cpp
    TriggerManagerItem.cpp
    PlayListDialog.cpp
    TrackButtons.cpp
)

# Headers:
SET(segment_executable_HDRS
    PlayListViewItem.h
    PlayListView.h
    PlayList.h
    ControlParameterItem.h
    MarkerEditorViewItem.h
    ControlParameterEditDialog.h
    ControlEditorDialog.h
    TrackVUMeter.h
    TriggerManagerItem.h
    PlayListDialog.h
    TrackButtons.h
    TrackLabel.h
    TrackEditor.h
    TrackHeader.h
    TriggerSegmentManager.h
    MarkerEditor.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${segment_executable_SRCS})

# actual target:
ADD_EXECUTABLE(segment ${segment_executable_SRCS})
TARGET_LINK_LIBRARIES(segment ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS segment DESTINATION bin)
