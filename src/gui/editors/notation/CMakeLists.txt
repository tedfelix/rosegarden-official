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
    NotationWidget.cpp
    GuitarChordInserter.cpp
    NotationGroup.cpp
    NoteFontViewer.cpp
    NoteCharacter.cpp
    ClefInserter.cpp
    NotationView.cpp
    NoteStyle.cpp
    NotationTool.cpp
    NotationEraser.cpp
    StaffLayout.cpp
    NoteCharacterNames.cpp
    NotationProperties.cpp
    NotePixmapFactory.cpp
    NoteStyleFactory.cpp
    NotationCommandRegistry.cpp
    NoteFontFactory.cpp
    NoteFontMap.cpp
    BarLineItem.cpp
    TextInserter.cpp
    NotationStrings.cpp
    NotationElement.cpp
    NotePixmapParameters.cpp
    NotationChord.cpp
    FontViewFrame.cpp
    TrackHeader.cpp
    NotationStaff.cpp
    SystemFontQt.cpp
    RestInserter.cpp
    NoteFont.cpp
    SystemFontXft.cpp
    NotationHLayout.cpp
    NotationToolBox.cpp
    NotationScene.cpp
    NoteStyleFileReader.cpp
    SystemFont.cpp
    NotationVLayout.cpp
    NoteInserter.cpp
    OldNotationView.cpp
    HeadersGroup.cpp
    NotationSelector.cpp
)

# Headers:
SET(notation_library_HDRS
    NotationTool.h
    NotationEraser.h
    NoteFont.h
    NotationToolBox.h
    StaffLayout.h
    NotationCommandRegistry.h
    HeadersGroup.h
    BarLineItem.h
    NotationSelector.h
    NotationStrings.h
    NotationElement.h
    NotationMouseEvent.h
    NotationGroup.h
    NotationChord.h
    TrackHeader.h
    NoteFontViewer.h
    NotationStaff.h
    NoteCharacter.h
    NotationView.h
    NoteStyle.h
    RestInserter.h
    SystemFontXft.h
    NotationHLayout.h
    NoteCharacterNames.h
    NotationProperties.h
    NoteStyleFileReader.h
    NotationScene.h
    SystemFont.h
    NotePixmapFactory.h
    NoteStyleFactory.h
    NotationVLayout.h
    NoteInserter.h
    NoteFontFactory.h
    OldNotationView.h
    NoteFontMap.h
    NotationWidget.h
    TextInserter.h
    NotePixmapPainter.h
    NotePixmapParameters.h
    FontViewFrame.h
    SystemFontQt.h
    ClefInserter.h
    GuitarChordInserter.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${notation_library_SRCS})

# actual target:
ADD_LIBRARY(notation ${notation_library}_SRCS ${notation_library_HDRS})
#SET_TARGET_PROPERTIES(notation PROPERTIES VERSION 0.0.0)
#TARGET_LINK_LIBRARIES(notation ${QT_LIBRARIES})

# add install target:
#INSTALL(TARGETS notation DESTINATION lib)
