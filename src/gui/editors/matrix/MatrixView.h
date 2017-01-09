/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NEW_MATRIX_VIEW_H
#define RG_NEW_MATRIX_VIEW_H

#include "base/Event.h"
#include "base/NotationTypes.h"

#include "gui/general/ActionFileClient.h"
#include "gui/general/SelectionManager.h"
#include "gui/general/EditViewBase.h"
#include "gui/widgets/DeferScrollArea.h"
#include "gui/dialogs/TempoDialog.h"

#include <QMainWindow>

#include <vector>

class QWidget;
class QLabel;
class QComboBox;

namespace Rosegarden
{

class RosegardenDocument;
class MatrixWidget;
class Segment;
class CommandRegistry;
class EventSelection;
class SnapGrid;
class Device;
class ControlRulerWidget;
class ControlParameter;

/**
 * MatrixView is the top-level window containing the matrix editor.
 * This class manages menus and toolbars, and provides implementations
 * of any functions carried out from menu and toolbar actions.  It
 * does not manage the editing tools (MatrixWidget does this) or the
 * selection state (MatrixScene does that).
 */
class MatrixView : public EditViewBase,
                   public SelectionManager
{
    Q_OBJECT

public:
    MatrixView(RosegardenDocument *doc,
		  std::vector<Segment *> segments,
		  bool drumMode,
		  QWidget *parent = 0);

    virtual ~MatrixView();

    void closeEvent(QCloseEvent *event);

    /**
     * Get the velocity currently set in the velocity menu.
     */
    int getCurrentVelocity() const;

    virtual Segment *getCurrentSegment();
    virtual EventSelection *getSelection() const;
    virtual void setSelection(EventSelection *s, bool preview);

    virtual void updateViewCaption() { }//!!!

    virtual timeT getInsertionTime() const;

signals:
    void editTriggerSegment(int);
    void play();
    void stop();
    void rewindPlayback();
    void fastForwardPlayback();
    void rewindPlaybackToBeginning();
    void fastForwardPlaybackToEnd();
    void panic();

    void stepByStepTargetRequested(QObject *);
    void changeTempo(timeT,  // tempo change time
                     tempoT,  // tempo value
                     tempoT,  // target value
                     TempoDialog::TempoDialogAction); // tempo action
    
    void noteInsertedFromKeyboard(Segment * segment, int pitch);

protected slots:
    /// Remove a segment from our list when it is deleted from the composition
    void slotSegmentDeleted(Segment *);

    /// All segments have been deleted (close editor)
    void slotSceneDeleted();

    void slotQuantize();
    void slotRepeatQuantize();
    void slotCollapseNotes();
    void slotLegato();
    void slotVelocityUp();
    void slotVelocityDown();
    void slotSetVelocities();
    void slotSetVelocitiesToCurrent();
    void slotEditCutControllers();
    void slotEditCopyControllers();
    void slotSetControllers();
    void slotPlaceControllers();
    
    void slotTriggerSegment();
    void slotRemoveTriggers();
    void slotSelectAll();
    void slotPreviewSelection();
    void slotClearLoop();
    void slotClearSelection();
    void slotFilterSelection();
    void slotEditAddKeySignature();

    void slotCurrentSegmentPrior();
    void slotCurrentSegmentNext();

    void slotSetPaintTool();
    void slotSetEraseTool();
    void slotSetSelectTool();
    void slotSetMoveTool();
    void slotSetResizeTool();
    void slotSetVelocityTool();

    /// Set the snaptime of the grid from an item in the snap combo
    void slotSetSnapFromIndex(int);

    /// Set the snaptime of the grid based on the name of the invoking action
    void slotSetSnapFromAction();

    /// Set the snaptime of the grid
    void slotSetSnap(timeT);

    /// Quantize a selection to a given level (when quantize combo changes)
    void slotQuantizeSelection(int);

    /// Set the velocity menu to the given value
    void slotSetCurrentVelocity(int);
    void slotSetCurrentVelocityFromSelection();

    void slotToggleTracking();

    void slotUpdateMenuStates();

    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotEditDelete();

    /// Show or hide rulers
    void slotToggleChordsRuler();
    void slotToggleTempoRuler();

    void slotToggleVelocityRuler();
    void slotTogglePitchbendRuler();
    void slotAddControlRuler(QAction*);
    
    /**
     * Call the Rosegaden about box.
     */
    void slotHelp();
    void slotTutorial();
    void slotBugGuidelines();
    void slotHelpAbout();
    void slotHelpAboutQt();
    void slotDonate();

    void slotShowContextHelp(const QString &);
        
    void slotAddTempo();
    void slotAddTimeSignature();
    
    // rescale
    void slotHalveDurations();
    void slotDoubleDurations();
    void slotRescale();

    // transpose
    void slotTransposeUp();
    void slotTransposeUpOctave();
    void slotTransposeDown();
    void slotTransposeDownOctave();
    void slotTranspose();
    void slotDiatonicTranspose();

    // invert
    void slotInvert();
    void slotRetrograde();
    void slotRetrogradeInvert();

    // jog events
    void slotJogLeft();
    void slotJogRight();
    
    void slotStepBackward();
    void slotStepForward(bool force = false);

    void slotExtendSelectionBackward();
    void slotExtendSelectionForward();
    void slotExtendSelectionBackwardBar();
    void slotExtendSelectionForwardBar();
    void slotExtendSelectionBackward(bool bar);
    void slotExtendSelectionForward(bool bar);

    /// keyboard insert
    void slotInsertNoteFromAction();

    /// Note-on received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOnReceived(int pitch, int velocity);

    /// Note-off received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOffReceived(int pitch, int velocity);

    /// Note-on or note-off received asynchronously -- as above
    void slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn);

    void slotPitchBendSequence();
    void slotControllerSequence();
    
    void slotToggleStepByStep();
    void slotStepByStepTargetRequested(QObject *);

    /** Update the window title.  If m is true (normally comes from a signal)
     * display a * at the extreme left of the title to indicate modified status
     */ 
    void slotUpdateWindowTitle(bool m = false);

    void slotToggleChordMode();

    void slotToggleGeneralToolBar();
    void slotToggleToolsToolBar();
    void slotToggleActionsToolBar();
    void slotToggleRulersToolBar();
    void slotToggleTransportToolBar();
    
protected:
    const SnapGrid *getSnapGrid() const;
    virtual void readOptions();
    void conformRulerSelectionState(void);
    void insertControllerSequence(const ControlParameter &cp);

private:
    RosegardenDocument *m_document;
    MatrixWidget *m_matrixWidget;
    CommandRegistry *m_commandRegistry;

    QComboBox *m_velocityCombo;
    QComboBox *m_quantizeCombo;
    QComboBox *m_snapGridCombo;

    bool m_tracking;

    std::vector<timeT> m_quantizations;
    std::vector<timeT> m_snapValues;

    void setupActions();
    void initActionsToolbar();
    void initRulersToolbar();

    bool m_drumMode;
    bool m_Thorn;
    bool m_inChordMode;

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

    virtual void initStatusBar();

    /**
     * Set the <<< << >> >>> buttons in the transport toolbar to auto repeat
     */
    void setRewFFwdToAutoRepeat();

};

}

#endif
