/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ROSEGARDENDOCUMENT_H
#define RG_ROSEGARDENDOCUMENT_H

#include "base/Composition.h"
#include "base/Configuration.h"
#include "base/Device.h"
#include "base/MidiProgram.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/Studio.h"
#include "gui/editors/segment/compositionview/AudioPeaksThread.h"
#include "sound/AudioFileManager.h"
#include "base/Event.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QProgressDialog>
#include <QPointer>
#include <QSharedPointer>

#include <map>
#include <vector>

class QLockFile;
class QWidget;
class QTextStream;
class NoteOnRecSet;

namespace Rosegarden
{


class SequenceManager;
class RosegardenMainViewWidget;
class MappedEventList;
class Event;
class EditViewBase;
class AudioPluginManager;


/// The document object for a document-view model.
/**
  * The RosegardenDocument class provides a document object that can be
  * used in conjunction with the classes RosegardenMainWindow and
  * RosegardenMainViewWidget to create a document-view model
  * based on QApplication and QMainWindow. Thereby, the
  * document object is created by the RosegardenMainWindow instance and
  * contains the document structure with related methods for
  * manipulation of the document data by RosegardenMainViewWidget
  * objects. Also, RosegardenDocument contains the methods for
  * serialization of the document data from and to files.
  *
  * The easiest way to get the current document is via the currentDocument
  * global variable:
  *
  *   RosegardenDocument *doc = RosegardenDocument::currentDocument;
  *
  * This class owns the following key objects:
  *
  *   * The Composition (getComposition())
  *   * The Studio (getStudio())
  *
  */
class ROSEGARDENPRIVATE_EXPORT RosegardenDocument : public QObject
{
    Q_OBJECT

public:

    /**
     * Constructor for the fileclass of the application
     *
     * clearCommandHistory
     * Our old command history under KDE didn't get wiped out when creating a
     * temporary document to use alongside the application's current one, but
     * that is no longer the case since Thorn.  We need to be able to avoid
     * clearing the command history here, or certain commands wipe out the
     * entire undo history needlessly.
     *
     * If the path is not empty the file will be loaded but the document will
     * still be considered "new"
     */
    RosegardenDocument(QObject *parent,
                       QSharedPointer<AudioPluginManager> audioPluginManager,
                       bool skipAutoload = false,
                       bool clearCommandHistory = true,
                       bool enableSound = true,
                       const QString& path = "");

    /// The current document.
    /**
     * RosegardenMainWindow is the primary setter of this.  All throughout
     * the system we need this, so a global makes perfect sense.
     *
     * If you are attaching and detaching from something related to
     * RosegardenDocument, e.g. Composition, beware that the document may
     * have changed when your destructor is called.  In these cases, keep a
     * local copy of the document pointer so that you can detach from the
     * correct document.  See, e.g. TempoAndTimeSignatureEditor which uses
     * EditViewBase::m_doc in this way.
     *
     * If we are crashing because this is null, be sure that when a document
     * is created, this pointer is set.  The rest of the system depends on
     * it.
     *
     * Previously we used to have to #include RosegardenMainWindow and
     * call RosegardenMainWindow::RosegardenDocument::currentDocument.  Now
     * just use RosegardenDocument::currentDocument.
     */
    static RosegardenDocument *currentDocument;

private:
    RosegardenDocument(const RosegardenDocument &doc);
    RosegardenDocument& operator=(const RosegardenDocument &doc);

public:
    static int FILE_FORMAT_VERSION_MAJOR;
    static int FILE_FORMAT_VERSION_MINOR;
    static int FILE_FORMAT_VERSION_POINT;

    /**
     * Destructor for the fileclass of the application
     */
    ~RosegardenDocument() override;

    /**
     * adds a view to the document which represents the document
     * contents. Usually this is your main view.
     */
    void attachView(RosegardenMainViewWidget *view);

    /**
     * removes a view from the list of currently connected views
     */
    // unused void detachView(RosegardenMainViewWidget *view);

    /**
     * adds an Edit View (notation, matrix, event list)
     */
    void attachEditView(EditViewBase*);

    /**
     * removes a view from the list of currently connected edit views
     */
    void detachEditView(EditViewBase*);

    /**
     * delete all Edit Views
     */
    void deleteEditViews();

    /// Set the modified flag but do not notify observers.
    /**
     * This also clears m_autoSaved.
     *
     * Use this rather than slotDocumentModified() when you do not
     * want the entire UI to refresh.  This can be used for very high
     * frequency changes that might cause high CPU usage if the UI
     * were refreshed every time.
     *
     * See slotDocumentModified() and emitDocumentModified().
     */
    void setModified();

