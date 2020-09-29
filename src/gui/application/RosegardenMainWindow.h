/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ROSEGARDENMAINWINDOW_H
#define RG_ROSEGARDENMAINWINDOW_H

#include "gui/application/RosegardenMainViewWidget.h"
#include "base/MidiProgram.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/widgets/ZoomSlider.h"
#include "gui/general/RecentFiles.h"
#include "base/Event.h"
#include "base/Selection.h"
#include "base/Typematic.h"
#include "sound/AudioFile.h"
#include "sound/ExternalController.h"
#include "sound/Midi.h"
#include "gui/general/ActionFileClient.h"

#include <QString>
#include <QVector>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QToolBar>
#include <QPointer>
#include <QSharedPointer>

#include <map>
#include <set>

#include <rosegardenprivate_export.h>

class QWidget;
class QTimer;
class QTextCodec;
class QShowEvent;
class QObject;
class QLabel;
class QShortcut;
class QTemporaryFile;
class QProcess;
class QAction;


namespace Rosegarden
{

class TriggerSegmentManager;
class TransportDialog;
class TrackParameterBox;
class TempoView;
class SynthPluginManagerDialog;
class StartupTester;
class SequenceManager;
class SegmentParameterBox;
class RosegardenParameterArea;
class RosegardenDocument;
class RealTime;
class ProgressBar;
class PlayListDialog;
class MidiMixerWindow;
class MarkerEditor;
class MappedEventList;
class LircCommander;
class LircClient;
class InstrumentParameterBox;
class DeviceManagerDialog;
class ControlEditorDialog;
class Composition;
class Clipboard;
class BankEditorDialog;
class AudioPluginOSCGUIManager;
class AudioPluginManager;
class AudioPluginDialog;
class AudioMixerWindow2;
class AudioManagerDialog;
class EditTempoController;
class SequencerThread;
class TranzportClient;
class WarningWidget;
class DocumentConfigureDialog;
class ConfigureDialog;

/// The main Rosegarden application window.
/**
  * This class sets up the main window and reads the config file as well as
  * providing a menubar, toolbar and statusbar.  The main widget
  * is a RosegardenMainViewWidget, connected to the RosegardenDocument.
  *
  * An instance of this class is created by main() in main.cpp.  That
  * instance can be accessed from anywhere via RosegardenMainWindow::self().
  *
  * This class owns many of the key objects in the system, including:
  *
  *   * getDocument() returns the RosegardenDocument instance.
  *   * m_sequencerThread is the SequencerThread.
  *   * m_segmentParameterBox is the "Segment Parameters" box.
  *   * m_trackParameterBox is the "Track Parameters" box.
  *   * m_instrumentParameterBox is the "Instrument Parameters" box.
  *   * getView() returns the RosegardenMainViewWidget instance which
  *     contains the TrackEditor.
  *   * getTransport() returns the TransportDialog instance.
  */
class ROSEGARDENPRIVATE_EXPORT RosegardenMainWindow : public QMainWindow, public ActionFileClient
{
    Q_OBJECT
    
    friend class RosegardenMainViewWidget;

public:

    /**
     * constructor of RosegardenMainWindow, calls all init functions to
     * create the application.
     * \arg useSequencer : if true, the sequencer is launched
     * @see initMenuBar initToolBar
     */
    RosegardenMainWindow(bool enableSound = true,
                         QObject *startupStatusMessageReceiver = nullptr);

    ~RosegardenMainWindow() override;

    /// Global access to the single instance of this class.
    static RosegardenMainWindow *self() { return m_myself; }
    
    /// Return current Main Toolbar
//    QToolBar* toolBar( const char* name="\0" );
    
    /**
     * returns a pointer to the current document connected to the
     * QMainWindow instance and is used by * the View class to access
     * the document object's methods
     */ 
    RosegardenDocument *getDocument() const;      

    RosegardenMainViewWidget* getView() { return m_view; }

    TransportDialog* getTransport();

    enum ImportType {
        ImportRG4,
        ImportMIDI,
        ImportRG21,
        //ImportHydrogen,
        ImportMusicXML,
        ImportCheckType,
        ImportRGD
    };

    /**
     * open a Rosegarden file
     */
    virtual void openFile(QString filePath) { openFile(filePath, ImportCheckType); }

    /**
     * open a file, explicitly specifying its type
     */
    void openFile(QString filePath, ImportType type);

    /**
     * decode and open a project file
     */
    void importProject(QString filePath);

    /**
     * open a URL
     */
    virtual void openURL(QString url);

