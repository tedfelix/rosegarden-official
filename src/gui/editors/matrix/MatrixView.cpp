/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical matrix editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MatrixView]"

#include "MatrixView.h"

#include "MatrixCommandRegistry.h"
#include "MatrixWidget.h"
#include "MatrixElement.h"
#include "MatrixViewSegment.h"
#include "MatrixScene.h"
#include "PianoKeyboard.h"

#include "misc/Debug.h"
#include "misc/Strings.h"

#include "misc/ConfigGroups.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"

#include "gui/dialogs/AboutDialog.h"
#include "gui/dialogs/QuantizeDialog.h"
#include "gui/dialogs/EventFilterDialog.h"
#include "gui/dialogs/EventParameterDialog.h"
#include "gui/dialogs/TriggerSegmentDialog.h"
#include "gui/dialogs/PitchBendSequenceDialog.h"
#include "gui/dialogs/KeySignatureDialog.h"

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

#include "commands/edit/InvertCommand.h"
#include "commands/edit/MoveCommand.h"
#include "commands/edit/PlaceControllersCommand.h"
#include "commands/edit/RescaleCommand.h"
#include "commands/edit/RetrogradeCommand.h"
#include "commands/edit/RetrogradeInvertCommand.h"
#include "commands/edit/TransposeCommand.h"

#include "commands/segment/AddTempoChangeCommand.h"
#include "commands/segment/AddTimeSignatureAndNormalizeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"

#include "commands/matrix/MatrixInsertionCommand.h"

#include "commands/notation/KeyInsertionCommand.h"
#include "commands/notation/MultiKeyInsertionCommand.h"

#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"

#include "gui/rulers/ControlRulerWidget.h"

#include "gui/general/ThornStyle.h"

#include "base/Quantizer.h"
#include "base/BasicQuantizer.h"
#include "base/LegatoQuantizer.h"
#include "base/BaseProperties.h"
#include "base/SnapGrid.h"
#include "base/Clipboard.h"
#include "base/AnalysisTypes.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/NotationTypes.h"
#include "base/Controllable.h"
#include "base/Studio.h"
#include "base/Instrument.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/SoftSynthDevice.h"
#include "base/MidiTypes.h"
#include "base/parameterpattern/ParameterPattern.h"

#include "gui/dialogs/RescaleDialog.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/dialogs/IntervalDialog.h"
#include "gui/dialogs/TimeSignatureDialog.h"

#include "gui/general/IconLoader.h"

#include <QWidget>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QLabel>
#include <QToolBar>
#include <QSettings>
#include <QComboBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QToolButton>
#include <QStatusBar>
#include <QDesktopServices>


namespace Rosegarden
{

MatrixView::MatrixView(RosegardenDocument *doc,
                 std::vector<Segment *> segments,
                 bool drumMode,
                 QWidget *parent) :
    EditViewBase(doc, segments, parent),
    m_tracking(true),
    m_quantizations(BasicQuantizer::getStandardQuantizations()),
    m_drumMode(drumMode),
    m_inChordMode(false)
{
    m_document = doc;
    m_matrixWidget = new MatrixWidget(m_drumMode);
    setCentralWidget(m_matrixWidget);
    m_matrixWidget->setSegments(doc, segments);
       
    // Many actions are created here
    m_commandRegistry = new MatrixCommandRegistry(this);
    
    setupActions();
    
    createMenusAndToolbars("matrix.rc");
     
    findToolbar("General Toolbar");

    m_Thorn = ThornStyle::isEnabled();

    initActionsToolbar();
    initRulersToolbar();
    initStatusBar();
    
    connect(m_matrixWidget, SIGNAL(editTriggerSegment(int)),
            this, SIGNAL(editTriggerSegment(int)));

    connect(m_matrixWidget, SIGNAL(showContextHelp(const QString &)),
            this, SLOT(slotShowContextHelp(const QString &)));

    slotUpdateMenuStates();
    slotTestClipboard();

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdateMenuStates()));

    connect(m_matrixWidget, SIGNAL(selectionChanged()),
            this, SLOT(slotUpdateMenuStates()));

    // Toggle the desired tool off and then trigger it on again, to
    // make sure its signal is called at least once (as would not
    // happen if the tool was on by default otherwise)
    QAction *toolAction = 0;
    if (!m_matrixWidget->segmentsContainNotes()) {
        toolAction = findAction("draw");
    } else {
        toolAction = findAction("select");
    }
    if (toolAction) {
        MATRIX_DEBUG << "initial state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked();
        if (toolAction->isChecked()) toolAction->toggle();
        MATRIX_DEBUG << "newer state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked();
        toolAction->trigger();
        MATRIX_DEBUG << "newest state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked();
    }

    m_matrixWidget->slotSetPlayTracking(m_tracking);

    slotUpdateWindowTitle();
    connect(m_document, SIGNAL(documentModified(bool)),
            this, SLOT(slotUpdateWindowTitle(bool)));

    // Set initial visibility ...
    bool view;

    QSettings settings;
    settings.beginGroup(MatrixViewConfigGroup);

    // ... of chord name ruler ...
    view = settings.value("Chords ruler shown",
                          findAction("show_chords_ruler")->isChecked()
                         ).toBool();
    findAction("show_chords_ruler")->setChecked(view);
    m_matrixWidget->setChordNameRulerVisible(view);

    // ... and tempo ruler
    view = settings.value("Tempo ruler shown",
                          findAction("show_tempo_ruler")->isChecked()
                         ).toBool();
    findAction("show_tempo_ruler")->setChecked(view);
    m_matrixWidget->setTempoRulerVisible(view);
    
    settings.endGroup();
    
    if (segments.size() > 1) {
        enterActionState("have_multiple_segments");
    } else {
        leaveActionState("have_multiple_segments");
    }

    if (m_drumMode) {
        enterActionState("in_percussion_matrix");
    } else {
        enterActionState("in_standard_matrix");
    }

    // Restore window geometry and toolbar/dock state
    settings.beginGroup(WindowGeometryConfigGroup);
    QString modeStr = (m_drumMode ? "Percussion_Matrix_View_Geometry" : "Matrix_View_Geometry");
    this->restoreGeometry(settings.value(modeStr).toByteArray());
    modeStr = (m_drumMode ? "Percussion_Matrix_View_State" : "Matrix_View_State");
    this->restoreState(settings.value(modeStr).toByteArray());
    settings.endGroup();

    connect(m_matrixWidget, SIGNAL(segmentDeleted(Segment *)),
            this, SLOT(slotSegmentDeleted(Segment *)));
    connect(m_matrixWidget, SIGNAL(sceneDeleted()),
            this, SLOT(slotSceneDeleted()));

    connect(this, SIGNAL(noteInsertedFromKeyboard(Segment *, int)),
            m_matrixWidget, SLOT(slotPlayPreviewNote(Segment *, int)));

    // Set the rewind and fast-forward buttons for auto-repeat.
    enableAutoRepeat("Transport Toolbar", "playback_pointer_back_bar");
    enableAutoRepeat("Transport Toolbar", "playback_pointer_forward_bar");
    enableAutoRepeat("Transport Toolbar", "cursor_back");
    enableAutoRepeat("Transport Toolbar", "cursor_forward");

    // Show the pointer as soon as matrix editor opens (update pointer position,
    // but don't scroll)
    m_matrixWidget->showInitialPointer();    
    
    readOptions();
}