    /**
     * returns if the document is modified or not. Use this to
     * determine if your document needs saving by the user on closing.
     */
    bool isModified() const { return m_modified; }

    /**
     * clears the 'modified' status of the document (sets it back to false).
     *
     */
    void clearModifiedStatus();

    /// Emit the documentModified() signal.
    /**
     * Use this in situations where you need a UI refresh, but the document
     * hasn't been modified to the point of requiring a save (e.g. the Track
     * selection has changed).
     *
     * See setModified() and slotDocumentModified().
     */
    void emitDocumentModified()  { emit documentModified(true); }

    /**
     * get the autosave interval in seconds
     */
    unsigned int getAutoSavePeriod() const;

    /**
     * Load the document by filename and format and emit the
     * updateViews() signal.  The "permanent" argument should be true
     * if this document is intended to be loaded to the GUI for real
     * editing work: in this case, any necessary device-synchronisation
     * with the sequencer will be carried out.  If permanent is false,
     * the sequencer's device list will be left alone.  If squelch is
     * true, no progress dialog will be shown.
     */
    bool openDocument(const QString &filename,
                      bool permanent = true,
                      bool squelchProgressDialog = false,
                      bool enableLock = true);

    /**
     * merge another document into this one
     */
    void mergeDocument(RosegardenDocument *srcDoc,
                       bool mergeAtEnd,
                       bool mergeTimesAndTempos);

    /**
     * saves the document under filename and format.
     *
     * errMsg will be set to a user-readable error message if save fails
     */
    bool saveDocument(const QString &filename, QString& errMsg,
                      bool autosave = false);

    /// Save under a new name.
    bool saveAs(const QString &newName, QString &errMsg);

    /**
     * exports all or part of the studio to a file.  If devices is
     * empty, exports all devices.
     */
    bool exportStudio(const QString &filename,
		      QString &errMsg,
                      std::vector<DeviceId> devices =
                      std::vector<DeviceId>());

    /**
     *   sets the path to the file connected with the document
     */
    void setAbsFilePath(const QString &filename);

    /**
     * returns the pathname of the current document file
     */
    const QString &getAbsFilePath() const;

    /**
     * removes the autosave file (e.g. after saving)
     */
    void deleteAutoSaveFile();

    /**
     * sets the filename of the document
     */
    void setTitle(const QString &title);

    /**
     * returns the title of the document
     */
    const QString &getTitle() const;

    /**
     * Returns true if the file is a regular Rosegarden ".rg" file,
     * false if it's an imported file or a new file (not yet saved)
     */
    bool isRegularDotRGFile() const;

    void setQuickMarker();
    void jumpToQuickMarker();
    timeT getQuickMarkerTime() { return m_quickMarkerTime; }

    Composition &getComposition()  { return m_composition; }
    const Composition &getComposition() const  { return m_composition; }

    /*
     * return the Studio
     */
    Studio& getStudio() { return m_studio;}

    const Studio& getStudio() const { return m_studio;}

    /*
     * return the AudioPeaksThread
     */
    AudioPeaksThread& getAudioPeaksThread()
        { return m_audioPeaksThread; }

    const AudioPeaksThread& getAudioPeaksThread() const
        { return m_audioPeaksThread; }

    /*
     * return the AudioFileManager
     */
    AudioFileManager& getAudioFileManager()
        { return m_audioFileManager; }

    const AudioFileManager& getAudioFileManager() const
        { return m_audioFileManager; }

    /*
     * return the Configuration object
     */
    DocumentConfiguration& getConfiguration() { return m_config; }

    const DocumentConfiguration& getConfiguration() const
        { return m_config; }

    /**
     * Returns whether playing sound is enabled at all
     */
    bool isSoundEnabled() const;

    /// Insert some recorded MIDI events into our recording Segment.
    /**
     * These MIDI events come from AlsaDriver::getMappedEventList() in
     * the sequencer thread.
     */
    void insertRecordedMidi(const MappedEventList &mC);

    /**
     * Update the recording value() -- called regularly from
     * RosegardenMainWindow::processRecordedEvents() while recording
     */
    void updateRecordingMIDISegment();

    /**
     * Update the recording value() for audio
     */
    void updateRecordingAudioSegments();

    /**
     * Tidy up the recording SegmentItems and other post record jobs
     */
    void stopRecordingMidi();
    void stopRecordingAudio();

    /**
     * And any post-play jobs
     */
    void stopPlaying();