    /**
     * merge a file with the existing document
     */ 
    virtual void mergeFile(QString filePath) { mergeFile(filePath, ImportCheckType); }
    
    /**
     * merge a file, explicitly specifying its type
     */
    void mergeFile(QString filePath, ImportType type);

    /**
     * open a URL
     */
    void openURL(const QUrl &url);

    /**
     * export a MIDI file
     */
    void exportMIDIFile(QString url);

    /**
     * export a Csound scorefile
     */
    void exportCsoundFile(QString url);

    /**
     * export a Mup file
     */
    void exportMupFile(QString url);

    /**
     * export a LilyPond file
     */
    bool exportLilyPondFile(QString url, bool forPreview = false);

    /**
     * export a MusicXml file
     */
    void exportMusicXmlFile(QString url);

    /**
     * Get the sequence manager object
     */
    SequenceManager* getSequenceManager() { return m_seqManager; }

    /**
     * Get a value() bar
     */
    ProgressBar *getCPUBar() { return m_cpuBar; }

    /**
     * Get a device manager object
     */
    QPointer<DeviceManagerDialog> getDeviceManager()  { return m_deviceManager; }

    /**
     * Create some new audio files for the sequencer and return the
     * paths for them as QStrings.
     */
    QVector<QString> createRecordAudioFiles(const QVector<InstrumentId> &);

    QVector<InstrumentId> getArmedInstruments();
 
    /**
     * Start the sequencer auxiliary process
     * (built in the 'sequencer' directory)
     *
     * @see slotSequencerExited()
     */
    bool launchSequencer();

#ifdef HAVE_LIBJACK
    /*
     * Launch and control JACK if required to by configuration 
     */
    bool launchJack();

#endif // HAVE_LIBJACK


    /**
     * Returns whether sound is enabled.
     * false if the '--nosound' option was given
     * true otherwise.
     */
    bool isUsingSequencer();

    /**
     * Returns whether there's a sequencer running.
     * The result is dynamically updated depending on the sequencer's
     * status.
     */
    bool isSequencerRunning();

    /*
     * The sequencer calls this method when it's running to
     * allow us to sync data with it.
     *
     */
    virtual void alive();
    
    /*
     * Tell the application whether this is the first time this
     * version of RG has been run
     */
    void setIsFirstRun(bool first) { m_firstRun = first; }

    /*
     * Wait in a sub-event-loop until all modal dialogs from the main
     * window have been cleared
     */
    void awaitDialogClearance();

    /*
     * Return the clipboard
     */
    Clipboard* getClipboard() { return m_clipboard; }

    /**
     * Return the plugin native GUI manager, if we have one
     */
    AudioPluginOSCGUIManager *getPluginGUIManager() { return m_pluginGUIManager; }

    /**
     * Plug a widget into our common shortcuts
     */
    void plugShortcuts(QWidget *widget, QShortcut *shortcut);

    /** Query the AudioFileManager to see if the audio path exists, is readable,
     * writable, etc., and offer to dump the user in the document properties
     * editor if some problem is found.  This is older code that simply tests
     * the audio path for validity, and does not create anything if the audio
     * path does not exist.
     */
    bool testAudioPath(QString op); // and open the dialog to set it if unset

    bool haveAudioImporter() const { return m_haveAudioImporter; }

    void uiUpdateKludge();

    void openWindow(ExternalController::Window window);

protected:

    /// Handle activation change.
    /**
     * We do this to make sure external controller is connected to the
     * current window.  Otherwise if the user clicks on the main window,
     * external controller events might still be controlling one of the
     * other windows.
     */
    void changeEvent(QEvent *event) override;

    /** Qt generates a QCloseEvent when the user clicks the close button on the
     * title bar.  We also get a close event when slotQuit() calls close().
     *
     * Control passes here where we call queryClose() to ask if the user wants
     * to save a modified document.  If queryClose() returns true, we accept the
     * close event.  Otherwise we ignore it, the closing breaks, and we keep
     * running.
     */
    void closeEvent(QCloseEvent *event) override;

    /// Handle Rosegarden-specific events.  Usually from the sequencer thread.
    /**
     * See SequencerDataBlock and slotHandleInputs() which also support
     * communication between the threads.
     */
    void customEvent(QEvent *event) override;

    RosegardenDocument *newDocument(bool skipAutoload = false);

    /**** File handling code that we don't want the outside world to use ****/
    /**/
    /**/

