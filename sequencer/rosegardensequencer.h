// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
  Rosegarden-4
  A sequencer and musical notation editor.

  This program is Copyright 2000-2004
  Guillaume Laurent   <glaurent@telegraph-road.org>,
  Chris Cannam        <cannam@all-day-breakfast.com>,
  Richard Bown        <bownie@bownie.com>

  The moral right of the authors to claim authorship of this work
  has been asserted.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#ifndef _ROSEGARDEN_SEQUENCER_APP_H_
#define _ROSEGARDEN_SEQUENCER_APP_H_
 
// RosegardenSequencerApp is the sequencer application for Rosegarden.
// It owns a Rosegarden::Sequencer object which wraps the ALSA, JACK,
// aRTS etc. level funtionality.  At this level we deal with comms with
// the Rosegarden GUI application, the high level marshalling of data 
// and main event loop of the sequencer.  [rwb]
//


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// include files for Qt
#include <qstrlist.h>

// include files for KDE 
#include <kapp.h>
#include <kmainwindow.h>
#include <kaccel.h>

#include <qtimer.h>

#include "Composition.h"
#include "rosegardendcop.h"
#include "rosegardensequenceriface.h"
#include "MappedComposition.h"
#include "Event.h"
#include "MappedStudio.h"
#include "ExternalTransport.h"
#include "mmappedsegment.h"
#include "sequencermapper.h"

#include <deque>

class KURL;
class KRecentFilesAction;

// forward declaration of the RosegardenGUI classes
class RosegardenGUIDoc;
class RosegardenGUIView;
class ControlBlockMmapper;

namespace Rosegarden { class MappedInstrument; class SoundDriver; }

/**
 * The sequencer application
 */
