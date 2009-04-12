# ########## Project setup ##########
PROJECT(sound)
CMAKE_MINIMUM_REQUIRED(VERSION 2.4.5)

# ########## Qt4 setup ##########
FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR} ${QT_INCLUDES})

# ########## sound library ##########
# Sources:
SET(sound_library_SRCS
    PluginIdentifier.cpp
    BWFAudioFile.cpp
    AudioProcess.cpp
    AudioFileTimeStretcher.cpp
    AudioTimeStretcher.cpp
    MidiFile.cpp
    LADSPAPluginInstance.cpp
    PeakFileManager.cpp
    AudioCache.cpp
    SoundFile.cpp
    MappedComposition.cpp
    MappedStudio.cpp
    SoundDriverFactory.cpp
    RIFFAudioFile.cpp
    MappedEvent.cpp
    SF2PatchExtractor.cpp
    MidiEvent.cpp
    PluginFactory.cpp
    SoundDriver.cpp
    LADSPAPluginFactory.cpp
    RunnablePluginInstance.cpp
    PlayableAudioFile.cpp
    PeakFile.cpp
    AlsaPort.cpp
    AudioPlayQueue.cpp
    AlsaDriver.cpp
    MappedInstrument.cpp
    SequencerDataBlock.cpp
    AudioFile.cpp
    Audit.cpp
    DSSIPluginInstance.cpp
    DSSIPluginFactory.cpp
    WAVAudioFile.cpp
    AudioFileManager.cpp
    MP3AudioFile.cpp
    JackDriver.cpp
    RecordableAudioFile.cpp
    MappedDevice.cpp
    ControlBlock.cpp
)

# Headers:
SET(sound_library_HDRS
    AlsaDriver.h
    AudioPlayQueue.h
    Audit.h
    DSSIPluginInstance.h
    RunnablePluginInstance.h
    DummyDriver.h
    AudioFileManager.h
    JackDriver.h
    MappedDevice.h
    RecordableAudioFile.h
    RingBuffer.h
    AudioProcess.h
    BWFAudioFile.h
    MidiFile.h
    Midi.h
    AudioFileTimeStretcher.h
    AudioTimeStretcher.h
    LADSPAPluginInstance.h
    LADSPAPluginFactory.h
    PlayableAudioFile.h
    Scavenger.h
    MappedStudio.h
    AlsaPort.h
    SoundFile.h
    AudioFile.h
    MappedComposition.h
    SoundDriverFactory.h
    SequencerDataBlock.h
    ExternalTransport.h
    MappedInstrument.h
    SF2PatchExtractor.h
    RIFFAudioFile.h
    MappedEvent.h
    DSSIPluginFactory.h
    WAVAudioFile.h
    MP3AudioFile.h
    MidiEvent.h
    PluginFactory.h
    ControlBlock.h
    PluginIdentifier.h
    SoundDriver.h
    PeakFileManager.h
    SampleWindow.h
    MappedCommon.h
    PeakFile.h
    AudioCache.h
)


# scan files and run moc on all that need it:
QT4_AUTOMOC(${sound_library_SRCS})

# actual target:
ADD_LIBRARY(sound ${sound_library}_SRCS ${sound_library_HDRS})
