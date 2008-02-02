/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NOTATIONVIEW_H_
#define _RG_NOTATIONVIEW_H_

#include "base/NotationTypes.h"
#include "base/Track.h"
#include "gui/general/EditView.h"
#include "gui/general/LinedStaff.h"
#include "gui/general/LinedStaffManager.h"
#include "NotationProperties.h"
#include "NotationCanvasView.h"
#include <string>
#include <kprocess.h>
#include <ktempfile.h>
#include <qmap.h>
#include <qsize.h>
#include <qstring.h>
#include <vector>
#include "base/Event.h"
#include "gui/general/ClefIndex.h"


class QWidget;
class QTimer;
class QPaintEvent;
class QObject;
class QMouseEvent;
class QLabel;
class QCursor;
class QCanvasItem;
class QCanvas;
class KProgress;
class KComboBox;
class KActionMenu;
class KAction;


namespace Rosegarden
{

class Staff;
class Segment;
class ScrollBoxDialog;
class RulerScale;
class RosegardenGUIDoc;
class RawNoteRuler;
class ProgressDialog;
class ProgressBar;
class NotePixmapFactory;
class NotationVLayout;
class NotationStaff;
class NotationHLayout;
class NotationElement;
class NoteActionData;
class NoteActionDataMap;
class MarkActionData;
class MarkActionDataMap;
class NoteChangeActionData;
class NoteChangeActionDataMap;
class Key;
class EventSelection;
class Event;
class Clef;
class ChordNameRuler;
class QDeferScrollView;
class HeadersGroup;


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
    friend class LilypondExporter;

    Q_OBJECT

public:
    explicit NotationView(RosegardenGUIDoc *doc,
                          std::vector<Segment *> segments,
                          QWidget *parent,
                          bool showProgressive); // update during initial render?

    /**
     * Constructor for printing only.  If parent is provided, a
     * progress dialog will be shown -- otherwise not.  If another
     * NotationView is provided, the fonts and other settings used
     * for printing will be taken from that view.
     */
    explicit NotationView(RosegardenGUIDoc *doc,
                          std::vector<Segment *> segments,
                          QWidget *parent,
                          NotationView *referenceView);

    ~NotationView();

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
    Staff *getStaff(int i) {
        return getLinedStaff(i);
    }

    /// Return a pointer to the staff corresponding to the given segment
    Staff *getStaff(const Segment &segment) {
        return getLinedStaff(segment);
    }

    /// Return a pointer to the staff at the specified index
    LinedStaff *getLinedStaff(int i);

    /// Return a pointer to the staff corresponding to the given segment
    LinedStaff *getLinedStaff(const Segment &segment);

    /// Return a pointer to the staff at the specified index
    NotationStaff *getNotationStaff(int i) {
        if (i >= 0 && unsigned(i) < m_staffs.size()) return m_staffs[i];
        else return 0;
    }

    /// Return a pointer to the staff corresponding to the given segment
    NotationStaff *getNotationStaff(const Segment &segment);

