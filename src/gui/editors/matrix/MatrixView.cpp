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

#include "MatrixView.h"

#include "document/RosegardenDocument.h"
#include "MatrixWidget.h"
#include "base/SnapGrid.h"
#include "base/Clipboard.h"

#include <QWidget>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QLabel>
#include <QToolBar>
#include <QSettings>
#include <QComboBox>

#include "misc/Debug.h"
#include "document/ConfigGroups.h"
#include "document/CommandHistory.h"

#include "gui/dialogs/QuantizeDialog.h"
#include "gui/dialogs/EventFilterDialog.h"
#include "gui/dialogs/EventParameterDialog.h"
#include "gui/dialogs/TriggerSegmentDialog.h"

#include "commands/edit/ChangeVelocityCommand.h"
#include "commands/edit/ClearTriggersCommand.h"
#include "commands/edit/CollapseNotesCommand.h"
#include "commands/edit/CopyCommand.h"
#include "commands/edit/CutCommand.h"
#include "commands/edit/EraseCommand.h"
#include "commands/edit/EventQuantizeCommand.h"
#include "commands/edit/EventUnquantizeCommand.h"
#include "commands/edit/PasteEventsCommand.h"
#include "commands/edit/SelectionPropertyCommand.h"
#include "commands/edit/SetTriggerCommand.h"

#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"

#include "base/Quantizer.h"
#include "base/BasicQuantizer.h"
#include "base/LegatoQuantizer.h"
#include "base/BaseProperties.h"

