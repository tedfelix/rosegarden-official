// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#ifndef ROSEGARDENGUI_H
#define ROSEGARDENGUI_H
 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qstrlist.h>
#include <qcursor.h>

#include <kapp.h>
#include <kmainwindow.h>
#include <kaccel.h>

#include "dialogs.h"
#include "rosegardendcop.h"
#include "rosegardenguiiface.h"
#include "segmentcanvas.h"

#include "AudioFile.h"
#include "Sound.h"
#include "MappedEvent.h"
#include "MappedCommon.h"

class KURL;
class KRecentFilesAction;
class KToggleAction;
class KProcess;

// forward declaration of the RosegardenGUI classes
class BankEditorDialog;
class RosegardenGUIDoc;
class RosegardenGUIView;
template <class T> class ZoomSlider;

namespace Rosegarden
{
    class AudioManagerDialog;
    class AudioPluginManager;
    class RosegardenTransportDialog;
    class SequenceManager;
    class Clipboard;
}

class RosegardenProgressBar;

/**
  * The base class for RosegardenGUI application windows. It sets up the main
  * window and reads the config file as well as providing a menubar, toolbar
  * and statusbar. An instance of RosegardenGUIView creates your center view, which is connected
  * to the window's Doc object.
  * RosegardenGUIApp reimplements the methods that KTMainWindow provides for main window handling and supports
  * full session management as well as keyboard accelerator configuration by using KAccel.
  * @see KTMainWindow
  * @see KApplication
  * @see KConfig
  * @see KAccel
  *
  * @author Source Framework Automatically Generated by KDevelop, (c) The KDevelop Team.
  * @version KDevelop version 0.4 code generation
  */
class RosegardenGUIApp : public KMainWindow, virtual public RosegardenIface
{
  Q_OBJECT

  friend class RosegardenGUIView;

public:

    /**
     * construtor of RosegardenGUIApp, calls all init functions to
     * create the application.
     * \arg useSequencer : if true, the sequencer is launched
     * @see initMenuBar initToolBar
     */
    RosegardenGUIApp(bool useSequencer = true,
                     QObject *startupStatusMessageReceiver = 0);

    virtual ~RosegardenGUIApp();

    /**
     * returns a pointer to the current document connected to the
     * KTMainWindow instance and is used by * the View class to access
     * the document object's methods
     */ 
    RosegardenGUIDoc *getDocument() const;      

    /**
     * open a file
     */
    virtual void openFile(const QString& filename);

    /**
     * open a URL
     */
    virtual void openURL(const QString& url);

    /**
     * open a URL
     */
    void openURL(const KURL& url);

    /**
     * imports a Rosegarden 2.1 file
     *
     * @param url : a string containing a url (protocol://foo/bar/file.rose)
     */
    virtual void importRG21File(const QString &url);

    /**
     * imports a MIDI file
     *
     * @param url : a string containing a url (protocol://foo/bar/file.mid)
     */
    virtual void importMIDIFile(const QString &url);

    /**
     * merges a MIDI file
     *
     * @param url : a string containing a url (protocol://foo/bar/file.mid)
     */
    virtual void mergeMIDIFile(const QString &url);

    /**
     * merges or imports a MIDI file
     */
    void importMIDIFile(const QString &url, bool merge);

    /**
     * export a MIDI file
     */
    void exportMIDIFile(const QString &url);

    /**
     * export a Csound scorefile
     */
    void exportCsoundFile(const QString &url);

    /**
     * export a Lilypond file
     */
    void exportLilypondFile(const QString &url);

    /**
     * export a MusicXml file
     */
    void exportMusicXmlFile(const QString &url);

    /**
     * The Sequencer calls this method to get a MappedCompositon
     * full of MappedEvents for it to play.
     */
    const Rosegarden::MappedComposition&
            getSequencerSlice(long sliceStartSec, long sliceStartUSec,
                              long sliceEndSec, long sliceEndUSec,
                              long firstFetch);

    /**
     * Get the sequence manager object
     */
    Rosegarden::SequenceManager* getSequenceManager() { return m_seqManager; }