MatrixView::~MatrixView()
{
    MATRIX_DEBUG << "MatrixView::~MatrixView()";
}

void
MatrixView::closeEvent(QCloseEvent *event)
{
    // Save window geometry and toolbar/dock state
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    QString modeStr = (m_drumMode ? "Percussion_Matrix_View_Geometry" : "Matrix_View_Geometry");
    settings.setValue(modeStr, this->saveGeometry());
    modeStr = (m_drumMode ? "Percussion_Matrix_View_State" : "Matrix_View_State");
    settings.setValue(modeStr, this->saveState());
    settings.endGroup();

    QWidget::closeEvent(event);
}

void
MatrixView::slotSegmentDeleted(Segment *s)
{
    MATRIX_DEBUG << "MatrixView::slotSegmentDeleted: " << s;

    // remove from vector
    for (std::vector<Segment *>::iterator i = m_segments.begin();
         i != m_segments.end(); ++i) {
        if (*i == s) {
            m_segments.erase(i);
            NOTATION_DEBUG << "MatrixView::slotSegmentDeleted: Erased segment from vector, have " << m_segments.size() << " segment(s) remaining";
            return;
        }
    }
}

void
MatrixView::slotSceneDeleted()
{
    NOTATION_DEBUG << "MatrixView::slotSceneDeleted";

    m_segments.clear();
    close();
}

void
MatrixView::slotUpdateWindowTitle(bool m)
{
    QString indicator = (m ? "*" : "");
    // Set client label
    //
    QString view = tr("Matrix");
    //&&&if (isDrumMode())
    //    view = tr("Percussion");

    if (m_segments.empty()) return;

    if (m_segments.size() == 1) {

        TrackId trackId = m_segments[0]->getTrack();
        Track *track =
            m_segments[0]->getComposition()->getTrackById(trackId);

        int trackPosition = -1;
        if (track)
            trackPosition = track->getPosition();

        QString segLabel = strtoqstr(m_segments[0]->getLabel());
        if (segLabel.isEmpty()) {
            segLabel = " ";
        } else {
            segLabel = QString(" \"%1\" ").arg(segLabel);
        }

        QString trkLabel = strtoqstr(track->getLabel());
        if (trkLabel.isEmpty() || trkLabel == tr("<untitled>")) {
            trkLabel = " ";
        } else {
            trkLabel = QString(" \"%1\" ").arg(trkLabel);
        }

        setWindowTitle(tr("%1%2 - Segment%3Track%4#%5 - %6")
                      .arg(indicator)
                      .arg(getDocument()->getTitle())
                      .arg(segLabel)
                      .arg(trkLabel)
                      .arg(trackPosition + 1)
                      .arg(view));

    } else if (m_segments.size() == getDocument()->getComposition().getNbSegments()) {

        setWindowTitle(tr("%1%2 - All Segments - %3")
                      .arg(indicator)
                      .arg(getDocument()->getTitle())
                      .arg(view));

    } else {

        setWindowTitle(tr("%1%2 - %n Segment(s) - %3", "", m_segments.size())
                      .arg(indicator)
                      .arg(getDocument()->getTitle())
                      .arg(view));
    }

    setWindowIcon(IconLoader().loadPixmap("window-matrix"));
}

