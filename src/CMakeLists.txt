IF(NOT ROSEGARDEN_SOURCE_DIR)
    MESSAGE(FATAL_ERROR "You need to run cmake or ccmake at the root directory")
ENDIF(NOT ROSEGARDEN_SOURCE_DIR)

IF(USE_PCH)
    ADD_PRECOMPILED_HEADER(precompiled_headers misc/stableheaders.h)
ENDIF(USE_PCH)

LINK_DIRECTORIES(
    ${KDE4_LIB_DIR} 
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
    ${KDE4_INCLUDE_DIR} 
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

# Common sources
INCLUDE(BaseFileList.txt)
INCLUDE(MiscFileList.txt)

# Sound sources
INCLUDE(SoundFileList.txt)

QT4_AUTOMOC(${common_sound_SRCS})

ADD_LIBRARY(RosegardenCommon STATIC
    ${base_SRCS}
    ${misc_SRCS}
    ${common_sound_SRCS}
)

IF(USE_PCH)
    ADD_DEPENDENCIES(RosegardenCommon precompiled_headers)
ENDIF(USE_PCH)

# sequencer executable
INCLUDE(SequencerFileList.txt)

IF(WANT_SOUND)
    SET(snd_SRCS ${sound_SRCS})
ELSE(WANT_SOUND)
    SET(snd_SRCS ${nosnd_SRCS})
ENDIF(WANT_SOUND)

# gui executable
INCLUDE(GUIFileList.txt)

KDE4_ADD_UI_FILES(gui_SRCS ${ui_SRCS})
QT4_AUTOMOC(${gui_SRCS})
QT4_AUTOMOC(${segmentcanvas_SRCS})
QT4_AUTOMOC(${commands_SRCS})
QT4_AUTOMOC(${document_SRCS})
QT4_AUTOMOC(${sequencer_SRCS})

ADD_LIBRARY(RosegardenExtended STATIC
    ${document_SRCS}
    ${gui_SRCS}
)

ADD_LIBRARY(RosegardenSequencer STATIC
    ${snd_SRCS}
    ${sequencer_SRCS}
)

ADD_LIBRARY(RosegardenCommands STATIC
    ${commands_SRCS}
)

IF(USE_PCH)
    ADD_DEPENDENCIES(RosegardenExtended precompiled_headers)
ENDIF(USE_PCH)

IF(USE_PCH)
    ADD_DEPENDENCIES(RosegardenCommands precompiled_headers)
ENDIF(USE_PCH)

ADD_LIBRARY(RosegardenSegmentCanvas STATIC
    ${segmentcanvas_SRCS}
)

IF(USE_PCH)
    ADD_DEPENDENCIES(RosegardenSegmentCanvas precompiled_headers)
ENDIF(USE_PCH)



KDE4_ADD_EXECUTABLE(rosegarden 
    #${commands_SRCS}
    #${document_SRCS}
    #${gui_SRCS}
    #${sequencer_SRCS}
    #${segmentcanvas_SRCS}
    gui/application/main.cpp
)

IF(USE_PCH)
    ADD_DEPENDENCIES(rosegarden precompiled_headers)
ENDIF(USE_PCH)

TARGET_LINK_LIBRARIES(rosegarden
    ${QT_AND_KDECORE_LIBS} 
    ${LIBLO_LIBS}
    ${LRDF_LIBS}
    ${FFTW3F_LIBS}
    ${XFT_LIBS}
    ${LIRC_LIBS}
    ${ALSA_LIBS} 
    ${JACK_LIBS}
    ${FFTW3F_LIBS}
    kio
    kdeui
    kdeprint
    RosegardenExtended
    RosegardenSequencer
    RosegardenSegmentCanvas
    RosegardenCommands
    RosegardenCommon
)

# Install targets
INSTALL(TARGETS rosegarden 
        RUNTIME DESTINATION ${BIN_INSTALL_DIR} )

# Install helper scripts
INSTALL(PROGRAMS
            helpers/rosegarden-lilypondview
            helpers/rosegarden-project-package
            helpers/rosegarden-audiofile-importer
        DESTINATION ${BIN_INSTALL_DIR} )

# Install GUI resource files
FILE(GLOB RC_FILES gui/ui/*.rc)
INSTALL(FILES ${RC_FILES}
        DESTINATION ${DATA_INSTALL_DIR}/rosegarden )

IF(WANT_TEST)
    INCLUDE(TestFileList.txt)

    CREATE_TEST_SOURCELIST(RosegardenTest
    	RosegardenTestDriver.cpp
        ${tests_SRCS}
#        base/test/transpose.cpp
#        base/test/segmenttransposecommand.cpp
    )

    KDE4_ADD_EXECUTABLE(RosegardenTestDriver
        RosegardenTestDriver.cpp
        ${tests_SRCS}
    )

    TARGET_LINK_LIBRARIES(RosegardenTestDriver
        ${QT_AND_KDECORE_LIBS} 
        ${LIBLO_LIBS}
        ${LRDF_LIBS}
        ${FFTW3F_LIBS}
        ${XFT_LIBS}
        ${LIRC_LIBS}
        ${JACK_LIBS}
        kio
        kdeui
        kdeprint
        RosegardenExtended
        RosegardenSequencer
        RosegardenSegmentCanvas
        RosegardenCommands
        RosegardenCommon
    )
    FOREACH(currenttest ${tests_SRCS})
        GET_FILENAME_COMPONENT(TestName ${currenttest} NAME_WE)
        ADD_TEST(test/${TestName} RosegardenTestDriver test/${TestName})
    ENDFOREACH(currenttest ${tests_SRCS})
ENDIF(WANT_TEST)

