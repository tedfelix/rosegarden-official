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

#ifndef NOTATIONVIEW_H
#define NOTATIONVIEW_H

#include <string>
#include <kmainwindow.h>

#include "editview.h"
#include "dialogs.h" // for TempoDialog::TempoDialogAction
#include "linedstaff.h"
#include "notationelement.h"
#include "notationcanvasview.h"
#include "notationproperties.h"
#include "zoomslider.h"
#include "NotationTypes.h"

class QLabel;
class QCanvasItem;
class KActionMenu;
class KPrinter;
class RosegardenGUIDoc;
class NotationTool;
class NotationToolBox;
class PositionCursor;
class ActiveItem;
class NoteActionData;
class MarkActionData;
class ChordNameRuler;
class RawNoteRuler;
class RosegardenProgressBar;
class RosegardenProgressDialog;
class KProgress;
class KProgressDialog;
class ProgressReporter;
class NotationHLayout;
class NotationVLayout;
class NotationStaff;
class ScrollBoxDialog;


/**
 * NotationView is a view for one or more Staff objects, each of
 * which contains the notation data associated with a Segment.
 * NotationView owns the Staff objects it displays.
 * 
 * This class manages the relationship between NotationHLayout/
 * NotationVLayout and Staff data, as well as using rendering the
 * actual notes (using NotePixmapFactory to generate the pixmaps).
 */

