// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef ROSEGARDENGUIDOC_H
#define ROSEGARDENGUIDOC_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

// include files for QT
#include <qobject.h>
#include <qstring.h>
#include <qptrlist.h>
#include <qxml.h>

#include "rosegardendcop.h"
#include "rosegardengui.h"

#include "Composition.h"
#include "MappedComposition.h"
#include "multiviewcommandhistory.h"
#include "AudioFileManager.h"
#include "Studio.h"
#include "Configuration.h"
#include "MappedDevice.h"

// forward declaration of the RosegardenGUI classes
class RosegardenGUIView;
class ViewElementsManager;
class SegmentItem;
class RosegardenProgressDialog;
class EditViewBase;
class KProgress;

namespace Rosegarden
{
    class AudioPluginManager; 
    class SequenceManager;
}

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
                     Rosegarden::AudioPluginManager *audioPluginManager = 0,
		     bool skipAutoload = false,
                     const char *name=0);

private:
    RosegardenGUIDoc(RosegardenGUIDoc *doc);
    RosegardenGUIDoc& operator=(const RosegardenGUIDoc &doc);

public:
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
     */	
    bool saveDocument(const QString &filename, const char *format=0,
		      bool autosave = false);

    /**
     * exports all or part of the studio to a file.  If devices is
     * empty, exports all devices.
     */	
    bool exportStudio(const QString &filename,
		      std::vector<Rosegarden::DeviceId> devices =
		      std::vector<Rosegarden::DeviceId>());

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

    /**
     * returns the global command history
     */
    MultiViewCommandHistory *getCommandHistory() {
	return m_commandHistory;
    }

    /**
     * returns the composition (the principal constituent of the document)
     */
    Rosegarden::Composition&       getComposition()       { return m_composition; }

    /**
     * returns the composition (the principal constituent of the document)
     */
    const Rosegarden::Composition& getComposition() const { return m_composition; }

    /*
     * return the Studio
     */
    Rosegarden::Studio& getStudio() { return m_studio;}

    const Rosegarden::Studio& getStudio() const { return m_studio;}

    /*
     * Copy the Studio from somewhere else
     */
    void copyStudio(const Rosegarden::Studio &);

    /*
     * return the AudioFileManager
     */
    Rosegarden::AudioFileManager& getAudioFileManager()
        { return m_audioFileManager; }

    const Rosegarden::AudioFileManager& getAudioFileManager() const
        { return m_audioFileManager; }

    /*
     * return the Configuration object
     */
    Rosegarden::DocumentConfiguration& getConfiguration() { return m_config; }

    const Rosegarden::DocumentConfiguration& getConfiguration() const 
        { return m_config; }

    /**
     * returns the cut/copy/paste clipboard
     */
    Rosegarden::Clipboard *getClipboard();

    /**
     * Returns whether the sequencer us running
     */
    bool isSequencerRunning();

    /**
     * insert some recorded MIDI events into our recording Segment
     */
    void insertRecordedMidi(const Rosegarden::MappedComposition &mc,
                            TransportStatus status);

    /*
     *  insert a recording SegmentItem for Audio with a given audio level
     */
    void insertRecordedAudio(const Rosegarden::RealTime &time,
                             TransportStatus status);

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
     * Cause the pointerPositionChanged signal to be emitted and any
     * associated internal work in the document to happen
     */
    void setPointerPosition(Rosegarden::timeT);

    /**
     * Cause the playPositionChanged signal to be emitted and any
     * associated internal work in the document to happen
     */
    void setPlayPosition(Rosegarden::timeT);

    /**
     * Cause the loopChanged signal to be emitted and any
     * associated internal work in the document to happen
     */
    void setLoop(Rosegarden::timeT, Rosegarden::timeT);

    /*
    * Sync device information with sequencer
    */
    void syncDevices();

    /*
     * Get a MappedDevice from the sequencer and add the
     * results to our Studio
     */
    void getMappedDevice(Rosegarden::DeviceId id);

    /*
     * Create a new audio file and return the path to it so that
     * the sequencer can use it to write to.
     */
    std::string createNewAudioFile();

    // Audio play and record latencies direct from the sequencer
    //
    Rosegarden::RealTime getAudioPlayLatency();
    Rosegarden::RealTime getAudioRecordLatency();

    // Complete the add of an audio file when a new file has finished
    // being recorded at the sequencer.  This method will ensure that
    // the audio file is added to the AudioFileManager, that
    // a preview is generated and that the sequencer also knows to add
    // the new file to its own hash table.  Flow of control is a bit
    // awkward around new audio files as timing is crucial - the gui can't
    // access the file until lead-out information has been written by the 
    // sequencer.
    //
    void finalizeAudioFile(Rosegarden::AudioFileId id);

    /*
    void setAudioRecordLatency(const Rosegarden::RealTime &latency)
        { m_audioRecordLatency = latency; }
    void setAudioPlayLatency(const Rosegarden::RealTime &latency)
        { m_audioPlayLatency = latency; }
        */

    // Return the AudioPluginManager
    //
    Rosegarden::AudioPluginManager* getPluginManager()
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
    Rosegarden::SequenceManager* getSequenceManager() 
        { return (dynamic_cast<RosegardenGUIApp*>(parent()))
                                         ->getSequenceManager(); }

    /**
     * return the list of the views currently connected to the document
     */
    QList<RosegardenGUIView>& getViewList() { return m_viewList; }

    bool isBeingDestroyed() { return m_beingDestroyed; }

    static const unsigned int MinNbOfTracks; // 64

    // Turn on and off audio monitoring for a particular instrument
    //
    void setAudioMonitoringState(bool value, Rosegarden::InstrumentId id);

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

    void slotSetPointerPosition(Rosegarden::timeT t) { setPointerPosition(t); }
    void slotSetPlayPosition(Rosegarden::timeT t) { setPlayPosition(t); }
    void slotSetLoop(Rosegarden::timeT s, Rosegarden::timeT e) {setLoop(s,e);}

    // Record button has changed - tell the sequencer if it's to
    // an audio track.
    //
    void slotNewRecordButton();

    void slotDocColoursChanged();

