IF(NOT ROSEGARDEN_SOURCE_DIR)
    MESSAGE(FATAL_ERROR "You need to run cmake or ccmake at the root directory")
ENDIF(NOT ROSEGARDEN_SOURCE_DIR)

IF(USE_PCH)
    ADD_PRECOMPILED_HEADER(precompiled_headers misc/stableheaders.h)
ENDIF(USE_PCH)

LINK_DIRECTORIES(
    ${ALSA_LIB_DIR}
    ${JACK_LIB_DIR}
    ${XFT_LIB_DIR}
    ${LIBLO_LIB_DIR}
    ${LRDF_LIB_DIR}
    ${LIRC_LIB_DIR}
    ${FFTW3F_LIB_DIR}    
)

INCLUDE_DIRECTORIES(AFTER
    ${QT_INCLUDE_DIR} 
    ${ALSA_INC_DIR}
    ${JACK_INC_DIR}
    ${XFT_INC_DIR}
    ${DSSI_INC_DIR}
    ${LIBLO_INC_DIR}
    ${LADSPA_INC_DIR}
    ${LRDF_INC_DIR}
    ${LIRC_INC_DIR}
    ${FFTW3F_INC_DIR}
    base
)

SET(CMAKE_INCLUDE_PATH ".")

INCLUDE(commands/segment/CMakeLists.txt)
INCLUDE(commands/matrix/CMakeLists.txt)
INCLUDE(commands/edit/CMakeLists.txt)
INCLUDE(commands/studio/CMakeLists.txt)
INCLUDE(commands/notation/CMakeLists.txt)
INCLUDE(commands/CMakeLists.txt)
INCLUDE(helpers/CMakeLists.txt)
INCLUDE(gui/dialogs/CMakeLists.txt)
INCLUDE(gui/application/CMakeLists.txt)
INCLUDE(gui/seqmanager/CMakeLists.txt)
INCLUDE(gui/widgets/CMakeLists.txt)
INCLUDE(gui/configuration/CMakeLists.txt)
INCLUDE(gui/studio/CMakeLists.txt)
INCLUDE(gui/editors/segment/compositionview/CMakeLists.txt)
INCLUDE(gui/editors/segment/CMakeLists.txt)
INCLUDE(gui/editors/parameters/CMakeLists.txt)
INCLUDE(gui/editors/markers/CMakeLists.txt)
INCLUDE(gui/editors/matrix/CMakeLists.txt)
INCLUDE(gui/editors/tempo/CMakeLists.txt)
INCLUDE(gui/editors/guitar/CMakeLists.txt)
INCLUDE(gui/editors/notation/CMakeLists.txt)
INCLUDE(gui/editors/eventlist/CMakeLists.txt)
INCLUDE(gui/editors/CMakeLists.txt)
INCLUDE(gui/rulers/CMakeLists.txt)
INCLUDE(gui/general/CMakeLists.txt)
INCLUDE(gui/CMakeLists.txt)
INCLUDE(sound/CMakeLists.txt)
INCLUDE(base/CMakeLists.txt)
INCLUDE(sequencer/CMakeLists.txt)
INCLUDE(misc/CMakeLists.txt)
INCLUDE(document/io/CMakeLists.txt)
INCLUDE(document/CMakeLists.txt)

IF(USE_PCH)
    ADD_DEPENDENCIES(rosegarden precompiled_headers)
ENDIF(USE_PCH)

TARGET_LINK_LIBRARIES(application
    ${QT_LIBRARIES}
    ${QT_QT3SUPPORT_LIBRARY}    
    ${LIBLO_LIBS}
    ${LRDF_LIBS}
    ${FFTW3F_LIBS}
    ${XFT_LIBS}
    ${LIRC_LIBS}
    ${ALSA_LIBS} 
    ${JACK_LIBS}
    ${FFTW3F_LIBS}
    segment_commands
    matrix_commands
    edit_commands
    studio
    studio_commands
    notation_commands
    commands
    helpers
    dialogs
    application
    seqmanager
    widgets
    configuration
    studio
    compositionview
    segment
    parameters
    markers
    matrix
    tempo
    guitar
    notation
    eventlist
    editors
    rulers
    general
    gui
    sound
    base
    sequencer
    misc
    io
    document
)

SET (BIN_INSTALL_DIR /tmp)
SET (DATA_INSTALLDIR /tmp)

# Install targets
INSTALL(TARGETS application
        RUNTIME DESTINATION ${BIN_INSTALL_DIR} )

# Install helper scripts
INSTALL(PROGRAMS
            helpers/rosegarden-lilypondview
            helpers/rosegarden-project-package
            helpers/rosegarden-audiofile-importer
        DESTINATION ${BIN_INSTALL_DIR} )

# Install GUI resource files
#FILE(GLOB RC_FILES gui/ui/*.rc)
#INSTALL(FILES ${RC_FILES}
#        DESTINATION ${DATA_INSTALL_DIR}/rosegarden )

