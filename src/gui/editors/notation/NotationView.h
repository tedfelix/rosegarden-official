/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTATION_VIEW_H
#define RG_NOTATION_VIEW_H

#include "gui/general/ActionFileClient.h"
#include "gui/general/SelectionManager.h"
#include "gui/general/EditViewBase.h"
#include "gui/widgets/ProgressBar.h"
#include "base/NotationTypes.h"
#include "base/Composition.h"

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>

#include <vector>

class QWidget;
class TestNotationViewSelection;

namespace Rosegarden
{

class Event;
class RosegardenDocument;
class NotationWidget;
class NotationElement;
class NotationStaff;
class Segment;
class CommandRegistry;
class ControlRulerWidget;
class ControlParameter;
class TriggerSegmentRec;

class ROSEGARDENPRIVATE_EXPORT NotationView : public EditViewBase,
                        public SelectionManager
{
    Q_OBJECT

public:
    typedef std::vector<Segment *> SegmentVector;
    typedef void (NotationView::*opOnEvent) (Event* e, Segment *containing);
    NotationView(const std::vector<Segment *>& segments);

    ~NotationView() override;

    void launchRulers(std::vector<Segment *> segments);

    Segment *getCurrentSegment() override;
    EventSelection *getSelection() const override;
    void setSelection(EventSelection* selection, bool preview = false) override;

    virtual void initLayoutToolbar();
    void initRulersToolbar();
    void initStatusBar();
    timeT getInsertionTime(bool allowEndTime = false) const;

    bool hasSegment(Segment * seg) const;

    // Adopt a segment that doesn't live in Composition.
    void adoptSegment(Segment *s);
    // Unadopt a segment that we previously adopted.
    void unadoptSegment(Segment *s);

    // Adopt a segment that does live in Composition.
    void adoptCompositionSegment(Segment *s);
    // Unadopt a segment that we previously adopted.
    void unadoptCompositionSegment(Segment *s);

signals:
    void play();
    void stop();
    void stepBackward();
    void stepForward();
    void rewindPlayback();
    void fastForwardPlayback();
    void rewindPlaybackToBeginning();
    void fastForwardPlaybackToEnd();
    void panic();
    void editTriggerSegment(int);
    void stepByStepTargetRequested(QObject *);

public slots:
    /// Note-on received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOnReceived(int pitch, int velocity);

    /// Note-off received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOffReceived(int pitch, int velocity);

    void slotStepByStepTargetRequested(QObject *);

protected slots:
    /// Some change occurs and the whole scene have to be redrawn.
    /// First remove segments from our list when they are deleted from the
    /// composition.
    void slotRegenerateScene();

    /// Update the window title during setup, and when document modified status
    /// changes
    void slotUpdateWindowTitle(bool m = false);

    /// Print with LilyPond (and lpr or the like)
    void slotPrintLilyPond();

    /// Preview with LilyPond (via Okular or the like)
    void slotPreviewLilyPond();

    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotEditDelete();
    void slotEditCutAndClose();
    void slotEditGeneralPaste();
    void slotEditAddClef();
    void slotEditAddClefLinkOnly();
    void slotEditAddKeySignature();
    void slotEditAddSustainDown();
    void slotEditAddSustainUp();
    void slotEditAddSustain(bool down);
    void slotEditTranspose();
    void slotEditSwitchPreset();

    void slotPreviewSelection();
    void slotClearLoop();
    void slotClearSelection();
    void slotEscapePressed();
    void slotEditSelectFromStart();
    void slotEditSelectToEnd();
    void slotEditSelectWholeStaff();
    void slotSearchSelect();
    void slotFilterSelection();
    void slotSelectEvenlySpacedNotes();
    void slotVelocityUp();
    void slotVelocityDown();
    void slotSetVelocities();
    void slotSetControllers();
    void slotPlaceControllers();

    void slotSetSelectTool();
    void slotSetSelectNoTiesTool();

    void slotSetEraseTool();

    /**
     * Restore NoteRestInserter as the current tool and recall its
     * state information.
     */
    void slotSetNoteRestInserter();

    void slotInsertNoteFromAction();
    void slotInsertRestFromAction();

    /**
     * Switch the NoteRestInserter to Note Insertion mode and update the gui.
     */
    void slotSwitchToNotes();

    /**
     * Switch the NoteRestInserter to Rest Insertion mode and update the gui.
     */
    void slotSwitchToRests();

    /**
     * Switch between dotted and plain variations on the current note or rest
     * duration being inserted (by whatever means insertion is ocurring)
     */
    void slotToggleDot();

    /**
     * Cycle through the dots from . to .. to _ back to ., relative to the point
     * of entry.
     */
    void slotAddDot();

    /**
     * Cycle through the dots from . to .. to _ back to ., relative to the point
     * of entry.  Do not change performance duration.  Change notation duration
     * only.  Maybe?  I have no idea, really, and almost dumped this function.
     */
    void slotAddDotNotationOnly();

    /**
     * Set notes of different types
     */
    void slotSetNoteType();
    void slotSetNoteTypeNotationOnly();

    /**
     * Process calls to insert a notes.
     */
    void slotNoteAction();
    void slotDummy1();
    void slotNoAccidental();
    void slotFollowAccidental();
    void slotSharp();
    void slotFlat();
    void slotNatural();
    void slotDoubleSharp();
    void slotDoubleFlat();
    void slotClefAction();
    void slotText();
    void slotGuitarChord();

    void slotLinearMode();
    void slotContinuousPageMode();
    void slotMultiPageMode();

    void slotHighlight();

    void slotShowHeadersGroup();

    void slotChangeFontFromAction();
    void slotChangeFontSizeFromAction();
    void slotChangeSpacingFromAction();

    void slotUpdateMenuStates();

    void slotTransformsNormalizeRests();
    void slotTransformsCollapseNotes();
    void slotTransformsQuantize();
    void slotTransformsInterpret();

    void slotMakeOrnament();
    void slotUseOrnament();
    void slotRemoveOrnament();
    void slotEditOrnamentInline();
    void slotShowOrnamentExpansion();
    void slotMaskOrnament();
    void slotUnmaskOrnament();
    void slotUnadoptSegment();

    void slotGroupSimpleTuplet();
    void slotGroupGeneralTuplet();
    void slotGroupTuplet(bool simple);
    void slotUpdateInsertModeStatus();
    void slotUpdateInsertModeStatusTriplet();
    void slotUpdateInsertModeStatusTuplet();
    void slotHalveDurations();
    void slotDoubleDurations();
    void slotRescale();
    void slotTransposeUp();
    void slotTransposeDown();
    void slotTransposeUpOctave();
    void slotTransposeDownOctave();
    void slotTranspose();
    void slotDiatonicTranspose();
    void slotInvert();
    void slotRetrograde();
    void slotRetrogradeInvert();
    void slotJogLeft();
    void slotJogRight();
    void slotEditLyrics();

    void slotStepBackward();
    void slotStepForward();

    void slotCurrentStaffUp();
    void slotCurrentStaffDown();
    void slotCurrentSegmentPrior();
    void slotCurrentSegmentNext();

    /// Show or hide rulers
    void slotToggleChordsRuler();
    void slotToggleRawNoteRuler();
    void slotToggleTempoRuler();

    void slotToggleVelocityRuler();
    void slotTogglePitchbendRuler();
    void slotToggleKeyPressureRuler();
    void slotToggleChannelPressureRuler();
    void slotAddControlRuler(QAction*);

    void slotAddTempo();
    void slotAddTimeSignature();

    void slotCheckForParallels();

    void slotToggleGeneralToolBar();
    void slotToggleToolsToolBar();
    void slotToggleDurationToolBar();
    void slotToggleInterpretToolBar();
    void slotToggleAccidentalsToolBar();
    void slotToggleClefsToolBar();
    void slotToggleMarksToolBar();
    void slotToggleGroupToolBar();
    void slotToggleSymbolsToolBar();
    void slotToggleLayoutToolBar();
    void slotToggleLayerToolBar();
    void slotToggleRulersToolBar();
    void slotToggleTransportToolBar();

    void slotScrollToFollow();

    void slotLoop();
    void slotLoopChanged();

    /// Note-on or note-off received asynchronously -- as above
    void slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn);

