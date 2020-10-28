/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ROSEGARDENSEQUENCER_H
#define RG_ROSEGARDENSEQUENCER_H

#include "gui/application/TransportStatus.h"

#include "sound/MappedEventList.h"
#include "sound/MappedStudio.h"
#include "sound/MappedBufMetaIterator.h"

#include "base/MidiDevice.h"

#include <QMutex>
#include <QObject>
#include <QString>

#include <deque>


namespace Rosegarden { 

class MappedInstrument;
class SoundDriver;

/// MIDI and Audio recording and playback
/**
 * RosegardenSequencer is a Singleton (see getInstance() and m_instance).
 * It runs in its own thread separate from the GUI (see SequencerThread).
 *
 * RosegardenSequencer owns a SoundDriver object (m_driver) which wraps the
 * ALSA and JACK functionality.  At this level we deal with communication with
 * the Rosegarden GUI application, the high level marshaling of data,
 * and the main event loop of the sequencer.
 */
class RosegardenSequencer : public QObject
{
    Q_OBJECT

public:
    ~RosegardenSequencer() override;

    /// Singleton
    static RosegardenSequencer *getInstance();

    /// Locking mechanism used throughout.  See the LOCKED #define.
    void lock();
    void unlock();

    /// Close the sequencer.
    void quit();
    
    /// Cleanup before deleting the sequencer
    void cleanup();

    /// Play from a given time with given parameters.
    /**
     *  Based on RealTime timestamps.
     */
    bool play(const RealTime &time);

    /// Record from a given time with given parameters.
    bool record(const RealTime &position, long recordMode);

    /// Punch out from recording to playback
    /**
     * For punch in, see SequenceManager::record().
     */
    bool punchOut();

    /// Set a loop on the sequencer.
    void setLoop(const RealTime &loopStart,
                 const RealTime &loopEnd);

    /// Set the sequencer to a given time.
    void jumpTo(const RealTime &rt);
 
    /// Return the Sound system status (audio/MIDI)
    unsigned getSoundDriverStatus();

    /// Add an audio file to the sequencer
    /**
     * @see removeAudioFile()
     */
    bool addAudioFile(const QString &fileName, int id);
    /// Remove an audio file from the sequencer
    /**
     * @see addAudioFile()
     * @see clearAllAudioFiles()
     */
    bool removeAudioFile(int id);

    /// Removes and closes all audio files
    void clearAllAudioFiles();

    /// Stop the sequencer
    void stop();

    /// Set a MappedInstrument at the Sequencer
    /**
     * Single set function as the MappedInstrument is so lightweight.
     * Any mods on the GUI are sent only through this method.
     */
    void setMappedInstrument(int type, unsigned int id);

    /// Puts a mapped event on the m_asyncOutQueue
    void processMappedEvent(MappedEvent mE);


    // --- DEVICES ---

#if 0  // !DEVPUSH
    /// Return device id following last existing one.
    /**
     * You can treat this as "number of devices" but there might be some
     * holes if devices were deleted, which you will recognise because
     * getMappedDevice(id) will return a device with id NO_DEVICE
     */
    unsigned int getDevices();
    /// Return device by number
    MappedDevice getMappedDevice(unsigned int id);
#endif

    /**
     * Create a device of the given type and direction (corresponding
     * to MidiDevice::DeviceDirection enum) and return its id.
     * The device will have no connection by default.  Direction is
     * currently ignored for non-MIDI devices.
     */
    bool addDevice(Device::DeviceType type,
                   DeviceId id,
                   InstrumentId baseInstrumentId,
                   MidiDevice::DeviceDirection direction);