    /**
     * Get a progress bar
     */
    RosegardenProgressBar *getProgressBar() { return m_progressBar; }

    /**
     * The Sequencer sends back a MappedComposition full of
     * any MappedEvents that it's recorded.
     *
     */
    void processRecordedMidi(const Rosegarden::MappedComposition &mC);

    /*
     * Retrieve an audio time update while recording
     *
     */
    void processRecordedAudio(long recordTimeSec,
                              long recordTimeUsec);

    /**
     * Process unexpected MIDI events for the benefit of the GUI
     *
     */
    void processAsynchronousMidi(const Rosegarden::MappedComposition &mC);

    /**
     * Equivalents of the GUI slots, for DCOP use
     */
    virtual void fileNew()    { slotFileNew(); }
    virtual void fileSave()   { slotFileSave(); }
    virtual void fileSaveAs() { slotFileSaveAs(); }
    virtual void fileClose()  { slotFileClose(); }
    virtual void quit()       { slotQuit(); }

    /**
     * Set the song position pointer - we use longs so that
     * this method is directly accesible from the sequencer
     * (longs are required over DCOP)
     */
    virtual void setPointerPosition(long posSec,
                                    long posUSec,
                                    long clearToSend);

    /**
     * If we've called stopping() at the SequenceManager then we'll
     * have set a flag that we must check for using this method.
     */
    void checkForStop();

    /**
     * Start the sequencer auxiliary process
     * (built in the 'sequencer' directory)
     *
     * @see slotSequencerExited()
     */
    bool launchSequencer();

    /**
     * Returns whether we're using a sequencer.
     * false if the '--nosequencer' option was given
     * true otherwise.
     * This doesn't give the state of the sequencer
     * @see #isSequencerRunning
     */
    bool isUsingSequencer() { return m_useSequencer; }

    /**
     * Returns whether there's a sequencer running.
     * The result is dynamically updated depending on the sequencer's
     * status.
     */
    bool isSequencerRunning() { return m_useSequencer && (m_sequencerProcess != 0); }

    /**
     * Returns true if the sequencer wasn't started by us
     */
    bool isSequencerExternal() { return m_useSequencer && (m_sequencerProcess == SequencerExternal); }
    
    /**
     * Set the sequencer status - pass through DCOP as an int
     */
    virtual void notifySequencerStatus(const int &status);

    /*
     * The sequencer calls this method when it's running to
     * allow us to sync data with it.
     *
     */
    virtual void alive();


    /*
     * If the sequencer is skipping slices due to the gui probably
     * not being to service it then it can call this method to let
     * the gui know and tell the user.
     */
    virtual void skippedSlices(unsigned int slices);

    /*
     * Return the clipboard
     */
    Rosegarden::Clipboard* getClipboard() { return m_clipboard; }

protected:

    static const void* SequencerExternal;

    /// Raise the transport along
    virtual void showEvent(QShowEvent*);

    virtual void keyPressEvent(QKeyEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);

    /**
     * read general Options again and initialize all variables like
     * the recent file list
     */
    void readOptions();

    /**
     * create menus and toolbars
     */
    void setupActions();

    /**
     * sets up the zoom toolbar
     */
    void initZoomToolbar();

    /**
     * sets up the statusbar for the main window by initialzing a
     * statuslabel.
     */
    void initStatusBar();

    /**
     * creates the centerwidget of the KTMainWindow instance and sets
     * it as the view
     */
    void initView();

    /**
     * queryClose is called by KTMainWindow on each closeEvent of a
     * window. Against the default implementation (only returns true),
     * this calls saveModified() on the document object to ask if the
     * document shall be saved if Modified; on cancel the closeEvent
     * is rejected.
     *
     * @see KTMainWindow#queryClose
     * @see KTMainWindow#closeEvent
     */
    virtual bool queryClose();

    /**
     * queryExit is called by KTMainWindow when the last window of the
     * application is going to be closed during the closeEvent().
     * Against the default implementation that just returns true, this
     * calls saveOptions() to save the settings of the last window's
     * properties.
     *
     * @see KTMainWindow#queryExit
     * @see KTMainWindow#closeEvent
     */
    virtual bool queryExit();