namespace Rosegarden
{

NewMatrixView::NewMatrixView(RosegardenDocument *doc,
			     std::vector<Segment *> segments,
			     bool drumMode,
			     QWidget *parent) :
    EditViewBase(doc, segments, parent),
    m_quantizations(BasicQuantizer::getStandardQuantizations())
{
    m_document = doc;
    m_matrixWidget = new MatrixWidget(drumMode);
    setCentralWidget(m_matrixWidget);
    m_matrixWidget->setSegments(doc, segments);

    setupActions();

    createGUI("matrix.rc");

    initActionsToolbar();
    initZoomToolbar();

    connect(m_matrixWidget, SIGNAL(editTriggerSegment(int)),
            this, SIGNAL(editTriggerSegment(int)));

    if (!m_matrixWidget->segmentsContainNotes()) {
        findAction("draw")->trigger();
    } else {
        findAction("select")->trigger();
    }
}

NewMatrixView::~NewMatrixView()
{
}

void
NewMatrixView::setupActions()
{
    EditViewBase::setupActions(true);

    createAction("select", SLOT(slotSetSelectTool()));
    createAction("draw", SLOT(slotSetPaintTool()));
    createAction("erase", SLOT(slotSetEraseTool()));
    createAction("move", SLOT(slotSetMoveTool()));
    createAction("resize", SLOT(slotSetResizeTool()));
    createAction("velocity", SLOT(slotSetVelocityTool()));
    createAction("chord_mode", SLOT(slotUpdateInsertModeStatus()));
    createAction("toggle_step_by_step", SLOT(slotToggleStepByStep()));
    createAction("quantize", SLOT(slotQuantize()));
    createAction("repeat_quantize", SLOT(slotRepeatQuantize()));
    createAction("collapse_notes", SLOT(slotCollapseNotes()));
    createAction("legatoize", SLOT(slotLegato()));
    createAction("velocity_up", SLOT(slotVelocityUp()));
    createAction("velocity_down", SLOT(slotVelocityDown()));
    createAction("set_to_current_velocity", SLOT(slotSetVelocitiesToCurrent()));
    createAction("set_velocities", SLOT(slotSetVelocities()));
    createAction("trigger_segment", SLOT(slotTriggerSegment()));
    createAction("remove_trigger", SLOT(slotRemoveTriggers()));
    createAction("select_all", SLOT(slotSelectAll()));
    createAction("delete", SLOT(slotEditDelete()));
    createAction("cursor_back", SLOT(slotStepBackward()));
    createAction("cursor_forward", SLOT(slotStepForward()));
    createAction("cursor_back_bar", SLOT(slotJumpBackward()));
    createAction("cursor_forward_bar", SLOT(slotJumpForward()));
    createAction("extend_selection_backward", SLOT(slotExtendSelectionBackward()));
    createAction("extend_selection_forward", SLOT(slotExtendSelectionForward()));
    createAction("extend_selection_backward_bar", SLOT(slotExtendSelectionBackwardBar()));
    createAction("extend_selection_forward_bar", SLOT(slotExtendSelectionForwardBar()));
    createAction("cursor_start", SLOT(slotJumpToStart()));
    createAction("cursor_end", SLOT(slotJumpToEnd()));
    createAction("cursor_to_playback_pointer", SLOT(slotJumpCursorToPlayback()));
    //&&& NB Play has two shortcuts (Enter and Ctrl+Return) -- need to
    // ensure both get carried across somehow
    createAction("play", SIGNAL(play()));
    createAction("stop", SIGNAL(stop()));
    createAction("playback_pointer_back_bar", SIGNAL(rewindPlayback()));
    createAction("playback_pointer_forward_bar", SIGNAL(fastForwardPlayback()));
    createAction("playback_pointer_start", SIGNAL(rewindPlaybackToBeginning()));
    createAction("playback_pointer_end", SIGNAL(fastForwardPlaybackToEnd()));
    createAction("playback_pointer_to_cursor", SLOT(slotJumpPlaybackToCursor()));
    createAction("toggle_solo", SLOT(slotToggleSolo()));
    createAction("toggle_tracking", SLOT(slotToggleTracking()));
    createAction("panic", SIGNAL(panic()));
    createAction("preview_selection", SLOT(slotPreviewSelection()));
    createAction("clear_loop", SLOT(slotClearLoop()));
    createAction("clear_selection", SLOT(slotClearSelection()));
    createAction("filter_selection", SLOT(slotFilterSelection()));

    timeT crotchetDuration = Note(Note::Crotchet).getDuration();
    m_snapValues.clear();
    m_snapValues.push_back(SnapGrid::NoSnap);
    m_snapValues.push_back(SnapGrid::SnapToUnit);
    m_snapValues.push_back(crotchetDuration / 16);
    m_snapValues.push_back(crotchetDuration / 12);
    m_snapValues.push_back(crotchetDuration / 8);
    m_snapValues.push_back(crotchetDuration / 6);
    m_snapValues.push_back(crotchetDuration / 4);
    m_snapValues.push_back(crotchetDuration / 3);
    m_snapValues.push_back(crotchetDuration / 2);
    m_snapValues.push_back((crotchetDuration * 3) / 4);
    m_snapValues.push_back(crotchetDuration);
    m_snapValues.push_back((crotchetDuration * 3) / 2);
    m_snapValues.push_back(crotchetDuration * 2);
    m_snapValues.push_back(SnapGrid::SnapToBeat);
    m_snapValues.push_back(SnapGrid::SnapToBar);

    for (unsigned int i = 0; i < m_snapValues.size(); i++) {

        timeT d = m_snapValues[i];

        if (d == SnapGrid::NoSnap) {
            createAction("snap_none", SLOT(slotSetSnapFromAction()));
        } else if (d == SnapGrid::SnapToUnit) {
        } else if (d == SnapGrid::SnapToBeat) {
            createAction("snap_beat", SLOT(slotSetSnapFromAction()));
        } else if (d == SnapGrid::SnapToBar) {
            createAction("snap_bar", SLOT(slotSetSnapFromAction()));
        } else {
            QString actionName = QString("snap_%1").arg(int((crotchetDuration * 4) / d));
            if (d == (crotchetDuration * 3) / 4) actionName = "snap_dotted_8";
            if (d == (crotchetDuration * 3) / 2) actionName = "snap_dotted_4";
            createAction(actionName, SLOT(slotSetSnapFromAction()));
        }
    }

    createAction("show_inst_parameters", SLOT(slotDockParametersBack()));
    createAction("show_chords_ruler", SLOT(slotToggleChordsRuler()));
    createAction("show_tempo_ruler", SLOT(slotToggleTempoRuler()));
}

void
NewMatrixView::initActionsToolbar()
{
    MATRIX_DEBUG << "MatrixView::initActionsToolbar" << endl;

    QToolBar *actionsToolbar = findToolbar("Actions Toolbar");
//	QToolBar *actionsToolbar = m_actionsToolBar;
	//actionsToolbar->setLayout( new QHBoxLayout(actionsToolbar) );
	
    if (!actionsToolbar) {
        MATRIX_DEBUG << "MatrixView::initActionsToolbar - "
        << "tool bar not found" << endl;
        return ;
    }

    // The SnapGrid combo and Snap To... menu items
    //
    QLabel *sLabel = new QLabel(tr(" Grid: "), actionsToolbar);
    sLabel->setIndent(10);
	actionsToolbar->addWidget( sLabel );

    QPixmap noMap = NotePixmapFactory::makeToolbarPixmap("menu-no-note");

    m_snapGridCombo = new QComboBox(actionsToolbar);
	actionsToolbar->addWidget( m_snapGridCombo );

    for (unsigned int i = 0; i < m_snapValues.size(); i++) {

        timeT d = m_snapValues[i];

        if (d == SnapGrid::NoSnap) {
            m_snapGridCombo->addItem(tr("None"));
        } else if (d == SnapGrid::SnapToUnit) {
            m_snapGridCombo->addItem(tr("Unit"));
        } else if (d == SnapGrid::SnapToBeat) {
            m_snapGridCombo->addItem(tr("Beat"));
        } else if (d == SnapGrid::SnapToBar) {
            m_snapGridCombo->addItem(tr("Bar"));
        } else {
            timeT err = 0;
            QString label = NotationStrings::makeNoteMenuLabel(d, true, err);
            QPixmap pixmap = NotePixmapFactory::makeNoteMenuPixmap(d, err);
            m_snapGridCombo->addItem((err ? noMap : pixmap), label);
        }

        if (getSnapGrid() && d == getSnapGrid()->getSnapSetting()) {
            m_snapGridCombo->setCurrentIndex(m_snapGridCombo->count() - 1);
        }
    }

    connect(m_snapGridCombo, SIGNAL(activated(int)),
            this, SLOT(slotSetSnapFromIndex(int)));

    // Velocity combo.  Not a spin box, because the spin box is too
    // slow to use unless we make it typeable into, and then it takes
    // focus away from our more important widgets

    QLabel *vlabel = new QLabel(tr(" Velocity: "), actionsToolbar);
    vlabel->setIndent(10);
    actionsToolbar->addWidget(vlabel);
    
    m_velocityCombo = new QComboBox(actionsToolbar);
    actionsToolbar->addWidget(m_velocityCombo);
	
    for (int i = 0; i <= 127; ++i) {
        m_velocityCombo->addItem(QString("%1").arg(i));
    }
    m_velocityCombo->setCurrentIndex(100); //!!! associate with segment
    connect(m_velocityCombo, SIGNAL(activated(int)),
            m_matrixWidget, SLOT(slotSetCurrentVelocity(int)));

    // Quantize combo
    //
    QLabel *qLabel = new QLabel(tr(" Quantize: "), actionsToolbar);
    qLabel->setIndent(10);
    actionsToolbar->addWidget( qLabel );

    m_quantizeCombo = new QComboBox(actionsToolbar);
    actionsToolbar->addWidget( m_quantizeCombo );

    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {

        timeT time = m_quantizations[i];
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);
        m_quantizeCombo->addItem(error ? noMap : pmap, label);
    }

