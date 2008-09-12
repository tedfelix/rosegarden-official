/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_ROSEGARDENGUIDOC_H_
#define _RG_ROSEGARDENGUIDOC_H_

#include "base/Composition.h"
#include "base/Configuration.h"
#include "base/Device.h"
#include "base/MidiProgram.h"
#include "base/RealTime.h"
#include "base/Segment.h"
#include "base/Studio.h"
#include "gui/editors/segment/segmentcanvas/AudioPreviewThread.h"
#include <map>
#include "sound/AudioFileManager.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <vector>
#include "base/Event.h"

#include <QProgressBar>
#include <QProgressDialog>

class QWidget;
class QTextStream;
class NoteOnRecSet;
//class QProgressBar;


namespace Rosegarden
{

class SequenceManager;
class RosegardenGUIView;
class ProgressDialog;
class MultiViewCommandHistory;
class MappedComposition;
class Event;
class EditViewBase;
class Clipboard;
class AudioPluginManager;


static const int MERGE_AT_END           = (1 << 0);
static const int MERGE_IN_NEW_TRACKS    = (1 << 1);
static const int MERGE_KEEP_OLD_TIMINGS = (1 << 2);
static const int MERGE_KEEP_NEW_TIMINGS = (1 << 3);


/**
  * RosegardenGUIDoc provides a document object for a document-view model.
  *
  * The RosegardenGUIDoc class provides a document object that can be
  * used in conjunction with the classes RosegardenGUIApp and
  * RosegardenGUIView to create a document-view model for standard KDE
  * applications based on KApplication and KTMainWindow. Thereby, the
  * document object is created by the RosegardenGUIApp instance and
  * contains the document structure with the according methods for
  * manipulation of the document data by RosegardenGUIView
  * objects. Also, RosegardenGUIDoc contains the methods for
  * serialization of the document data from and to files.
  *
  * RosegardenGUIDoc owns the Composition in the document.
  */

class RosegardenGUIDoc : public QObject
{
    Q_OBJECT
public:

    /**
     * Constructor for the fileclass of the application
     */
    RosegardenGUIDoc(QWidget *parent,
                     AudioPluginManager *audioPluginManager = 0,
                     bool skipAutoload = false,
                     const char *name=0);

private:
    RosegardenGUIDoc(RosegardenGUIDoc *doc);
    RosegardenGUIDoc& operator=(const RosegardenGUIDoc &doc);

public:
    static int FILE_FORMAT_VERSION_MAJOR;
    static int FILE_FORMAT_VERSION_MINOR;
    static int FILE_FORMAT_VERSION_POINT;

    /**
     * Destructor for the fileclass of the application
     */
    ~RosegardenGUIDoc();

    /**
     * adds a view to the document which represents the document
     * contents. Usually this is your main view.
     */
    void attachView(RosegardenGUIView *view);

    /**
     * removes a view from the list of currently connected views
     */
    void detachView(RosegardenGUIView *view);

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

protected:
    /**
     * sets the modified flag for the document after a modifying
     * action on the view connected to the document.
     *
     * this is just an accessor, other components should call
     * slotDocumentModified() and clearModifiedStatus() instead of
     * this method, which perform all the related housework.
     * 
     */
    void setModified(bool m=true);

public:
    /**
     * returns if the document is modified or not. Use this to
     * determine if your document needs saving by the user on closing.
     */
    bool isModified() const { return m_modified; };

    /**
     * clears the 'modified' status of the document (sets it back to false).
     * 
     */
    void clearModifiedStatus();

    /**
     * "save modified" - asks the user for saving if the document is
     * modified
     */
    bool saveIfModified();

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
     * the sequencer's device list will be left alone.
     */
    bool openDocument(const QString &filename, bool permanent = true,
                      const char *format=0);

    /**
     * merge another document into this one
     */
    void mergeDocument(RosegardenGUIDoc *doc, int options);

    /**
     * saves the document under filename and format.
     *
     * errMsg will be set to a user-readable error message if save fails
     */ 
    bool saveDocument(const QString &filename, QString& errMsg,
                      bool autosave = false);