    /**
     * Create document from a file
     */
    RosegardenDocument* createDocument(
            QString filePath, ImportType type = ImportRG4, bool lock = true);
    
    /**
     * Create a document from RG file
     */
    RosegardenDocument* createDocumentFromRGFile(
            const QString &filePath, bool lock = true);

    /**
     * Create document from MIDI file
     */
    RosegardenDocument* createDocumentFromMIDIFile(QString filePath);

    /**
     * Create document from RG21 file
     */
    RosegardenDocument* createDocumentFromRG21File(QString filePath);

    /**
     * Create document from Hydrogen drum machine file
     */
//    RosegardenDocument* createDocumentFromHydrogenFile(QString filePath);

    /**
     * Create document from MusicXML file
     */
    RosegardenDocument* createDocumentFromMusicXMLFile(QString filePath);

    /**/
    /**/
    /***********************************************************************/

    static const void* SequencerExternal;

    /// Raise the transport along
    void showEvent(QShowEvent*) override;

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
     * creates the centerwidget of the QMainWindow instance and sets
     * it as the view
     */
    void initView();

    /**
     * queryClose is called by closeEvent() to ask whether it is OK to close.
     * This is part of a legacy KDE mechanism, but has been left in place for
     * convenience
     */
    bool queryClose();


 //!!! I left the following code here, but this is related to KDE session
 // management, and I don't think we can do anything reasonable with any of this
 // now.
 //
 /////////////////////////////////////////////////////////////////////
    /**
     * saves the window properties for each open window during session
     * end to the session config file, including saving the currently
     * opened file by a temporary filename provided by KApplication.
     *
     * @see KTMainWindow#saveProperties
     */
    virtual void saveGlobalProperties();

    /**
     * reads the session config file and restores the application's
     * state including the last opened files and documents by reading
     * the temporary files saved by saveProperties()
     *
     * @see KTMainWindow#readProperties
     */
    //virtual void readGlobalProperties();
///////////////////////////////////////////////////////////////////////

    QString getAudioFilePath();

    /**
     * Show a sequencer error to the user.  This is for errors from
     * the framework code; the playback code uses mapped compositions
     * to send these things back asynchronously.
     */
    void showError(QString error);

    /*
     * Return AudioManagerDialog
     */
    AudioManagerDialog* getAudioManagerDialog() { return m_audioManagerDialog; }

    /**
     * Ask the user for a file to save to, and check that it's
     * good and that (if it exists) the user agrees to overwrite.
     * Return a null string if the write should not go ahead.
     */
    QString getValidWriteFileName(QString extension, QString label);

    /**
     * Find any non-ASCII strings in a composition that has been
     * generated by MIDI import or any other procedure that produces
     * events with unknown text encoding, and ask the user what
     * encoding to translate them from.  This assumes all text strings
     * in the composition are of the same encoding, and that it is not
     * (known to be) utf8 (in which case no transcoding would be
     * necessary).
     */
    void fixTextEncodings(Composition *);
    QTextCodec *guessTextCodec(std::string);

    /**
     * Set the current document
     *
     * Do all the needed housework when the current document changes
     * (like closing edit views, emitting documentLoaded signal, etc...)
     */
    void setDocument(RosegardenDocument*);

    /**
     * Jog a selection of segments by an amount
     */
    void jogSelection(timeT amount);

    void createAndSetupTransport();

    /**
     * Open a file dialog pointing to directory, if target is empty, this opens
     * a generic file open dialog at the last location the user used
     */
    void openFileDialogAt(QString target);

    /**
     * Returns a suitable location for storing user data, typically
     * ~/.local/share/ 
     */
    QString getDataLocation();

    /**
     * Override (non-virtual) ActionFileClient's version to allow for more
     * complex enable/disable behavior.
     */
    void enterActionState(QString stateName);
    /**
     * Override (non-virtual) ActionFileClient's version to allow for more
     * complex enable/disable behavior.
     */
    void leaveActionState(QString stateName);

signals:
    void startupStatusMessage(QString message);

    /// emitted just before the document is changed
    void documentAboutToChange();

    /// Emitted when a new document is loaded.
    void documentLoaded(RosegardenDocument *);

    /// emitted when the set of selected segments changes (relayed from RosegardenMainViewWidget)
    void segmentsSelected(const SegmentSelection &);

    /// emitted when the composition state (selected track, solo, etc...) changes
    void compositionStateUpdate();

    /// emitted when a plugin dialog selects a plugin
    void pluginSelected(InstrumentId, int, int);