    m_quantizeCombo->addItem(noMap, tr("Off"));

    connect(m_quantizeCombo, SIGNAL(activated(int)),
            this, SLOT(slotQuantizeSelection(int)));
}

void
NewMatrixView::initZoomToolbar()
{
    QToolBar *zoomToolbar = findToolbar("Zoom Toolbar");
    if (!zoomToolbar) {
        MATRIX_DEBUG << "MatrixView::initZoomToolbar - "
                     << "tool bar not found" << endl;
        return ;
    }
    
    std::vector<double> zoomSizes;
    static double z[] = { 0.025, 0.05, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9,
			  1.0,
                          1.1, 1.2, 1.5, 1.9, 2.5,
                          3.5, 5.0, 7.0, 10.0, 20.0 };
//    static double z[] = { 0.025, 0.05, 0.1, 0.2, 0.5,
//			  1.0, 1.5, 2.5, 5.0, 10.0, 20.0 };
    for (int i = 0; i < sizeof(z)/sizeof(z[0]); ++i) zoomSizes.push_back(z[i]);
    
    m_hZoomSlider = new ZoomSlider<double>
        (zoomSizes, -1, Qt::Horizontal, zoomToolbar);
    m_hZoomSlider->setTracking(true);
    m_hZoomSlider->setFocusPolicy(Qt::NoFocus);

    QLabel *label = new QLabel(tr("  Zoom:  "));
    zoomToolbar->addWidget(label);

    m_zoomLabel = new QLabel();
    m_zoomLabel->setIndent(10);
    m_zoomLabel->setFixedWidth(80);
    m_zoomLabel->setText(tr("%1%").arg(m_hZoomSlider->getCurrentSize()*100.0));

    connect(m_hZoomSlider,
            SIGNAL(valueChanged(int)),
            SLOT(slotChangeHorizontalZoom(int)));

    zoomToolbar->addWidget(m_hZoomSlider);
    zoomToolbar->addWidget(m_zoomLabel);
}

