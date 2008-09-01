/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _ROSEGARDEN_SEQUENCER_H_
#define _ROSEGARDEN_SEQUENCER_H_
 
// RosegardenSequencer is the sequencer application for Rosegarden.
// It owns a Sequencer object which wraps the ALSA 
// and JACK funtionality.  At this level we deal with comms with
// the Rosegarden GUI application, the high level marshalling of data 
// and main event loop of the sequencer.  [rwb]
//


// include files for Qt
#include <QStringList>

#include <QMutex>

#include "base/Composition.h"
#include "gui/application/TransportStatus.h"

#include "RosegardenSequencerIface.h"

#include "sound/MappedComposition.h"
#include "base/Event.h"
#include "sound/MappedStudio.h"
#include "sound/ExternalTransport.h"

#include "MmappedSegment.h"
#include "SequencerMmapper.h"

#include <deque>

namespace Rosegarden { 

// forward declaration of the RosegardenGUI classes
class RosegardenGUIDoc;
class RosegardenGUIView;
class MmappedControlBlock;

class MappedInstrument;
class SoundDriver;

/**
 * The sequencer application
 */
class RosegardenSequencer : public RosegardenSequencerIface,
                            public ExternalTransport
{
public:
    ~RosegardenSequencer();

    static RosegardenSequencer *getInstance();

    void lock();
    void unlock();
	
    //      -------- START OF DCOP INTERFACE METHODS --------
    //
    //


    // Quit
    virtual void quit();

    // Based on RealTime timestamps
    //
    bool play(const RealTime &position,
              const RealTime &readAhead,
              const RealTime &audioMix,
              const RealTime &audioRead,
              const RealTime &audioWrite,
              long smallFileSize);

    // recording
    bool record(const RealTime &position,
                const RealTime &readAhead,
                const RealTime &audioMix,
                const RealTime &audioRead,
                const RealTime &audioWrite,
                long smallFileSize,
                long recordMode);

    virtual bool punchOut();

    // looping
    void setLoop(const RealTime &loopStart,
                 const RealTime &loopEnd);

    // Jump to a pointer in the playback
    virtual void jumpTo(const RealTime &rt);
 
    // Return the Sound system status (audio/MIDI)
    //
    virtual unsigned int getSoundDriverStatus(const QString &guiVersion);

    // Add and remove Audio files on the sequencer
    //
    virtual bool addAudioFile(const QString &fileName, int id);
    virtual bool removeAudioFile(int id);

    // Deletes all the audio files and clears down any flapping i/o handles
    //
    virtual void clearAllAudioFiles();

    // stops the sequencer
    //
    virtual void stop();

    // Set a MappedInstrument at the Sequencer
    //
    virtual void setMappedInstrument(int type, unsigned char channel,
                                     unsigned int id);

    // The sequencer will process the MappedComposition as soon as it
    // gets the chance.
    //
    virtual void processSequencerSlice(MappedComposition mC);

    virtual void processMappedEvent(MappedEvent mE);

    virtual unsigned int getDevices();
    virtual MappedDevice getMappedDevice(unsigned int id);

    virtual int canReconnect(Device::DeviceType deviceType);
    virtual unsigned int addDevice(Device::DeviceType type,
                                   MidiDevice::DeviceDirection direction);
    virtual void removeDevice(unsigned int id);
    virtual void renameDevice(unsigned int id, QString name);
    virtual unsigned int getConnections(Device::DeviceType type,
                                        MidiDevice::DeviceDirection direction);
    virtual QString getConnection(Device::DeviceType type,
                                  MidiDevice::DeviceDirection direction,
                                  unsigned int connectionNo);
    virtual void setConnection(unsigned int deviceId,
                               QString connection);
    virtual void setPlausibleConnection(unsigned int deviceId,
                                        QString idealConnection);
    
    virtual unsigned int getTimers();
    virtual QString getTimer(unsigned int n);
    virtual QString getCurrentTimer();
    virtual void setCurrentTimer(QString timer);

    virtual void setLowLatencyMode(bool);

    // Audio latencies
    //
    virtual RealTime getAudioPlayLatency();
    virtual RealTime getAudioRecordLatency();

    // Set a MappedObject 
    //
    virtual void setMappedProperty(int id,
                                   const QString &property,
                                   float value);

    // Set many properties on many MappedObjects
    //
    virtual void setMappedProperties(const MappedObjectIdList &ids,
                                     const MappedObjectPropertyList &properties,
                                     const MappedObjectValueList &values);

    // Set a MappedObject to a string
    //
    virtual void setMappedProperty(int id,
                                   const QString &property,
                                   const QString &value);

    // Set a MappedObject to a property list
    //
    virtual void setMappedPropertyList(int id,
                                       const QString &property,
                                       const MappedObjectPropertyList &values);

    // Get a MappedObject for a type
    //
    virtual int getMappedObjectId(int type);

    // Get a Property list from an Object
    //
    virtual std::vector<QString> getPropertyList(int id,
                                                 const QString &property);

    virtual std::vector<QString> getPluginInformation();

    virtual QString getPluginProgram(int id, int bank, int program);

    virtual unsigned long getPluginProgram(int id, const QString &name);

    // Set a plugin port
    //
    virtual void setMappedPort(int pluginId,
                               unsigned long portId,
                               float value);

    virtual float getMappedPort(int pluginId,
                                unsigned long portId);

    // Create a MappedObject
    virtual int createMappedObject(int type);

    // Destroy an object
    //
    virtual bool destroyMappedObject(int id);

    // Connect two objects
    //
    virtual void connectMappedObjects(int id1, int id2);
    
    // Disconnect two objects
    //
    virtual void disconnectMappedObjects(int id1, int id2);

    // Disconnect an object from everything
    //
    virtual void disconnectMappedObject(int id);

    // Sample rate
    //
    virtual unsigned int getSampleRate() const;

    // Clear the studio
    //
    virtual void clearStudio();

    // Debug stuff, to check MmappedSegment::iterator
    virtual void dumpFirstSegment();

    virtual void remapSegment(const QString& filename, size_t newSize);
    virtual void addSegment(const QString& filename);
    virtual void deleteSegment(const QString& filename);
    virtual void closeAllSegments();
    virtual void remapTracks();

    // Set Quarter note length
    //
    virtual void setQuarterNoteLength(RealTime rt);

    // Get a status report
    // 
    virtual QString getStatusLog();

    bool getNextTransportRequest(TransportRequest &request, RealTime &time);

    //
    //
    //
    //      -------- END OF DCOP INTERFACE --------




    void setStatus(TransportStatus status)
            { m_transportStatus = status; }
    TransportStatus getStatus() { return m_transportStatus; }
   
    // Process the first chunk of Sequencer events
    bool startPlaying();

    // Process all subsequent events
    bool keepPlaying();

    // Update internal clock and send GUI position pointer movement
    void updateClocks();

//!!!    bool checkExternalTransport();

    // Sends status changes up to GUI
//!!!    void notifySequencerStatus();

    // Send latest slice information back to GUI for display
//!!!    void notifyVisuals(MappedComposition *mC);

    // These two methods process any pending MIDI or audio
    // and send them up to the gui for storage and display
    //
    void processRecordedMidi();
    void processRecordedAudio();

    // Called during stopped or playing operation to process
    // any pending incoming MIDI events that aren't being
    // recorded (i.e. for display in Transport or on Mixer)
    //
    void processAsynchronousEvents();

    // Sleep for the given time, approximately.  Called from the main
    // loop in order to lighten CPU load (i.e. the timing quality of
    // the sequencer does not depend on this being accurate).  A good
    // implementation of this call would return right away when an
    // incoming MIDI event needed to be handled.
    //
    void sleep(const RealTime &rt);

    // Removes from a MappedComposition the events not matching
    // the supplied filer.
    //
    void applyFiltering(MappedComposition *mC,
                        MidiFilter filter,
                        bool filterControlDevice);

    // This method assigns an Instrument to each MappedEvent 
    // belongin to the MappedComposition, and sends the 
    // transformed events to the driver to be played.
    //
    void routeEvents(MappedComposition *mC, bool useSelectedTrack);

    // Are we looping?
    //
    bool isLooping() const { return !(m_loopStart == m_loopEnd); }

    // the call itself
    void sequencerAlive();

    // check for new external clients (ALSA sequencer or whatever) --
    // polled regularly
    void checkForNewClients();

    /*
    // Audio latencies
    //
    RealTime getAudioPlaybackLatency()
        { return m_audioPlayLatency; }
    void setAudioPlaybackLatency(const RealTime &latency)
        { m_audioPlayLatency = latency; }

    RealTime getAudioRecordLatency()
        { return m_audioRecordLatency; }
    void setAudioRecordLatency(const RealTime &latency)
        { m_audioRecordLatency = latency; }
    */

    // Initialise the virtual studio at this end of the link
    //
    void initialiseStudio();


    // --------- EXTERNAL TRANSPORT INTERFACE METHODS --------
    //
    // Whereas the DCOP interface (above) is for the GUI to call to
    // make the sequencer follow its wishes, this interface is for
    // external clients to call (via some low-level audio callback)
    // and requires sychronising with the GUI.
    
    TransportToken transportChange(TransportRequest);
    TransportToken transportJump(TransportRequest, RealTime);
    bool isTransportSyncComplete(TransportToken token);
    TransportToken getInvalidTransportToken() const { return 0; }

protected:
    RosegardenSequencer();

    // get events whilst handling loop
    //
    void fetchEvents(MappedComposition &,
                     const RealTime &start,
                     const RealTime &end,
                     bool firstFetch);

    // just get a slice of events between markers
    //
    void getSlice(MappedComposition &,
                  const RealTime &start,
                  const RealTime &end,
                  bool firstFetch);

    // adjust event times according to relative instrument latencies
    //
    void applyLatencyCompensation(MappedComposition &);

    // mmap-related stuff
    MmappedSegment* mmapSegment(const QString&);
    void cleanupMmapData();
    void initMetaIterator();

    void rationalisePlayingAudio();
    void setEndOfCompReached(bool e) { m_isEndOfCompReached = e; }
    bool isEndOfCompReached() { return m_isEndOfCompReached; }
    void incrementTransportToken();

    //--------------- Data members ---------------------------------

    SoundDriver *m_driver;
    TransportStatus m_transportStatus;

    // Position pointer
    RealTime m_songPosition;
    RealTime m_lastFetchSongPosition;

    RealTime m_readAhead;
    RealTime m_audioMix;
    RealTime m_audioRead;
    RealTime m_audioWrite;
    int m_smallFileSize;

    /*

    // Not required at the sequencer

    // Two more latencies for audio play and record - when we 
    // use an unsynchronised audio and MIDI system such as
    // ALSA and JACK we need to use these additional values
    // to help time-keeping.
    //
    RealTime m_audioPlayLatency;
    RealTime m_audioRecordLatency;

    */


    RealTime m_loopStart;
    RealTime m_loopEnd;

    std::vector<MappedInstrument*> m_instruments;

    // MappedStudio holds all of our session-persistent information -
    // sliders and what have you.  It's also streamable over DCOP
    // so you can reconstruct it at either end of the link for 
    // presentation, storage etc.
    //
    MappedStudio *m_studio;

    // mmap segments
    // 
    QString m_segmentFilesPath;
    MmappedSegmentsMetaIterator::mmappedsegments m_mmappedSegments;
    MmappedSegmentsMetaIterator* m_metaIterator;
    RealTime m_lastStartTime;

    MappedComposition m_mC;
    MmappedControlBlock *m_controlBlockMmapper;
    SequencerMmapper m_sequencerMapper;

    typedef std::pair<TransportRequest, RealTime> TransportPair;
    std::deque<TransportPair> m_transportRequests;
    TransportToken m_transportToken;

    bool m_isEndOfCompReached;
    
    QMutex m_mutex;
    QMutex m_transportRequestMutex;

    static RosegardenSequencer *m_instance;
    static QMutex m_instanceMutex;
};

}
 
#endif // _ROSEGARDEN_SEQUENCER_APP_H_