class NotationView : public EditView,
                     public LinedStaffManager
{
    friend class NoteInserter;
    friend class ClefInserter;
    friend class NotationEraser;
    friend class NotationSelectionPaster;

    Q_OBJECT

public:
    explicit NotationView(RosegardenGUIDoc *doc,
			  std::vector<Rosegarden::Segment *> segments,
			  QWidget *parent,
			  bool showProgressive); // update during initial render?

    /**
     * Constructor for printing only.  If parent is provided, a
     * progress dialog will be shown -- otherwise not.  If another
     * NotationView is provided, the fonts and other settings used
     * for printing will be taken from that view.
     */
    explicit NotationView(RosegardenGUIDoc *doc,
			  std::vector<Rosegarden::Segment *> segments,
			  QWidget *parent,
			  NotationView *referenceView);

    ~NotationView();

    static const char* const ConfigGroup;
    
//     void initialLayout();

    /// constructed successfully? (main reason it might not is user hit Cancel)
    bool isOK() const { return m_ok; }

    /**
     * Return the view-local PropertyName definitions for this view
     */
    const NotationProperties &getProperties() const;

    /// Return the number of staffs
    int getStaffCount() { return m_staffs.size(); }

    /// Return a pointer to the staff at the specified index
    NotationStaff *getStaff(int i) {
        if (i >= 0 && unsigned(i) < m_staffs.size()) return m_staffs[i];
        else return 0;
    }

    /// Return a pointer to the staff corresponding to the given segment
    NotationStaff *getStaff(const Rosegarden::Segment &segment);

    QCanvas* canvas() { return getCanvasView()->canvas(); }
    
    void setCanvasCursor(const QCursor &cursor) {
        getCanvasView()->viewport()->setCursor(cursor);
    }

    void setHeightTracking(bool t) {
        getCanvasView()->setHeightTracking(t);
    }

    /**
     * Returns true if the view is actually for printing
     */
    bool isInPrintMode() { return m_printMode; }

    /**
     * Set the note or rest selected by the user from the toolbars
     */
    void setCurrentSelectedNote(const char *pixmapName,
                                bool isRest, Rosegarden::Note::Type,
                                int dots = 0);

    /**
     * Set the note or rest selected by the user from the toolbars
     */
    void setCurrentSelectedNote(NoteActionData);
    
    /**
     * Discover whether chord-mode insertions are enabled (as opposed
     * to the default melody-mode)
     */
    bool isInChordMode();
    
    /**
     * Discover whether triplet-mode insertions are enabled
     */
    bool isInTripletMode();

    /**
     * Discover whether annotations are being displayed or not
     */
    bool areAnnotationsVisible() { return m_annotationsVisible; }

    /**
     * Set the current event selection.
     *
     * If preview is true, sound the selection as well.
     *
     * If redrawNow is true, recolour the elements on the canvas;
     * otherwise just line up a refresh for the next paint event.
     *
     * (If the selection has changed as part of a modification to a
     * segment, redrawNow should be unnecessary and undesirable, as a
     * paint event will occur in the next event loop following the
     * command invocation anyway.)
     */
    virtual void setCurrentSelection(Rosegarden::EventSelection*,
                                     bool preview = false,
                                     bool redrawNow = false);

    /**
     * Set the current event selection to a single event
     */
    void setSingleSelectedEvent(int staffNo,
                                Rosegarden::Event *event,
                                bool preview = false,
                                bool redrawNow = false);

    /**
     * Set the current event selection to a single event
     */
    void setSingleSelectedEvent(Rosegarden::Segment &segment,
                                Rosegarden::Event *event,
                                bool preview = false,
                                bool redrawNow = false);

    /// Show and sound the given note
    void showPreviewNote(int staffNo, double layoutX,
                         int pitch, int height,
                         const Rosegarden::Note &note);

    /// Remove any visible preview note
    void clearPreviewNote();

    /// Sound the given note
    void playNote(Rosegarden::Segment &segment, int pitch);

    /// Switches between page- and linear- layout modes
    void setPageMode(LinedStaff::PageMode mode);

    /// Returns the page width according to the layout mode (page/linear)
    int getPageWidth();

    /// Returns the page height according to the layout mode (page/linear)
    int getPageHeight();

    /// Returns the margins within the page (zero if not in MultiPageMode)
    void getPageMargins(int &left, int &top);

    /// Scrolls the view such that the given time is centered
    void scrollToTime(Rosegarden::timeT t);

    NotePixmapFactory *getNotePixmapFactory() const {
        return m_notePixmapFactory;
    }

    /**
     * get a NotePixmapFactory suitable for creating toolbar pixmaps
     * (but not guaranteed to be any good for anything else)
     */
    NotePixmapFactory *getToolbarNotePixmapFactory() {
        return &m_toolbarNotePixmapFactory;
    }

    virtual void refreshSegment(Rosegarden::Segment *segment,
                                Rosegarden::timeT startTime = 0,
                                Rosegarden::timeT endTime = 0);

    /**
     * From LinedStaffManager
     */
    virtual LinedStaff* getStaffForCanvasCoords(int x, int y) const;


    /**
     * Overridden from EditView
     */
    virtual void updateView();

    /**
     * Render segments on printing painter.  This uses the current
     * font size and layout, rather than the optimal ones for the
     * printer configuration (notation editing is not quite WYSIWYG,
     * and we may be in a non-page mode).
     * 
     * To print optimally use slotFilePrint, which will create
     * another NotationView with the optimal settings and call print
     * on that.
     */
    virtual void print(bool previewOnly = false);

    
public slots:

    /**
     * Print the current set of segments, by creating another
     * NotationView with the printing configuration but the same
     * segments, font etc as this view and asking it to print.
     */
    void slotFilePrint();

    /**
     * Preview the current set of segments, by creating another
     * NotationView with the printing configuration but the same
     * segments, font etc as this view and asking it to preview.
     */
    void slotFilePrintPreview();

    /**
     * put the marked text/object into the clipboard and remove it
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
     * cut the selection and close the gap, moving subsequent events
     * towards the start of the segment
     */
    void slotEditCutAndClose();

    /**
     * paste the clipboard into the document, offering a choice for how
     */
    void slotEditGeneralPaste();

    /**
     * delete the selection (cut without the copy)
     */
    void slotEditDelete();

    /**
     * toggles the tools toolbar
     */
    void slotToggleToolsToolBar();

    /**
     * toggles the notes toolbar
     */
    void slotToggleNotesToolBar();

    /**
     * toggles the rests toolbar
     */
    void slotToggleRestsToolBar();

    /**
     * toggles the accidentals toolbar
     */
    void slotToggleAccidentalsToolBar();

    /**
     * toggles the clefs toolbar
     */
    void slotToggleClefsToolBar();

    /**
     * toggles the marks toolbar
     */
    void slotToggleMarksToolBar();

    /**
     * toggles the group toolbar
     */
    void slotToggleGroupToolBar();

    /**
     * toggles the font toolbar
     */
    void slotToggleLayoutToolBar();

    /**
     * toggles the transport toolbar
     */
    void slotToggleTransportToolBar();

    /**
     * toggles the meta toolbar
     */
    void slotToggleMetaToolBar();

    /// note switch slot
    void slotNoteAction();

    /// switch to last selected note
    void slotLastNoteAction();

    /// accidental switch slots
    void slotNoAccidental();
    void slotSharp();
    void slotFlat();
    void slotNatural();
    void slotDoubleSharp();
    void slotDoubleFlat();

    /// clef switch slots
    void slotTrebleClef();
    void slotTenorClef();
    void slotAltoClef();
    void slotBassClef();
    
    /// text tool
    void slotText();

    /// editing tools
    void slotEraseSelected();
    void slotSelectSelected();

    void slotToggleStepByStep();

    /// status stuff
    void slotUpdateInsertModeStatus();
    void slotUpdateAnnotationsStatus();

    /// edit menu
    void slotPreviewSelection();
    void slotClearLoop();
    void slotClearSelection();
    void slotEditSelectFromStart();
    void slotEditSelectToEnd();
    void slotEditSelectWholeStaff();
    void slotFilterSelection();

    /// view menu
    void slotLinearMode();
    void slotContinuousPageMode();
    void slotMultiPageMode();
    void slotToggleChordsRuler();
    void slotToggleRawNoteRuler();
    void slotToggleTempoRuler();
    void slotToggleAnnotations();
    void slotEditLyrics();

    /// group slots
    void slotGroupBeam();
    void slotGroupAutoBeam();
    void slotGroupBreak();
    void slotGroupSimpleTuplet();
    void slotGroupGeneralTuplet();
    void slotGroupTuplet(bool simple);
    void slotGroupUnTuplet();
    void slotGroupGrace();
    void slotGroupUnGrace();
    void slotGroupSlur();
    void slotGroupCrescendo();
    void slotGroupDecrescendo();
    void slotGroupMakeChord();

    /// transforms slots
    void slotTransformsNormalizeRests();
    void slotTransformsCollapseRests();
    void slotTransformsCollapseNotes();
    void slotTransformsTieNotes();
    void slotTransformsUntieNotes();
    void slotTransformsMakeNotesViable();
    void slotTransformsDeCounterpoint();
    void slotTransformsStemsUp();
    void slotTransformsStemsDown();
    void slotTransformsRestoreStems();
    void slotTransformsQuantize();
    void slotTransformsFixQuantization();
    void slotTransformsInterpret();

    void slotRespellDoubleFlat();
    void slotRespellFlat();
    void slotRespellSharp();
    void slotRespellDoubleSharp();
    void slotRespellUp();
    void slotRespellDown();
    void slotRespellRestore();

    void slotSetStyleFromAction();
    void slotInsertNoteFromAction();
    void slotInsertRest();
    void slotSwitchFromRestToNote();
    void slotSwitchFromNoteToRest();

    void slotAddMark();
    void slotMarksAddTextMark();
    void slotMarksRemoveMarks();

    void slotAddSlashes();

    void slotEditAddClef();
    void slotEditAddKeySignature();

    void slotDebugDump();

    /// Canvas actions slots

    /**
     * Called when a mouse press occurred on a notation element
     * or somewhere on a staff
     */
    void slotItemPressed(int height, int staffNo, QMouseEvent*, NotationElement*);

    void slotMouseMoved(QMouseEvent*);
    void slotMouseReleased(QMouseEvent*);

    /**
     * Called when the mouse cursor moves over a different height on
     * the staff
     *
     * @see NotationCanvasView#hoveredOverNoteChange()
     */
    void slotHoveredOverNoteChanged(const QString&);

    /**
     * Called when the mouse cursor moves over a note which is at a
     * different time on the staff
     *
     * @see NotationCanvasView#hoveredOverAbsoluteTimeChange()
     */
    void slotHoveredOverAbsoluteTimeChanged(unsigned int);

    /// Set the time pointer position during playback (purely visual, doesn't affect playback)
    void slotSetPointerPosition(Rosegarden::timeT position, bool scroll = true);

    /// Set the current staff to the one containing the given canvas Y coord
    void slotSetCurrentStaff(double canvasX, int canvasY);

    /**
     * Set the insert cursor position (from the top LoopRuler).
     * If the segment has recently been changed and no refresh has
     * occurred since, pass updateNow false; then the move will
     * happen on the next update.
     */
    void slotSetInsertCursorPosition(Rosegarden::timeT position,
                                     bool scroll, bool updateNow);

    virtual void slotSetInsertCursorPosition(Rosegarden::timeT position) {
        slotSetInsertCursorPosition(position, true, true);
    }

    /// Set the insert cursor position from a mouse event location
    void slotSetInsertCursorPosition(double canvasX, int canvasY,
                                     bool scroll, bool updateNow);

    void slotSetInsertCursorPosition(double canvasX, int canvasY) {
        slotSetInsertCursorPosition(canvasX, canvasY, true, true);
    }

    /**
     * Set the insert cursor position and scroll so it's at given point.
     * If the segment has recently been changed and no refresh has
     * occurred since, pass updateNow false; then the move will
     * happen on the next update.
     */
    void slotSetInsertCursorAndRecentre(Rosegarden::timeT position,
                                        double cx, int cy,
                                        bool updateNow = true);

    void slotSetInsertCursorAndRecentre(Rosegarden::timeT position,
                                        double cx, double cy) {
        slotSetInsertCursorAndRecentre(position, cx, static_cast<int>(cy), true);
    }

    /// Set insert cursor to playback pointer position
    void slotJumpCursorToPlayback();

    /// Set playback pointer to insert cursor position (affects playback)
    void slotJumpPlaybackToCursor();

    /// Change the current staff to the one preceding the current one
    void slotCurrentStaffUp();

    /// Change the current staff to the one following the current one
    void slotCurrentStaffDown();

    /// Changes the font of the staffs on the view, gets font name from sender
    void slotChangeFontFromAction();

    /// Changes the font of the staffs on the view
    void slotChangeFont(std::string newFont);

    /// Changes the font and font size of the staffs on the view
    void slotChangeFont(std::string newFont, int newSize);
    
    /// Changes the font of the staffs on the view
    void slotChangeFont(const QString &newFont);

    /// Changes the font size of the staffs on the view
    void slotChangeFontSize(int newSize);

    /// Changes the font size of the staffs on the view, gets size from sender
    void slotChangeFontSizeFromAction();

    /// Changes the font size of the staffs on the view to the nth size in the available size list
    void slotChangeFontSizeFromIndex(int n);

    /// Changes the hlayout spacing of the staffs on the view
    void slotChangeSpacing(int newSpacing);

    /// Changes the hlayout spacing of the staffs on the view
    void slotChangeSpacingFromIndex(int newSpacingIndex);

    /// Changes the hlayout spacing of the staffs on the view
    void slotChangeSpacingFromAction();

    /// Note-on received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOnReceived(int pitch);

    /// Note-off received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOffReceived(int pitch);

    /// Note-on or note-off received asynchronously -- as above
    void slotInsertableNoteEventReceived(int pitch, bool noteOn);

    /// A timer set when a note-on event was received has elapsed
    void slotInsertableTimerElapsed();

    /// The given QObject has originated a step-by-step-editing request
    void slotStepByStepTargetRequested(QObject *);

    /// Do on-demand rendering for a region.
    void slotCheckRendered(double cx0, double cx1);

signals:
    /**
     * Emitted when the note selected in the palette changes
     */
    void changeCurrentNote(bool isRest, Rosegarden::Note::Type);

    /**
     * Emitted when a new accidental has been choosen by the user
     */
    void changeAccidental(Rosegarden::Accidental);

    /**
     * Emitted when the selection has been cut or copied
     *
     * @see NotationSelector#hideSelection
     */
    void usedSelection();

    void play();
    void stop();
    void fastForwardPlayback();
    void rewindPlayback();
    void fastForwardPlaybackToEnd();
    void rewindPlaybackToBeginning();
    void jumpPlaybackTo(Rosegarden::timeT);

    /// progress Report
    void setProgress(int);
    void incrementProgress(int);
    void setOperationName(QString);

    void stepByStepTargetRequested(QObject *);

protected:

    virtual Rosegarden::RulerScale* getHLayout();

    virtual void paintEvent(QPaintEvent* e);

    /**
     * init the action maps for notes, marks etc
     */
    void initActionDataMaps();

protected slots:
    /**
     * save general Options like all bar positions and status as well
     * as the geometry and the recent file list to the configuration
     * file
     */
    virtual void slotSaveOptions();

protected:
    /**
     * read general Options again and initialize all variables like the recent file list
     */
    virtual void readOptions();

    void setOneToolbar(const char *actionName, 
		       const char *toolbarName);

    /**
     * create menus and toolbars
     */
    virtual void setupActions();

    /**
     * create or re-initialise (after font change) the font size menu
     */
    virtual void setupFontSizeMenu(std::string oldFontName = "");

    /**
     * Set KDE3+ menu states based on the current selection
     */
    virtual void setMenuStates();

    /**
     * setup status bar
     */
    virtual void initStatusBar();

    /**
     * Place the staffs at the correct x & y coordinates (before layout)
     */
    void positionStaffs();

    /**
     * Place the page pixmaps (if any) at the correct x & y
     * coordinates (after layout)
     */
    void positionPages();

    /**
     * setup the layout/font toolbar
     */
    void initLayoutToolbar();

    /**
     * Helper function to toggle a toolbar given its name
     * If \a force point to a bool, then the bool's value
     * is used to show/hide the toolbar.
     */
    void toggleNamedToolBar(const QString& toolBarName, bool* force = 0);

    /// Calls all the relevant preparse and layout methods
    virtual bool applyLayout(int staffNo = -1,
                             Rosegarden::timeT startTime = 0,
                             Rosegarden::timeT endTime = 0);

    /**
     * Readjust the size of the canvas after a layout
     *
     * Checks the total width computed by the horizontal layout
     *
     * @see NotationHLayout#getTotalWidth()
     */
    void readjustCanvasSize();

    /**
     * Override from EditView
     * @see EditView#getViewSize
     */
    virtual QSize getViewSize();

    /**
     * Override from EditView
     * @see EditView#setViewSize
     */
    virtual void setViewSize(QSize);

    /**
     * Set the note pixmap factory
     *
     * The previous pixmap factory is deleted
     */
    void setNotePixmapFactory(NotePixmapFactory*);

    virtual NotationCanvasView* getCanvasView();

    virtual Rosegarden::Segment *getCurrentSegment();
    virtual Rosegarden::Staff *getCurrentStaff();

    /**
     * Return the time at which the insert cursor may be found.
     */
    virtual Rosegarden::timeT getInsertionTime();

    /**
     * Return the time at which the insert cursor may be found,
     * and the time signature, clef and key at that time.
     */
    virtual Rosegarden::timeT getInsertionTime(Rosegarden::Clef &clef,
                                               Rosegarden::Key &key);

    void doDeferredCursorMove();

    void removeViewLocalProperties(Rosegarden::Event*);

    void setupProgress(KProgress*);
    void setupProgress(RosegardenProgressDialog*);
    void setupDefaultProgress();
    void disconnectProgress();
    void installProgressEventFilter();
    void removeProgressEventFilter();

    /**
     * Test whether we've had too many preview notes recently
     */
    bool canPreviewAnotherNote();

    //--------------- Data members ---------------------------------

    NotationProperties m_properties;

    /// Displayed in the status bar, shows number of events selected
    QLabel *m_selectionCounter;

    /// Displayed in the status bar, shows insertion mode
    QLabel *m_insertModeLabel;

    /// Displayed in the status bar, shows when annotations are hidden
    QLabel *m_annotationsLabel;

    /// Displayed in the status bar, shows progress of current operation
    RosegardenProgressBar *m_progressBar;

    /// Displayed in the status bar, holds the pixmap of the current note
    QLabel* m_currentNotePixmap;

    /// Displayed in the status bar, shows the pitch the cursor is at
    QLabel* m_hoveredOverNoteName;

    /// Displayed in the status bar, shows the absolute time the cursor is at
    QLabel* m_hoveredOverAbsoluteTime;

    std::vector<NotationStaff*> m_staffs;
    int m_currentStaff;
    int m_lastFinishingStaff;

    std::vector<QCanvasItem *> m_pages;
    std::vector<QCanvasItem *> m_pageNumbers;

    Rosegarden::timeT m_insertionTime;
    enum {
        NoCursorMoveNeeded,
        CursorMoveOnly,
        CursorMoveAndMakeVisible,
        CursorMoveAndScrollToPosition
    } m_deferredCursorMove;
    double m_deferredCursorScrollToX;

    Rosegarden::Accidental m_currentAccidental;
    QString m_lastNoteAction;

    std::string m_fontName;
    int m_fontSize;
    LinedStaff::PageMode m_pageMode;

    NotePixmapFactory *m_notePixmapFactory;
    NotePixmapFactory m_toolbarNotePixmapFactory;
    
    NotationHLayout* m_hlayout;
    NotationVLayout* m_vlayout;

    ChordNameRuler *m_chordNameRuler;
    QWidget *m_tempoRuler;
    RawNoteRuler *m_rawNoteRuler;
    bool m_annotationsVisible;
    
    KAction* m_selectDefaultNote;

    typedef QMap<QString, NoteActionData> NoteActionDataMap;
    static NoteActionDataMap* m_noteActionDataMap;

    typedef QMap<QString, MarkActionData> MarkActionDataMap;
    static MarkActionDataMap *m_markActionDataMap;

    QComboBox *m_fontCombo;
    ZoomSlider<int> *m_fontSizeSlider;
    ZoomSlider<int> *m_spacingSlider;
    KActionMenu *m_fontSizeActionMenu;
    ScrollBoxDialog *m_pannerDialog;

    enum { PROGRESS_NONE,
           PROGRESS_BAR,
           PROGRESS_DIALOG } m_progressDisplayer;
    bool m_progressEventFilterInstalled;

    bool m_inhibitRefresh;
    bool m_ok;

    bool m_printMode;
    int m_printSize;
};

#endif