void
NewMatrixView::slotChangeHorizontalZoom(int)
{
    double zoomSize = m_hZoomSlider->getCurrentSize();

    m_zoomLabel->setText(tr("%1%").arg(zoomSize * 100.0));

    MATRIX_DEBUG << "MatrixView::slotChangeHorizontalZoom() : zoom factor = "
                 << zoomSize << endl;

    m_matrixWidget->setZoomFactor(zoomSize);

    QSettings settings;
    settings.beginGroup(MatrixViewConfigGroup);
    settings.setValue("Zoom Level", zoomSize);
    settings.endGroup();
}

void
NewMatrixView::slotSetPaintTool()
{
    if (m_matrixWidget) m_matrixWidget->slotSetPaintTool();
}

void
NewMatrixView::slotSetEraseTool()
{
    if (m_matrixWidget) m_matrixWidget->slotSetEraseTool();
}

void
NewMatrixView::slotSetSelectTool()
{
    if (m_matrixWidget) m_matrixWidget->slotSetSelectTool();
}

void
NewMatrixView::slotSetMoveTool()
{
    if (m_matrixWidget) m_matrixWidget->slotSetMoveTool();
}

void
NewMatrixView::slotSetResizeTool()
{
    if (m_matrixWidget) m_matrixWidget->slotSetResizeTool();
}

void
NewMatrixView::slotSetVelocityTool()
{
    if (m_matrixWidget) m_matrixWidget->slotSetVelocityTool();
}

Segment *
NewMatrixView::getCurrentSegment()
{
    if (m_matrixWidget) return m_matrixWidget->getCurrentSegment();
    else return 0;
}

EventSelection *
NewMatrixView::getSelection() const
{
    if (m_matrixWidget) return m_matrixWidget->getSelection();
    else return 0;
}

void
NewMatrixView::setSelection(EventSelection *s, bool preview)
{
    if (m_matrixWidget) m_matrixWidget->setSelection(s, preview);
}

const SnapGrid *
NewMatrixView::getSnapGrid() const
{
    if (m_matrixWidget) return m_matrixWidget->getSnapGrid();
    else return 0;
}

void
NewMatrixView::slotSetSnapFromIndex(int s)
{
    slotSetSnap(m_snapValues[s]);
}

void
NewMatrixView::slotSetSnapFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    if (name.left(5) == "snap_") {
        int snap = name.right(name.length() - 5).toInt();
        if (snap > 0) {
            slotSetSnap(Note(Note::Semibreve).getDuration() / snap);
        } else if (name.left(12) == "snap_dotted_") {
            snap = name.right(name.length() - 12).toInt();
            slotSetSnap((3*Note(Note::Semibreve).getDuration()) / (2*snap));
        } else if (name == "snap_none") {
            slotSetSnap(SnapGrid::NoSnap);
        } else if (name == "snap_beat") {
            slotSetSnap(SnapGrid::SnapToBeat);
        } else if (name == "snap_bar") {
            slotSetSnap(SnapGrid::SnapToBar);
        } else if (name == "snap_unit") {
            slotSetSnap(SnapGrid::SnapToUnit);
        } else {
            MATRIX_DEBUG << "Warning: NewMatrixView::slotSetSnapFromAction: unrecognised action " << name << endl;
        }
    }
}

