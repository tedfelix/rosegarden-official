// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#ifndef NOTATIONVIEW_H
#define NOTATIONVIEW_H

#include <string>
#include <kmainwindow.h>

#include "editview.h"
#include "dialogs.h" // for TempoDialog::TempoDialogAction
#include "notationelement.h"
#include "notationhlayout.h"
#include "notationvlayout.h"
#include "notationcanvasview.h"
#include "notationstaff.h"
#include "notationproperties.h"
#include "zoomslider.h"
#include "NotationTypes.h"

class QLabel;
class QCanvasItem;
class KActionMenu;
namespace Rosegarden { class Segment; class EventSelection; class MappedEvent; }
class RosegardenGUIDoc;
class NotationTool;
class NotationToolBox;
class PositionCursor;
class ActiveItem;
class NoteActionData;
class MarkActionData;
class ChordNameRuler;
class RosegardenProgressDialog;

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
		     public LinedStaffManager<NotationElement>
{
    friend class NoteInserter;
    friend class ClefInserter;
    friend class NotationEraser;
    friend class NotationSelectionPaster;

    Q_OBJECT

public:
    NotationView(RosegardenGUIDoc *doc,
                 std::vector<Rosegarden::Segment *> segments,
                 QWidget *parent,
		 bool showProgressive = true); // update during initial render?
    ~NotationView();

    /**
     * Return the view-local PropertyName definitions for this view
     */
    const NotationProperties &getProperties();

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
     * Set the current event selection
     * @see NotationSelector
     */
    void setCurrentSelection(Rosegarden::EventSelection*);

    /**
     * Set the current event selection to a single event
     */
    void setSingleSelectedEvent(int staffNo,
				Rosegarden::Event *event);

    /**
     * Set the current event selection to a single event
     */
    void setSingleSelectedEvent(Rosegarden::Segment &segment,
				Rosegarden::Event *event);

    /// Show and sound the given note
    void showPreviewNote(int staffNo, double layoutX,
			 int pitch, int height,
			 const Rosegarden::Note &note);

    /// Remove any visible preview note
    void clearPreviewNote();

    /// Switches between page- and linear- layout mode
    void setPageMode(bool pageMode);

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
    virtual NotationStaff *getStaffForCanvasY(int y) const;


    /**
     * Overridden from EditView
     */
    virtual void updateView();

    /**
     * Render segments on printing painter
     *
     * @see NotationCanvasView#print
     */
    virtual void print(QPainter*);

public slots:

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
     * toggles the font toolbar
     */
    void slotToggleFontToolBar();

    /// note switch slot
    void slotNoteAction();

    void slotToggleTriplet();

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

    /// edition tools
    void slotEraseSelected();
    void slotSelectSelected();

    /// edit menu
    void slotEditSelectFromStart();
    void slotEditSelectToEnd();
    void slotEditSelectWholeStaff();

    /// view menu
    void slotLinearMode();
    void slotPageMode();
    void slotLabelChords();
    void slotShowTempos();

    /// group slots
    void slotGroupBeam();
    void slotGroupAutoBeam();
    void slotGroupBreak();
    void slotGroupSimpleTuplet();
    void slotGroupGeneralTuplet();
    void slotGroupTuplet(bool simple);
    void slotGroupGrace();
    void slotGroupUnGrace();
    void slotGroupSlur();
    void slotGroupCrescendo();
    void slotGroupDecrescendo();

    /// transforms slots
    void slotTransformsNormalizeRests();
    void slotTransformsCollapseRests();
    void slotTransformsCollapseNotes();
    void slotTransformsTieNotes();
    void slotTransformsUntieNotes();
    void slotTransformsStemsUp();
    void slotTransformsStemsDown();
    void slotTransformsRestoreStems();
    void slotTransformsClassicalStyle();
    void slotTransformsCrossStyle();
    void slotTransformsTriangleStyle();
    void slotTransformsMensuralStyle();
    void slotTransformsTranspose();
    void slotTransformsTransposeUp();
    void slotTransformsTransposeUpOctave();
    void slotTransformsTransposeDown();
    void slotTransformsTransposeDownOctave();
    void slotTransformsQuantize();

    void slotAddMark();
    void slotMarksAddTextMark();
    void slotMarksRemoveMarks();

    void slotAddSlashes();

    void slotEditAddClef();
    void slotEditAddTempo();
    void slotEditAddTimeSignature();
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
    void slotSetCurrentStaff(int canvasY);

    /**
     * Set the insert cursor position (from the top LoopRuler).
     * If the segment has recently been changed and no refresh has
     * occurred since, pass updateNow false; then the move will
     * happen on the next update.
     */
    void slotSetInsertCursorPosition(Rosegarden::timeT position,
				     bool scroll, bool updateNow);

    void slotSetInsertCursorPosition(Rosegarden::timeT position) {
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
	slotSetInsertCursorAndRecentre(position, cx, cy, true);
    }

    /// Step back one event with the insert cursor position
    void slotStepBackward();

    /// Step forward one event with the insert cursor position
    void slotStepForward();

    /// Step back one bar with the insert cursor position
    void slotJumpBackward();

    /// Step forward one bar with the insert cursor position
    void slotJumpForward();

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

    /// Changes the display quantization of the staffs on the view
    void slotChangeLegato(int newLegatoIndex);

    /// The document has been destroyed, and we're about to go with it
    void slotDocumentDestroyed();

    /**
     * A command has happened; check the clipboard in case we
     * need to change state
     */
    void slotTestClipboard();

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
     * Emitted when the tuplet mode has been toggled by the user
     */
    void changeTupletMode(bool newTupletMode);

    /**
     * Emitted when the selection has been cut or copied
     *
     * @see NotationSelector#hideSelection
     */
    void usedSelection();

    void notePlayed(Rosegarden::MappedEvent *);

    void changeTempo(Rosegarden::timeT,  // tempo change time
                     double,             // tempo value
                     TempoDialog::TempoDialogAction); // tempo action

    void fastForwardPlayback();
    void rewindPlayback();
    void fastForwardPlaybackToEnd();
    void rewindPlaybackToBeginning();
    void jumpPlaybackTo(Rosegarden::timeT);

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

    /**
     * create menus and toolbars
     */
    virtual void setupActions();

    /**
     * create or re-initialise (after font change) the font size menu
     */
    virtual void setupFontSizeMenu(std::string oldFontName = "");

    /**
     * setup status bar
     */
    virtual void initStatusBar();

    /**
     * Place the staffs at the correct x & y coordinates
     */
    void positionStaffs();

    /**
     * setup the "zoom" toolbar
     */
    void initFontToolbar(int legatoUnit);

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

    /**
     * Return the time at which the insert cursor may be found.
     */
    Rosegarden::timeT getInsertionTime();

    /**
     * Return the time at which the insert cursor may be found,
     * and the time signature, clef and key at that time.
     */
    Rosegarden::timeT getInsertionTime(Rosegarden::Event *&clefEvt,
				       Rosegarden::Event *&keyEvt);

    void doDeferredCursorMove();

    void removeViewLocalProperties(Rosegarden::Event *);

    //--------------- Data members ---------------------------------

    NotationProperties m_properties;

    /// The current selection of Events (for cut/copy/paste)
    Rosegarden::EventSelection* m_currentEventSelection;

    Rosegarden::Quantizer *m_legatoQuantizer;

    /// Displayed in the status bar, holds the pixmap of the current note
    QLabel* m_currentNotePixmap;

    /// Displayed in the status bar, shows the pitch the cursor is at
    QLabel* m_hoveredOverNoteName;

    /// Displayed in the status bar, shows the absolute time the cursor is at
    QLabel* m_hoveredOverAbsoluteTime;

    std::vector<NotationStaff*> m_staffs;
    int m_currentStaff;
    int m_lastFinishingStaff;

    Rosegarden::timeT m_insertionTime;
    enum {
	NoCursorMoveNeeded,
	CursorMoveOnly,
	CursorMoveAndMakeVisible,
	CursorMoveAndScrollToPosition
    } m_deferredCursorMove;
    double m_deferredCursorScrollToX;

    Rosegarden::Accidental m_currentAccidental;

    std::string m_fontName;
    int m_fontSize;

    NotePixmapFactory *m_notePixmapFactory;
    NotePixmapFactory m_toolbarNotePixmapFactory;
    
    NotationHLayout m_hlayout;
    NotationVLayout m_vlayout;

    BarButtons *m_topBarButtons;
    BarButtons *m_bottomBarButtons;
    ChordNameRuler *m_chordNameRuler;
    QWidget *m_tempoRuler;
    bool m_chordNamesVisible;
    bool m_temposVisible;

    bool m_tupletMode;
    
    std::vector<int> m_legatoDurations;

    KAction* m_selectDefaultNote;

    typedef QMap<QString, NoteActionData> NoteActionDataMap;
    static NoteActionDataMap* m_noteActionDataMap;

    typedef QMap<QString, MarkActionData> MarkActionDataMap;
    static MarkActionDataMap *m_markActionDataMap;

    QComboBox *m_fontCombo;
    ZoomSlider<int> *m_fontSizeSlider;
    ZoomSlider<double> *m_spacingSlider;
    ZoomSlider<int> *m_smoothingSlider;
    KActionMenu *m_fontSizeActionMenu;

    RosegardenProgressDialog *m_progressDlg;
    bool m_inhibitRefresh;
    bool m_documentDestroyed;
};

#endif