    /// emitted when a plugin dialog (un)bypasses a plugin
    void pluginBypassed(InstrumentId, int, bool);

public slots:

    /** Update the title bar to prepend a * to the document title when the
     * document is modified. (I thought we already did this ages ago, but
     * apparenty not.)
     */
    void slotUpdateTitle(bool m = false);

    /**
     * open a URL - used for Dn'D
     *
     * @param url : a string containing a url (protocol://foo/bar/file.rg)
     */
    virtual void slotOpenDroppedURL(QString url);

    /**
     * Open the document properties dialog on the Audio page
     */
    virtual void slotOpenAudioPathSettings();

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
     * open a file dialog on the examples directory
     */
    void slotFileOpenExample();

    /**
     * open a file dialog on the templates directory
     */
    void slotFileOpenTemplate();

    /**
     * opens a file from the recent files menu (according to action name)
     */
    void slotFileOpenRecent();

    /**
     * save a document
     */
    void slotFileSave();

    /**
     * save a document by a new filename; if asTemplate is true, the file will
     * be saved read-only, to make it harder to overwrite by accident in the
     * future
     */
    bool slotFileSaveAs(bool asTemplate = false);
    void slotFileSaveAsTemplate() { slotFileSaveAs(true); }

    /**
     * asks for saving if the file is modified, then closes the actual
     * file and window
     */
    void slotFileClose();

    /**
     * Let the user select a Rosegarden Project file for import
     */
    void slotImportProject();

    /**
     * Let the user select a MIDI file for import
     */
    void slotImportMIDI();

    /**
     * Revert to last loaded file
     */
    void slotRevertToSaved();

    /**
     * Let the user select a Rosegarden 2.1 file for import 
     */
    void slotImportRG21();

    /**
     * Select a Hydrogen drum machine file for import
     */
//    void slotImportHydrogen();

    /**
     * Let the user select a MusicXML file for import
     */
    void slotImportMusicXML();

    /**
     * Let the user select a MIDI file for merge
     */
    void slotMerge();

    /**
     * Let the user select a MIDI file for merge
     */
    void slotMergeMIDI();

    /**
     * Let the user select a MIDI file for merge
     */
    void slotMergeRG21();

    /**
     * Select a Hydrogen drum machine file for merge
     */
//    void slotMergeHydrogen();

    /**
     * Let the user select a MusicXML file for merge
     */
    void slotMergeMusicXML();

    /**
     * Let the user export a Rosegarden Project file
     */
    void slotExportProject();

    /**
     * Let the user enter a MIDI file to export to
     */
    void slotExportMIDI();

    /**
     * Let the user enter a Csound scorefile to export to
     */
    void slotExportCsound();

    /**
     * Let the user enter a Mup file to export to
     */
    void slotExportMup();

    /**
     * Let the user enter a LilyPond file to export to
     */
    void slotExportLilyPond();

    /**
     * Export to a temporary file and process
     */
    void slotPrintLilyPond();
    void slotPreviewLilyPond();

    /**
     * Let the user enter a MusicXml file to export to
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
     * paste the clipboard into the document, as linked segments
     */
    void slotEditPasteAsLinks();
    
    /**
     * Cut a time range (sections of segments, tempo, and time
     * signature events within that range).
     */
    void slotCutRange();

    /**
     * Copy a time range.
     */
    void slotCopyRange();

    /**
     * Paste the clipboard at the current pointer position, moving all
     * subsequent material along to make space.
     */
    void slotPasteRange();
    
    /**
     * Delete a time range.
     */
    void slotDeleteRange();
    
    /**
     * Insert a time range (asking the user for a duration).
     */
    void slotInsertRange();

    /**
     * Paste just tempi and time signatures from the clipboard at the
     * current pointer position.
     */
    void slotPasteConductorData();

    /**
     * Clear a time range of tempos
     */
    void slotEraseRangeTempos();
    
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
     * Quantize the selected segments by repeating the last iterative quantize
     */
    void slotRepeatQuantizeSelection();

    /**
     * Calculate timing/tempo info based on selected segment
     */
    void slotGrooveQuantize();

    /**
     * Fit existing notes to beats based on selected segment
     */
    void slotFitToBeats();

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
     * Jog a selection left or right by an amount
     */
    void slotJogRight();
    void slotJogLeft();

    /**
     * Split the selected segments by pitch
     */
    void slotSplitSelectionByPitch();

    /**
     * Split the selected segments by recorded source
     */
    void slotSplitSelectionByRecordedSrc();