    /**
     * saves the window properties for each open window during session
     * end to the session config file, including saving the currently
     * opened file by a temporary filename provided by KApplication.
     *
     * @see KTMainWindow#saveProperties
     */
    virtual void saveProperties(KConfig *_cfg);

    /**
     * reads the session config file and restores the application's
     * state including the last opened files and documents by reading
     * the temporary files saved by saveProperties()
     *
     * @see KTMainWindow#readProperties
     */
    virtual void readProperties(KConfig *_cfg);

    /*
     * Send the result of getSequencerSlice (operated by the
     * Sequencer) to the GUI so as to get visual representation
     * of the events/sounds going out
     */
    void showVisuals(Rosegarden::MappedComposition *mC);

    /*
     * place clicktrack events into the global MappedComposition
     *
     */
    void insertMetronomeClicks(Rosegarden::timeT sliceStart,
                               Rosegarden::timeT sliceEnd);

    /**
     * Plug a widget into our common accelerators
     */
    void plugAccelerators(QWidget *widget, QAccel *accel);

    /**
     * Create a new audio file for the sequencer and return the
     * path to it as a QString.
     */
    QString createNewAudioFile();

    /**
     * Ask the user for a file to save to, and check that it's
     * good and that (if it exists) the user agrees to overwrite.
     * Return a null string if the write should not go ahead.
     */
    QString getValidWriteFile(const QString &extension, const QString &label);

    /**
     * Set the current document
     *
     * Do all the needed housework when the current document changes
     * (like closing edit views, emitting documentChanged signal, etc...)
     */
    void setDocument(RosegardenGUIDoc*);

signals:
    void startupStatusMessage(const QString &message);

    /// emitted when the current document changes
    void documentChanged(RosegardenGUIDoc*);

public slots:

    /**
     * open a URL - used for Dn'D
     *
     * @param url : a string containing a url (protocol://foo/bar/file.rg)
     */
    virtual void slotOpenDroppedURL(QString url);

    /**
     * open a new application window by creating a new instance of
     * RosegardenGUIApp
     */
    void slotFileNewWindow();

    /**
     * clears the document in the actual view to reuse it as the new
     * document
     */
    void slotFileNew();

    /**
     * open a file and load it into the document
     */
    void slotFileOpen();

    /**
     * opens a file from the recent files menu
     */
    void slotFileOpenRecent(const KURL&);

    /**
     * save a document
     */
    void slotFileSave();

    /**
     * save a document by a new filename
     */
    void slotFileSaveAs();

    /**
     * asks for saving if the file is modified, then closes the actual
     * file and window
     */
    void slotFileClose();

    /**
     * print the actual file
     */
    void slotFilePrint();

    /**
     * Let the user select a MIDI file for import
     */
    void slotImportMIDI();

    /**
     * Let the user select a MIDI file for merge
     */
    void slotMergeMIDI();

    /**
     * Revert to last loaded file
     */
    void slotRevertToSaved();

    /**
     * Let the user select a Rosegarden 2.1 file for import 
     */
    void slotImportRG21();


    /**
     * Let the user enter a MIDI file to export to
     */
    void slotExportMIDI();

    /**
     * Let the user enter a Csound scorefile to export to
     */
    void slotExportCsound();

    /**
     * Let the user enter a Lilypond scorefile to export to
     */
    void slotExportLilypond();

    /**
     * Let the user enter a MusicXml scorefile to export to
     */
    void slotExportMusicXml();

    /**
     * closes all open windows by calling close() on each memberList
     * item until the list is empty, then quits the application.  If
     * queryClose() returns false because the user canceled the
     * saveModified() dialog, the closing breaks.
     */
    void slotQuit();
    
    /**
     * put the marked text/object into the clipboard and remove * it
     * from the document
     */
    void slotEditCut();

    /**
     * put the marked text/object into the clipboard
     */
    void slotEditCopy();

    /**
     * paste the clipboard into the document
     */
    void slotEditPaste();

    /**
     * select all segments on all tracks
     */
    void slotSelectAll();