    /// Return true if the staff at the specified index is the current one
    bool isCurrentStaff(int i);

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
                                bool isRest, Note::Type,
                                int dots = 0);

    /**
     * Set the note or rest selected by the user from the toolbars
     */
    void setCurrentSelectedNote(const NoteActionData &);
    
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
     * Discover whether grace-mode insertions are enabled
     */
    bool isInGraceMode();

    /**
     * Discover whether annotations are being displayed or not
     */
    bool areAnnotationsVisible() { return m_annotationsVisible; }

    /**
     * Discover whether LilyPond directives are being displayed or not
     */
    bool areLilyPondDirectivesVisible() { return m_lilypondDirectivesVisible; }

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
    virtual void setCurrentSelection(EventSelection*,
                                     bool preview = false,
                                     bool redrawNow = false);

    /**
     * Set the current event selection to a single event
     */
    void setSingleSelectedEvent(int staffNo,
                                Event *event,
                                bool preview = false,
                                bool redrawNow = false);

    /**
     * Set the current event selection to a single event
     */
    void setSingleSelectedEvent(Segment &segment,
                                Event *event,
                                bool preview = false,
                                bool redrawNow = false);

    /**
     * Show and sound the given note.  The height is used for display,
     * the pitch for performance, so the two need not correspond (e.g.
     * under ottava there may be octave differences).
     */
    void showPreviewNote(int staffNo, double layoutX,
                         int pitch, int height,
                         const Note &note,
                         bool grace,
                         int velocity = -1);

    /// Remove any visible preview note
    void clearPreviewNote();

    /// Sound the given note
    void playNote(Segment &segment, int pitch, int velocity = -1);

    /// Switches between page- and linear- layout modes
    void setPageMode(LinedStaff::PageMode mode);

    /// Returns the page width according to the layout mode (page/linear)
    int getPageWidth();

    /// Returns the page height according to the layout mode (page/linear)
    int getPageHeight();

    /// Returns the margins within the page (zero if not in MultiPageMode)
    void getPageMargins(int &left, int &top);

    /// Scrolls the view such that the given time is centered
    void scrollToTime(timeT t);

    NotePixmapFactory *getNotePixmapFactory() const {
        return m_notePixmapFactory;
    }

    virtual void refreshSegment(Segment *segment,
                                timeT startTime = 0,
                                timeT endTime = 0);

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

    /**
     * Return X of the left of the canvas visible part.
     */
    double getCanvasLeftX() { return getCanvasView()->contentsX(); }

    virtual RulerScale* getHLayout();

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
     * export a Lilypond file
     */
    bool exportLilypondFile(QString url, bool forPreview = false);

    /**
     * Export to a temporary file and process
     */
    void slotPrintLilypond();
    void slotPreviewLilypond();
    void slotLilypondViewProcessExited(KProcess *);

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
     * move the selection to the staff above
     */
    void slotMoveEventsUpStaff();

    /**
     * move the selection to the staff below
     */
    void slotMoveEventsDownStaff();

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
     * toggles the layout toolbar
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
    void slotFollowAccidental();
    void slotSharp();
    void slotFlat();
    void slotNatural();
    void slotDoubleSharp();
    void slotDoubleFlat();

    /// clef switch slots
    void slotTrebleClef();
    void slotAltoClef();
    void slotTenorClef();
    void slotBassClef();
    
    /// text tool
    void slotText();

    /// guitar chord tool
    void slotGuitarChord();

    /// editing tools
    void slotEraseSelected();
    void slotSelectSelected();

    void slotToggleStepByStep();

    /// status stuff
    void slotUpdateInsertModeStatus();
    void slotUpdateAnnotationsStatus();
    void slotUpdateLilyPondDirectivesStatus();

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
    void slotToggleLilyPondDirectives();
    void slotEditLyrics();

    /// Notation header slots
    void slotShowHeadersGroup();
    void slotHideHeadersGroup();
    void slotVerticalScrollHeadersGroup(int);

    /// Adjust notation header view when bottom ruler added or removed
    void slotCanvasBottomWidgetHeightChanged(int);

    /// group slots
    void slotGroupBeam();
    void slotGroupAutoBeam();
    void slotGroupBreak();
    void slotGroupSimpleTuplet();
    void slotGroupGeneralTuplet();
    void slotGroupTuplet(bool simple);
    void slotGroupUnTuplet();