void
NewMatrixView::slotSetSnap(timeT t)
{
    m_matrixWidget->slotSetSnap(t);

    for (unsigned int i = 0; i < m_snapValues.size(); ++i) {
        if (m_snapValues[i] == t) {
            m_snapGridCombo->setCurrentIndex(i);
            break;
        }
    }
}

void
NewMatrixView::slotEditCut()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CutCommand(*selection, m_document->getClipboard()));
}

void
NewMatrixView::slotEditCopy()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CopyCommand(*selection, m_document->getClipboard()));
//    emit usedSelection();//!!!
}

void
NewMatrixView::slotEditPaste()
{
    if (m_document->getClipboard()->isEmpty()) return;

    PasteEventsCommand *command = new PasteEventsCommand
        (*m_matrixWidget->getCurrentSegment(),
         m_document->getClipboard(),
         getInsertionTime(),
         PasteEventsCommand::MatrixOverlay);

    if (!command->isPossible()) {
        return;
    } else {
        CommandHistory::getInstance()->addCommand(command);
        setSelection(new EventSelection(command->getPastedEvents()), false);
    }
}

void
NewMatrixView::slotEditDelete()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand(new EraseCommand(*selection));
}

void
NewMatrixView::slotQuantizeSelection(int q)
{
    MATRIX_DEBUG << "NewMatrixView::slotQuantizeSelection\n";

    timeT unit =
        ((unsigned int)q < m_quantizations.size() ? m_quantizations[q] : 0);

    Quantizer *quant =
        new BasicQuantizer
        (unit ? unit :
         Note(Note::Shortest).getDuration(), false);

    EventSelection *selection = getSelection();
    if (!selection) return;

    if (unit) {
        if (selection && selection->getAddedEvents()) {
            CommandHistory::getInstance()->addCommand
                (new EventQuantizeCommand(*selection, quant));
        } else {
            Segment *s = m_matrixWidget->getCurrentSegment();
            if (s) {
                CommandHistory::getInstance()->addCommand
                    (new EventQuantizeCommand
                     (*s, s->getStartTime(), s->getEndMarkerTime(), quant));
            }
        }
    } else {
        if (selection && selection->getAddedEvents()) {
            CommandHistory::getInstance()->addCommand
                (new EventUnquantizeCommand(*selection, quant));
        } else {
            Segment *s = m_matrixWidget->getCurrentSegment();
            if (s) {
                CommandHistory::getInstance()->addCommand
                    (new EventUnquantizeCommand
                     (*s, s->getStartTime(), s->getEndMarkerTime(), quant));
            }
        }
    }
}

void
NewMatrixView::slotQuantize()
{
    if (!getSelection()) return;

    QuantizeDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand
            (new EventQuantizeCommand
             (*getSelection(),
              dialog.getQuantizer()));
    }
}

void
NewMatrixView::slotRepeatQuantize()
{
    if (!getSelection()) return;
    CommandHistory::getInstance()->addCommand
        (new EventQuantizeCommand
         (*getSelection(),
          "Quantize Dialog Grid", false)); // no i18n (config group name)
}

void
NewMatrixView::slotCollapseNotes()
{
    if (!getSelection()) return;
    CommandHistory::getInstance()->addCommand
        (new CollapseNotesCommand(*getSelection()));
}

void
NewMatrixView::slotLegato()
{
    if (!getSelection()) return;
    CommandHistory::getInstance()->addCommand
        (new EventQuantizeCommand
         (*getSelection(),
          new LegatoQuantizer(0))); // no quantization
}

void
NewMatrixView::slotVelocityUp()
{
    if (!getSelection()) return;

    CommandHistory::getInstance()->addCommand
        (new ChangeVelocityCommand(10, *getSelection()));

    slotSetCurrentVelocityFromSelection();
}