    /**
     * exports all or part of the studio to a file.  If devices is
     * empty, exports all devices.
     */ 
    bool exportStudio(const QString &filename,
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
     * sets the filename of the document
     */
    void setTitle(const QString &_t);

    /**
     * returns the title of the document
     */
    const QString &getTitle() const;

    /**
     * Returns true if the file is a regular Rosegarden ".rg" file,
     * false if it's an imported file or a new file (not yet saved)
     */
    bool isRegularDotRGFile();

    void setQuickMarker();
    void jumpToQuickMarker();    
    timeT getQuickMarkerTime() { return m_quickMarkerTime; }

    /**
     * returns the global command history
     */
    MultiViewCommandHistory *getCommandHistory() {
        return m_commandHistory;
    }

    /**
     * returns the composition (the principal constituent of the document)
     */
    Composition&       getComposition()       { return m_composition; }

    /**
     * returns the composition (the principal constituent of the document)
     */
    const Composition& getComposition() const { return m_composition; }

    /*
     * return the Studio
     */
    Studio& getStudio() { return m_studio;}

    const Studio& getStudio() const { return m_studio;}

    /*
     * return the AudioPreviewThread
     */
    AudioPreviewThread& getAudioPreviewThread()
        { return m_audioPreviewThread; }

    const AudioPreviewThread& getAudioPreviewThread() const
        { return m_audioPreviewThread; }

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
     * returns the cut/copy/paste clipboard
     */
    Clipboard *getClipboard();

    /**
     * Returns whether the sequencer us running
     */
    bool isSequencerRunning();

    /**
     * insert some recorded MIDI events into our recording Segment
     */
    void insertRecordedMidi(const MappedComposition &mc);

    /**
     * Update the recording value() -- called regularly from
     * RosegardenGUIApp::slotUpdatePlaybackPosition() while recording
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
     * Register audio samples at the sequencer
     */
    void prepareAudio();

    /**
     * Cause the playPositionChanged signal to be emitted and any
     * associated internal work in the document to happen
     */
    void setPlayPosition(timeT);

    /**
     * Cause the loopChanged signal to be emitted and any
     * associated internal work in the document to happen
     */
    void setLoop(timeT, timeT);

    /**
     * Cause the document to use the given time as the origin
     * when inserting any subsequent recorded data
     */
    void setRecordStartTime(timeT t) { m_recordStartTime = t; }

    /*
    * Sync device information with sequencer
    */
    void syncDevices();

    /*
     * Get a MappedDevice from the sequencer and add the
     * results to our Studio
     */
    void getMappedDevice(DeviceId id);

    void addRecordMIDISegment(TrackId);
    void addRecordAudioSegment(InstrumentId, AudioFileId);

    // Audio play and record latencies direct from the sequencer
    //
    RealTime getAudioPlayLatency();
    RealTime getAudioRecordLatency();
    void updateAudioRecordLatency();

    // Complete the add of an audio file when a new file has finished
    // being recorded at the sequencer.  This method will ensure that
    // the audio file is added to the AudioFileManager, that
    // a preview is generated and that the sequencer also knows to add
    // the new file to its own hash table.  Flow of control is a bit
    // awkward around new audio files as timing is crucial - the gui can't
    // access the file until lead-out information has been written by the 
    // sequencer.
    //
    // Note that the sequencer doesn't know the audio file id (yet),
    // only the instrument it was recorded to.  (It does know the
    // filename, but the instrument id is enough for us.)
    //
    void finalizeAudioFile(InstrumentId instrument);

    // Tell the document that an audio file has been orphaned.  An
    // orphaned audio file is a file that was created by recording in
    // Rosegarden during the current session, but that has been
    // unloaded from the audio file manager.  It's therefore likely
    // that no other application will be using it, and that that user
    // doesn't want to keep it.  We can offer to delete these files
    // permanently when the document is saved.
    //
    void addOrphanedRecordedAudioFile(QString fileName);
    void addOrphanedDerivedAudioFile(QString fileName);

    // Consider whether to orphan the given audio file which is about
    // to be removed from the audio file manager.
    //
    void notifyAudioFileRemoval(AudioFileId id);

    /*
    void setAudioRecordLatency(const RealTime &latency)
        { m_audioRecordLatency = latency; }
    void setAudioPlayLatency(const RealTime &latency)
        { m_audioPlayLatency = latency; }
        */

    // Return the AudioPluginManager
    //
    AudioPluginManager* getPluginManager()
        { return m_pluginManager; }

    // Clear all plugins from sequencer and from gui
    //
    void clearAllPlugins();

    // Initialise the MIDI controllers after we've loaded a file
    //
    void initialiseControllers();

    // Clear the studio at the sequencer
    //
    void clearStudio();

    // Initialise the Studio with a new document's settings
    //
    void initialiseStudio();

    // Get the sequence manager from the app
    //
    SequenceManager* getSequenceManager();

    QStringList getTimers();
    QString getCurrentTimer();
    void setCurrentTimer(QString);

    /**
     * return the list of the views currently connected to the document
     */
    QList<RosegardenGUIView*>& getViewList() { return m_viewList; } //### prepended *

    bool isBeingDestroyed() { return m_beingDestroyed; }

    static const unsigned int MinNbOfTracks; // 64

public slots:
    /**
     * calls repaint() on all views connected to the document object
     * and is called by the view by which the document has been
     * changed.  As this view normally repaints itself, it is excluded
     * from the paintEvent.
     */
    void slotUpdateAllViews(RosegardenGUIView *sender);

    /**
     * set the 'modified' flag of the document to true,
     * clears the 'autosaved' flag, emits the 'documentModified' signal.
     *
     * always call this when changes have occurred on the document.
     */
    void slotDocumentModified();
    void slotDocumentRestored();

    /**
     * saves the document to a suitably-named backup file
     */
    void slotAutoSave();

    void slotSetPointerPosition(timeT);
    void slotSetPlayPosition(timeT t) { setPlayPosition(t); }
    void slotSetLoop(timeT s, timeT e) {setLoop(s,e);}

    void slotDocColoursChanged();

signals:
    /**
     * Emitted when document is modified or saved
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

    void makeTrackVisible(int trackPosition);

    void stoppedAudioRecording();
    void stoppedMIDIRecording();
    void audioFileFinalized(Segment*);

    void playPositionChanged(timeT);
    void loopChanged(timeT, timeT);
    void docColoursChanged();
    void devicesResyncd();

protected:
    /**
     * initializes the document generally
     */
    void newDocument();

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
                  ProgressDialog *progress,
                  unsigned int elementCount,
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
    void saveSegment(QTextStream&, Segment*, QProgressBar*,
                     long totalNbOfEvents, long &count,
                     QString extraAttributes = QString::null);

    bool deleteOrphanedAudioFiles(bool documentWillNotBeSaved);


    /**
     * A struct formed by a Segment pointer and an iterator to the same 
     * Segment, used in NoteOn calculations when recording MIDI.
     */
    struct NoteOnRec {
        Segment *m_segment;
        Segment::iterator m_segmentIterator;
    };

    /**
     * A vector of NoteOnRec elements, necessary in multitrack MIDI 
     * recording for NoteOn calculations
     */
    typedef std::vector<NoteOnRec>  NoteOnRecSet;

    /**
     * Store a single NoteOnRec element in the m_noteOnEvents map
     */
    void storeNoteOnEvent( Segment *s, Segment::iterator it, 
                           int device, int channel );

    /**
     * Replace recorded Note events in one or several segments, returning the
     * resulting NoteOnRecSet
     */
    NoteOnRecSet* replaceRecordedEvent(NoteOnRecSet &rec_vec, Event *fresh);
    
    /**
     * Insert a recorded event in one or several segments
     */
    void insertRecordedEvent(Event *ev, int device, int channel, bool isNoteOn);

    //--------------- Data members ---------------------------------

    /**
     * the list of the views currently connected to the document
     */
	QList<RosegardenGUIView*> m_viewList;		//@@@ shouldn't this be a ptr: QList<RosegardenGUIView*> instead QList<RosegardenGUIView> ? changed !!

    /**
     * the list of the edit views currently editing a part of this document
     */
    QList<EditViewBase*> m_editViewList;	//### added *

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
    AudioPreviewThread m_audioPreviewThread;

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
    typedef std::map<int, NoteOnRecSet>                         PitchMap;
    
    /**
     * a map[Channel] of PitchMap
     */
    typedef std::map<int, PitchMap>                             ChanMap;
    
    /**
     * a map[Port] of ChanMap
     */
    typedef std::map<int, ChanMap>                              NoteOnMap;

    /**
     * During recording, we collect note-ons that haven't yet had a note-off
     * in here
     */
    NoteOnMap m_noteOnEvents;


    MultiViewCommandHistory *m_commandHistory;

    /**
     * the Studio
     */
    Studio m_studio;

    /*
     * A configuration object
     *
     */
    DocumentConfiguration m_config;

    // AudioPluginManager - sequencer and local plugin management
    //
    AudioPluginManager *m_pluginManager;

    RealTime m_audioRecordLatency;

    timeT m_recordStartTime;

    timeT m_quickMarkerTime;

    std::vector<QString> m_orphanedRecordedAudioFiles;
    std::vector<QString> m_orphanedDerivedAudioFiles;

    // Autosave period for this document in seconds
    //
    int m_autoSavePeriod;

    // Set to true when the dtor starts
    bool m_beingDestroyed;
};


}

#endif