void
MatrixView::setupActions()
{
    
    setupBaseActions(true);
    
    createAction("select", SLOT(slotSetSelectTool()));
    createAction("draw", SLOT(slotSetPaintTool()));
    createAction("erase", SLOT(slotSetEraseTool()));
    createAction("move", SLOT(slotSetMoveTool()));
    createAction("resize", SLOT(slotSetResizeTool()));
    createAction("velocity", SLOT(slotSetVelocityTool()));
    createAction("chord_mode", SLOT(slotToggleChordMode()));
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
    createAction("extend_selection_backward", SLOT(slotExtendSelectionBackward()));
    createAction("extend_selection_forward", SLOT(slotExtendSelectionForward()));
    createAction("extend_selection_backward_bar", SLOT(slotExtendSelectionBackwardBar()));
    createAction("extend_selection_forward_bar", SLOT(slotExtendSelectionForwardBar()));
    //&&& NB Play has two shortcuts (Enter and Ctrl+Return) -- need to
    // ensure both get carried across somehow
    createAction("play", SIGNAL(play()));
    createAction("stop", SIGNAL(stop()));
    createAction("playback_pointer_back_bar", SIGNAL(rewindPlayback()));
    createAction("playback_pointer_forward_bar", SIGNAL(fastForwardPlayback()));
    createAction("playback_pointer_start", SIGNAL(rewindPlaybackToBeginning()));
    createAction("playback_pointer_end", SIGNAL(fastForwardPlaybackToEnd()));
    createAction("cursor_prior_segment", SLOT(slotCurrentSegmentPrior()));
    createAction("cursor_next_segment", SLOT(slotCurrentSegmentNext()));
    createAction("toggle_solo", SLOT(slotToggleSolo()));
    createAction("toggle_tracking", SLOT(slotToggleTracking()));
    createAction("panic", SIGNAL(panic()));
    createAction("preview_selection", SLOT(slotPreviewSelection()));
    createAction("clear_loop", SLOT(slotClearLoop()));
    createAction("clear_selection", SLOT(slotClearSelection()));
    createAction("filter_selection", SLOT(slotFilterSelection()));

    createAction("pitch_bend_sequence", SLOT(slotPitchBendSequence()));    

    //"controllers" Menubar menu
    createAction("controller_sequence", SLOT(slotControllerSequence()));
    createAction("copy_controllers",  SLOT(slotEditCopyControllers()));
    createAction("cut_controllers",   SLOT(slotEditCutControllers()));
    createAction("set_controllers",   SLOT(slotSetControllers()));
    createAction("place_controllers", SLOT(slotPlaceControllers()));

    createAction("show_chords_ruler", SLOT(slotToggleChordsRuler()));
    createAction("show_tempo_ruler", SLOT(slotToggleTempoRuler()));
    
    createAction("toggle_velocity_ruler", SLOT(slotToggleVelocityRuler()));
    createAction("toggle_pitchbend_ruler", SLOT(slotTogglePitchbendRuler()));
    createAction("add_control_ruler", "");
    
    createAction("add_tempo_change", SLOT(slotAddTempo()));
    createAction("add_time_signature", SLOT(slotAddTimeSignature()));
    createAction("add_key_signature", SLOT(slotEditAddKeySignature()));

    createAction("halve_durations", SLOT(slotHalveDurations()));
    createAction("double_durations", SLOT(slotDoubleDurations()));
    createAction("rescale", SLOT(slotRescale()));
    createAction("transpose_up", SLOT(slotTransposeUp()));
    createAction("transpose_up_octave", SLOT(slotTransposeUpOctave()));
    createAction("transpose_down", SLOT(slotTransposeDown()));
    createAction("transpose_down_octave", SLOT(slotTransposeDownOctave()));
    createAction("general_transpose", SLOT(slotTranspose()));
    createAction("general_diatonic_transpose", SLOT(slotDiatonicTranspose()));
    createAction("invert", SLOT(slotInvert()));
    createAction("retrograde", SLOT(slotRetrograde()));
    createAction("retrograde_invert", SLOT(slotRetrogradeInvert()));    
    createAction("jog_left", SLOT(slotJogLeft()));
    createAction("jog_right", SLOT(slotJogRight()));
    
    QMenu *addControlRulerMenu = new QMenu;
    Controllable *c =
        dynamic_cast<MidiDevice *>(getCurrentDevice());
    if (!c) {
        c = dynamic_cast<SoftSynthDevice *>(getCurrentDevice());
    }

    if (c) {

        const ControlList &list = c->getControlParameters();

        QString itemStr;

        for (ControlList::const_iterator it = list.begin();
             it != list.end(); ++it) {

            // Pitch Bend is treated separately now, and there's no
            // point in adding "unsupported" controllers to the menu,
            // so skip everything else
            if (it->getType() != Controller::EventType) continue;

            QString hexValue;
            hexValue.sprintf("(0x%x)", it->getControllerValue());

            // strings extracted from data files must be QObject::tr()
            itemStr = QObject::tr("%1 Controller %2 %3")
                .arg(QObject::tr(it->getName().c_str()))
                .arg(it->getControllerValue())
                .arg(hexValue);

            addControlRulerMenu->addAction(itemStr);
        }
    }

    connect(addControlRulerMenu, SIGNAL(triggered(QAction*)),
            SLOT(slotAddControlRuler(QAction*)));

    findAction("add_control_ruler")->setMenu(addControlRulerMenu);

    // (ported from NotationView) 
    //JAS insert note section is a rewrite
    //JAS from EditView::createInsertPitchActionMenu()
    for (int octave = 0; octave <= 2; ++octave) {
        QString octaveSuffix;
        if (octave == 1) octaveSuffix = "_high";
        else if (octave == 2) octaveSuffix = "_low";

        createAction(QString("insert_0%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_0_sharp%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_1_flat%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_1%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_1_sharp%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_2_flat%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_2%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_3%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_3_sharp%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_4_flat%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_4%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_4_sharp%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_5_flat%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_5%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_5_sharp%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_6_flat%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
        createAction(QString("insert_6%1").arg(octaveSuffix),
                     SLOT(slotInsertNoteFromAction()));
    }

    createAction("options_show_toolbar", SLOT(slotToggleGeneralToolBar()));
    createAction("show_tools_toolbar", SLOT(slotToggleToolsToolBar()));
    createAction("show_transport_toolbar", SLOT(slotToggleTransportToolBar()));
    createAction("show_actions_toolbar", SLOT(slotToggleActionsToolBar()));
    createAction("show_rulers_toolbar", SLOT(slotToggleRulersToolBar()));

    
    createAction("manual", SLOT(slotHelp()));
    createAction("tutorial", SLOT(slotTutorial()));
    createAction("guidelines", SLOT(slotBugGuidelines()));
    createAction("help_about_app", SLOT(slotHelpAbout()));
    createAction("help_about_qt", SLOT(slotHelpAboutQt()));
    createAction("donate", SLOT(slotDonate()));
    
    // grid snap values
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
}


void
MatrixView::initActionsToolbar()
{
    MATRIX_DEBUG << "MatrixView::initActionsToolbar";

    QToolBar *actionsToolbar = findToolbar("Actions Toolbar");
//    QToolBar *actionsToolbar = m_actionsToolBar;
    //actionsToolbar->setLayout(new QHBoxLayout(actionsToolbar));

    if (!actionsToolbar) {
        MATRIX_DEBUG << "MatrixView::initActionsToolbar - "
        << "tool bar not found";
        return ;
    }

    // The SnapGrid combo and Snap To... menu items
    //
    QLabel *sLabel = new QLabel(tr(" Grid: "), actionsToolbar);
    sLabel->setIndent(10);
    actionsToolbar->addWidget(sLabel);

    QPixmap noMap = NotePixmapFactory::makeToolbarPixmap("menu-no-note");

    m_snapGridCombo = new QComboBox(actionsToolbar);
    actionsToolbar->addWidget(m_snapGridCombo);

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
    actionsToolbar->addWidget(qLabel);

    m_quantizeCombo = new QComboBox(actionsToolbar);
    actionsToolbar->addWidget(m_quantizeCombo);

    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {

        timeT time = m_quantizations[i];
        timeT error = 0;
        QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
        QPixmap pmap = NotePixmapFactory::makeNoteMenuPixmap(time, error);
        m_quantizeCombo->addItem(error ? noMap : pmap, label);
    }

    m_quantizeCombo->addItem(noMap, tr("Off"));

    // default to Off to mirror Classic behavior
    m_quantizeCombo->setCurrentIndex(m_quantizeCombo->count() - 1);

    m_quantizeCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    connect(m_quantizeCombo, SIGNAL(activated(int)),
            this, SLOT(slotQuantizeSelection(int)));
}

void
MatrixView::initRulersToolbar()
{
    QToolBar *rulersToolbar = findToolbar("Rulers Toolbar");
    if (!rulersToolbar) {
        RG_WARNING << "MatrixView::initRulersToolbar() - rulers toolbar not found!";
        return;
    }

    // set the "ruler n" tool button to pop up its menu instantly
    QToolButton *tb = dynamic_cast<QToolButton *>(findToolbar("Rulers Toolbar")->widgetForAction(findAction("add_control_ruler")));
    tb->setPopupMode(QToolButton::InstantPopup);
}

void
MatrixView::readOptions()
{
    EditViewBase::readOptions();

    setCheckBoxState("options_show_toolbar", "General Toolbar");
    setCheckBoxState("show_tools_toolbar", "Tools Toolbar");
    setCheckBoxState("show_transport_toolbar", "Transport Toolbar");
    setCheckBoxState("show_actions_toolbar", "Actions Toolbar");
    setCheckBoxState("show_rulers_toolbar", "Rulers Toolbar");  
}

void
MatrixView::initStatusBar()
{
    statusBar();
}

void
MatrixView::slotShowContextHelp(const QString &help)
{
    statusBar()->showMessage(help, 10000);
}

void
MatrixView::slotUpdateMenuStates()
{
    EventSelection *s = getSelection();
    if (s && !s->getSegmentEvents().empty()) {
        enterActionState("have_selection");
    } else {
        leaveActionState("have_selection");
    }
    conformRulerSelectionState();
}

void
MatrixView::
conformRulerSelectionState(void)
{
    ControlRulerWidget * cr = m_matrixWidget->getControlsWidget();
    if (cr->isAnyRulerVisible())
        {
            cr->slotSelectionChanged(getSelection());

            enterActionState("have_control_ruler");
            if (cr->hasSelection())
                { enterActionState("have_controller_selection"); }
            else
                { leaveActionState("have_controller_selection"); }
        }
    else {
        leaveActionState("have_control_ruler");
        // No ruler implies no controller selection
        leaveActionState("have_controller_selection"); 
    }
 }

void
MatrixView::slotSetPaintTool()
{
    if (m_matrixWidget) m_matrixWidget->slotSetPaintTool();
}

void
MatrixView::slotSetEraseTool()
{
    if (m_matrixWidget) m_matrixWidget->slotSetEraseTool();
}

void
MatrixView::slotSetSelectTool()
{
    MATRIX_DEBUG << "MatrixView::slotSetSelectTool";
    if (m_matrixWidget) m_matrixWidget->slotSetSelectTool();
}

void
MatrixView::slotSetMoveTool()
{
    if (m_matrixWidget) m_matrixWidget->slotSetMoveTool();
}

void
MatrixView::slotSetResizeTool()
{
    if (m_matrixWidget) m_matrixWidget->slotSetResizeTool();
}

void
MatrixView::slotSetVelocityTool()
{
    if (m_matrixWidget) m_matrixWidget->slotSetVelocityTool();
}

Segment *
MatrixView::getCurrentSegment()
{
    if (m_matrixWidget) return m_matrixWidget->getCurrentSegment();
    else return 0;
}

EventSelection *
MatrixView::getSelection() const
{
    if (m_matrixWidget) return m_matrixWidget->getSelection();
    else return 0;
}

void
MatrixView::setSelection(EventSelection *s, bool preview)
{
    if (m_matrixWidget) m_matrixWidget->setSelection(s, preview);
}

timeT
MatrixView::getInsertionTime() const
{
    if (!m_document) return 0;
    return m_document->getComposition().getPosition();
}

const SnapGrid *
MatrixView::getSnapGrid() const
{
    if (m_matrixWidget) return m_matrixWidget->getSnapGrid();
    else return 0;
}

void
MatrixView::slotSetSnapFromIndex(int s)
{
    slotSetSnap(m_snapValues[s]);
}

void
MatrixView::slotSetSnapFromAction()
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
            MATRIX_DEBUG << "Warning: MatrixView::slotSetSnapFromAction: unrecognised action " << name;
        }
    }
}