    /**
     * Register audio samples at the sequencer
     */
    void prepareAudio();

    /**
     * Cause the document to use the given time as the origin
     * when inserting any subsequent recorded data
     */
    void setRecordStartTime(timeT t) { m_recordStartTime = t; }

    /**
     * Cause the document to use the given time as the pointer
     * position before recording
     */
    void setPointerPositionBeforeRecord(timeT t) { m_pointerBeforeRecord = t; }

    /*
     * Get a MappedDevice from the sequencer and add the
     * results to our Studio
     */
/*!DEVPUSH
    void getMappedDevice(DeviceId id);
*/

    void addRecordMIDISegment(TrackId);
    void addRecordAudioSegment(InstrumentId, AudioFileId);

    /**
     * Audio play and record latencies direct from the sequencer
     */

    RealTime getAudioPlayLatency();
    RealTime getAudioRecordLatency();
    void updateAudioRecordLatency();

    /** Complete the add of an audio file when a new file has finished
     * being recorded at the sequencer.  This method will ensure that
     * the audio file is added to the AudioFileManager, that
     * a preview is generated and that the sequencer also knows to add
     * the new file to its own hash table.  Flow of control is a bit
     * awkward around new audio files as timing is crucial - the gui can't
     * access the file until lead-out information has been written by the
     * sequencer.
     *
     * Note that the sequencer doesn't know the audio file id (yet),
     * only the instrument it was recorded to.  (It does know the
     * filename, but the instrument id is enough for us.)
     */

    void finalizeAudioFile(InstrumentId instrument);

    /** Tell the document that an audio file has been orphaned.  An
    * orphaned audio file is a file that was created by recording in
    * Rosegarden during the current session, but that has been
    * unloaded from the audio file manager.  It's therefore likely
    * that no other application will be using it, and that that user
    * doesn't want to keep it.  We can offer to delete these files
    * permanently when the document is saved.
    */
    void addOrphanedRecordedAudioFile(QString fileName);
    void addOrphanedDerivedAudioFile(QString fileName);

    /*
     * Consider whether to orphan the given audio file which is about
     * to be removed from the audio file manager.
     */
    void notifyAudioFileRemoval(AudioFileId id);

    /*
    void setAudioRecordLatency(const RealTime &latency)
        { m_audioRecordLatency = latency; }
    void setAudioPlayLatency(const RealTime &latency)
        { m_audioPlayLatency = latency; }
        */

    /**
     * Return the AudioPluginManager
     */
    QSharedPointer<AudioPluginManager> getPluginManager()
        { return m_pluginManager; }

    /**
     * Return the instrument that plays segment
     */
    Instrument *
    getInstrument(Segment *segment);

    /**
     * Clear all plugins from sequencer and from gui
     */
    void clearAllPlugins();

    /// Send channel setups (BS/PC/CCs) for each Track.
    void sendChannelSetups(bool reset);

    /**
     * Initialise the Studio with a new document's settings
     */
    void initialiseStudio();

    /*
     * Get the sequence manager from the app
     */
    SequenceManager* getSequenceManager();

    /**
     * Set the sequence manager (called by SequenceManager itself)
     */
    void setSequenceManager(SequenceManager *sm);

    /**
     * return the list of the views currently connected to the document
     */
    QList<RosegardenMainViewWidget*>& getViewList() { return m_viewList; }

    bool isBeingDestroyed() { return m_beingDestroyed; }

    static const unsigned int MinNbOfTracks; // 64

    /// Verify that the audio path exists and can be written to.
    void checkAudioPath(Track *track);

    bool deleteOrphanedAudioFiles(bool documentWillNotBeSaved);

    void stealLockFile(RosegardenDocument *other);

    /// Consistent loop button behavior across all windows.
    void loopButton(bool checked);

public slots:
    /**
     * calls repaint() on all views connected to the document object
     * and is called by the view by which the document has been
     * changed.  As this view normally repaints itself, it is excluded
     * from the paintEvent.
     */
    void slotUpdateAllViews(RosegardenMainViewWidget *sender);

    /// Set the modified flag and notify observers via documentModified().
    /**
     * This also clears m_autoSaved and emits documentModified().
     *
     * Call this when modifications have been made to the document and
     * an immediate update to the UI is needed.  Do not call this too
     * frequently as it causes a refresh of the entire UI which is very
     * expensive.  For high-frequency changes, use setModified() and
     * let the UI update on a timer at a reasonable pace.
     *
     * See setModified() and emitDocumentModified().
     */
    void slotDocumentModified();
    void slotDocumentRestored();

