# ########## Project setup ##########
PROJECT(studio_commands)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## studio_commands library ##########
# Sources:
SET(studio_commands_library_SRCS
    ReconnectDeviceCommand.cpp
    RemoveControlParameterCommand.cpp
    ModifyControlParameterCommand.cpp
    CreateOrDeleteDeviceCommand.cpp
    ModifyDeviceCommand.cpp
    ModifyDeviceMappingCommand.cpp
    RenameDeviceCommand.cpp
    ModifyInstrumentMappingCommand.cpp
    AddControlParameterCommand.cpp
)

# Headers:
SET(studio_commands_library_HDRS
    ModifyInstrumentMappingCommand.h
    AddControlParameterCommand.h
    RemoveControlParameterCommand.h
    ReconnectDeviceCommand.h
    CreateOrDeleteDeviceCommand.h
    ModifyDeviceCommand.h
    ModifyDeviceMappingCommand.h
    RenameDeviceCommand.h
    ModifyControlParameterCommand.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${studio_commands_library_SRCS})

# actual target:
ADD_LIBRARY(studio_commands ${studio_commands_library}_SRCS ${studio_commands_library_HDRS})
#SET_TARGET_PROPERTIES(studio_commands PROPERTIES VERSION 0.0.0)
#TARGET_LINK_LIBRARIES(studio_commands ${QT_LIBRARIES})

# add install target:
#INSTALL(TARGETS studio_commands DESTINATION lib)