void
MatrixView::slotSetSnap(timeT t)
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
MatrixView::slotEditCut()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CutCommand(*selection, getClipboard()));
}

void
MatrixView::slotEditCopy()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CopyCommand(*selection, getClipboard()));
//    emit usedSelection();//!!!
}

void
MatrixView::slotEditPaste()
{
    if (getClipboard()->isEmpty()) return;

    PasteEventsCommand *command = new PasteEventsCommand
        (*m_matrixWidget->getCurrentSegment(),
         getClipboard(),
         getInsertionTime(),
         PasteEventsCommand::MatrixOverlay);

    if (!command->isPossible()) {
        return;
    } else {
        CommandHistory::getInstance()->addCommand(command);
        setSelection(command->getSubsequentSelection(), false);
    }
}

void
MatrixView::slotEditDelete()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand(new EraseCommand(*selection));
}

void
MatrixView::slotQuantizeSelection(int q)
{
    MATRIX_DEBUG << "MatrixView::slotQuantizeSelection\n";

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
MatrixView::slotQuantize()
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
MatrixView::slotRepeatQuantize()
{
    if (!getSelection()) return;
    CommandHistory::getInstance()->addCommand
        (new EventQuantizeCommand
         (*getSelection(),
          "Quantize Dialog Grid", // no tr (config group name)
          EventQuantizeCommand::QUANTIZE_NORMAL));
}

void
MatrixView::slotCollapseNotes()
{
    if (!getSelection()) return;
    CommandHistory::getInstance()->addCommand
        (new CollapseNotesCommand(*getSelection()));
}

void
MatrixView::slotLegato()
{
    if (!getSelection()) return;
    CommandHistory::getInstance()->addCommand
        (new EventQuantizeCommand
         (*getSelection(),
          new LegatoQuantizer(0))); // no quantization
}

void
MatrixView::slotVelocityUp()
{
    if (!getSelection()) return;

    CommandHistory::getInstance()->addCommand
        (new ChangeVelocityCommand(10, *getSelection()));

    slotSetCurrentVelocityFromSelection();
}

void
MatrixView::slotVelocityDown()
{
    if (!getSelection()) return;

    CommandHistory::getInstance()->addCommand
        (new ChangeVelocityCommand(-10, *getSelection()));

    slotSetCurrentVelocityFromSelection();
}

void
MatrixView::slotSetVelocities()
{
    ParameterPattern::
        setVelocities(this, getSelection(), getCurrentVelocity());
}

void
MatrixView::slotSetVelocitiesToCurrent()
{
    ParameterPattern::
        setVelocitiesFlat(getSelection(), getCurrentVelocity());
}

void
MatrixView::slotEditCopyControllers()
{
    ControlRulerWidget *cr = m_matrixWidget->getControlsWidget();
    EventSelection *selection = cr->getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CopyCommand(*selection, getClipboard()));
}

void
MatrixView::slotEditCutControllers()
{
    ControlRulerWidget *cr = m_matrixWidget->getControlsWidget();
    EventSelection *selection = cr->getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CutCommand(*selection, getClipboard()));
}

void
MatrixView::slotSetControllers()
{
    ControlRulerWidget * cr = m_matrixWidget->getControlsWidget();
    ParameterPattern::
        setProperties(this, cr->getSituation(),
                      &ParameterPattern::VelocityPatterns);
}

void
MatrixView::slotPlaceControllers()
{
    EventSelection *selection = getSelection();
    if (!selection) { return; }
    
    ControlRulerWidget *cr = m_matrixWidget->getControlsWidget();
    if (!cr) { return; }
    
    ControlParameter *cp = cr->getControlParameter();
    if (!cp) { return; }

    const Instrument *instrument =
        getDocument()->getInstrument(getCurrentSegment());
    if (!instrument) { return; }
    
    PlaceControllersCommand *command =
        new PlaceControllersCommand(*selection,
                                    instrument,
                                    cp);
    CommandHistory::getInstance()->addCommand(command);
}


void
MatrixView::slotTriggerSegment()
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
MatrixView::slotRemoveTriggers()
{
    if (!getSelection()) return;

    CommandHistory::getInstance()->addCommand
        (new ClearTriggersCommand(*getSelection(),
                                  tr("Remove Triggers")));
}

void
MatrixView::slotSelectAll()
{
    if (m_matrixWidget) m_matrixWidget->slotSelectAll();
}

void
MatrixView::slotCurrentSegmentPrior()
{
    if (m_matrixWidget) m_matrixWidget->slotCurrentSegmentPrior();
}

void
MatrixView::slotCurrentSegmentNext()
{
    if (m_matrixWidget) m_matrixWidget->slotCurrentSegmentNext();
}

void
MatrixView::slotPreviewSelection()
{
    if (!getSelection()) {
        return;
    }

    m_document->slotSetLoop(getSelection()->getStartTime(),
                            getSelection()->getEndTime());
}

void
MatrixView::slotClearLoop()
{
    m_document->slotSetLoop(0, 0);
}

