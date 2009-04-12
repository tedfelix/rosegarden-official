# ########## Project setup ##########
PROJECT(compositionview)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## compositionview library ##########
# Sources:
SET(compositionview_library_SRCS
    CompositionModelImpl.cpp
    SegmentTool.cpp
    SegmentEraser.cpp
    CompositionColourCache.cpp
    CompositionRect.cpp
    SegmentItemPreview.cpp
    CompositionModel.cpp
    SegmentSplitter.cpp
    AudioPreviewThread.cpp
    CompositionView.cpp
    PreviewRect.cpp
    SegmentSelector.cpp
    CompositionItemImpl.cpp
    SegmentResizer.cpp
    SegmentJoiner.cpp
    SegmentToolBox.cpp
    SegmentMover.cpp
    CompositionItem.cpp
    CompositionItemHelper.cpp
    SegmentOrderer.cpp
    AudioPreviewPainter.cpp
    AudioPreviewUpdater.cpp
    SegmentPencil.cpp
)

# Headers:
SET(compositionview_library_HDRS
    SegmentSplitter.h
    AudioPreviewThread.h
    CompositionView.h
    PreviewRect.h
    CompositionItemImpl.h
    SegmentSelector.h
    SegmentResizer.h
    SegmentJoiner.h
    SegmentToolBox.h
    SegmentMover.h
    CompositionItemHelper.h
    CompositionItem.h
    SegmentOrderer.h
    AudioPreviewPainter.h
    AudioPreviewUpdater.h
    SegmentPencil.h
    SegmentTool.h
    SegmentEraser.h
    CompositionModelImpl.h
    CompositionColourCache.h
    CompositionRect.h
    SegmentItemPreview.h
    CompositionModel.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${compositionview_library_SRCS})

# actual target:
ADD_LIBRARY(compositionview ${compositionview_library}_SCRS ${compositionview_library_HDRS})
SET_TARGET_PROPERTIES(compositionview PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(compositionview ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS compositionview DESTINATION lib)