    /**
     * Split the selected segments at some time
     */
    void slotSplitSelectionAtTime();

    /**
     * Split the selected segments by drum, ie. each discrete pitch goes into a
     * separate segment of its own
     */
    void slotSplitSelectionByDrum();

    /**
     * Produce a harmony segment from the selected segments
     */
    void slotHarmonizeSelection();

    /**
     * Expand the composition to the left by one bar, and move the selected
     * segment(s) into this space by the amount chosen with a dialog
     */
    void slotCreateAnacrusis();

    /**
     * Set the start times of the selected segments
     */
    void slotSetSegmentStartTimes();

    /**
     * Set the durations of the selected segments
     */
    void slotSetSegmentDurations();

    /**
     * Merge the selected segments
     */
    void slotJoinSegments();

    /**
     * Expand block-chord segments by figuration
     */
    void slotExpandFiguration();

    /**
     * Update existing figurations
     */
    void slotUpdateFigurations();
    
    /**
     * Tempo to Segment length
     */
    void slotTempoToSegmentLength();
    void slotTempoToSegmentLength(QWidget* parent);

    /**
     * toggle segment labels
     */
    void slotToggleSegmentLabels();
    
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
     * open a percussion matrix view for each of the currently-selected segments
     */
    void slotEditInPercussionMatrix();

    /**
     * open a pitch tracker view for the currently-selected segments
     *    (NB: only accepts 1 segment)
     */
    void slotEditInPitchTracker();

    /**
     * open a notation view with all currently-selected segments in it
     */
    void slotEditAsNotation();

    /**
     * open a tempo/timesig edit view
     */
    void slotEditTempos();
    void slotEditTempos(timeT openAtTime);

    /**
     * Edit the tempo - called from a Transport signal
     */
    void slotEditTempo();
    void slotEditTempo(timeT atTime);
    void slotEditTempo(QWidget *parent);
    void slotEditTempo(QWidget *parent, timeT atTime);

    /**
     * Edit the time signature - called from a Transport signal
     */
    void slotEditTimeSignature();
    void slotEditTimeSignature(timeT atTime);
    void slotEditTimeSignature(QWidget *parent);
    void slotEditTimeSignature(QWidget *parent, timeT atTime);

    /**
     * Edit the playback pointer position - called from a Transport signal
     */
    void slotEditTransportTime();
    void slotEditTransportTime(QWidget *parent);

    /**
     * Change the length of the composition
     */
    void slotChangeCompositionLength();

    /**
     * open a dialog for document properties
     */
    void slotEditDocumentProperties();
    
    /**
     * Reset m_configDlg when the configuration dialog is closing.
     */
    void slotResetConfigDlg(); 
    
    /**
     * Reset m_docConfigDlg when the document properties dialog is closing.
     */
    void slotResetDocConfigDlg(); 

    /**
     * Manage MIDI Devices
     */
    void slotManageMIDIDevices();

    /**
     * Manage plugin synths
     */
    void slotManageSynths();

    /**
     * Show the mixers
     */
    void slotOpenAudioMixer();
    void slotOpenMidiMixer();
    
    /**
     * Edit Banks/Programs
     */
    void slotEditBanks();

    /**
     * Edit Banks/Programs for a particular device
     */
    void slotEditBanks(DeviceId);

    /**
     * Edit Control Parameters for a particular device
     */
    void slotEditControlParameters(DeviceId);

    /**
     * Edit Document Markers
     */
    void slotEditMarkers();

    /**
     * Not an actual action slot : populates the set_track_instrument sub menu
     */
    void slotPopulateTrackInstrumentPopup();

    /**
     * Remap instruments
     */
    void slotRemapInstruments();

    /**
     * Modify MIDI filters
     */
    void slotModifyMIDIFilters();

    /**
     * Manage Metronome
     */
    void slotManageMetronome();

    /**
     * Save Studio as Default
     */
    void slotSaveDefaultStudio();

    /**
     * Import Studio from File
     */
    void slotImportStudio();

    /**
     * Import Studio from Autoload
     */
    void slotImportDefaultStudio();

    /**
     * Import Studio from File
     */
    void slotImportStudioFromFile(const QString &file);

    /**
     * Send MIDI_RESET to all MIDI devices
     */
    void slotResetMidiNetwork();
    
    /**
     * toggles the toolbar
     */
    void slotToggleToolBar();

    /**
     * toggles the transport window
     */
    void slotToggleTransport();
    void slotToggleTransportVisibility();

