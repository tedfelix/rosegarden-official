# sound libraries: libRosegardenSequencer.a and libRosegardenSound.a

INCLUDE_DIRECTORIES( 
    ${KDE3_INCLUDE_DIR} 
    ${QT_INCLUDE_DIR} 
    ${ALSA_INCLUDE_DIR} 
    ${OTHER_INCLUDES}
    ${ROSEGARDEN_SOURCE_DIR}/src/base
    ${ROSEGARDEN_SOURCE_DIR}/src/sound
    ${ROSEGARDEN_BINARY_DIR}/src/sound
)

LINK_DIRECTORIES (
    ${ROSEGARDEN_BINARY_DIR}/src/base
)

SET(main_SRCS
	AudioCache.cpp
	AudioCache.h
	AudioFile.cpp
	AudioFile.h
	AudioFileManager.cpp
	AudioFileManager.h
	AudioPlayQueue.cpp
	AudioPlayQueue.h
	AudioProcess.cpp
	AudioProcess.h
	Audit.cpp
	Audit.h
	BWFAudioFile.cpp
	BWFAudioFile.h
	ControlBlock.cpp
	ControlBlock.h
	DSSIPluginFactory.cpp
	DSSIPluginFactory.h
	DSSIPluginInstance.cpp
	DSSIPluginInstance.h
	ExternalTransport.h
	LADSPAPluginFactory.cpp
	LADSPAPluginFactory.h
	LADSPAPluginInstance.cpp
	LADSPAPluginInstance.h
	MappedCommon.h
	MappedComposition.cpp
	MappedComposition.h
	MappedDevice.cpp
	MappedDevice.h
	MappedEvent.cpp
	MappedEvent.h
	MappedInstrument.cpp
	MappedInstrument.h
	MappedRealTime.cpp
	MappedRealTime.h
	MappedStudio.cpp
	MappedStudio.h
	MidiEvent.cpp
	MidiEvent.h
	MidiFile.cpp
	MidiFile.h
	Midi.h
	MP3AudioFile.cpp
	MP3AudioFile.h
	PeakFile.cpp
	PeakFile.h
	PeakFileManager.cpp
	PeakFileManager.h
	PluginFactory.cpp
	PluginFactory.h
	PluginIdentifier.cpp
	PluginIdentifier.h
	RIFFAudioFile.cpp
	RIFFAudioFile.h
	RingBuffer.h
	RunnablePluginInstance.cpp
	RunnablePluginInstance.h
	Scavenger.h
	SequencerDataBlock.cpp
	SequencerDataBlock.h
	SF2PatchExtractor.cpp
	SF2PatchExtractor.h
	SoundFile.cpp
	SoundFile.h
	WAVAudioFile.cpp
	WAVAudioFile.h 
)

SET(sound_SRCS
	AlsaDriver.cpp
	AlsaDriver.h
	AlsaPort.cpp
	AlsaPort.h
	DummyDriver.h
	JackDriver.cpp
	JackDriver.h
	PlayableAudioFile.cpp
	PlayableAudioFile.h
	SoundDriver.cpp
	SoundDriver.h
	SoundDriverFactory.cpp
	SoundDriverFactory.h 
)

SET(nosnd_SRCS
	DummyDriver.h
	PlayableAudioFile.cpp
	PlayableAudioFile.h
	SoundDriver.cpp
	SoundDriver.h
	SoundDriverFactory.cpp
	SoundDriverFactory.h
)

KDE3_AUTOMOC(${main_SRCS})

IF(HAVE_ALSA)
	ADD_LIBRARY(RosegardenSound STATIC ${main_SRCS} ${sound_SRCS})
	ADD_LIBRARY(RosegardenSequencer STATIC ${sound_SRCS})
ELSE(HAVE_ALSA)
	ADD_LIBRARY(RosegardenSound STATIC ${main_SRCS} ${nosnd_SRCS})
	ADD_LIBRARY(RosegardenSequencer STATIC ${nosnd_SRCS})
ENDIF(HAVE_ALSA)

TARGET_LINK_LIBRARIES(RosegardenSound base)
TARGET_LINK_LIBRARIES(RosegardenSequencer base)
