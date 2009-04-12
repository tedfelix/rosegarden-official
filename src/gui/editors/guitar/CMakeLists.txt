# ########## Project setup ##########
PROJECT(guitar)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## guitar library ##########
# Sources:
SET(guitar_library_SRCS
    Fingering.cpp
    Chord.cpp
    NoteSymbols.cpp
    GuitarChordEditorDialog.cpp
    ChordMap.cpp
    FingeringListBoxItem.cpp
    ChordXmlHandler.cpp
    FingeringBox.cpp
    GuitarChordSelectorDialog.cpp
)

# Headers:
SET(guitar_library_HDRS
    GuitarChordSelectorDialog.h
    FingeringListBoxItem.h
    ChordMap.h
    Chord.h
    Fingering.h
    GuitarChordEditorDialog.h
    NoteSymbols.h
    ChordXmlHandler.h
    FingeringBox.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${guitar_library_SRCS})

# actual target:
ADD_LIBRARY(guitar ${guitar_library_SRCS} ${guitar_library_HDRS})
SET_TARGET_PROPERTIES(guitar PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(guitar ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS guitar DESTINATION lib)
