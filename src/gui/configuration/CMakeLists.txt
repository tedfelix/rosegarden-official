# ########## Project setup ##########
PROJECT(configuration)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## configuration executable ##########
# Sources:
SET(configuration_executable_SRCS
    HeadersConfigurationPage.cpp
    GeneralConfigurationPage.cpp
    ConfigurationPage.cpp
    TabbedConfigurationPage.cpp
    MIDIConfigurationPage.cpp
    DocumentMetaConfigurationPage.cpp
    AudioPropertiesPage.cpp
    NotationConfigurationPage.cpp
    LatencyConfigurationPage.cpp
    AudioConfigurationPage.cpp
    ColourConfigurationPage.cpp
    MatrixConfigurationPage.cpp
)

# Headers:
SET(configuration_executable_HDRS
    DocumentMetaConfigurationPage.h
    MatrixConfigurationPage.h
    HeadersConfigurationPage.h
    GeneralConfigurationPage.h
    ConfigurationPage.h
    TabbedConfigurationPage.h
    MIDIConfigurationPage.h
    AudioPropertiesPage.h
    NotationConfigurationPage.h
    LatencyConfigurationPage.h
    AudioConfigurationPage.h
    ColourConfigurationPage.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${configuration_executable_SRCS})

# actual target:
ADD_EXECUTABLE(configuration ${configuration_executable_SRCS})
TARGET_LINK_LIBRARIES(configuration ${QT_LIBRARIES})

# add install target:
INSTALL(TARGETS configuration DESTINATION bin)
