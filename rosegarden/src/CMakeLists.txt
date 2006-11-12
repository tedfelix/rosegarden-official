
LINK_DIRECTORIES(
	${KDE3_LIB_DIR} 
	${ALSA_LIB_DIR}
	${JACK_LIB_DIR}
	${XFT_LIB_DIR}
	${LO_LIB_DIR}
)

INCLUDE_DIRECTORIES( 
	${QT_INCLUDE_DIR} 
	${KDE3_INCLUDE_DIR} 
	${ALSA_INC_DIR}
	${JACK_INC_DIR}
	${XFT_INC_DIR}
	${DSSI_INC_DIR}
	${LO_INC_DIR}
	${LADSPA_INC_DIR}
	${LRDF_INC_DIR}
	${LIRC_INC_DIR}
    base
)

SET(CMAKE_INCLUDE_PATH ".")

# Common sources
INCLUDE(BaseFileList.txt)
INCLUDE(MiscFileList.txt)
INCLUDE(SoundFileList.txt)

# sequencer executable
INCLUDE(SequencerFileList.txt)

IF(WANT_SOUND)
	SET(snd_SRCS ${common_sound_SRCS} ${sound_SRCS})
ELSE(WANT_SOUND)
	SET(snd_SRCS ${common_sound_SRCS} ${nosound_SRCS})
ENDIF(WANT_SOUND)

KDE3_ADD_DCOP_SKELS(sequencer_SRCS ${seq_dcop_SRCS})
KDE3_ADD_DCOP_STUBS(sequencer_SRCS ${seq_dcop_SRCS}) 
KDE3_AUTOMOC(${common_sound_SRCS})
KDE3_AUTOMOC(${sequencer_SRCS})

KDE3_ADD_EXECUTABLE(rosegardensequencer 
	${misc_SRCS}
	${base_SRCS}
	${snd_SRCS}
	${sequencer_SRCS}
)

TARGET_LINK_LIBRARIES(rosegardensequencer 
	${QT_AND_KDECORE_LIBS} 
	${ALSA_LIBS} 
	${JACK_LIBS}
	${LO_LIBS}
	${LRDF_LIBS}
	kdeui
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
	${misc_SRCS}
	${base_SRCS}
	${common_sound_SRCS}
	${commands_SRCS}
	${document_SRCS}
	${gui_SRCS}
)

TARGET_LINK_LIBRARIES(rosegarden
	${QT_AND_KDECORE_LIBS} 
	${LO_LIBS}
	${LRDF_LIBS}
	${XFT_LIBS}
	${LIRC_LIBS}
	${JACK_LIBS}
	kio
	kdeui
	kdeprint
)

# Install targets
INSTALL(TARGETS 
			rosegarden 
			rosegardensequencer
		RUNTIME DESTINATION bin
)

# Install helper scripts
INSTALL(PROGRAMS
			helpers/rosegarden-lilypondview
			helpers/rosegarden-project-package
		DESTINATION bin )