    void slotToggleStepByStep();

    /// YG: Only for debug
    void slotDebugDump();
    void slotBarDataDump();

    /**
     * Insert a Symbol
     */
    void slotSymbolAction();

    void slotMoveEventsUpStaffInteractive();
    void slotMoveEventsDownStaffInteractive();
    void slotMoveEventsUpStaff();
    void slotMoveEventsDownStaff();

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

    void slotShowContextHelp(const QString &);

    /** The old font size combo handling code was convoluted and unhappy in this
     * new home.  This is a new implementation that takes advantage of the fact
     * that someone already got the View -> Font Size menu working.
     */
    void slotFontComboChanged(int);
    void slotSizeComboChanged(int);
    void slotSpacingComboChanged(int);

    /** Decides what editor to open when the user double clicks on an event.
     * Triggered by the editElement() signal emitted by NotationSelector and
     * relayed through NotationWidget.
     */
    void slotEditElement(NotationStaff *, NotationElement *);

    void slotExtendSelectionBackward();
    void slotExtendSelectionForward();
    void slotExtendSelectionBackwardBar();
    void slotExtendSelectionForwardBar();
    void slotExtendSelectionBackward(bool bar);
    void slotExtendSelectionForward(bool bar);

    void slotHelp();
    void slotTutorial();
    void slotBugGuidelines();
    void slotHelpAbout();
    void slotHelpAboutQt();
    void slotDonate();