void
MatrixView::slotClearSelection()
{
    if (m_matrixWidget) m_matrixWidget->slotClearSelection();
}

void
MatrixView::slotFilterSelection()
{
    MATRIX_DEBUG << "MatrixView::slotFilterSelection";

    if (!m_matrixWidget) return;

    Segment *segment = m_matrixWidget->getCurrentSegment();
    EventSelection *existingSelection = getSelection();
    if (!segment || !existingSelection) return;

    EventFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        MATRIX_DEBUG << "slotFilterSelection- accepted";

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
MatrixView::getCurrentVelocity() const
{
    return m_velocityCombo->currentIndex();
}

void
MatrixView::slotSetCurrentVelocity(int value)
{
    m_velocityCombo->setCurrentIndex(value);
}

void
MatrixView::slotSetCurrentVelocityFromSelection()
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

void
MatrixView::slotToggleTracking()
{
    m_tracking = !m_tracking;
    m_matrixWidget->slotSetPlayTracking(m_tracking);
}

void
MatrixView::slotToggleChordsRuler()
{
    bool view = findAction("show_chords_ruler")->isChecked();

    m_matrixWidget->setChordNameRulerVisible(view);

    QSettings settings;
    settings.beginGroup(MatrixViewConfigGroup);
    settings.setValue("Chords ruler shown", view);
    settings.endGroup();
}

void
MatrixView::slotToggleVelocityRuler()
{
    m_matrixWidget->slotToggleVelocityRuler();
    conformRulerSelectionState();
}

void
MatrixView::slotTogglePitchbendRuler()
{
    m_matrixWidget->slotTogglePitchbendRuler();
    conformRulerSelectionState();
}

void
MatrixView::slotAddControlRuler(QAction *action)
{
    m_matrixWidget->slotAddControlRuler(action);
    conformRulerSelectionState();
}

void
MatrixView::slotToggleTempoRuler()
{
    bool view = findAction("show_tempo_ruler")->isChecked();

    m_matrixWidget->setTempoRulerVisible(view);

    QSettings settings;
    settings.beginGroup(MatrixViewConfigGroup);
    settings.setValue("Tempo ruler shown", view);
    settings.endGroup();
}

// start of code formerly located in EditView.cpp
// --

void MatrixView::slotAddTempo()
{
    timeT insertionTime = getInsertionTime();

    TempoDialog tempoDlg(this, getDocument());

    connect(&tempoDlg,
             SIGNAL(changeTempo(timeT,
                    tempoT,
                    tempoT,
                    TempoDialog::TempoDialogAction)),
                    this,
                    SIGNAL(changeTempo(timeT,
                           tempoT,
                           tempoT,
                           TempoDialog::TempoDialogAction)));

    tempoDlg.setTempoPosition(insertionTime);
    tempoDlg.exec();
}

void MatrixView::slotAddTimeSignature()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return ;
    Composition *composition = segment->getComposition();
    timeT insertionTime = getInsertionTime();

    TimeSignatureDialog *dialog = 0;
    int timeSigNo = composition->getTimeSignatureNumberAt(insertionTime);

    if (timeSigNo >= 0) {

        dialog = new TimeSignatureDialog
                (this, composition, insertionTime,
                 composition->getTimeSignatureAt(insertionTime));

    } else {

        timeT endTime = composition->getDuration();
        if (composition->getTimeSignatureCount() > 0) {
            endTime = composition->getTimeSignatureChange(0).first;
        }

        CompositionTimeSliceAdapter adapter
                (composition, insertionTime, endTime);
        AnalysisHelper helper;
        TimeSignature timeSig = helper.guessTimeSignature(adapter);

        dialog = new TimeSignatureDialog
                (this, composition, insertionTime, timeSig, false,
                 tr("Estimated time signature shown"));
    }

    if (dialog->exec() == QDialog::Accepted) {

        insertionTime = dialog->getTime();

        if (dialog->shouldNormalizeRests()) {

            CommandHistory::getInstance()->addCommand(new AddTimeSignatureAndNormalizeCommand
                    (composition, insertionTime,
                     dialog->getTimeSignature()));

        } else {

            CommandHistory::getInstance()->addCommand(new AddTimeSignatureCommand
                    (composition, insertionTime,
                     dialog->getTimeSignature()));
        }
    }

    delete dialog;
}



void MatrixView::slotHalveDurations()
{
    EventSelection *selection = getSelection();
    if (!selection) return;

    CommandHistory::getInstance()->addCommand( new RescaleCommand
                            (*selection,
                            selection->getTotalDuration() / 2,
                            false)
                       );
}

void MatrixView::slotDoubleDurations()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand(new RescaleCommand(*selection,
                                            selection->getTotalDuration() * 2,
                                                    false)
                       );
}

void MatrixView::slotRescale()
{
    EventSelection *selection = getSelection();
    if (!selection) return;

    RescaleDialog dialog(this,
                         &getDocument()->getComposition(),
                         selection->getStartTime(),
                         selection->getEndTime() -
                             selection->getStartTime(),
                         1,
                         true,
                         true
                        );

    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand(new RescaleCommand
                (*selection,
                  dialog.getNewDuration(),
                                        dialog.shouldCloseGap()));
    }
}

void MatrixView::slotTranspose()
{
    EventSelection *selection = getSelection();
    if (!selection) {
        RG_WARNING << "Hint: selection is NULL in slotTranpose()";
        return;
    }

    QSettings settings;
    settings.beginGroup(MatrixViewConfigGroup);

    int dialogDefault = settings.value("lasttransposition", 0).toInt() ;

    bool ok = false;
    int min = -127;
    int max = 127;
    int step = 1;
    int semitones = QInputDialog::getInt(
            this,
            tr("Transpose"),
            tr("By number of semitones: "),
            dialogDefault,
            min,
            max,
            step,
            &ok);

    if (!ok || semitones == 0) return;

    settings.setValue("lasttransposition", semitones);

    CommandHistory::getInstance()->addCommand(new TransposeCommand
            (semitones, *selection));

    settings.endGroup();
}

void MatrixView::slotDiatonicTranspose()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    
    QSettings settings;
    settings.beginGroup(MatrixViewConfigGroup);
    
    IntervalDialog intervalDialog(this);
    int ok = intervalDialog.exec();
    //int dialogDefault = settings.value("lasttransposition", 0).toInt() ;
    int semitones = intervalDialog.getChromaticDistance();
    int steps = intervalDialog.getDiatonicDistance();
    settings.endGroup();
    
    if (!ok || (semitones == 0 && steps == 0)) return;
    
    if (intervalDialog.getChangeKey())
    {
        RG_WARNING << "Transposing changing keys is not currently supported on selections";
    }
    else
    {
    // Transpose within key
        //std::cout << "Transposing semitones, steps: " << semitones << ", " << steps;
        CommandHistory::getInstance()->addCommand(new TransposeCommand
                (semitones, steps, *selection));
    }
}

void MatrixView::slotTransposeUp()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;
    CommandHistory::getInstance()->addCommand(new TransposeCommand(1, *selection));
}

void MatrixView::slotTransposeUpOctave()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;
    CommandHistory::getInstance()->addCommand(new TransposeCommand(12, *selection));
}