    /// Remove the device of the given id.
    /**
     * Ignored if driver does not permit changing the number of devices
     */
    void removeDevice(unsigned int id);
    /// Remove all of the devices (of types that can be added or removed).
    /**
     * Ignored if driver does not permit changing the number of devices
     */
    void removeAllDevices();
    /// Rename the given device.
    /**
     * Ignored if the driver does not permit this operation.
     */
    void renameDevice(unsigned int id, QString name);
    /**
     * Return the number of permissible connections for a device of
     * the given type and direction (corresponding to MidiDevice::
     * DeviceDirection enum).  Direction is ignored for non-MIDI devices.
     * Returns zero if devices of this type are non-reconnectable
     */
    unsigned int getConnections(Device::DeviceType type,
                                MidiDevice::DeviceDirection direction);
    /**
     * Return one of the set of permissible connections for a device of
     * the given type and direction (corresponding to
     * MidiDevice::DeviceDirection enum).  Direction is ignored for non-MIDI
     * devices.
     *
     * Returns the empty string for invalid parameters.
     */
    QString getConnection(Device::DeviceType type,
                          MidiDevice::DeviceDirection direction,
                          unsigned int connectionNo);
    /**
     * Return the current connection for the given device, or the
     * empty string if the driver does not permit reconnections or the
     * device is not connected.
     */
    QString getConnection(DeviceId id);
    /// Reconnect a particular device.
    /**
     * Ignored if driver does not permit reconnections or the connection
     * is not one of the permissible set for that device.
     */
    void setConnection(unsigned int deviceId,
                       QString connection);
    /**
     * Reconnect a device to a particular connection or to the closest
     * thing to that connection currently available (using some heuristic).
     * Ignored if driver does not permit reconnections.
     */
    void setPlausibleConnection(unsigned int deviceId,
                                QString idealConnection);
    /**
     * Ensure that at least one playback device is connected to
     * something, if there is at least one very obvious candidate to
     * connect it to
     */
    void connectSomething();

    // --- TIMERS ---

    /**
     * Return the number of different timers we are capable of
     * sychronising against.  This may return 0 if the driver has no
     * ability to change the current timer.
     */
    unsigned int getTimers();
    /// Return the name of a timer from the available set.
    /**
     * @param[in] n is between 0 and the return value from getTimers() - 1
     */
    QString getTimer(unsigned int n);
    /// The name of the timer we are currently synchronising against.
    QString getCurrentTimer();
    /// Set the timer we are currently synchronising against.
    /**
     * Invalid arguments are simply ignored.
     */
    void setCurrentTimer(QString timer);

    RealTime getAudioPlayLatency();
    RealTime getAudioRecordLatency();

    // --- PROPERTIES ---

    /// Set a property on a MappedObject
    void setMappedProperty(int id,
                           const QString &property,
                           float value);

    /// Set many properties on many MappedObjects
    void setMappedProperties(const MappedObjectIdList &ids,
                             const MappedObjectPropertyList &properties,
                             const MappedObjectValueList &values);

    /// Set a string property on a MappedObject
    void setMappedProperty(int id,
                           const QString &property,
                           const QString &value);

    /// Set a MappedObject to a property list.
    /**
     * Return value is error string if any.
     */
    QString setMappedPropertyList(int id,
                                  const QString &property,
                                  const MappedObjectPropertyList &values);

    /// Get a MappedObject ID for an object type
    int getMappedObjectId(int type);

    /// Get a list of properties of a certain type from an object
    std::vector<QString> getPropertyList(int id,
                                         const QString &property);

    /// Get a list of available plugins
    std::vector<QString> getPluginInformation();

    /**
     * Nasty hack: program name/number mappings are one thing that
     * mapped object properties can't cope with
     */
    QString getPluginProgram(int id, int bank, int program);

    /// Nastier hack: return value is bank << 16 + program
    unsigned long getPluginProgram(int id, const QString &name);

    /// Set a plugin port
    /**
     * Cheat - we can't use a call (getPropertyList) during playback
     * so we use this method to set port N on plugin X.
     */
    void setMappedPort(int pluginId,
                       unsigned long portId,
                       float value);

    float getMappedPort(int pluginId,
                        unsigned long portId);

    /// Create a (transient, writeable) MappedObject
    int createMappedObject(int type);