    void slotCycleSlashes();

    void slotAddLayer();
    void slotNewLayerFromSelection();

    void slotConfigure() override;

    // Open insert pitch bends sequence dialog
    void slotExpressionSequence();  // Controllers > Insert Expression Controller Sequence...
    void slotPitchBendSequence();   // Controllers > Insert Pitch Bend Sequence...
    void slotControllerSequence();  // Controllers > Insert Controller Sequence...

    // Update the "Show staff headers" check box in the menu
    void slotCheckShowHeadersMenu(bool checked);

    /// Select everything in the active segment and run interpret according to
    // the checked options on the toolbar
    void slotInterpretActivate();

    void slotRulerSelectionUpdate();

private:
    friend class ::TestNotationViewSelection;
    /**
     * export a LilyPond file (used by slotPrintLilyPond and
     * slotPreviewLilyPond)
     */
    bool exportLilyPondFile(QString file, bool forPreview = false);

    /**
     * Use QTemporaryFile to obtain a tmp filename that is guaranteed to be unique.
     */
    QString getLilyPondTmpFilename();

    int getPitchFromNoteInsertAction(QString name,
                                     Accidental &accidental,
                                     const Clef &clef,
                                     const Rosegarden::Key &key);

    /**
     * Helper function to toggle a toolbar given its name
     * If \a force point to a bool, then the bool's value
     * is used to show/hide the toolbar.
     */
    void toggleNamedToolBar(const QString& toolBarName, bool* force = nullptr);

    /**
     * Return the device of the current segment, if any
     */
    Device *getCurrentDevice();

    void generalMoveEventsToStaff(bool upStaff, bool useDialog);

    /**
     * The DurationMonobar needs to know how to convolute itself somehow to
     * morph into what used to be separate toolbars in a cleaner and simpler
     * world.
     */
    typedef enum { InsertingNotes,
                   InsertingDottedNotes,
                   InsertingRests,
                   InsertingDottedRests
                 } DurationMonobarModeType;

