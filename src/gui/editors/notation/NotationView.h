/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NEW_NOTATION_VIEW_H_
#define _RG_NEW_NOTATION_VIEW_H_

#include "gui/general/ActionFileClient.h"
#include "gui/general/SelectionManager.h"
#include "gui/general/EditViewBase.h"
#include "gui/widgets/ProgressBar.h"
#include "base/NotationTypes.h"

#include <QMainWindow>
#include <QLabel>

#include <vector>

class QWidget;

namespace Rosegarden
{

class RosegardenDocument;
class NotationWidget;
class Segment;
class CommandRegistry;
	
class NewNotationView : public EditViewBase,
                        public SelectionManager
{
    Q_OBJECT

public:
    NewNotationView(RosegardenDocument *doc,
                    std::vector<Segment *> segments,
                    QWidget *parent = 0);

    virtual ~NewNotationView();

    virtual Segment *getCurrentSegment();
    virtual EventSelection *getSelection() const;
    virtual void setSelection(EventSelection* s, bool preview = false);

    virtual void initStatusBar();
    virtual void updateViewCaption() { }//!!!

    virtual timeT getInsertionTime() const;

signals:
    void play();
    void stop();
    void rewindPlayback();
    void fastForwardPlayback();
    void rewindPlaybackToBeginning();
    void fastForwardPlaybackToEnd();
    void panic();


protected slots:
    void slotPrintLilyPond();
    void slotPreviewLilyPond();
    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotEditDelete();
    void slotEditCutAndClose();
    void slotEditGeneralPaste();
    void slotEditAddClef();
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
    void slotFilterSelection();
    void slotVelocityUp();
    void slotVelocityDown();
    void slotSetVelocities();

    void slotSetSelectTool();
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
     * Process calls to insert a notes.
     */
    void slotNoteAction();
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

    void slotUpdateMenuStates();

    void slotTransformsQuantize();
    void slotTransformsInterpret();

    void slotMakeOrnament();
    void slotUseOrnament();
    void slotRemoveOrnament();

    void slotGroupSimpleTuplet();
    void slotGroupGeneralTuplet();
    void slotGroupTuplet(bool simple);
    void slotUpdateInsertModeStatus();
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

    /// Show or hide rulers
    void slotToggleChordsRuler();
    void slotToggleRawNoteRuler();
    void slotToggleTempoRuler();

    void slotAddTempo();
    void slotAddTimeSignature();

    void slotToggleGeneralToolBar();
    void slotToggleToolsToolBar();
    void slotToggleDurationToolBar();
    void slotToggleAccidentalsToolBar();
    void slotToggleClefsToolBar();
    void slotToggleMarksToolBar();
    void slotToggleGroupToolBar();
    void slotToggleSymbolsToolBar();
    void slotToggleLayoutToolBar();
    void slotToggleTransportToolBar();

    void slotToggleTracking();

    /// Note-on received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOnReceived(int pitch, int velocity);

    /// Note-off received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOffReceived(int pitch, int velocity);

    /// Note-on or note-off received asynchronously -- as above
    void slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn);

    /**
     * Insert a Symbol
     */
    void slotSymbolAction();

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

    /**
     * Get the average velocity of the selected notes
     */
    int getVelocityFromSelection();


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
    void updateWindowTitle();
    bool isInChordMode();
    bool isInTripletMode();
    bool isInGraceMode();

    RosegardenDocument *m_document;
    NotationWidget *m_notationWidget;
    CommandRegistry *m_commandRegistry;
    DurationMonobarModeType m_durationMode;  // Stores morph state.
    QAction *m_durationPressed;  //Store the last duration button pressed.
    QAction *m_accidentalPressed;  //Store the last accidental button pressed.

//    NotationProperties m_properties;

    /// Displayed in the status bar, shows number of events selected
    QLabel *m_selectionCounter;

    /// Displayed in the status bar, shows insertion mode
    QLabel *m_insertModeLabel;

    /// Displayed in the status bar, shows when annotations are hidden
    QLabel *m_annotationsLabel;

    /// Displayed in the status bar, shows when LilyPond directives are hidden
    QLabel *m_lilyPondDirectivesLabel;

    /// Displayed in the status bar, shows value() of current operation
    ProgressBar *m_progressBar;

    /// Displayed in the status bar, holds the pixmap of the current note
    QLabel* m_currentNotePixmap;

    /// Displayed in the status bar, shows the pitch the cursor is at
    QLabel* m_hoveredOverNoteName;

    /// Displayed in the status bar, shows the absolute time the cursor is at
    QLabel* m_hoveredOverAbsoluteTime;
};

}

#endif
