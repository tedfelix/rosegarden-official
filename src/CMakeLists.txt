IF(NOT ROSEGARDEN_SOURCE_DIR)
    MESSAGE(FATAL_ERROR "You need to run cmake or ccmake at the root directory")
ENDIF(NOT ROSEGARDEN_SOURCE_DIR)

IF(USE_PCH)
	#INCLUDE(QTKDEPCHsplit)
	#INCLUDE(QTKDEPCHjoin)
	#ADD_QTKDE_PRECOMPILED_HEADERS(${CMAKE_CURRENT_BINARY_DIR}/pch)
	
	# 3er try. This allows to use a hand made, fine tuned header
	INCLUDE(UsePCH)
	ADD_PRECOMPILED_HEADER(misc/stableheaders.h)
ENDIF(USE_PCH)

LINK_DIRECTORIES(
    ${KDE3_LIB_DIR} 
    ${ALSA_LIB_DIR}
    ${JACK_LIB_DIR}
    ${XFT_LIB_DIR}
    ${LIBLO_LIB_DIR}
)

INCLUDE_DIRECTORIES(AFTER
    ${QT_INCLUDE_DIR} 
    ${KDE3_INCLUDE_DIR} 
    ${ALSA_INC_DIR}
    ${JACK_INC_DIR}
    ${XFT_INC_DIR}
    ${DSSI_INC_DIR}
    ${LIBLO_INC_DIR}
    ${LADSPA_INC_DIR}
    ${LRDF_INC_DIR}
    ${LIRC_INC_DIR}
    base
)

SET(CMAKE_INCLUDE_PATH ".")

# Common sources
INCLUDE(BaseFileList.txt)
INCLUDE(MiscFileList.txt)

# Sound sources
INCLUDE(SoundFileList.txt)

KDE3_AUTOMOC(${common_sound_SRCS})

ADD_LIBRARY(RosegardenCommon STATIC
    ${misc_SRCS}
    ${base_SRCS}
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

KDE3_ADD_DCOP_SKELS(sequencer_SRCS ${seq_dcop_SRCS})
KDE3_ADD_DCOP_STUBS(sequencer_SRCS ${seq_dcop_SRCS}) 
KDE3_AUTOMOC(${sequencer_SRCS})

KDE3_ADD_EXECUTABLE(rosegardensequencer 
    ${snd_SRCS}
    ${sequencer_SRCS}
)

IF(USE_PCH)
    ADD_DEPENDENCIES(rosegardensequencer precompiled_headers)
ENDIF(USE_PCH)

TARGET_LINK_LIBRARIES(rosegardensequencer 
    ${QT_AND_KDECORE_LIBS} 
    ${ALSA_LIBS} 
    ${JACK_LIBS}
    ${LIBLO_LIBS}
    ${LRDF_LIBS}
    kdeui
    RosegardenCommon
)

# gui executable
INCLUDE(GUIFileList.txt)

KDE3_ADD_DCOP_SKELS(gui_SRCS ${gui_dcop_SRCS})
KDE3_ADD_DCOP_STUBS(gui_SRCS ${gui_dcop_SRCS}) 
KDE3_ADD_UI_FILES(gui_SRCS ${ui_SRCS})
KDE3_AUTOMOC(${gui_SRCS})
KDE3_AUTOMOC(${commands_SRCS})
KDE3_AUTOMOC(${document_SRCS})

KDE3_ADD_EXECUTABLE(rosegarden 
    ${commands_SRCS}
    ${document_SRCS}
    ${gui_SRCS}
)

IF(USE_PCH)
    ADD_DEPENDENCIES(rosegarden precompiled_headers)
ENDIF(USE_PCH)

TARGET_LINK_LIBRARIES(rosegarden
    ${QT_AND_KDECORE_LIBS} 
    ${LIBLO_LIBS}
    ${LRDF_LIBS}
    ${XFT_LIBS}
    ${LIRC_LIBS}
    ${JACK_LIBS}
    kio
    kdeui
    kdeprint
    RosegardenCommon
)

# Install targets
INSTALL(TARGETS rosegarden rosegardensequencer
        RUNTIME DESTINATION bin)

# Install helper scripts
INSTALL(PROGRAMS
            helpers/rosegarden-lilypondview
            helpers/rosegarden-project-package
        DESTINATION bin )

# Install GUI resource files
FILE(GLOB RC_FILES gui/ui/*.rc)
INSTALL(FILES ${RC_FILES}
        DESTINATION share/apps/rosegarden )
