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

/// Top-level window containing the matrix editor.
/**
 * This class manages menus and toolbars, and provides implementations
 * of any functions carried out from menu and toolbar actions.  It
 * does not manage the editing tools (MatrixWidget does this) or the
 * selection state (MatrixScene does that).
 *
 * This class creates and owns the MatrixWidget instance (m_matrixWidget).
 */
class MatrixView : public EditViewBase,
                   public SelectionManager
{
    Q_OBJECT

public:
    MatrixView(RosegardenDocument *doc,
               const std::vector<Segment *>& segments,
               bool drumMode);

    ~MatrixView() override;

    /// To allow clients to launch the rulers after the view is up.
    void launchRulers(std::vector<Segment *> segments);

    void closeEvent(QCloseEvent *event) override;

    /**
     * Get the velocity currently set in the velocity menu.
     */
    int getCurrentVelocity() const;

    Segment *getCurrentSegment() override;

    void setSelection(EventSelection *s, bool preview) override;
    EventSelection *getSelection() const override;
    EventSelection *getRulerSelection() const;

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

    void noteInsertedFromKeyboard(Segment * segment, int pitch);

public slots:
    /// Note-on received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOnReceived(int pitch, int velocity);

    /// Note-off received asynchronously -- consider step-by-step editing
    void slotInsertableNoteOffReceived(int pitch, int velocity);

    void slotStepByStepTargetRequested(QObject *);

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
    void slotSetControllers();
    void slotPlaceControllers();

    void slotTriggerSegment();
    void slotRemoveTriggers();
    void slotSelectAll();
    void slotPreviewSelection();
    void slotClearLoop();
    void slotClearSelection();
    void slotEscapePressed();
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
    /**
     * ??? This is never used as a slot.  Move to private and rename.
     */
    void slotSetSnap(timeT);

    /// Quantize a selection to a given level (when quantize combo changes)
    void slotQuantizeSelection(int);

    /// Set the velocity menu to the given value
    void slotSetCurrentVelocity(int);
    void slotSetCurrentVelocityFromSelection();

    void slotScrollToFollow();
    void slotLoop();
    void slotLoopChanged();

    void slotUpdateMenuStates();
    void slotRulerSelectionUpdate();

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

    // view
    void slotShowNames();
    void slotHighlight();

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
    void slotStepForward()  { stepForward(false); }

    void slotExtendSelectionBackward();
    void slotExtendSelectionForward();
    void slotExtendSelectionBackwardBar();
    void slotExtendSelectionForwardBar();
    void slotExtendSelectionBackward(bool bar);
    void slotExtendSelectionForward(bool bar);

    /// keyboard insert
    void slotInsertNoteFromAction();

    /// Note-on or note-off received asynchronously -- as above
    void slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn);

    void slotPitchBendSequence();
    void slotControllerSequence();

    void slotConstrainedMove();
    void slotToggleStepByStep();

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
    void insertControllerSequence(const ControlParameter &controlParameter);

private:
    RosegardenDocument *m_document;
    MatrixWidget *m_matrixWidget;
    QSharedPointer<CommandRegistry> m_commandRegistry;

    QComboBox *m_velocityCombo;
    QComboBox *m_quantizeCombo;
    QComboBox *m_snapGridCombo;

    bool m_scrollToFollow;

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
    void toggleNamedToolBar(const QString& toolBarName, bool* force = nullptr);

    /**
     * Return the device of the current segment, if any
     */
    Device *getCurrentDevice();

    void initStatusBar();

    void readOptions();

    void stepForward(bool force);

};

}

#endif