    /**
     * hides the transport window
     */
    //### Not used -- use slotToggleTransport
    // void slotHideTransport();

    /**
     * toggles the tools toolbar
     */
    void slotToggleToolsToolBar();

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
    void slotStatusMsg(QString text);

    /**
     * changes the status message of the whole statusbar for two
     * seconds, then restores the last status. This is used to display
     * statusbar messages that give information about actions for
     * toolbar icons and menuentries.
     *
     * @param text the text that is displayed in the statusbar
     */
    void slotStatusHelpMsg(QString text);

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
     * Add one new track
     */
    void slotAddTrack();

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
     * timeT version of the same
     */
    void slotSetPointerPosition(timeT t);

    /**
     * Set the pointer position and start playing (from LoopRuler)
     */
    void slotSetPlayPosition(timeT position);

    /**
     * Set a loop
     */
    void slotSetLoop(timeT lhs, timeT rhs);


    /**
     * Update the transport with the bar, beat and unit times for
     * a given timeT
     */
    void slotDisplayBarTime(timeT t);


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
    void slotJumpToTime(RealTime);
    void slotStartAtTime(RealTime);
    void slotRefreshTimeDisplay();
    void slotToggleTracking();

    /**
     * Called when the sequencer auxiliary process exits
     */
    //void slotSequencerExited(QProcess*); QThread::finished has no QProcess param
    //  current slotSequenceExited implementation doesn't use (or name!) this param
    void slotSequencerExited();

    /// When the transport closes 
    void slotCloseTransport();

    /**
     * called by RosegardenApplication when session management tells
     * it to save its state. This is to avoid saving the transport as
     * a 2nd main window
     */
    void slotDeleteTransport();
    
    /**
     * Put the GUI into a given Tool edit mode
     */
    void slotActivateTool(QString toolName);

    /**
     * Toggles either the play or record metronome according
     * to Transport status
     */
    void slotToggleMetronome();

    /*
     * Toggle the solo mode
     */
    void slotToggleSolo(bool);

    /**
     * Set and unset the loop from the transport loop button with
     * these slots.
     */
    void slotSetLoop();
    void slotUnsetLoop();

    /**
     * Set and unset the loop start/end time from the transport loop start/stop buttons with
     * these slots.
     */
    void slotSetLoopStart();
    void slotSetLoopStop();

    /**
     * Toggle the track labels on the TrackEditor
     */
    void slotToggleTrackLabels();

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
     * Re-dock the parameters box to its initial position
     */
    void slotHideShowParameterArea();

    /**
     * The parameters box was hidden
     */
    void slotParameterAreaHidden();

    /**
     * Display tip-of-day dialog on demand
     */
    void slotShowTip();

    void slotSelectPreviousTrack();
    void slotSelectNextTrack();

    /**
     * Toggle arm (record) current track
     */
    void slotToggleRecordCurrentTrack();
    
    /**
     * Show the configure dialog
     */
    void slotConfigure();

    /**
     * Update the toolbars after edition
     */
    void slotUpdateToolbars();

    /**
     * Zoom slider moved
     */
    void slotChangeZoom(int index);

    void slotZoomIn();
    void slotZoomOut();

    /**
     * Add marker
     */
    void slotAddMarker(timeT time);

    /**
     * Remove a marker
     */
    void slotDeleteMarker(int id,
                          timeT time,
                          QString name,
                          QString description);

    /**
     * Document modified
     */
    void slotDocumentModified(bool modified = true);


    /**
     * This slot is here to be connected to RosegardenMainViewWidget's
     * stateChange signal.
     */
    void slotStateChanged(QString, bool noReverse);

    /**
     * A command has happened; check the clipboard in case we
     * need to change state
     */
    void slotTestClipboard();

    /**
     * Show a 'play list' dialog
     */
    void slotPlayList();

    /**
     * Play the requested URL
     *
     * Stop current playback, close current document,
     * open specified document and play it.
     */
    void slotPlayListPlay(QString url);

    void slotHelp();

    /**
     * Call up the online tutorial
     */
    void slotTutorial();

    /**
     * Surf to the bug reporting guidelines
     */
    void slotBugGuidelines();
    
    /**
     * Call the Rosegaden about box.
     */
    void slotHelpAbout();

    void slotHelpAboutQt();

    void slotDonate();

    /**
     * View the trigger segments manager
     */
    void slotManageTriggerSegments();

    /**
     * View the audio file manager - and some associated actions
     */
    void slotAudioManager();