    /**
     * delete selected segments, duh
     */
    void slotDeleteSelectedSegments();

    /**
     * Quantize the selected segments (after asking the user how)
     */
    void slotQuantizeSelection();

    /**
     * Rescale the selected segments by a factor requested from
     * the user
     */
    void slotRescaleSelection();

    /**
     * Split the selected segments on silences (or new timesig, etc)
     */
    void slotAutoSplitSelection();

    /**
     * Split the selected segments by pitch
     */
    void slotSplitSelectionByPitch();

    /**
     * Produce a harmony segment from the selected segments
     */
    void slotHarmonizeSelection();

    /**
     * Merge the selected segments
     */
    void slotMergeSegments();

    /**
     * Tempo to Segment length
     */
    void slotTempoToSegmentLength();

    /**
     * open the default editor for each of the currently-selected segments
     */
    void slotEdit();

    /**
     * open an event list view for each of the currently-selected segments
     */
    void slotEditInEventList();

    /**
     * open a matrix view for each of the currently-selected segments
     */
    void slotEditInMatrix();

    /**
     * open a notation view with all currently-selected segments in it
     */
    void slotEditAsNotation();

    /**
     * Edit the tempo - called from a Transport signal
     */
    void slotEditTempo();
    void slotEditTempo(QWidget *parent);

    /**
     * Edit the time signature - called from a Transport signal
     */
    void slotEditTimeSignature();
    void slotEditTimeSignature(QWidget *parent);

    /**
     * Change the length of the composition
     */
    void slotChangeCompositionLength();

    /**
     * open a dialog for document properties
     */
    void slotEditDocumentProperties();

    /**
     * Edit Devices
     */
    void slotEditDevices();

    /**
     * Edit Banks/Programs
     */
    void slotEditBanks();

    /**
     * Remap instruments
     */
    void slotRemapInstruments();

    /**
     * Modify MIDI filters
     */
    void slotModifyMIDIFilters();

    /**
     * Save Studio as Default
     */
    void slotSaveDefaultStudio();

    /**
     * toggles the toolbar
     */
    void slotToggleToolBar();

    /**
     * toggles the transport window
     */
    void slotToggleTransport();

    /**
     * toggles the tracks toolbar
     */
    void slotToggleTracksToolBar();

    /**
     * toggles the editors toolbar
     */
    void slotToggleEditorsToolBar();

    /**
     * toggles the transport toolbar
     */
    void slotToggleTransportToolBar();

    /**
     * toggles the zoom toolbar
     */
    void slotToggleZoomToolBar();

    /**
     * toggles the statusbar
     */
    void slotToggleStatusBar();

    /**
     * changes the statusbar contents for the standard label
     * permanently, used to indicate current actions.
     *
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusMsg(const QString &text);

    /**
     * changes the status message of the whole statusbar for two
     * seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for
     * toolbar icons and menuentries.
     *
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusHelpMsg(const QString &text);

    /**
     * enables/disables the transport window
     */
    void slotEnableTransport(bool);

    /**
     * segment select tool
     */
    void slotPointerSelected();

    /**
     * segment eraser tool is selected
     */
    void slotEraseSelected();
    
    /**
     * segment draw tool is selected
     */
    void slotDrawSelected();
    
    /**
     * segment move tool is selected
     */
    void slotMoveSelected();

    /**
     * segment resize tool is selected
     */
    void slotResizeSelected();

    /*
     * Segment join tool
     *
     */
    void slotJoinSelected();

    /*
     * Segment split tool
     *
     */
    void slotSplitSelected();

    /**
     * Add new tracks
     */
    void slotAddTracks();

    /*
     * Delete Tracks
     */
    void slotDeleteTrack();

    /*
     * Modify track position
     */
    void slotMoveTrackUp();
    void slotMoveTrackDown();

    /**
     * Set the song position pointer
     */
    void slotSetPointerPosition(Rosegarden::RealTime time);

    /**
     * timeT version of the same
     */
    void slotSetPointerPosition(Rosegarden::timeT t);

    /**
     * Set the pointer position and start playing (from LoopRuler)
     */
    void slotSetPlayPosition(Rosegarden::timeT position);