    /**
     * saves the document to a suitably-named backup file
     */
    void slotAutoSave();

    void slotSetPointerPosition(timeT);

    void slotDocColoursChanged();

signals:
    /// Emitted when the document is modified.
    /**
     * See slotDocumentModified().
     *
     * ??? These signals should be moved out of RosegardenDocument into
     *     a separate global signal class.  Then the document can come and
     *     go, and the various observers of these signals can stay up and
     *     connected.  RosegardenDocument should not derive from QObject.
     */
    void documentModified(bool);

    /**
     * Emitted during playback, to suggest that views should track along,
     * as well as when pointer is moved via a click on the loop ruler.
     */
    void pointerPositionChanged(timeT);

    /**
     * Emitted during recording, to indicate that some new notes (it's
     * only emitted for notes) have appeared in the recording segment
     * and anything tracking should track.  updatedFrom gives the
     * start of the new region, which is presumed to extend up to the
     * end of the segment.
     */
    void recordMIDISegmentUpdated(Segment *recordSegment,
                                  timeT updatedFrom);

    /**
     * Emitted when a new MIDI recording segment is set
     */
    void newMIDIRecordingSegment(Segment*);

    /**
     * Emitted when a new audio recording segment is set
     */
    void newAudioRecordingSegment(Segment*);

    void stoppedAudioRecording();
    void stoppedMIDIRecording();
    void audioFileFinalized(Segment*);

    void playPositionChanged(timeT);

    /// Emitted whenever the Composition loop fields change.
    void loopChanged();

    /**
     * We probably want to keep this notification as a special case.
     * The reason being that to detect a change to the color list will
     * require comparing a list of 420 strings.  That's a bit too much.
     * I guess we could implement some sort of trickery like a hash
     * or a change count that clients can cache and compare with the
     * current value to detect a change.  Clever.  But is it too
     * clever?  Which is easier to understand?  A special notification
     * or a change count?
     */
    void docColoursChanged();
    void devicesResyncd();

private:
    /**
     * initializes the document generally
     */
    void newDocument(const QString& path = "");

    /**
     * Autoload
     */
    void performAutoload();

    /**
     * Parse the Rosegarden file in \a file
     *
     * \a errMsg will contains the error messages
     * if parsing failed.
     *
     * @return false if parsing failed
     * @see RoseXmlHandler
     */
    bool xmlParse(QString fileContents, QString &errMsg,
                  bool permanent,
                  bool &cancelled);

    /**
     * Set the "auto saved" status of the document
     * Doc. modification sets it to false, autosaving
     * sets it to true
     */
    void setAutoSaved(bool s) { m_autoSaved = s; }

    /**
     * Returns whether the document should be auto-saved
     */
    bool isAutoSaved() const { return m_autoSaved; }

    /**
     * Returns the name of the autosave file
     */
    QString getAutoSaveFileName();

    /**
     * Save document to the given file.  This function does the actual
     * save of the file to the given filename; saveDocument() wraps
     * this, saving to a temporary file and then renaming to the
     * required file, so as not to lose the original if a failure
     * occurs during overwriting.
     */
    bool saveDocumentActual(const QString &filename, QString& errMsg,
                            bool autosave = false);

    /**
     * Save one segment to the given text stream
     */
    void saveSegment(QTextStream&, Segment*,
                     long totalNbOfEvents, long &count,
                     QString extraAttributes = QString());

    /// Identifies a specific event within a specific segment.
    /**
     * A struct formed by a Segment pointer and an iterator into the same
     * Segment, used in NoteOn calculations when recording MIDI.
     */
    struct NoteOnRec {
        Segment *m_segment;
        Segment::iterator m_segmentIterator;
    };

    /// A vector of note-on events that have been recorded.
    /**
     * A vector of NoteOnRec elements, necessary in multitrack MIDI
     * recording for NoteOn calculations
     */
    typedef std::vector<NoteOnRec> NoteOnRecSet;

    /**
     * Store a single NoteOnRec element in the m_noteOnEvents map
     */
    void storeNoteOnEvent( Segment *s, Segment::iterator it,
                           int device, int channel );

    /// Adjust the end time for a list of overlapping note events.
    /**
     * Adjusts the end time for all the note-on events for a
     * device/channel/pitch.
     *
     * Replace recorded Note events in one or several segments, returning the
     * resulting NoteOnRecSet
     */
    NoteOnRecSet* adjustEndTimes(const NoteOnRecSet &rec_vec, timeT endTime);