    /// Destroy an object
    bool destroyMappedObject(int id);

    /// Connect two objects
    void connectMappedObjects(int id1, int id2);
    
    /// Disconnect two objects
    void disconnectMappedObjects(int id1, int id2);

    /// Disconnect an object from everything
    void disconnectMappedObject(int id);

    /// Driver sample rate
    unsigned int getSampleRate() const;

    /**
     * Initialise/Reinitialise the studio back down to read only objects
     * and set to defaults.
     */
    void clearStudio();

    /// Debug stuff, to check MappedEventBuffer::iterator
    void dumpFirstSegment();

    void segmentModified(QSharedPointer<MappedEventBuffer>);
    void segmentAdded(QSharedPointer<MappedEventBuffer>);
    void segmentAboutToBeDeleted(QSharedPointer<MappedEventBuffer>);
    /// Close all mapped segments
    void compositionAboutToBeDeleted();
    /**
     * Update mute (etc) statuses while playing. The sequencer handles
     * this automatically (with no need for this call) for MIDI events,
     * but it needs to be prodded when an already-playing audio segment
     * drops in or out.
     */
    void remapTracks();

    /**
     * Allow the GUI to tell the sequence the duration of a quarter
     * note when the TEMPO changes - this is to allow the sequencer
     * to generate MIDI clock (at 24 PPQN).
     */
    void setQuarterNoteLength(RealTime rt);

    enum TransportRequest {
        TransportNoChange,
        TransportStop,
        TransportStart,
        TransportPlay,
        TransportRecord,
        TransportJumpToTime, // time arg required
        TransportStartAtTime, // time arg required
        TransportStopAtTime // time arg required
    };

    /// Called by RosegardenMainWindow::slotHandleInputs().
    /**
     * Get the next transport request from the thread-safe request queue
     * (m_transportRequests).
     *
     * ??? Instead of this, we should send QEvents over to
     *     RosegardenMainWindow.  See RosegardenMainWindow::customEvent()
     *     which is already used for this.
     */
    bool getNextTransportRequest(TransportRequest &request, RealTime &time);

    MappedEventList pullAsynchronousMidiQueue();

    void setStatus(TransportStatus status)
            { m_transportStatus = status; }
    TransportStatus getStatus() { return m_transportStatus; }
   
    /// Process the first chunk of Sequencer events
    /**
     * How does this differ from play() and record()?
     */
    bool startPlaying();

    /// Process all subsequent events
    bool keepPlaying();

    /// Update internal clock and send GUI position pointer movement
    void updateClocks();

    /**
     * Gets incoming MIDI events from AlsaDriver::getMappedEventList()
     * and passes them on to SequencerDataBlock::addRecordedEvents()
     * for storage and display.  Also routes MIDI events for MIDI thru
     * via routeEvents().
     */
    void processRecordedMidi();
    /// Process any audio data that is waiting and send it to be stored.
    void processRecordedAudio();

    /**
     * Called during stopped or playing operation to process any pending
     * incoming MIDI events that aren't being recorded (i.e. for display
     * in Transport or on Mixer).  It also echoes incoming MIDI back out
     * when stopped.
     */
    void processAsynchronousEvents();

    /// Sleep for the given time, approximately.
    /**
     * Called from the main loop in order to lighten CPU load (i.e. the
     * timing quality of the sequencer does not depend on this being
     * accurate).  A good implementation of this call would return right
     * away when an incoming MIDI event needed to be handled.
     */
    void sleep(const RealTime &rt);

    /// Removes events not matching a MidiFilter from a MappedEventsList.
    /**
     * From the menu, Studio > Modify MIDI Filters... allows the user to
     * control this filtering.
     */
    void applyFiltering(MappedEventList *mC,
                        MidiFilter filter,
                        bool filterControlDevice);