    /**
     * Set a loop
     */
    void slotSetLoop(Rosegarden::timeT lhs, Rosegarden::timeT rhs);


    /**
     * Update the transport with the bar, beat and unit times for
     * a given timeT
     */
    void slotDisplayBarTime(Rosegarden::timeT t);


    /**
     * Transport controls
     */
    void slotPlay();
    void slotStop();
    void slotRewind();
    void slotFastforward();
    void slotRecord();
    void slotToggleRecord();
    void slotRewindToBeginning();
    void slotFastForwardToEnd();
    void slotRefreshTimeDisplay();


    /**
     * Called when the sequencer auxiliary process exits
     */
    void slotSequencerExited(KProcess*);

    /// When the transport closes 
    void slotCloseTransport();

    /**
     * Put the GUI into a given Tool edit mode
     */
    void slotActivateTool(const QString& toolName);

    /**
     * Toggles either the play or record metronome according
     * to Transport status
     */
    void slotToggleMetronome();

    /*
     * Toggle the solo mode
     */
    void slotToggleSolo();


    /**
     * Set and unset the loop from the transport loop button with
     * these slots.
     */
    void slotSetLoop();
    void slotUnsetLoop();

    /**
     * Toggle the track labels on the TrackEditor
     */
    void slotToggleTrackLabels();

    /**
     * Toggle the segment parameters box on the TrackEditor
     */
    void slotToggleSegmentParameters();

    /**
     * Toggle the instrument parameters box on the TrackEditor
     */
    void slotToggleInstrumentParameters();

    /**
     * Toggle the rulers on the TrackEditor
     * (aka bar buttons)
     */
    void slotToggleRulers();

    /**
     * Toggle the tempo ruler on the TrackEditor
     */
    void slotToggleTempoRuler();

    /**
     * Toggle the chord-name ruler on the TrackEditor
     */
    void slotToggleChordNameRuler();

    /**
     * Toggle the segment canvas previews
     */
    void slotTogglePreviews();

    /**
     * Toggle everything except the Transport Toolbar simultaneously
     */
    void slotToggleAll();

    /**
     * Toggle tips at startup
     */
    void slotToggleShowTipsOnStartup();

    /**
     * Display tip-of-day dialog on demand
     */
    void slotShowTips();

    /*
     * Select Track up or down
     */
    void slotTrackUp();
    void slotTrackDown();

    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */         
    void slotSaveOptions();

    /**
     * Show the configure dialog
     */
    void slotConfigure();

    /**
     * Show the key mappings
     *
     */
    void slotEditKeys();

    /**
     * Edit toolbars
     */
    void slotEditToolbars();

    /**
     * Update the toolbars after edition
     */
    void slotUpdateToolbars();

    /**
     * Zoom slider moved
     */
    void slotChangeZoom(int index);

    /**
     * Modify tempo
     */
    void slotChangeTempo(Rosegarden::timeT time,
                         double value,      
                         TempoDialog::TempoDialogAction action);

    /**
     * Document modified
     */
    void slotDocumentModified(bool modified = true);


    /**
     * This slot is here to be connected to RosegardenGUIView's
     * stateChange signal. We use a bool for the 2nd arg rather than a
     * KXMLGUIClient::ReverseStateChange to spare the include of
     * kxmlguiclient.h just for one typedef.
     *
     * Hopefully we'll be able to get rid of this eventually,
     * I should slip this in KMainWindow for KDE 4.
     */
    void slotStateChanged(const QString&, bool noReverse);

    /**
     * A command has happened; check the clipboard in case we
     * need to change state
     */
    void slotTestClipboard();

    /**
     * View the audio file manager - and some associated actions
     */
    void slotAudioManager();

    void slotAddAudioFile(Rosegarden::AudioFileId);
    void slotDeleteAudioFile(Rosegarden::AudioFileId);
    void slotPlayAudioFile(Rosegarden::AudioFileId,
                           const Rosegarden::RealTime &,
                           const Rosegarden::RealTime &);
    void slotCancelAudioPlayingFile(Rosegarden::AudioFileId);
    void slotDeleteAllAudioFiles();

