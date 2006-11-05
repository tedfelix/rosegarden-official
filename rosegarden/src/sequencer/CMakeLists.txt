# sequencer executable: rosegardensequencer

INCLUDE_DIRECTORIES ( 
    ${ROSEGARDEN_SOURCE_DIR}/src
    ${ROSEGARDEN_SOURCE_DIR}/src/base
    ${ROSEGARDEN_SOURCE_DIR}/src/sound
    ${ROSEGARDEN_BINARY_DIR}/src/sequencer
)

LINK_DIRECTORIES (
    ${ROSEGARDEN_BINARY_DIR}/src/base
    ${ROSEGARDEN_BINARY_DIR}/src/sound
)

SET(sequencer_SRCS
	ControlBlockMmapper.cpp
	ControlBlockMmapper.h
	main.cpp
	MmappedSegment.cpp
	MmappedSegment.h
	RosegardenSequencerApp.cpp
	RosegardenSequencerApp.h
	RosegardenSequencerIface.h
	SequencerMmapper.cpp
	SequencerMmapper.h
)

KDE3_ADD_DCOP_SKELS(sequencer_SRCS RosegardenSequencerIface.h)
KDE3_ADD_DCOP_STUBS(sequencer_SRCS RosegardenSequencerIface.h) 
KDE3_AUTOMOC(${sequencer_SRCS})
KDE3_ADD_EXECUTABLE(rosegardensequencer ${sequencer_SRCS})

TARGET_LINK_LIBRARIES(rosegardensequencer 
	${QT_AND_KDECORE_LIBS} 
	${ALSA_LIBS} 
	kdeui
	base
	RosegardenSound
	RosegardenSequencer
)