    void slotAddAudioFile(AudioFileId);
    void slotDeleteAudioFile(AudioFileId);
    void slotPlayAudioFile(AudioFileId,
                           const RealTime &,
                           const RealTime &);
    void slotCancelAudioPlayingFile(AudioFileId);
    void slotDeleteAllAudioFiles();

    /**
     * Reflect segment deletion from the audio manager
     */
    void slotDeleteSegments(const SegmentSelection&);

    void slotRepeatingSegments();
    void slotLinksToCopies();
    void slotRelabelSegments();
    void slotTransposeSegments();
    void slotTransposeSemitones();
    void slotSwitchPreset();
    
    /// Panic button pressed
    void slotPanic();

    // Auto-save
    //
    void slotAutoSave();

    // Auto-save update interval changes
    //
    void slotUpdateAutoSaveInterval(unsigned int interval);

    /**
     * called when the PlayList is being closed
     */
    void slotPlayListClosed();

    /**
     * called when the BankEditor is being closed
     */
    void slotBankEditorClosed();

    /**
     * called when the synth manager is being closed
     */
    void slotSynthPluginManagerClosed();

    /**
     * called when the Mixer is being closed
     */
    void slotMidiMixerClosed();

    /**
     * when ControlEditor is being closed
     */
    void slotControlEditorClosed();

    /**
     * when MarkerEditor is being closed
     */
    void slotMarkerEditorClosed();

    /**
     * when TempoView is being closed
     */
    void slotTempoViewClosed();

    /**
     * when TriggerManager is being closed
     */
    void slotTriggerManagerClosed();

    /**
     * when AudioManagerDialog is being closed
     */
    void slotAudioManagerClosed();

    /**
     * Update the monitor levels from the sequencer mmapped file when not
     * playing.  Called by slotUpdateUI().
     */
    void slotUpdateMonitoring();

    /**
     * Create a plugin dialog for a given instrument and slot, or
     * raise an exising one.
     */
    void slotShowPluginDialog(QWidget *parent,
                              InstrumentId instrument,
                              int index);

    void slotPluginSelected(InstrumentId instrument,
                            int index, int plugin);

    /**
     * An external GUI has requested a port change.
     */
    void slotChangePluginPort(InstrumentId instrument,
                              int index, int portIndex, float value);

    /**
     * Our internal GUI has made a port change -- the
     * PluginPortInstance already contains the new value, but we need
     * to inform the sequencer and update external GUIs.
     */
    void slotPluginPortChanged(InstrumentId instrument,
                               int index, int portIndex);

    /**
     * An external GUI has requested a program change.
     */
    void slotChangePluginProgram(InstrumentId instrument,
                                 int index, QString program);

    /**
     * Our internal GUI has made a program change -- the
     * AudioPluginInstance already contains the new program, but we
     * need to inform the sequencer, update external GUIs, and update
     * the port values for the new program.
     */
    void slotPluginProgramChanged(InstrumentId instrument,
                                  int index);

    /**
     * An external GUI has requested a configure call.  (This can only
     * happen from an external GUI, we have no way to manage these
     * internally.)
     */
    void slotChangePluginConfiguration(InstrumentId, int index,
                                       bool global, QString key, QString value);
    void slotPluginDialogDestroyed(InstrumentId instrument,
                                   int index);
    void slotPluginBypassed(InstrumentId,
                            int index, bool bypassed);

    void slotShowPluginGUI(InstrumentId, int index);
    void slotStopPluginGUI(InstrumentId, int index);
    void slotPluginGUIExited(InstrumentId, int index);

    void slotDocumentDevicesResyncd();

    void slotTestStartupTester();
    
    void slotDebugDump();

    void slotShowToolHelp(const QString &);

    void slotNewerVersionAvailable(QString);
    
    void slotSetQuickMarker();
    
    void slotJumpToQuickMarker();    
    
    void slotDisplayWarning(int type,
                            QString text,
                            QString informativeText);

protected slots:
    void setupRecentFilesMenu();

private:
    /** Use QTemporaryFile to obtain a tmp filename that is guaranteed to be
     * unique
     */
    QString getLilyPondTmpFilename();

    /** Checks to see if the audio path exists.  If it does not, attempts to
     * create it.  If creation fails, sends notification to the user via the
     * WarningWidget.  This was originally an accidental overload of
     * testAudioPath(QString op), which does some of, but not all of the same
     * work.  This method attempts to create a missing audio path before doing
     * anything else, and then informs the user via the warning widget, instead
     * of a popup dialog.  If they get through all of this without fixing a
     * problem, they might wind up dealing with testAudioPath() further on.
     */
    void checkAudioPath();