void MatrixView::slotTransposeDown()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;
    CommandHistory::getInstance()->addCommand(new TransposeCommand( -1, *selection));
}

void MatrixView::slotTransposeDownOctave()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;
    CommandHistory::getInstance()->addCommand(new TransposeCommand( -12, *selection));
}

void MatrixView::slotInvert()
{
    RG_DEBUG << "slotInvert() called";

    EventSelection *selection = getSelection();
    if (!selection) {
        RG_WARNING << "Hint: selection is NULL in slotInvert()";
        return;
    }

    int semitones = 0;    
    CommandHistory::getInstance()->addCommand(new InvertCommand
            (semitones, *selection));
}

void MatrixView::slotRetrograde()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;
    
    int semitones = 0;
    CommandHistory::getInstance()->addCommand(new RetrogradeCommand
            (semitones, *selection));
}

void MatrixView::slotRetrogradeInvert()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;

    int semitones = 0;
    CommandHistory::getInstance()->addCommand(new RetrogradeInvertCommand
            (semitones, *selection));
}

void
MatrixView::slotHelp()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:matrix-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:matrix-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
MatrixView::slotTutorial()
{
    QString tutorialURL = tr("http://www.rosegardenmusic.com/tutorials/en/chapter-0.html");
    QDesktopServices::openUrl(QUrl(tutorialURL));
}

void
MatrixView::slotBugGuidelines()
{
    QString glURL = tr("http://rosegarden.sourceforge.net/tutorial/bug-guidelines.html");
     QDesktopServices::openUrl(QUrl(glURL));
}

void
MatrixView::slotHelpAbout()
{
    new AboutDialog(this);
}

void
MatrixView::slotHelpAboutQt()
{
    QMessageBox::aboutQt(this, tr("Rosegarden"));
}

void
MatrixView::slotDonate()
{
    QString url("https://sourceforge.net/project/project_donations.php?group_id=4932");
    QDesktopServices::openUrl(QUrl(url));
}

void
MatrixView::slotStepBackward()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;

    // Sanity check.  Move postion marker inside segmet if not
    timeT time = getInsertionTime();  // Un-checked current insertion time
    
    timeT segmentEndTime = segment->getEndMarkerTime();
    if (time > segmentEndTime) {
        // Move to inside the current segment
        time = segment->getStartTime();
    }
        
    time = getSnapGrid()->snapTime(time - 1, SnapGrid::SnapLeft);

    if (time < segment->getStartTime()){
        m_document->slotSetPointerPosition(segment->getStartTime());
    } else {
        m_document->slotSetPointerPosition(time);
    }
}

void
MatrixView::slotStepForward(bool force)
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;

    // Sanity check.  Move postion marker inside segmet if not
    timeT time = getInsertionTime();  // Un-checked current insertion time
    
    timeT segmentStartTime = segment->getStartTime();

    if (!force && ((time < segmentStartTime) ||
            (time > segment->getEndMarkerTime()))) {
        // Move to inside the current segment
        time = segmentStartTime;
    }
        
    time = getSnapGrid()->snapTime(time + 1, SnapGrid::SnapRight);
    
    if (!force && (time > segment->getEndMarkerTime())){
        m_document->slotSetPointerPosition(segment->getEndMarkerTime());
    } else {
        m_document->slotSetPointerPosition(time);
    }
}

void
MatrixView::slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn)
{
    QAction *action = findAction("toggle_step_by_step");

    if (!action) {
        MATRIX_DEBUG << "WARNING: No toggle_step_by_step action";
        return ;
    }

    // return if not in step recording mode
    if (!action->isChecked()) return;

    // return if this window isn't active, to avoid filling a forgotten edit
    // view with garbage while banging away in some other window.
    //
    // NOTE: This prevents using something like VMPK for step recording, but I
    // see no better alternative.
    if (!isActiveWindow()) return;


    Segment *segment = getCurrentSegment();

    // If the segment is transposed, we want to take that into
    // account.  But the note has already been played back to the user
    // at its untransposed pitch, because that's done by the MIDI THRU
    // code in the sequencer which has no way to know whether a note
    // was intended for step recording.  So rather than adjust the
    // pitch for playback according to the transpose setting, we have
    // to adjust the stored pitch in the opposite direction.

    pitch -= segment->getTranspose();

//    TmpStatusMsg msg(tr("Inserting note"), this);

    static int numberOfNotesOn = 0;
    static timeT insertionTime = getInsertionTime();
    static time_t lastInsertionTime = 0;

    if (!noteOn) {
        numberOfNotesOn--;
        return ;
    }
    // Rules:
    //
    // * If no other note event has turned up within half a
    //   second, insert this note and advance.
    //
    // * Relatedly, if this note is within half a second of
    //   the previous one, they're chords.  Insert the previous
    //   one, don't advance, and use the same rules for this.
    //
    // * If a note event turns up before that time has elapsed,
    //   we need to wait for the note-off events: if the second
    //   note happened less than half way through the first,
    //   it's a chord.
    //
    // We haven't implemented these yet... For now:
    //
    // Rules (hjj):
    //
    // * The overlapping notes are always included in to a chord.
    //   This is the most convenient for step inserting of chords.
    //
    // * The timer resets the numberOfNotesOn, if noteOff signals were
    //   drop out for some reason (which has not been encountered yet).
    time_t now;
    time (&now);
    double elapsed = difftime(now, lastInsertionTime);
    time (&lastInsertionTime);

    if (numberOfNotesOn <= 0 || elapsed > 10.0 ) {
        numberOfNotesOn = 0;
        insertionTime = getInsertionTime();
    }
    numberOfNotesOn++;
    

    MATRIX_DEBUG << "Inserting note at pitch " << pitch;

    Event modelEvent(Note::EventType, 0, 1);
    modelEvent.set<Int>(BaseProperties::PITCH, pitch);
    modelEvent.set<Int>(BaseProperties::VELOCITY, velocity);

    timeT segStartTime = segment->getStartTime();
    if ((insertionTime < segStartTime) ||
            (insertionTime > segment->getEndMarkerTime())) {
        MATRIX_DEBUG << "WARNING: off of segment -- "
                     <<"moving to start of segment";
        insertionTime = segStartTime;
    }

    timeT endTime(insertionTime + getSnapGrid()->getSnapTime(insertionTime));

    if (endTime <= insertionTime) {
        // Fail silently, as in notation view
        return;
    }

    MatrixInsertionCommand* command = 
        new MatrixInsertionCommand(*segment, insertionTime,
                                   endTime, &modelEvent);

    CommandHistory::getInstance()->addCommand(command);

    if (!m_inChordMode) {
        m_document->slotSetPointerPosition(endTime);
    }
}

void
MatrixView::slotInsertableNoteOnReceived(int pitch, int velocity)
{
    MATRIX_DEBUG << "MatrixView::slotInsertableNoteOnReceived: " << pitch;
    slotInsertableNoteEventReceived(pitch, velocity, true);
}

