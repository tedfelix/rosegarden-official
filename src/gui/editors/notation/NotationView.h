/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
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
#include "gui/dialogs/TempoDialog.h"
#include "base/NotationTypes.h"
#include "base/Composition.h"

#include <QMainWindow>
#include <QLabel>
#include <QComboBox>

#include <vector>

class QWidget;
class Event;

namespace Rosegarden
{

class RosegardenDocument;
class NotationWidget;
class NotationElement;
class NotationStaff;
class Segment;
class CommandRegistry;
class ControlRulerWidget;
class ControlParameter;
class TriggerSegmentRec;
 
class NotationView : public EditViewBase,
                        public SelectionManager
{
    Q_OBJECT

public:
    typedef std::vector<Segment *> SegmentVector;
    typedef void (NotationView::*opOnEvent) (Event* e, Segment *containing);
    NotationView(RosegardenDocument *doc,
                    std::vector<Segment *> segments,
                    QWidget *parent = 0);

    virtual ~NotationView();

    virtual Segment *getCurrentSegment();
    virtual EventSelection *getSelection() const;
    virtual void setSelection(EventSelection* s, bool preview = false);

    virtual void initLayoutToolbar();
    void initRulersToolbar();
    virtual void initStatusBar();
    virtual timeT getInsertionTime() const;
    
    bool hasSegment(Segment * seg) const;

    /** This turns out to be cruft that is rather annoying to eliminate.  We
     * don't use this for anything, and provide an empty implementation.
     */
    virtual void updateViewCaption() { }

    // Adopt a segment that doesn't live in Composition.
    void adoptSegment(Segment *s);
    // Unadopt a segment that we previously adopted.
    void unadoptSegment(Segment *s);

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
    void changeTempo(timeT,  // tempo change time
                     tempoT,  // tempo value
                     tempoT,  // target value
                     TempoDialog::TempoDialogAction); // tempo action

protected:
    virtual void readOptions();

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
    void slotEditSelectFromStart();
    void slotEditSelectToEnd();
    void slotEditSelectWholeStaff();
    void slotSearchSelect();
    void slotFilterSelection();
    void slotSelectEvenlySpacedNotes();
    void slotVelocityUp();
    void slotVelocityDown();
    void slotSetVelocities();
    void slotEditCutControllers();
    void slotEditCopyControllers();
    void slotSetControllers();
    void slotPlaceControllers();

    void slotSetSelectTool();
    void slotSetSelectNoTiesTool(void);

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

    void slotToggleTracking();

    /// Note-on received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOnReceived(int pitch, int velocity);

    /// Note-off received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOffReceived(int pitch, int velocity);

    /// Note-on or note-off received asynchronously -- as above
    void slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn);

    void slotToggleStepByStep();
    void slotStepByStepTargetRequested(QObject *);

    /// YG: Only for debug
    void slotDebugDump();
    void slotBarDataDump();

    /**
     * Insert a Symbol
     */
    void slotSymbolAction();

    void slotMoveEventsUpStaffInteractive(void);
    void slotMoveEventsDownStaffInteractive(void);
    void slotMoveEventsUpStaff(void);
    void slotMoveEventsDownStaff(void);

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
    void slotEditElement(NotationStaff *, NotationElement *, bool advanced);

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

    virtual void slotConfigure();

    // Open insert pitch bends sequence dialog
    void slotExpressionSequence();
    void slotPitchBendSequence();
    void slotControllerSequence();

    // Update the "Show staff headers" check box in the menu
    void slotCheckShowHeadersMenu(bool checked);

    /// Select everything in the active segment and run interpret according to
    // the checked options on the toolbar
    void slotInterpretActivate();

private:
    /**
     * export a LilyPond file (used by slotPrintLilyPond and
     * slotPreviewLilyPond)
     */
    bool exportLilyPondFile(QString url, bool forPreview = false);

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
    void toggleNamedToolBar(const QString& toolBarName, bool* force = 0);

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
    void closeEvent(QCloseEvent *event);
    void setupActions();
    bool isInChordMode();
    bool isInTripletMode();
    bool isInTupletMode();
    bool isInGraceMode();

    void setCurrentNotePixmap(QPixmap);
    void setCurrentNotePixmapFrom(QAction *);

    void conformRulerSelectionState(void);
    void insertControllerSequence(const ControlParameter &cp);
    bool isShowable(Event *e);
    void setWidgetSegments(void);
    void EditOrnamentInline(Event *trigger, Segment *containing);
    void ShowOrnamentExpansion(Event *trigger, Segment *containing);
    SegmentVector::iterator findAdopted(Segment *s);
    void ForAllSelection(opOnEvent op);
    void setCurrentStaff(NotationStaff *staff);

// FIXME: likely to be debated. --gp     Used for subclassing in pitchtracker
protected:
    // !!! Duplicates m_doc in base class
    RosegardenDocument *m_document;
    NotationWidget *m_notationWidget;
    
private:
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
    int              m_spacing;

    bool m_Thorn;

    std::vector<QString> m_availableFontNames;
    std::vector<int>     m_availableFontSizes;
    std::vector<int>     m_availableSpacings;

    // !!! Is m_segments ever different than m_segments in base class?
    SegmentVector      m_segments;      // I do not own these
    // These Segments are not in Composition, they are dummies for
    // viewing a triggered segment's expansion.
    SegmentVector      m_adoptedSegments;    // I own these

    /**
     * Set the <<< << >> >>> buttons in the transport toolbar to auto repeat
     */
    void setRewFFwdToAutoRepeat();

};

}

#endif
