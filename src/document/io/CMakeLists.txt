# ########## Project setup ##########
PROJECT(io)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## io library ##########
# Sources:
SET(io_library_SRCS
    HydrogenXMLHandler.cpp
    RG21Loader.cpp
    LilyPondExporter.cpp
    CsoundExporter.cpp
    HydrogenLoader.cpp
    MupExporter.cpp
    MusicXmlExporter.cpp
)

# Headers:
SET(io_library_HDRS
    MupExporter.h
    MusicXmlExporter.h
    HydrogenXMLHandler.h
    RG21Loader.h
    LilyPondExporter.h
    CsoundExporter.h
    HydrogenLoader.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${io_library_SRCS})

# actual target:
ADD_LIBRARY(io ${io_library}_SCRS ${io_library_HDRS})
SET_TARGET_PROPERTIES(io PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(io ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS io DESTINATION lib)
