# ########## Project setup ##########
PROJECT(document)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## document library ##########
# Sources:
SET(document_library_SRCS
    RosegardenDocument.cpp
    XmlStorableEvent.cpp
    CommandHistory.cpp
    ConfigGroups.cpp
    CommandRegistry.cpp
    XmlSubHandler.cpp
    GzipFile.cpp
    Command.cpp
    BasicCommand.cpp
    RoseXmlHandler.cpp
    BasicSelectionCommand.cpp
)

# Headers:
SET(document_library_HDRS
    XmlStorableEvent.h
    CommandHistory.h
    ConfigGroups.h
    CommandRegistry.h
    XmlSubHandler.h
    GzipFile.h
    Command.h
    BasicCommand.h
    BasicSelectionCommand.h
    RoseXmlHandler.h
    RosegardenDocument.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${document_library_SRCS})

# actual target:
ADD_LIBRARY(document ${document_library}_SRCS ${document_library_HDRS})
SET_TARGET_PROPERTIES(document PROPERTIES VERSION 0.0.0)
TARGET_LINK_LIBRARIES(document ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS document DESTINATION lib)