void
MatrixView::slotInsertableNoteOffReceived(int pitch, int velocity)
{
    MATRIX_DEBUG << "MatrixView::slotInsertableNoteOffReceived: " << pitch;
    slotInsertableNoteEventReceived(pitch, velocity, false);
}

void
MatrixView::slotPitchBendSequence()
{
    insertControllerSequence(ControlParameter::getPitchBend());
}

void
MatrixView::slotControllerSequence()
{
    ControlRulerWidget *cr = m_matrixWidget->getControlsWidget();
    if (!cr) { return; }
    
    const ControlParameter *cp = cr->getControlParameter();
    if (!cp) { return; }

    insertControllerSequence(*cp);
}

void
MatrixView::
insertControllerSequence(const ControlParameter &cp)
{
    timeT startTime=0;
    timeT endTime=0;

    if (getSelection()) {
        startTime=getSelection()->getStartTime();
        endTime=getSelection()->getEndTime();
    } else {
        startTime = getInsertionTime();
    }

    PitchBendSequenceDialog dialog(this, getCurrentSegment(), cp, startTime,
                                    endTime);
    dialog.exec();
}

void
MatrixView::slotInsertNoteFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    int pitch = 0;

    Accidental accidental =
        Accidentals::NoAccidental;

    timeT time(getInsertionTime());
    if (time >= segment->getEndMarkerTime()) {
        MATRIX_DEBUG << "WARNING: off end of segment";
        return ;
    }
    ::Rosegarden::Key key = segment->getKeyAtTime(time);
    Clef clef = segment->getClefAtTime(time);

    try {

        pitch = getPitchFromNoteInsertAction(name, accidental, clef, key);

    } catch (...) {

        QMessageBox::warning(this, tr("Rosegarden"), tr("Unknown note insert action %1").arg(name));
        return ;
    }

//    TmpStatusMsg msg(tr("Inserting note"), this);

    MATRIX_DEBUG << "Inserting note at pitch " << pitch;

    Event modelEvent(Note::EventType, 0, 1);
    modelEvent.set<Int>(BaseProperties::PITCH, pitch);
    modelEvent.set<String>(BaseProperties::ACCIDENTAL, accidental);
    timeT endTime(time + getSnapGrid()->getSnapTime(time));

    MatrixInsertionCommand* command =
        new MatrixInsertionCommand(*segment, time, endTime, &modelEvent);

    CommandHistory::getInstance()->addCommand(command);

    if (!m_inChordMode) {
        m_document->slotSetPointerPosition(endTime);
    }

    emit noteInsertedFromKeyboard(segment, pitch);
    //  ==> MatrixWidget::slotPlayPreviewNote() ==> MatrixScene::playNote()
}

void
MatrixView::slotToggleChordMode()
{
    m_inChordMode = !m_inChordMode;

    // bits to update status bar if/when we ever have one again
}


int
MatrixView::getPitchFromNoteInsertAction(QString name,
                                              Accidental &accidental,
                                              const Clef &clef,
                                              const Rosegarden::Key &key)
{
    using namespace Accidentals;

    accidental = NoAccidental;

    if (name.left(7) == "insert_") {

        name = name.right(name.length() - 7);

        // int modify = 0;
        int octave = 0;

        if (name.right(5) == "_high") {

            octave = 1;
            name = name.left(name.length() - 5);

        } else if (name.right(4) == "_low") {

            octave = -1;
            name = name.left(name.length() - 4);
        }

        if (name.right(6) == "_sharp") {

            // modify = 1;
            accidental = Sharp;
            name = name.left(name.length() - 6);

        } else if (name.right(5) == "_flat") {

            // modify = -1;
            accidental = Flat;
            name = name.left(name.length() - 5);
        }

        int scalePitch = name.toInt();

        if (scalePitch < 0 || scalePitch > 7) {
            NOTATION_DEBUG << "MatrixView::getPitchFromNoteInsertAction: pitch "
            << scalePitch << " out of range, using 0";
            scalePitch = 0;
        }

        Pitch clefPitch(clef.getAxisHeight(), clef, key, NoAccidental);

        int pitchOctave = clefPitch.getOctave() + octave;

        MATRIX_DEBUG << "MatrixView::getPitchFromNoteInsertAction:"
                  << " key = " << key.getName() 
                  << ", clef = " << clef.getClefType() 
                  << ", octaveoffset = " << clef.getOctaveOffset();
        MATRIX_DEBUG << "MatrixView::getPitchFromNoteInsertAction: octave = " << pitchOctave;

        // We want still to make sure that when (i) octave = 0,
        //  (ii) one of the noteInScale = 0..6 is
        //  (iii) at the same heightOnStaff than the heightOnStaff of the key.
        int lowestNoteInScale = 0;
        Pitch lowestPitch(lowestNoteInScale, clefPitch.getOctave(), key, NoAccidental);

        int heightToAdjust = (clefPitch.getHeightOnStaff(clef, key) - lowestPitch.getHeightOnStaff(clef, key));
        for (; heightToAdjust < 0; heightToAdjust += 7) pitchOctave++;
        for (; heightToAdjust > 6; heightToAdjust -= 7) pitchOctave--;

        MATRIX_DEBUG << "MatrixView::getPitchFromNoteInsertAction: octave = " << pitchOctave << " (adjusted)";

        Pitch pitch(scalePitch, pitchOctave, key, accidental);
        return pitch.getPerformancePitch();

    } else {

        throw Exception("Not an insert action",
                        __FILE__, __LINE__);
    }
}


void
MatrixView::toggleNamedToolBar(const QString& toolBarName, bool* force)
{
    QToolBar *namedToolBar = findChild<QToolBar*>(toolBarName);

    if (!namedToolBar) {
        MATRIX_DEBUG << "MatrixView::toggleNamedToolBar() : toolBar "
                       << toolBarName << " not found";
        return ;
    }

    if (!force) {

        if (namedToolBar->isVisible())
            namedToolBar->hide();
        else
            namedToolBar->show();
    } else {

        if (*force)
            namedToolBar->show();
        else
            namedToolBar->hide();
    }
}

void
MatrixView::slotToggleGeneralToolBar()
{
    toggleNamedToolBar("General Toolbar");
}

void
MatrixView::slotToggleToolsToolBar()
{
    toggleNamedToolBar("Tools Toolbar");
}

void
MatrixView::slotToggleTransportToolBar()
{
    toggleNamedToolBar("Transport Toolbar");
}

void
MatrixView::slotToggleActionsToolBar()
{
    toggleNamedToolBar("Actions Toolbar");
}

void
MatrixView::slotToggleRulersToolBar()
{
    toggleNamedToolBar("Rulers Toolbar");
}

void
MatrixView::slotToggleStepByStep()
{
    QAction *action = findAction("toggle_step_by_step");
            
    if (!action) {
        MATRIX_DEBUG << "WARNING: No toggle_step_by_step action";
        return ;
    }
    if (action->isChecked()) { // after toggling, that is
        emit stepByStepTargetRequested(this);
    } else {
        emit stepByStepTargetRequested(0);
    }
}