void
NewMatrixView::slotVelocityDown()
{
    if (!getSelection()) return;

    CommandHistory::getInstance()->addCommand
        (new ChangeVelocityCommand( -10, *getSelection()));

    slotSetCurrentVelocityFromSelection();
}

void
NewMatrixView::slotSetVelocities()
{
    if (!getSelection()) return;

    EventParameterDialog dialog(this,
                                tr("Set Event Velocities"),
                                BaseProperties::VELOCITY,
                                getCurrentVelocity());

    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand
            (new SelectionPropertyCommand
             (getSelection(),
              BaseProperties::VELOCITY,
              dialog.getPattern(),
              dialog.getValue1(),
              dialog.getValue2()));
    }
}

void
NewMatrixView::slotSetVelocitiesToCurrent()
{
    if (!getSelection()) return;

    CommandHistory::getInstance()->addCommand
        (new SelectionPropertyCommand
         (getSelection(),
          BaseProperties::VELOCITY,
          FlatPattern,
          getCurrentVelocity(),
          getCurrentVelocity()));
}

void
NewMatrixView::slotTriggerSegment()
{
    if (!getSelection()) return;

    TriggerSegmentDialog dialog(this, &m_document->getComposition());
    if (dialog.exec() != QDialog::Accepted) return;

    CommandHistory::getInstance()->addCommand
        (new SetTriggerCommand(*getSelection(),
                               dialog.getId(),
                               true,
                               dialog.getRetune(),
                               dialog.getTimeAdjust(),
                               Marks::NoMark,
                               tr("Trigger Segment")));
}

void
NewMatrixView::slotRemoveTriggers()
{
    if (!getSelection()) return;

    CommandHistory::getInstance()->addCommand
        (new ClearTriggersCommand(*getSelection(),
                                  tr("Remove Triggers")));
}

void
NewMatrixView::slotSelectAll()
{
    if (m_matrixWidget) m_matrixWidget->slotSelectAll();
}

void
NewMatrixView::slotPreviewSelection()
{
    if (!getSelection()) {
        return;
    }

    m_document->slotSetLoop(getSelection()->getStartTime(),
                            getSelection()->getEndTime());
}

void
NewMatrixView::slotClearLoop()
{
    m_document->slotSetLoop(0, 0);
}

void
NewMatrixView::slotClearSelection()
{
    if (m_matrixWidget) m_matrixWidget->slotClearSelection();
}

void
NewMatrixView::slotFilterSelection()
{
    RG_DEBUG << "MatrixView::slotFilterSelection" << endl;

    if (!m_matrixWidget) return;

    Segment *segment = m_matrixWidget->getCurrentSegment();
    EventSelection *existingSelection = getSelection();
    if (!segment || !existingSelection) return;

    EventFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotFilterSelection- accepted" << endl;

        bool haveEvent = false;

        EventSelection *newSelection = new EventSelection(*segment);
        EventSelection::eventcontainer &ec =
            existingSelection->getSegmentEvents();
        for (EventSelection::eventcontainer::iterator i =
                    ec.begin(); i != ec.end(); ++i) {
            if (dialog.keepEvent(*i)) {
                haveEvent = true;
                newSelection->addEvent(*i);
            }
        }

        if (haveEvent) setSelection(newSelection, false);
        else setSelection(0, false);
    }
}

int
NewMatrixView::getCurrentVelocity() const
{
    return m_velocityCombo->currentIndex();
}

void
NewMatrixView::slotSetCurrentVelocity(int value)
{
    m_velocityCombo->setCurrentIndex(value);
}


void
NewMatrixView::slotSetCurrentVelocityFromSelection()
{
    if (!getSelection()) return;

    float totalVelocity = 0;
    int count = 0;

    for (EventSelection::eventcontainer::iterator i =
             getSelection()->getSegmentEvents().begin();
         i != getSelection()->getSegmentEvents().end(); ++i) {

        if ((*i)->has(BaseProperties::VELOCITY)) {
            totalVelocity += (*i)->get<Int>(BaseProperties::VELOCITY);
            ++count;
        }
    }

    if (count > 0) {
        slotSetCurrentVelocity((totalVelocity / count) + 0.5);
    }
}


}

#include "MatrixView.moc"