    void slotSelectSegments(Rosegarden::SegmentSelection&);
    void slotDeleteSegments(Rosegarden::SegmentSelection&);
    void slotSegmentsSelected(const Rosegarden::SegmentSelection &);

    void slotRepeatingSegments();
    void slotRelabelSegments();

    // Panic button pressed
    //
    void slotPanic();

    // Auto-save
    //
    void slotAutoSave();

    // Auto-save update interval changes
    //
    void slotUpdateAutoSaveInterval(unsigned int interval);

    // BankEditor closed
    //
    void slotBankEditorClosed();

private:

    //--------------- Data members ---------------------------------

    /**
     * the configuration object of the application
     */
    KConfig* m_config;

    bool m_actionsSetup;

    KRecentFilesAction* m_fileRecent;

    /**
     * view is the main widget which represents your working area. The
     * View class should handle all events of the view widget.  It is
     * kept empty so you can create your view according to your
     * application's needs by changing the view class.
     */
    RosegardenGUIView* m_view;

    /**
     * doc represents your actual document and is created only
     * once. It keeps information such as filename and does the
     * serialization of your files.
     */
    RosegardenGUIDoc* m_doc;

    /**
     * KAction pointers to enable/disable actions
     */
    KRecentFilesAction* m_fileOpenRecent;

    KToggleAction* m_viewToolBar;
    KToggleAction* m_viewTracksToolBar;
    KToggleAction* m_viewEditorsToolBar;
    KToggleAction* m_viewZoomToolBar;
    KToggleAction* m_viewStatusBar;
    KToggleAction* m_viewTransport;
    KToggleAction* m_viewTransportToolBar;
    KToggleAction* m_viewTrackLabels;
    KToggleAction* m_viewSegmentParameters;
    KToggleAction* m_viewInstrumentParameters;
    KToggleAction* m_viewRulers;
    KToggleAction* m_viewTempoRuler;
    KToggleAction* m_viewChordNameRuler;
    KToggleAction* m_viewPreviews;
    KAction* m_viewAll;
    KToggleAction* m_viewTipsOnStartup;
    KAction* m_viewTipsNow;

    KAction *m_playTransport;
    KAction *m_stopTransport;
    KAction *m_rewindTransport;
    KAction *m_ffwdTransport; 
    KAction *m_recordTransport;
    KAction *m_rewindEndTransport;
    KAction *m_ffwdEndTransport;

    KProcess* m_sequencerProcess;

    ZoomSlider<double> *m_zoomSlider;
    QLabel             *m_zoomLabel;

    RosegardenProgressBar *m_progressBar;

    // SequenceManager
    //
    Rosegarden::SequenceManager *m_seqManager;

    // Transport dialog pointer
    //
    Rosegarden::RosegardenTransportDialog *m_transport;

    // Dialogs which depend on the document

    // Audio file manager
    //
    Rosegarden::AudioManagerDialog *m_audioManagerDialog;

    bool m_originatingJump;

    // Use these in conjucntion with the loop button to
    // remember where a loop was if we've ever set one.
    Rosegarden::timeT m_storedLoopStart;
    Rosegarden::timeT m_storedLoopEnd;

    bool m_useSequencer;

    Rosegarden::AudioPluginManager *m_pluginManager;

    QTimer* m_autoSaveTimer;

    Rosegarden::Clipboard *m_clipboard;

};

/**
 * Temporarily change the global cursor to waitCursor
 */
class SetWaitCursor
{
public:
//     SetWaitCursor() { QApplication::setOverrideCursor(QCursor(Qt::waitCursor)); }
//     ~SetWaitCursor() { QApplication::restoreOverrideCursor(); }

    SetWaitCursor() : m_currentCursor(kapp->mainWidget()->cursor())
    { kapp->mainWidget()->setCursor(QCursor(Qt::waitCursor)); }
    ~SetWaitCursor() { kapp->mainWidget()->setCursor(m_currentCursor); }
protected:
    QCursor m_currentCursor;
};
 
#endif // ROSEGARDENGUI_H