    /**
     * Contort the DurationMonobar with a long and complicated series of hide and
     * show operations that pretty much make my stomach churn.
     *
     * \p mode is one of InsertingNotes, InsertingDottedNotes, InsertingRests,
     * etc. (see the typedef DurationMonobarModeType for a complete list)
     */
    void morphDurationMonobar();

    /**
     * Initialize NoteRestInserter and Duration Tooolbar.
     * This is done here since we are certain to have access
     * to the getdocument() and the TimeSignature.
     */
     void initializeNoteRestInserter();

    /**
     * Manage the setting of the accidental modes.
     * Function enforces exclusive state of buttons and triggers
     * SetNoteRestInserter if not currently in Note/Rest mode.
     */
     void manageAccidentalAction(QString actionName);

    /** Curiously enough, the window geometry code never fired in the dtor.  I
     * can only conclude the dtor is never being called for some reason, and
     * it's probably a memory leak for the command registry object it wants to
     * delete, but I won't try to work that one out at the moment.  I'll just
     * implement closeEvent() and whistle right on past that other thing.
     */
    void closeEvent(QCloseEvent *event) override;
    void setupActions();
    bool isInChordMode();
    bool isInTripletMode();
    bool isInTupletMode();
    bool isInGraceMode();

    void setCurrentNotePixmap(QPixmap);
    void setCurrentNotePixmapFrom(QAction *);

    void insertControllerSequence(const ControlParameter &controlParameter);
    bool isShowable(Event *e);
    void setWidgetSegments();
    void EditOrnamentInline(Event *trigger, Segment *containing);
    void ShowOrnamentExpansion(Event *trigger, Segment *containing);
    SegmentVector::iterator findAdopted(Segment *s);
    void ForAllSelection(opOnEvent op);
    void setCurrentStaff(NotationStaff *staff);

    void readOptions();

// FIXME: likely to be debated. --gp     Used for subclassing in pitchtracker
protected:
    NotationWidget *m_notationWidget;
    EventSelection *getRulerSelection() const;

private:
    void extendSelectionHelper(bool forward, EventSelection *es, const std::vector<Event *> &eventVec, bool select);

    CommandRegistry *m_commandRegistry;
    DurationMonobarModeType m_durationMode;  // Stores morph state.
    QAction *m_durationPressed;  //Store the last duration button pressed.
    QAction *m_accidentalPressed;  //Store the last accidental button pressed.

    /// Displayed in the status bar, shows number of events selected
    QLabel *m_selectionCounter;

    /// Displayed in the status bar, shows insertion mode
    QLabel *m_insertModeLabel;

    /// Displayed in the status bar, shows when annotations are hidden
    QLabel *m_annotationsLabel;

    /// Displayed in the status bar, shows when LilyPond directives are hidden
    QLabel *m_lilyPondDirectivesLabel;

    /// Displayed in the status bar, holds the pixmap of the current note
    QLabel* m_currentNotePixmap;

    /// Displayed in the status bar, shows the pitch the cursor is at
    QLabel* m_hoveredOverNoteName;

    /// Displayed in the status bar, shows the absolute time the cursor is at
    QLabel* m_hoveredOverAbsoluteTime;

    QComboBox       *m_fontCombo;
    QComboBox       *m_fontSizeCombo;
    QComboBox       *m_spacingCombo;
    QString          m_fontName;
    int              m_fontSize;

    bool m_Thorn;

    std::vector<QString> m_availableFontNames;
    std::vector<int>     m_availableFontSizes;
    std::vector<int>     m_availableSpacings;

    // These Segments are not in Composition, they are dummies for
    // viewing a triggered segment's expansion.
    SegmentVector      m_adoptedSegments;    // I own these

    timeT m_oldPointerPosition;
    timeT m_cursorPosition;

    timeT m_currentNoteDuration;
};

}

#endif
