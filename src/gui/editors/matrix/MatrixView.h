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

#ifndef _RG_NEW_MATRIX_VIEW_H_
#define _RG_NEW_MATRIX_VIEW_H_

#include "base/Event.h"

#include "gui/general/ActionFileClient.h"
#include "gui/general/SelectionManager.h"
#include "gui/general/EditViewBase.h"
#include "gui/widgets/ZoomSlider.h"
#include "gui/widgets/DeferScrollArea.h"

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

/**
 * NewMatrixView is the top-level window containing the matrix editor.
 * This class manages menus and toolbars, and provides implementations
 * of any functions carried out from menu and toolbar actions.  It
 * does not manage the editing tools (MatrixWidget does this) or the
 * selection state (MatrixScene does that).
 */
class NewMatrixView : public EditViewBase,
                      public SelectionManager
{
    Q_OBJECT

public:
    NewMatrixView(RosegardenDocument *doc,
		  std::vector<Segment *> segments,
		  bool drumMode,
		  QWidget *parent = 0);

    virtual ~NewMatrixView();

    void closeEvent(QCloseEvent *event);

    /**
     * Get the velocity currently set in the velocity menu.
     */
    int getCurrentVelocity() const;

    virtual Segment *getCurrentSegment();
    virtual EventSelection *getSelection() const;
    virtual void setSelection(EventSelection *s, bool preview);

    virtual void initStatusBar() { }//!!!
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


protected slots:
    void slotChangeHorizontalZoom(int);

    void slotQuantize();
    void slotRepeatQuantize();
    void slotCollapseNotes();
    void slotLegato();
    void slotVelocityUp();
    void slotVelocityDown();
    void slotSetVelocities();
    void slotSetVelocitiesToCurrent();
    void slotTriggerSegment();
    void slotRemoveTriggers();
    void slotSelectAll();
    void slotPreviewSelection();
    void slotClearLoop();
    void slotClearSelection();
    void slotFilterSelection();

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

    void slotEditCut();
    void slotEditCopy();
    void slotEditPaste();
    void slotEditDelete();

    /// Show or hide rulers
    void slotToggleChordsRuler();
    void slotToggleTempoRuler();

    void slotToggleVelocityRuler();
    void slotTogglePitchbendRuler();
    void slotAddControlRuler();
    
    /**
     * Call the Rosegaden about box.
     */
    void slotHelpAbout();

        
    // start of slots, formerly in located in EditView.h (which is obsolete now)
    // --
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
//     void slotJogLeft();
//     void slotJogRight();
    
    // --
    // end of slots, formerly in located in EditView.h (which is obsolete now)
    
    
    
protected:
    const SnapGrid *getSnapGrid() const;

private:
    RosegardenDocument *m_document;
    MatrixWidget *m_matrixWidget;
    CommandRegistry *m_commandRegistry;

    ZoomSlider<double> *m_hZoomSlider;
    QLabel *m_zoomLabel;

    QComboBox *m_velocityCombo;
    QComboBox *m_quantizeCombo;
    QComboBox *m_snapGridCombo;

    bool m_tracking;

    std::vector<timeT> m_quantizations;
    std::vector<timeT> m_snapValues;

    void updateWindowTitle();
    void setupActions();
    void initZoomToolbar();
    void initActionsToolbar();
    void initRulersToolbar();

    bool m_drumMode;
};

}

#endif