class RosegardenSequencerApp : public KMainWindow,
                               virtual public RosegardenSequencerIface,
			       public Rosegarden::ExternalTransport
{
    Q_OBJECT

public:
    RosegardenSequencerApp();
    ~RosegardenSequencerApp();

    //      -------- START OF DCOP INTERFACE METHODS --------
    //
    //


    // Quit
    virtual void quit();

    // Based on RealTime timestamps
    //
    int play(const Rosegarden::RealTime &position,
             const Rosegarden::RealTime &readAhead,
	     const Rosegarden::RealTime &audioMix,
	     const Rosegarden::RealTime &audioRead,
	     const Rosegarden::RealTime &audioWrite,
	     long smallFileSize);

    // recording
    int record(const Rosegarden::RealTime &position,
               const Rosegarden::RealTime &readAhead,
	       const Rosegarden::RealTime &audioMix,
	       const Rosegarden::RealTime &audioRead,
	       const Rosegarden::RealTime &audioWrite,
	       long smallFileSize,
               int recordMode);

    // looping
    void setLoop(const Rosegarden::RealTime &loopStart,
                 const Rosegarden::RealTime &loopEnd);


    // Play wrapper for DCOP
    //
    virtual int play(long timeSec,
                     long timeNsec,
                     long readAheadSec,
                     long readAheadNsec,
		     long audioMixSec,
		     long audioMixNsec,
		     long audioReadSec,
		     long audioReadNsec,
		     long audioWriteSec,
		     long audioWriteNsec,
		     long smallFileSize);

    // Record wrapper for DCOP
    //
    virtual int record(long timeSec,
                       long timeNsec,
                       long readAheadSec,
                       long readAheadNsec,
		       long audioMixSec,
		       long audioMixNsec,
		       long audioReadSec,
		       long audioReadNsec,
		       long audioWriteSec,
		       long audioWriteNsec,
		       long smallFileSize,
                       int recordMode);

    
    // Jump to a pointer in the playback (uses longs instead
    // of RealTime for DCOP)
    //
    //
    virtual void jumpTo(long posSec, long posNsec);

    // Set a loop on the Sequencer
    //
    virtual void setLoop(long loopStartSec, long loopStartNsec,
                         long loopEndSec, long loopEndNsec);
 
    // Return the Sound system status (audio/MIDI)
    //
    virtual unsigned int getSoundDriverStatus();

    // Add and remove Audio files on the sequencer
    //
    virtual int addAudioFile(const QString &fileName, int id);
    virtual int removeAudioFile(int id);

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
    virtual void processSequencerSlice(Rosegarden::MappedComposition mC);

    // Yeuch!
    //
    virtual void processMappedEvent(unsigned int id,
                                    int type,
                                    unsigned char pitch,
                                    unsigned char velocity,
                                    long absTimeSec,
                                    long absTimeNsec,
                                    long durationSec,
                                    long durationNsec,
                                    long audioStartMarkerSec,
                                    long audioStartMarkerNsec);

    // And now do it properly
    //
    virtual void processMappedEvent(Rosegarden::MappedEvent mE);

    virtual unsigned int getDevices();
    virtual Rosegarden::MappedDevice getMappedDevice(unsigned int id);

    virtual int canReconnect(int deviceType);
    virtual unsigned int addDevice(int type, unsigned int direction);
    virtual void removeDevice(unsigned int id);
    virtual unsigned int getConnections(int type, unsigned int direction);
    virtual QString getConnection(int type, unsigned int direction,
				  unsigned int connectionNo);
    virtual void setConnection(unsigned int deviceId, QString connection);
    virtual void setPlausibleConnection(unsigned int deviceId,
					QString idealConnection);
    
    virtual unsigned int getTimers();
    virtual QString getTimer(unsigned int n);
    virtual QString getCurrentTimer();
    virtual void setCurrentTimer(QString timer);

    virtual void setLowLatencyMode(bool);

    // Audio monitoring
    //
    virtual void setAudioMonitoring(long value);
    virtual void setAudioMonitoringInstrument(unsigned int id);

    // Audio latencies
    //
    virtual Rosegarden::MappedRealTime getAudioPlayLatency();
    virtual Rosegarden::MappedRealTime getAudioRecordLatency();

    // Set a MappedObject 
    //
    virtual void setMappedProperty(int id,
                                   const QString &property,
                                   float value);

    // Set a MappedObject to a string
    //
    virtual void setMappedProperty(int id,
                                   const QString &property,
                                   const QString &value);

    // Set a MappedObject to a property list
    //
    virtual void setMappedPropertyList(int id,
				       const QString &property,
				       const Rosegarden::MappedObjectPropertyList &values);

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
    virtual int destroyMappedObject(int id);

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
    virtual void setQuarterNoteLength(long timeSec, long timeNsec);

    // Get a status report
    // 
    virtual QString getStatusLog();

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

    bool checkExternalTransport();

    // Sends status changes up to GUI
    void notifySequencerStatus();

    // Send latest slice information back to GUI for display
    void notifyVisuals(Rosegarden::MappedComposition *mC);

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
    void sleep(const Rosegarden::RealTime &rt);

    void applyFiltering(Rosegarden::MappedComposition *mC,
			Rosegarden::MidiFilter filter);

    // Are we looping?
    //
    bool isLooping() const { return !(m_loopStart == m_loopEnd); }

    // the call itself
    void sequencerAlive();

    /*
    // Audio latencies
    //
    Rosegarden::RealTime getAudioPlaybackLatency()
        { return m_audioPlayLatency; }
    void setAudioPlaybackLatency(const Rosegarden::RealTime &latency)
        { m_audioPlayLatency = latency; }

    Rosegarden::RealTime getAudioRecordLatency()
        { return m_audioRecordLatency; }
    void setAudioRecordLatency(const Rosegarden::RealTime &latency)
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
    TransportToken transportJump(TransportRequest, Rosegarden::RealTime);
    bool isTransportSyncComplete(TransportToken token);

public slots:

    // Check for new clients - on timeout
    //
    void slotCheckForNewClients();

protected:

    // get events whilst handling loop
    //
    void fetchEvents(Rosegarden::MappedComposition &,
		     const Rosegarden::RealTime &start,
		     const Rosegarden::RealTime &end,
		     bool firstFetch);

    // just get a slice of events between markers
    //
    void getSlice(Rosegarden::MappedComposition &,
		  const Rosegarden::RealTime &start,
		  const Rosegarden::RealTime &end,
		  bool firstFetch);

    // mmap-related stuff
    MmappedSegment* mmapSegment(const QString&);
    void cleanupMmapData();
    void initMetaIterator();

    void rationalisePlayingAudio();
    void setEndOfCompReached(bool e) { m_isEndOfCompReached = e; }
    bool isEndOfCompReached() { return m_isEndOfCompReached; }
    void incrementTransportToken();

    //--------------- Data members ---------------------------------

    Rosegarden::SoundDriver *m_driver;
    TransportStatus m_transportStatus;

    // Position pointer
    Rosegarden::RealTime m_songPosition;
    Rosegarden::RealTime m_lastFetchSongPosition;

    Rosegarden::RealTime m_readAhead;
    Rosegarden::RealTime m_audioMix;
    Rosegarden::RealTime m_audioRead;
    Rosegarden::RealTime m_audioWrite;
    int m_smallFileSize;

    /*

    // Not required at the sequencer

    // Two more latencies for audio play and record - when we 
    // use an unsynchronised audio and MIDI system such as
    // ALSA and JACK we need to use these additional values
    // to help time-keeping.
    //
    Rosegarden::RealTime m_audioPlayLatency;
    Rosegarden::RealTime m_audioRecordLatency;

    */


    Rosegarden::RealTime m_loopStart;
    Rosegarden::RealTime m_loopEnd;

    std::vector<Rosegarden::MappedInstrument*> m_instruments;

    // MappedStudio holds all of our session-persistent information -
    // sliders and what have you.  It's also streamable over DCOP
    // so you can reconstruct it at either end of the link for 
    // presentation, storage etc.
    //
    Rosegarden::MappedStudio       *m_studio;

    // Slice revert storage
    //
    Rosegarden::RealTime            m_oldSliceSize;
    QTimer                         *m_sliceTimer;

    // Timer to check for new clients
    //
    QTimer                         *m_newClientTimer;

    // mmap segments
    //
    QString                         m_segmentFilesPath;
    MmappedSegmentsMetaIterator::mmappedsegments    m_mmappedSegments;
    MmappedSegmentsMetaIterator*    m_metaIterator;
    Rosegarden::RealTime            m_lastStartTime;

    Rosegarden::MappedComposition   m_mC;
    ControlBlockMmapper            *m_controlBlockMmapper;
    SequencerMmapper                m_sequencerMapper;

    typedef std::pair<TransportRequest, Rosegarden::RealTime> TransportPair;
    std::deque<TransportPair>       m_transportRequests;
    TransportToken                  m_transportToken;

    bool                            m_isEndOfCompReached;
};
 
#endif // _ROSEGARDEN_SEQUENCER_APP_H_