    /**
     * "save modified" - asks the user for saving if the document is
     * modified
     */
    bool saveIfModified();

    //--------------- Data members ---------------------------------

    bool m_actionsSetup;

    // Action States
    bool m_notPlaying;
    bool m_haveSelection;
    bool m_haveRange;
    void updateActions();

    RosegardenMainViewWidget* m_view;

    /**
     * doc represents your actual document and is created only
     * once. It keeps information such as filename and does the
     * serialization of your files.
     */
    RosegardenDocument* m_doc;
    
    
    /**
     *    Menus
     */
    RecentFiles m_recentFiles;
    
    SequencerThread *m_sequencerThread;
    bool m_sequencerCheckedIn;

#ifdef HAVE_LIBJACK
    QProcess* m_jackProcess;
#endif // HAVE_LIBJACK

    /// CPU meter in the main window status bar.
    /**
     * This is NOT a general-purpose progress indicator.  You want to use
     * QProgressDialog for that.
     */
    ProgressBar *m_cpuBar;
    
    ZoomSlider<double> *m_zoomSlider;
    QLabel             *m_zoomLabel;

    
//    QLabel* m_statusBarLabel1;
    // SequenceManager
    //
    SequenceManager *m_seqManager;

    // Transport dialog pointer
    //
    TransportDialog *m_transport;

    // Dialogs which depend on the document

    // Audio file manager
    //
    AudioManagerDialog *m_audioManagerDialog;

    bool m_originatingJump;

    // Use these in conjunction with the loop button to
    // remember where a loop was if we've ever set one.
    timeT m_storedLoopStart;
    timeT m_storedLoopEnd;
    void toggleLoop();

    bool m_useSequencer;

    QSharedPointer<AudioPluginManager> m_pluginManager;

    QTimer* m_autoSaveTimer;

    Clipboard *m_clipboard;

    SegmentParameterBox           *m_segmentParameterBox;
    InstrumentParameterBox        *m_instrumentParameterBox;
    TrackParameterBox             *m_trackParameterBox;

    PlayListDialog        *m_playList;
    SynthPluginManagerDialog *m_synthManager;
    QPointer<AudioMixerWindow2> m_audioMixerWindow2;
    MidiMixerWindow       *m_midiMixer;
    BankEditorDialog      *m_bankEditor;
    MarkerEditor          *m_markerEditor;
    TempoView             *m_tempoView;
    TriggerSegmentManager *m_triggerSegmentManager;
    ConfigureDialog       *m_configDlg;
    DocumentConfigureDialog *m_docConfigDlg;
    std::set<ControlEditorDialog *> m_controlEditors;
    /// List of plugin dialogs to make sure we don't launch more than one.
    std::map<int, AudioPluginDialog*> m_pluginDialogs;
    AudioPluginOSCGUIManager *m_pluginGUIManager;

    static RosegardenMainWindow *m_myself;

    static std::map<QProcess *, QTemporaryFile *> m_lilyTempFileMap;

    QTimer *m_updateUITimer;
    QTimer *m_inputTimer;

    EditTempoController *m_editTempoController;

    StartupTester *m_startupTester;

    bool m_firstRun;
    bool m_haveAudioImporter;

    RosegardenParameterArea *m_parameterArea;
    
#ifdef HAVE_LIRC        
    LircClient *m_lircClient;
    LircCommander *m_lircCommander;
#endif     
    TranzportClient *m_tranzport;
        
    QPointer<DeviceManagerDialog> m_deviceManager;

    WarningWidget *m_warningWidget;

    // Ladish lv1 support
    static int sigpipe[2];
    static void handleSignal(int);
    bool installSignalHandlers();

    // See slotUpdateCPUMeter()
    QTimer *m_cpuMeterTimer;

    void processRecordedEvents();

    void muteAllTracks(bool mute = true);

    void updateTitle();

    /// For the rewind button on a keyboard controller.
    Typematic m_rewindTypematic;
    /// For the fast-forward button on a keyboard controller.
    Typematic m_fastForwardTypematic;

private slots:
    void signalAction(int);

    // New routines to handle inputs and UI updates
    void slotHandleInputs();
    void slotUpdateUI();

    /**
     * Update the CPU level meter
     */
    void slotUpdateCPUMeter();

    /// Toggles mute state of the currently selected track.
    void slotToggleMute();
    void slotMuteAllTracks();
    void slotUnmuteAllTracks();

};


}

#endif