signals:
    /**
     * Emitted when document is modified or saved
     */
    void documentModified(bool);

    void pointerPositionChanged(Rosegarden::timeT);
    void playPositionChanged(Rosegarden::timeT);
    void loopChanged(Rosegarden::timeT, Rosegarden::timeT);
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
     * Parse the Rosegarden file \a file
     *
     * \errMsg will contains the error messages
     * if parsing failed.
     *
     * @return false if parsing failed
     * @see RoseXmlHandler
     */
    bool xmlParse(QString& fileContents, QString &errMsg,
                  RosegardenProgressDialog *progress,
		  bool permanent,
                  bool &cancelled);

    /**
     * Write the given string to the given file, compressed.
     * @return false for failure
     */
    bool writeToFile(const QString &fileName, const QString &text);

    /**
     * Read the contents of the given string into the given text stream,
     * uncompressing as you go.
     * @return false for failure
     */
    bool readFromFile(const QString &fileName, QString &text);

    /*
     * Recording might have to insert NOTE ONs and NOTE OFFs
     * (NOTE ONs marked with negative duration) - this rolls
     * them out.
     *
     */
    void convertToSinglePoint(Rosegarden::Segment *segment);

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
     * Save one segment to the given text stream
     */
    void saveSegment(QTextStream&, Rosegarden::Segment*, KProgress* = 0, int totalNbOfEvents = 0);

    //--------------- Data members ---------------------------------

    /**
     * the list of the views currently connected to the document
     */
    QList<RosegardenGUIView> m_viewList;	

    /**
     * the list of the edit views currently editing a part of this document
     */
    QList<EditViewBase> m_editViewList;

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
    Rosegarden::Composition m_composition;

    /**
     * stores AudioFile mappings
     */
    Rosegarden::AudioFileManager m_audioFileManager;

    /**
     * a Segment onto which we can record events
     */
    Rosegarden::Segment *m_recordSegment;
    Rosegarden::timeT m_endOfLastRecordedNote;  // we use this for rest filling

    MultiViewCommandHistory *m_commandHistory;

    /**
     * the Studio
     */
    Rosegarden::Studio m_studio;

    /*
     * A configuration object
     *
     */
    Rosegarden::DocumentConfiguration m_config;

    // AudioPluginManager - sequencer and local plugin management
    //
    Rosegarden::AudioPluginManager *m_pluginManager;

    // Autosave period for this document in seconds
    //
    int m_autoSavePeriod;

    // Set to true when the dtor starts
    bool m_beingDestroyed;
};

#endif // ROSEGARDENGUIDOC_H