    /**
     * Insert a recorded event in one or several segments
     */
    void insertRecordedEvent(Event *ev, int device, int channel, bool isNoteOn);

    /**
     * Transpose an entire segment relative to its destination track.  This is
     * used for transposing a source MIDI recording segment on a per-track
     * basis, so that the results all come out with the same sound as the
     * original recording.
     */
    void transposeRecordedSegment(Segment *s);

    // File locking functions to prevent multiple users from editing
    // the same file.

    /// Returns true if the lock was successful.
    bool lock();
    void release();

    static QLockFile *createLock(const QString &absFilePath);
    static QString lockFilename(const QString &absFilePath);

    //--------------- Data members ---------------------------------

    /**
     * the list of the views currently connected to the document
     */
    QList<RosegardenMainViewWidget*> m_viewList;

    /**
     * the list of the edit views currently editing a part of this document
     */
    QList<EditViewBase*> m_editViewList;

    /**
     * the modified flag of the current document
     */
    bool m_modified;

    /**
     * the autosaved status of the current document
     */
    bool m_autoSaved;

    /**
     * the title of the current document
     */
    QString m_title;

    /**
     * absolute file path of the current document
     */
    QString m_absFilePath;

    /// Lock file to prevent multiple users editing at the same time.
    QLockFile *m_lockFile;

    /**
     * the composition this document is wrapping
     */
    Composition m_composition;

    /**
     * stores AudioFile mappings
     */
    AudioFileManager m_audioFileManager;

    /**
     * calculates AudioFile previews
     */
    AudioPeaksThread m_audioPeaksThread;

    typedef std::map<InstrumentId, Segment *> RecordingSegmentMap;

    /**
     * Segments onto which we can record MIDI events
     */
    //Segment *m_recordMIDISegment;
    RecordingSegmentMap m_recordMIDISegments;

    /**
     * Segments for recording audio (per instrument)
     */
    RecordingSegmentMap m_recordAudioSegments;

    /**
     * a map[Pitch] of NoteOnRecSet elements, for NoteOn calculations
     */
    typedef std::map<int /*pitch*/, NoteOnRecSet> PitchMap;

    /**
     * a map[Channel] of PitchMap
     */
    typedef std::map<int /*channel*/, PitchMap> ChanMap;

    /**
     * a map[Port] of ChanMap
     */
    typedef std::map<int /*device*/, ChanMap> NoteOnMap;

    /**
     * During recording, we collect note-ons that haven't yet had a note-off
     * in here
     */
    NoteOnMap m_noteOnEvents;

    /**
     * the Studio
     */
    Studio m_studio;

    /**
     * A configuration object
     */
    DocumentConfiguration m_config;

    SequenceManager *m_seqManager;

    /**
     * AudioPluginManager - sequencer and local plugin management
     */
    QSharedPointer<AudioPluginManager> m_pluginManager;

    RealTime m_audioRecordLatency;

    timeT m_recordStartTime;
    timeT m_pointerBeforeRecord;
    timeT m_quickMarkerTime;

    std::vector<QString> m_orphanedRecordedAudioFiles;
    std::vector<QString> m_orphanedDerivedAudioFiles;

    /**
     *  Autosave period for this document in seconds
     */
    int m_autoSavePeriod;

    // Set to true when the dtor starts
    bool m_beingDestroyed;

    /**
     * Tells this document whether it should clear the command history upon
     * construction and destruction.  Usually true.  Set this to false for
     * temporary documents (like when merging).
     *
     * ??? Since the CommandHistory is so closely coupled with the document,
     *     *it should be a member*.  That way it goes away when the document
     *     goes away.  It is created (and maybe not even used) when a new
     *     document is created.  This would simplify things for parts of the
     *     system that need temporary documents and don't want the command
     *     history cleared.  Right now it's a tangled mess.
     */
    bool m_clearCommandHistory;

    /// Enable/disable control over ALSA and JACK.
    /**
     * Normally, RosegardenDocument directly makes changes to AlsaDriver
     * to synchronize the MIDI and audio hardware to the document that is
     * loaded.  However, there are times when we don't want RosegardenDocument
     * to change any ALSA/JACK devices and connections.  E.g. when we are
     * creating a temporary document for merging.  In those cases, set this
     * to false and ALSA/JACK will not be touched by this RosegardenDocument
     * instance.
     *
     * Some parts of the system refer to this as "permanent".  See
     * openDocument().
     */
    bool m_soundEnabled;

    /// Allow file lock to be released.
    bool m_release;

    QPointer<QProgressDialog> m_progressDialog;
};


}

#endif