/*!!!
    void slotGroupGrace();
    void slotGroupUnGrace();
*/
    void slotGroupSlur();
    void slotGroupPhrasingSlur();
    void slotGroupGlissando();
    void slotGroupCrescendo();
    void slotGroupDecrescendo();
    void slotGroupMakeChord();
    void slotGroupOctave2Up();
    void slotGroupOctaveUp();
    void slotGroupOctaveDown();
    void slotGroupOctave2Down();
    void slotAddIndication(std::string type, QString cat);

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
    void slotTransformsSlursAbove();
    void slotTransformsSlursBelow();
    void slotTransformsRestoreSlurs();
    void slotTransformsQuantize();
    void slotTransformsFixQuantization();
    void slotTransformsRemoveQuantization();
    void slotTransformsInterpret();

    void slotRespellDoubleFlat();
    void slotRespellFlat();
    void slotRespellNatural();
    void slotRespellSharp();
    void slotRespellDoubleSharp();
    void slotRespellUp();
    void slotRespellDown();
    void slotRespellRestore();
    void slotShowCautionary();
    void slotCancelCautionary();

    void slotSetStyleFromAction();
    void slotInsertNoteFromAction();
    void slotInsertRest();
    void slotSwitchFromRestToNote();
    void slotSwitchFromNoteToRest();
    void slotToggleDot();

    void slotAddMark();
    void slotMarksAddTextMark();
    void slotMarksAddFingeringMark();
    void slotMarksAddFingeringMarkFromAction();
    void slotMarksRemoveMarks();
    void slotMarksRemoveFingeringMarks();
    void slotMakeOrnament();
    void slotUseOrnament();
    void slotRemoveOrnament();

    void slotNoteChangeAction();
    void slotSetNoteDurations(Note::Type, bool notationOnly);
    void slotAddDot();
    void slotAddDotNotationOnly();

    void slotAddSlashes();

    void slotEditAddClef();
    void slotEditAddKeySignature();
    void slotEditAddSustainDown();
    void slotEditAddSustainUp();
    void slotEditAddSustain(bool down);
    void slotEditTranspose();
    void slotEditSwitchPreset();
    void slotEditElement(NotationStaff *, NotationElement *, bool advanced);

    void slotFinePositionLeft();
    void slotFinePositionRight();
    void slotFinePositionUp();
    void slotFinePositionDown();
    void slotFinePositionRestore();

    void slotMakeVisible();
    void slotMakeInvisible();

    void slotDebugDump();

    /// Canvas actions slots

    /**
     * Called when a mouse press occurred on a notation element
     * or somewhere on a staff
     */
    void slotItemPressed(int height, int staffNo, QMouseEvent*, NotationElement*);

    /**
     * Called when a mouse press occurred on a non-notation element 
     */
    void slotNonNotationItemPressed(QMouseEvent *e, QCanvasItem *i);

    /**
     * Called when a mouse press occurred on a QCanvasText
     */
    void slotTextItemPressed(QMouseEvent *e, QCanvasItem *i);

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

    /**
     * Set the time pointer position during playback (purely visual,
     * doesn't affect playback).  This is also at liberty to highlight
     * some notes, if it so desires...
     */
    void slotSetPointerPosition(timeT position);

    /**
     * As above, but with the ability to specify whether to scroll or
     * not to follow the pointer (above method uses the play tracking
     * setting to determine that)
     */
    void slotSetPointerPosition(timeT position, bool scroll);

    /**
     * Update the recording segment if it's one of the ones in the
     * view
     */
    void slotUpdateRecordingSegment(Segment *recordingSegment,
                                    timeT updatedFrom);

    /// Set the current staff to the one containing the given canvas Y coord
    void slotSetCurrentStaff(double canvasX, int canvasY);

    /// Set the current staff to that with the given id
    void slotSetCurrentStaff(int staffNo);

    /**
     * Set the insert cursor position (from the top LoopRuler).
     * If the segment has recently been changed and no refresh has
     * occurred since, pass updateNow false; then the move will
     * happen on the next update.
     */
    void slotSetInsertCursorPosition(timeT position,
                                     bool scroll, bool updateNow);

    virtual void slotSetInsertCursorPosition(timeT position) {
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
    void slotSetInsertCursorAndRecentre(timeT position,
                                        double cx, int cy,
                                        bool updateNow = true);

    void slotSetInsertCursorAndRecentre(timeT position,
                                        double cx, double cy) {
        slotSetInsertCursorAndRecentre(position, cx, static_cast<int>(cy), true);
    }

    /// Set insert cursor to playback pointer position
    void slotJumpCursorToPlayback();

    /// Set playback pointer to insert cursor position (affects playback)
    void slotJumpPlaybackToCursor();

    /// Toggle tracking with the position pointer during playback
    void slotToggleTracking();

    /// Change the current staff to the one preceding the current one
    void slotCurrentStaffUp();

    /// Change the current staff to the one following the current one
    void slotCurrentStaffDown();

    /// Change the current segment to the one following the current one
    void slotCurrentSegmentPrior();

    /// Change the current segment to the one preceding the current one
    void slotCurrentSegmentNext();

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
    void slotChangeFontSizeFromStringValue(const QString&);

    /// Changes to the next font size up
    void slotZoomIn();

    /// Changes to the next font size down
    void slotZoomOut();

    /// Changes the hlayout spacing of the staffs on the view
    void slotChangeSpacing(int newSpacing);

    /// Changes the hlayout spacing of the staffs on the view
    void slotChangeSpacingFromStringValue(const QString&);

    /// Changes the hlayout spacing of the staffs on the view
    void slotChangeSpacingFromAction();

    /// Changes the hlayout proportion of the staffs on the view
    void slotChangeProportion(int newProportion);

    /// Changes the hlayout proportion of the staffs on the view
    void slotChangeProportionFromIndex(int newProportionIndex);

    /// Changes the hlayout proportion of the staffs on the view
    void slotChangeProportionFromAction();

    /// Note-on received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOnReceived(int pitch, int velocity);

    /// Note-off received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOffReceived(int pitch, int velocity);

    /// Note-on or note-off received asynchronously -- as above
    void slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn);

    /// A timer set when a note-on event was received has elapsed
    void slotInsertableTimerElapsed();

    /// The given QObject has originated a step-by-step-editing request
    void slotStepByStepTargetRequested(QObject *);

    /// Do on-demand rendering for a region.
    void slotCheckRendered(double cx0, double cx1);

    /// Do some background rendering work.
    void slotRenderSomething();

    void slotSetOperationNameAndStatus(QString);

    // Update notation view based on track/staff name change
    void slotUpdateStaffName();

    // Lilypond Directive slots
    void slotBeginLilypondRepeat();