    /**
     * This method assigns an Instrument to each MappedEvent belonging to
     * the MappedEventList, and sends the transformed events to the driver
     * to be played.
     *
     * Used by processAsynchronousEvents() and processRecordedMidi().
     */
    void routeEvents(MappedEventList *mC, bool recording);

    /// Are we looping?
    bool isLooping() const { return !(m_loopStart == m_loopEnd); }

    /// Check for new external clients (ALSA sequencer or whatever).
    /**
     * Polled regularly.
     */
    void checkForNewClients();

    /// Initialise the virtual studio at this end of the link.
    void initialiseStudio();


    // --------- Transport Interface --------
    //
    // Whereas the interface (above) is for the GUI to call to
    // make the sequencer follow its wishes, this interface is for
    // external clients to call (via some low-level audio callback)
    // and requires synchronising with the GUI.

    typedef unsigned long TransportToken;

    TransportToken transportChange(TransportRequest);
    TransportToken transportJump(TransportRequest, RealTime);
    bool isTransportSyncComplete(TransportToken token);

    // The value returned here is a constant that is guaranteed never
    // to be returned by any of the transport request methods.
    TransportToken getInvalidTransportToken() const  { return 0; }

    // ---------- End of Transport Interface -----------

private slots:
    /// Connected to InstrumentStaticSignals::controlChange().
    void slotControlChange(Instrument *instrument, int cc);

private:
    /// Singleton.  See getInstance().
    RosegardenSequencer();

    /// get events whilst handling loop
    void fetchEvents(MappedEventList &mappedEventList,
                     const RealTime &start,
                     const RealTime &end,
                     bool firstFetch);

    /// just get a slice of events between markers
    void getSlice(MappedEventList &mappedEventList,
                  const RealTime &start,
                  const RealTime &end,
                  bool firstFetch);

    /// adjust event times according to relative instrument latencies
    void applyLatencyCompensation(MappedEventList &);

    void rationalisePlayingAudio();
    void incrementTransportToken();

    //--------------- Data members ---------------------------------

    SoundDriver *m_driver;
    TransportStatus m_transportStatus;

    /// Position pointer
    RealTime m_songPosition;
    RealTime m_lastFetchSongPosition;

    RealTime m_readAhead;
    // ??? Rumor has it that this is ignored in low latency mode.  Track
    //     this down and decide whether to remove altogether.  We default to
    //     low latency mode now and likely have no need for "high latency"
    //     mode.
    RealTime m_audioMix;
    RealTime m_audioRead;
    RealTime m_audioWrite;
    int m_smallFileSize;

    RealTime m_loopStart;
    RealTime m_loopEnd;

    std::vector<MappedInstrument*> m_instruments;

    /**
     * MappedStudio holds all of our session-persistent information -
     * sliders and what have you.  It's also streamable over DCOP
     * so you can reconstruct it at either end of the link for
     * presentation, storage etc.
     */
    MappedStudio *m_studio;

    // mmap segments
    // 
    MappedBufMetaIterator m_metaIterator;
    RealTime m_lastStartTime;

    /**
     * m_asyncOutQueue is not a MappedEventList: order of receipt
     * matters in ordering, timestamp doesn't
     */
    std::deque<MappedEvent *> m_asyncOutQueue;

    /**
     * m_asyncInQueue is a MappedEventList because its events are
     * properly timestamped and should be ordered thus
     */
    MappedEventList m_asyncInQueue;

    typedef std::pair<TransportRequest, RealTime> TransportPair;
    std::deque<TransportPair> m_transportRequests;
    TransportToken m_transportToken;

    /// UNUSED
    /**
     * Was used to stop playback at the end of the composition, but it must
     * not have worked as it was commented out everywhere it was used.
     */
    bool m_isEndOfCompReached;
    
    QMutex m_mutex;
    QMutex m_transportRequestMutex;
    QMutex m_asyncQueueMutex;

    static RosegardenSequencer *m_instance;
    static QMutex m_instanceMutex;
};

}
 
#endif // RG_ROSEGARDENSEQUENCER_H