void
MatrixView::slotStepByStepTargetRequested(QObject *obj)
{
    QAction *action = findAction("toggle_step_by_step");
          
    if (!action) {
        MATRIX_DEBUG << "WARNING: No toggle_step_by_step action";
        return ;
    }
    action->setChecked(obj == this);
}

Device *
MatrixView::getCurrentDevice()
{
    if (m_matrixWidget) return m_matrixWidget->getCurrentDevice();
    else return 0;
}

void
MatrixView::slotExtendSelectionBackward()
{
    slotExtendSelectionBackward(false);
}

void
MatrixView::slotExtendSelectionBackwardBar()
{
    slotExtendSelectionBackward(true);
}

void
MatrixView::slotExtendSelectionBackward(bool bar)
{
    // If there is no current selection, or the selection is entirely
    // to the right of the cursor, move the cursor left and add to the
    // selection

    timeT oldTime = getInsertionTime();

    if (bar) emit rewindPlayback();
    else slotStepBackward();

    timeT newTime = getInsertionTime();

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    MatrixViewSegment *vs = m_matrixWidget->getScene()->getCurrentViewSegment();
    ViewElementList *vel = vs->getViewElementList(); 
    EventSelection *s = getSelection();
    EventSelection *es = new EventSelection(*segment);

    if (s && &s->getSegment() == segment) es->addFromSelection(s);

    if (!s || &s->getSegment() != segment
           || s->getSegmentEvents().size() == 0
           || s->getStartTime() >= oldTime) {

        ViewElementList::iterator extendFrom = vel->findTime(oldTime);

        while (extendFrom != vel->begin() &&
                (*--extendFrom)->getViewAbsoluteTime() >= newTime) {

            //!!! This should actually grab every sort of event, and not just
            // notes, but making that change makes the selection die every time
            // the endpoint of an indication is encountered, and I'm just not
            // seeing why, so I'm giving up on that and leaving it in the same
            // stupid state I found it in (and it's probably in this state
            // because the last guy had the same problem with indications.)
            //
            // I don't like this, because it makes it very easy to go along and
            // orphan indications, text events, controllers, and all sorts of
            // whatnot.  However, I have to call it quits for today, and have no
            // idea if I'll ever remember to come back to this, so I'm leaving a
            // reminder to someone that all of this is stupid.
            //
            // Note that here in the matrix, we still wouldn't want to orphan
            // indications, etc., even though they're not visible from here.
            
            if ((*extendFrom)->event()->isa(Note::EventType)) {
                es->addEvent((*extendFrom)->event());
            }
        }

    } else { // remove an event

        EventSelection::eventcontainer::iterator i =
            es->getSegmentEvents().end();

        std::vector<Event *> toErase;

        while (i != es->getSegmentEvents().begin() &&
                (*--i)->getAbsoluteTime() >= newTime) {
            toErase.push_back(*i);
        }

        for (unsigned int j = 0; j < toErase.size(); ++j) {
            es->removeEvent(toErase[j]);
        }
    }

    setSelection(es, true);
}

void
MatrixView::slotExtendSelectionForward()
{
    slotExtendSelectionForward(false);
}

void
MatrixView::slotExtendSelectionForwardBar()
{
    slotExtendSelectionForward(true);
}

void
MatrixView::slotExtendSelectionForward(bool bar)
{
    // If there is no current selection, or the selection is entirely
    // to the left of the cursor, move the cursor right and add to the
    // selection

    timeT oldTime = getInsertionTime();

    if (bar) emit fastForwardPlayback();
    else slotStepForward(true);

    timeT newTime = getInsertionTime();

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    MatrixViewSegment *vs = m_matrixWidget->getScene()->getCurrentViewSegment();
    ViewElementList *vel = vs->getViewElementList(); 
    EventSelection *s = getSelection();
    EventSelection *es = new EventSelection(*segment);

    if (s && &s->getSegment() == segment) es->addFromSelection(s);

    if (!s || &s->getSegment() != segment
           || s->getSegmentEvents().size() == 0
           || s->getEndTime() <= oldTime) {

        ViewElementList::iterator extendFrom = vel->findTime(oldTime);

        while (extendFrom != vel->end() &&
                (*extendFrom)->getViewAbsoluteTime() < newTime)  {
            if ((*extendFrom)->event()->isa(Note::EventType)) {
                es->addEvent((*extendFrom)->event());
            }
            ++extendFrom;
        }

    } else { // remove an event

        EventSelection::eventcontainer::iterator i =
            es->getSegmentEvents().begin();

        std::vector<Event *> toErase;

        while (i != es->getSegmentEvents().end() &&
                (*i)->getAbsoluteTime() < newTime) {
            toErase.push_back(*i);
            ++i;
        }

        for (unsigned int j = 0; j < toErase.size(); ++j) {
            es->removeEvent(toErase[j]);
        }
    }

    setSelection(es, true);
}


void
MatrixView::slotEditAddKeySignature()
{
    Segment *segment = getCurrentSegment();
    timeT insertionTime = getInsertionTime();
    Clef clef = segment->getClefAtTime(insertionTime);
    Key key = AnalysisHelper::guessKeyForSegment(insertionTime, segment);

    MatrixScene *scene = m_matrixWidget->getScene();
    if (!scene) return;

    NotePixmapFactory npf;

    KeySignatureDialog dialog(this,
                              &npf,
                              clef,
                              key,
                              true,
                              true,
                              tr("Estimated key signature shown"));

    if (dialog.exec() == QDialog::Accepted &&
        dialog.isValid()) {

        KeySignatureDialog::ConversionType conversion =
            dialog.getConversionType();

        bool transposeKey = dialog.shouldBeTransposed();
        bool applyToAll = dialog.shouldApplyToAll();
        bool ignorePercussion = dialog.shouldIgnorePercussion();

        if (applyToAll) {
            CommandHistory::getInstance()->addCommand(
                    new MultiKeyInsertionCommand(
                            getDocument(),
                            insertionTime, dialog.getKey(),
                            conversion == KeySignatureDialog::Convert,
                            conversion == KeySignatureDialog::Transpose,
                            transposeKey,
                            ignorePercussion));
        } else {
            CommandHistory::getInstance()->addCommand(
                    new KeyInsertionCommand(*segment,
                                            insertionTime,
                                            dialog.getKey(),
                                            conversion == KeySignatureDialog::Convert,
                                            conversion == KeySignatureDialog::Transpose,
                                            transposeKey,
                                            false));
        }
    }
}

void
MatrixView::slotJogLeft()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;

    MATRIX_DEBUG << "MatrixView::slotJogLeft";

    bool useNotationTimings = false;

    CommandHistory::getInstance()->addCommand(new MoveCommand
                                              (*getCurrentSegment(),
                                              -Note(Note::Demisemiquaver).getDuration(),
                                              useNotationTimings,
                                              *selection));
}

void
MatrixView::slotJogRight()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;

    MATRIX_DEBUG << "MatrixView::slotJogRight";

    bool useNotationTimings = false;

    CommandHistory::getInstance()->addCommand(new MoveCommand
                                              (*getCurrentSegment(),
                                              Note(Note::Demisemiquaver).getDuration(),
                                              useNotationTimings,
                                              *selection));
}


}