signals:
    /**
     * Emitted when the note selected in the palette changes
     */
    void changeCurrentNote(bool isRest, Note::Type);

    /**
     * Emitted when a new accidental has been choosen by the user
     */
    void changeAccidental(Accidental, bool follow);

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
    void jumpPlaybackTo(timeT);
    void panic();

    /// progress Report
    void setProgress(int);
    void incrementProgress(int);
    void setOperationName(QString);

    void stepByStepTargetRequested(QObject *);

    void renderComplete();

    void editTimeSignature(timeT);

    void editMetadata(QString);

    void editTriggerSegment(int);

    void staffLabelChanged(TrackId id, QString label);

protected:

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
     * Update the panner thumbnail images.  If complete is true,
     * copy the entire mini-canvas.
     */
    void updateThumbnails(bool complete);

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
                             timeT startTime = 0,
                             timeT endTime = 0);

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

    virtual Segment *getCurrentSegment();
    virtual Staff *getCurrentStaff() { return getCurrentLinedStaff(); }
    virtual LinedStaff *getCurrentLinedStaff();

    virtual LinedStaff *getStaffAbove();
    virtual LinedStaff *getStaffBelow();
        
    virtual bool hasSegment(Segment *segment);

    /**
     * Return the time at which the insert cursor may be found.
     */
    virtual timeT getInsertionTime();

    /**
     * Return the time at which the insert cursor may be found,
     * and the time signature, clef and key at that time.
     */
    virtual timeT getInsertionTime(Clef &clef,
                                   Rosegarden::Key &key);

    void doDeferredCursorMove();

    void removeViewLocalProperties(Event*);

    void setupProgress(KProgress*);
    void setupProgress(ProgressDialog*);
    void setupDefaultProgress();
    void disconnectProgress();

    /**
     * Test whether we've had too many preview notes recently
     */
    bool canPreviewAnotherNote();

    virtual void updateViewCaption();

    void showHeadersGroup();
    void hideHeadersGroup();


    //--------------- Data members ---------------------------------

    NotationProperties m_properties;

    /// Displayed in the status bar, shows number of events selected
    QLabel *m_selectionCounter;

    /// Displayed in the status bar, shows insertion mode
    QLabel *m_insertModeLabel;

    /// Displayed in the status bar, shows when annotations are hidden
    QLabel *m_annotationsLabel;

    /// Displayed in the status bar, shows when LilyPond directives are hidden
    QLabel *m_lilypondDirectivesLabel;

    /// Displayed in the status bar, shows progress of current operation
    ProgressBar *m_progressBar;

    /// Displayed in the status bar, holds the pixmap of the current note
    QLabel* m_currentNotePixmap;

    /// Displayed in the status bar, shows the pitch the cursor is at
    QLabel* m_hoveredOverNoteName;

    /// Displayed in the status bar, shows the absolute time the cursor is at
    QLabel* m_hoveredOverAbsoluteTime;

    std::vector<NotationStaff*> m_staffs;
    int m_currentStaff;
    int m_lastFinishingStaff;

    QCanvasItem *m_title;
    QCanvasItem *m_subtitle;
    QCanvasItem *m_composer;
    QCanvasItem *m_copyright;
    std::vector<QCanvasItem *> m_pages;
    std::vector<QCanvasItem *> m_pageNumbers;

    timeT m_insertionTime;
    enum DeferredCursorMoveType {
        NoCursorMoveNeeded,
        CursorMoveOnly,
        CursorMoveAndMakeVisible,
        CursorMoveAndScrollToPosition
    };
    DeferredCursorMoveType m_deferredCursorMove;
    double m_deferredCursorScrollToX;

    QString m_lastNoteAction;

    std::string m_fontName;
    int m_fontSize;
    LinedStaff::PageMode m_pageMode;
    int m_leftGutter;

    NotePixmapFactory *m_notePixmapFactory;
    
    NotationHLayout* m_hlayout;
    NotationVLayout* m_vlayout;

    ChordNameRuler *m_chordNameRuler;
    QWidget *m_tempoRuler;
    RawNoteRuler *m_rawNoteRuler;
    bool m_annotationsVisible;
    bool m_lilypondDirectivesVisible;
    
    KAction* m_selectDefaultNote;

    typedef QMap<QString, NoteActionData *> NoteActionDataMap;
    static NoteActionDataMap* m_noteActionDataMap;

    typedef QMap<QString, NoteChangeActionData *> NoteChangeActionDataMap;
    static NoteChangeActionDataMap* m_noteChangeActionDataMap;

    typedef QMap<QString, MarkActionData *> MarkActionDataMap;
    static MarkActionDataMap *m_markActionDataMap;

    KComboBox       *m_fontCombo;
    KComboBox       *m_fontSizeCombo;
    KComboBox       *m_spacingCombo;
    KActionMenu     *m_fontSizeActionMenu;
    ScrollBoxDialog *m_pannerDialog;
    QTimer *m_renderTimer;

    bool m_playTracking;

    std::vector<std::pair<int, int> > m_pendingInsertableNotes;

    enum { PROGRESS_NONE,
           PROGRESS_BAR,
           PROGRESS_DIALOG } m_progressDisplayer;

    bool m_inhibitRefresh;
    bool m_ok;

    bool m_printMode;
    int m_printSize;

    static std::map<KProcess *, KTempFile *> m_lilyTempFileMap;

    int m_showHeadersGroup;
    QDeferScrollView * m_headersGroupView;
    HeadersGroup * m_headersGroup;
    QFrame * m_headersTopFrame;
};


}

#endif
