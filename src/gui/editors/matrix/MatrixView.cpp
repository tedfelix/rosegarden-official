/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical matrix editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MatrixView]"
#define RG_NO_DEBUG_PRINT 1

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
#include "misc/Preferences.h"
#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"

#include "gui/dialogs/AboutDialog.h"
#include "gui/dialogs/QuantizeDialog.h"
#include "gui/dialogs/EventFilterDialog.h"
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

#include <algorithm>


namespace Rosegarden
{


MatrixView::MatrixView(RosegardenDocument *doc,
                       const std::vector<Segment *>& segments,
                       bool drumMode) :
    EditViewBase(segments),
    m_quantizations(Quantizer::getQuantizations()),
    m_drumMode(drumMode),
    m_inChordMode(false)
{
    m_document = doc;
    m_matrixWidget = new MatrixWidget(m_drumMode);
    setCentralWidget(m_matrixWidget);
    m_matrixWidget->setSegments(doc, segments);

    // Many actions are created here
    // Actually, just tie and untie.
    m_commandRegistry.reset(new MatrixCommandRegistry(this));

    setupActions();

    createMenusAndToolbars("matrix.rc");

    findToolbar("General Toolbar");

    m_Thorn = ThornStyle::isEnabled();

    initActionsToolbar();
    initRulersToolbar();
    initStatusBar();

    connect(m_matrixWidget, &MatrixWidget::editTriggerSegment,
            this, &MatrixView::editTriggerSegment);

    connect(m_matrixWidget, &MatrixWidget::showContextHelp,
            this, &MatrixView::slotShowContextHelp);

    slotUpdateMenuStates();
    slotUpdateClipboardActionState();

    connect(CommandHistory::getInstance(), &CommandHistory::commandExecuted,
            this, &MatrixView::slotUpdateMenuStates);

    connect(m_matrixWidget, &MatrixWidget::selectionChanged,
            this, &MatrixView::slotUpdateMenuStates);
    connect(m_matrixWidget, &MatrixWidget::rulerSelectionChanged,
            this, &MatrixView::slotUpdateMenuStates);
    connect(m_matrixWidget, &MatrixWidget::rulerSelectionUpdate,
            this, &MatrixView::slotRulerSelectionUpdate);

    // Toggle the desired tool off and then trigger it on again, to
    // make sure its signal is called at least once (as would not
    // happen if the tool was on by default otherwise)
    QAction *toolAction = nullptr;
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

    m_scrollToFollow = m_document->getComposition().getEditorFollowPlayback();
    findAction("scroll_to_follow")->setChecked(m_scrollToFollow);
    m_matrixWidget->setScrollToFollowPlayback(m_scrollToFollow);

    slotUpdateWindowTitle();
    connect(m_document, &RosegardenDocument::documentModified,
            this, &MatrixView::slotUpdateWindowTitle);

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

    findAction("show_note_names")->
        setChecked(Preferences::getShowNoteNames());
    MatrixScene::HighlightType chosenHighlightType =
        static_cast<MatrixScene::HighlightType>(
            settings.value("highlight_type",
                           MatrixScene::HT_BlackKeys).toInt());
    settings.endGroup();
    switch (chosenHighlightType) {
    case MatrixScene::HT_BlackKeys:
        findAction("highlight_black_notes")->setChecked(true);
        break;
    case MatrixScene::HT_Triads:
        findAction("highlight_triads")->setChecked(true);
        break;
    default:
        findAction("highlight_black_notes")->setChecked(true);
        break;
    }
    settings.endGroup();

    bool constrain = Preferences::getMatrixConstrainNotes();
    findAction("constrained_move")->setChecked(constrain);

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
    restoreGeometry(settings.value(modeStr).toByteArray());
    modeStr = (m_drumMode ? "Percussion_Matrix_View_State" : "Matrix_View_State");
    restoreState(settings.value(modeStr).toByteArray());
    settings.endGroup();

    connect(m_matrixWidget, SIGNAL(segmentDeleted(Segment *)),
            this, SLOT(slotSegmentDeleted(Segment *)));
    connect(m_matrixWidget, &MatrixWidget::sceneDeleted,
            this, &MatrixView::slotSceneDeleted);

    connect(this, &MatrixView::noteInsertedFromKeyboard,
            m_matrixWidget, &MatrixWidget::slotPlayPreviewNote);

    // Set the rewind and fast-forward buttons for auto-repeat.
    enableAutoRepeat("Transport Toolbar", "playback_pointer_back_bar");
    enableAutoRepeat("Transport Toolbar", "playback_pointer_forward_bar");
    enableAutoRepeat("Transport Toolbar", "cursor_back");
    enableAutoRepeat("Transport Toolbar", "cursor_forward");

    connect(RosegardenDocument::currentDocument,
                &RosegardenDocument::loopChanged,
            this, &MatrixView::slotLoopChanged);
    // Make sure we are in sync.
    slotLoopChanged();

    // Show the pointer as soon as matrix editor opens (update pointer position,
    // but don't scroll)
    m_matrixWidget->showInitialPointer();

    readOptions();

    show();
    // We have to do this after show() because the rulers need information
    // that isn't available until the MatrixView is shown.  (xScale)
    launchRulers(segments);
}

MatrixView::~MatrixView()
{
    MATRIX_DEBUG << "MatrixView::~MatrixView()";
}

void
MatrixView::launchRulers(std::vector<Segment *> segments)
{
    if (!m_matrixWidget)
        return;

    ControlRulerWidget *controlRulerWidget =
            m_matrixWidget->getControlsWidget();

    if (!controlRulerWidget)
        return;

    controlRulerWidget->launchMatrixRulers(segments);
    // and tell the rulers the snap setting
    controlRulerWidget->setSnapFromEditor(getSnapGrid()->getSnapSetting());
}

void
MatrixView::closeEvent(QCloseEvent *event)
{
    // Save window geometry and toolbar/dock state
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    QString modeStr = (m_drumMode ? "Percussion_Matrix_View_Geometry" : "Matrix_View_Geometry");
    settings.setValue(modeStr, saveGeometry());
    modeStr = (m_drumMode ? "Percussion_Matrix_View_State" : "Matrix_View_State");
    settings.setValue(modeStr, saveState());
    settings.endGroup();

    QWidget::closeEvent(event);
}

void
MatrixView::slotSegmentDeleted(Segment *s)
{
    RG_DEBUG << "slotSegmentDeleted()";

    std::vector<Segment *>::const_iterator segmentIter =
            std::find(m_segments.begin(), m_segments.end(), s);

    // If found, delete it.
    if (segmentIter != m_segments.end())
        m_segments.erase(segmentIter);

    RG_DEBUG << "  Segments remaining:" << m_segments.size();
}

void
MatrixView::slotSceneDeleted()
{
    NOTATION_DEBUG << "MatrixView::slotSceneDeleted";

    m_segments.clear();
    close();
}

void
MatrixView::slotUpdateWindowTitle(bool)
{
    // Set client label
    //
    QString view = tr("Matrix");
    //&&&if (isDrumMode())
    //    view = tr("Percussion");

    if (m_segments.empty()) return;

    setWindowTitle(getTitle(view));

    setWindowIcon(IconLoader::loadPixmap("window-matrix"));
}

void
MatrixView::setupActions()
{

    setupBaseActions();

    createAction("edit_cut", SLOT(slotEditCut()));
    createAction("edit_copy", SLOT(slotEditCopy()));
    createAction("edit_paste", SLOT(slotEditPaste()));

    createAction("select", SLOT(slotSetSelectTool()));
    createAction("draw", SLOT(slotSetPaintTool()));
    createAction("erase", SLOT(slotSetEraseTool()));
    createAction("move", SLOT(slotSetMoveTool()));
    createAction("resize", SLOT(slotSetResizeTool()));
    createAction("velocity", SLOT(slotSetVelocityTool()));
    createAction("chord_mode", SLOT(slotToggleChordMode()));
    QAction *action = createAction(
            "constrained_move", &MatrixView::slotConstrainedMove);
    action->setStatusTip(tr("Preserves precise horizontal and vertical position when moving a note."));
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
    createAction("scroll_to_follow", SLOT(slotScrollToFollow()));
    createAction("loop", SLOT(slotLoop()));
    createAction("panic", SIGNAL(panic()));
    createAction("preview_selection", SLOT(slotPreviewSelection()));
    createAction("clear_loop", SLOT(slotClearLoop()));
    createAction("clear_selection", SLOT(slotClearSelection()));
    createAction("reset_selection", SLOT(slotEscapePressed()));
    createAction("filter_selection", SLOT(slotFilterSelection()));

    createAction("pitch_bend_sequence", SLOT(slotPitchBendSequence()));

    //"controllers" Menubar menu
    createAction("controller_sequence", SLOT(slotControllerSequence()));
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

            const QString hexValue =
                QString::asprintf("(0x%x)", it->getControllerNumber());

            // strings extracted from data files and related to MIDI
            // controller are in MIDI_CONTROLLER translation context
            itemStr = tr("%1 Controller %2 %3")
                    .arg(QCoreApplication::translate("MIDI_CONTROLLER",
                                                    it->getName().c_str()))
                    .arg(it->getControllerNumber())
                    .arg(hexValue);

            addControlRulerMenu->addAction(itemStr);
        }
    }

    connect(addControlRulerMenu, &QMenu::triggered,
            this, &MatrixView::slotAddControlRuler);

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

    createAction("show_note_names", SLOT(slotShowNames()));
    createAction("highlight_black_notes", SLOT(slotHighlight()));
    createAction("highlight_triads", SLOT(slotHighlight()));
    // add the highlight actions to an ActionGroup
    QActionGroup *ag = new QActionGroup(this);
    ag->addAction(findAction("highlight_black_notes"));
    ag->addAction(findAction("highlight_triads"));

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
    // Put some space between this and the previous widget.
    sLabel->setContentsMargins(10,0,0,0);
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

    m_snapGridCombo->setMaxVisibleItems(m_snapValues.size());

    connect(m_snapGridCombo,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &MatrixView::slotSetSnapFromIndex);

    // Velocity combo.  Not a spin box, because the spin box is too
    // slow to use unless we make it typeable into, and then it takes
    // focus away from our more important widgets

    QLabel *vlabel = new QLabel(tr(" Velocity: "), actionsToolbar);
    // Put some space between this and the previous widget.
    vlabel->setContentsMargins(10,0,0,0);
    QString toolTip = tr("<qt>Velocity for new notes.</qt>");
    vlabel->setToolTip(toolTip);
    actionsToolbar->addWidget(vlabel);

    m_velocityCombo = new QComboBox(actionsToolbar);
    m_velocityCombo->setToolTip(toolTip);
    actionsToolbar->addWidget(m_velocityCombo);

    for (int i = 0; i <= 127; ++i) {
        m_velocityCombo->addItem(QString("%1").arg(i));
    }
    // ??? Would be nice to persist this on a segment-by-segment basis.
    m_velocityCombo->setCurrentIndex(100);
    connect(m_velocityCombo,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            m_matrixWidget, &MatrixWidget::slotSetCurrentVelocity);

    // Quantize combo
    //
    QLabel *qLabel = new QLabel(tr(" Quantize: "), actionsToolbar);
    // Put some space between this and the previous widget.
    qLabel->setContentsMargins(10,0,0,0);
    toolTip = tr("<qt><p>Quantize the display.</p><p>Notes with start times that are not aligned to the quantize setting are displayed as being aligned.</p></qt>");
    qLabel->setToolTip(toolTip);
    actionsToolbar->addWidget(qLabel);

    m_quantizeCombo = new QComboBox(actionsToolbar);
    m_quantizeCombo->setToolTip(toolTip);
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

    connect(m_quantizeCombo,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &MatrixView::slotQuantizeSelection);
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
    // ??? Can we move these to setupActions()?  Is setupActions() called
    //     after the toolbars are restored (restoreState())?  No.  It is called
    //     in the ctor before restoreState().  Probably need to review and
    //     reorganize the ctor.

    // ??? findAction() and findToolbar() are both in ActionFileClient.
    //     Make this clumsy two-liner a member of ActionFileClient:
    //       syncToolbarCheck(const QString &action, const QString &toolbar);
    findAction("options_show_toolbar")->setChecked(
            !findToolbar("General Toolbar")->isHidden());
    findAction("show_tools_toolbar")->setChecked(
            !findToolbar("Tools Toolbar")->isHidden());
    findAction("show_transport_toolbar")->setChecked(
            !findToolbar("Transport Toolbar")->isHidden());
    findAction("show_actions_toolbar")->setChecked(
            !findToolbar("Actions Toolbar")->isHidden());
    findAction("show_rulers_toolbar")->setChecked(
            !findToolbar("Rulers Toolbar")->isHidden());
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
    EventSelection *selection = getSelection();

    // Note Selection

    const bool haveNoteSelection =
            (selection  &&  !selection->getSegmentEvents().empty());

    if (haveNoteSelection)
        enterActionState("have_note_selection");
    else
        leaveActionState("have_note_selection");

    // Controller Selection

    ControlRulerWidget *controlRulerWidget =
            m_matrixWidget->getControlsWidget();

    bool haveControllerSelection = false;

    if (controlRulerWidget->isAnyRulerVisible()) {
        enterActionState("have_control_ruler");

        if (controlRulerWidget->hasSelection()) {
            enterActionState("have_controller_selection");
            haveControllerSelection = true;
        } else {
            leaveActionState("have_controller_selection");
        }
    } else {
        leaveActionState("have_control_ruler");
        // No ruler implies no controller selection
        leaveActionState("have_controller_selection");
    }

    // "have_selection" is enabled when either of the others is.
    if (haveNoteSelection  ||  haveControllerSelection)
        enterActionState("have_selection");
    else
        leaveActionState("have_selection");
}

void
MatrixView::slotRulerSelectionUpdate()
{
    // Special case for the velocity ruler.  At the end of a velocity
    // adjustment, sync up the ruler's selection with the matrix.
    // This will allow adjustment of velocity bars one after another
    // if nothing is selected on the Matrix.

    // Called by ControlRuler::updateSelection() via signal chain.
    // See ControlRuler::updateSelection() for details.

    ControlRulerWidget *crw = m_matrixWidget->getControlsWidget();
    if (!crw)
        return;

    // No ruler visible?  Bail.
    if (!crw->isAnyRulerVisible())
        return;

    crw->slotSelectionChanged(getSelection());
}

void
MatrixView::slotSetPaintTool()
{
    if (m_matrixWidget)
        m_matrixWidget->setDrawTool();
}

void
MatrixView::slotSetEraseTool()
{
    if (m_matrixWidget)
        m_matrixWidget->setEraseTool();
}

void
MatrixView::slotSetSelectTool()
{
    if (m_matrixWidget)
        m_matrixWidget->setSelectAndEditTool();
}

void
MatrixView::slotSetMoveTool()
{
    if (m_matrixWidget)
        m_matrixWidget->setMoveTool();
}

void
MatrixView::slotSetResizeTool()
{
    if (m_matrixWidget)
        m_matrixWidget->setResizeTool();
}

void
MatrixView::slotSetVelocityTool()
{
    if (m_matrixWidget)
        m_matrixWidget->setVelocityTool();
}

Segment *
MatrixView::getCurrentSegment()
{
    if (m_matrixWidget) return m_matrixWidget->getCurrentSegment();
    else return nullptr;
}

EventSelection *
MatrixView::getSelection() const
{
    if (!m_matrixWidget)
        return nullptr;

    return m_matrixWidget->getSelection();
}

void
MatrixView::setSelection(EventSelection *s, bool preview)
{
    if (m_matrixWidget) m_matrixWidget->setSelection(s, preview);
}

EventSelection *
MatrixView::getRulerSelection() const
{
    if (!m_matrixWidget)
        return nullptr;

    return m_matrixWidget->getRulerSelection();
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
    else return nullptr;
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
    m_matrixWidget->setSnap(t);

    for (unsigned int i = 0; i < m_snapValues.size(); ++i) {
        if (m_snapValues[i] == t) {
            m_snapGridCombo->setCurrentIndex(i);
            break;
        }
    }
    // and tell the rulers
    ControlRulerWidget * cr = m_matrixWidget->getControlsWidget();
    cr->setSnapFromEditor(t);

}

void
MatrixView::slotEditCut()
{
    const bool haveSelection = (getSelection()  &&  !getSelection()->empty());
    const bool haveRulerSelection =
            (getRulerSelection()  &&  !getRulerSelection()->empty());

    // Have neither?  Bail.
    if (!haveSelection  &&  !haveRulerSelection)
        return;

    CommandHistory::getInstance()->addCommand(
            new CutCommand(getSelection(),
                           getRulerSelection(),
                           Clipboard::mainClipboard()));
}

void
MatrixView::slotEditCopy()
{
    const bool haveSelection = (getSelection()  &&  !getSelection()->empty());
    const bool haveRulerSelection =
            (getRulerSelection()  &&  !getRulerSelection()->empty());

    // Have neither?  Bail.
    if (!haveSelection  &&  !haveRulerSelection)
        return;

    CommandHistory::getInstance()->addCommand(
            new CopyCommand(getSelection(),
                           getRulerSelection(),
                           Clipboard::mainClipboard()));
}

void
MatrixView::slotEditPaste()
{
    if (Clipboard::mainClipboard()->isEmpty()) return;

    PasteEventsCommand *command = new PasteEventsCommand
        (*m_matrixWidget->getCurrentSegment(),
         Clipboard::mainClipboard(),
         getInsertionTime(),
         PasteEventsCommand::MatrixOverlay);

    if (!command->isPossible()) {
        return;
    } else {
        CommandHistory::getInstance()->addCommand(command);

        // PasteEventsCommand class does not override its
        // BasicCommand base class's getSubsequentSelection()
        // which always returns null.
        // Therefore setSelection() always simply clears the current
        // selection, which is unneccesary and counter-intuitive.
     // setSelection(command->getSubsequentSelection(), false);
    }
}

void
MatrixView::slotEditDelete()
{
    const bool haveSelection = (getSelection()  &&  !getSelection()->empty());
    const bool haveRulerSelection =
            (getRulerSelection()  &&  !getRulerSelection()->empty());

    // Have neither?  Bail.
    if (!haveSelection  &&  !haveRulerSelection)
        return;

    CommandHistory::getInstance()->addCommand(
            new EraseCommand(getSelection(),
                             getRulerSelection()));
}


void
MatrixView::slotQuantizeSelection(int q)
{
    MATRIX_DEBUG << "MatrixView::slotQuantizeSelection\n";

    timeT unit =
        ((unsigned int)q < m_quantizations.size() ? m_quantizations[q] : 0);

    std::shared_ptr<Quantizer> quant(new BasicQuantizer(
            unit ? unit : Note(Note::Shortest).getDuration(),  // unit
            false));  // doDurations

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
        if (selection  &&  !selection->empty()) {
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
    if (!getSelection())
        return;

    QuantizeDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand(new EventQuantizeCommand(
                *getSelection(),
                dialog.getQuantizer()));
    }
}

void
MatrixView::slotRepeatQuantize()
{
    if (!getSelection())
        return;

    CommandHistory::getInstance()->addCommand(new EventQuantizeCommand(
            *getSelection(),  // selection
            "Quantize Dialog Grid",  // settingsGroup
            EventQuantizeCommand::QUANTIZE_NORMAL));  // scope
}

void
MatrixView::slotCollapseNotes()
{
    if (!getSelection())
        return;

    // in matrix editor do not split notes at bars
    CommandHistory::getInstance()->addCommand(new CollapseNotesCommand(
            *getSelection(),  // selection
            false,  // makeViable
            false));  // autoBeam
}

void
MatrixView::slotLegato()
{
    if (!getSelection())
        return;

    // No quantization.
    std::shared_ptr<Quantizer> quantizer(new LegatoQuantizer(0));

    CommandHistory::getInstance()->addCommand(new EventQuantizeCommand(
            *getSelection(),  // selection
            quantizer));
}

void
MatrixView::slotVelocityUp()
{
    if (!getSelection())
        return;

    CommandHistory::getInstance()->addCommand(new ChangeVelocityCommand(
            10,  // delta
            *getSelection()));  // selection

    slotSetCurrentVelocityFromSelection();
}

void
MatrixView::slotVelocityDown()
{
    if (!getSelection())
        return;

    CommandHistory::getInstance()->addCommand(new ChangeVelocityCommand(
            -10,  // delta
            *getSelection()));  //selection

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
MatrixView::slotSetControllers()
{
    ControlRulerWidget * cr = m_matrixWidget->getControlsWidget();
    ParameterPattern::setProperties(
            this,
            tr("Set Controller Values"),
            cr->getSituation(),
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
        RosegardenDocument::currentDocument->getInstrument(getCurrentSegment());
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
    if (m_matrixWidget)
        m_matrixWidget->selectAll();
}

void
MatrixView::slotCurrentSegmentPrior()
{
    if (m_matrixWidget)
        m_matrixWidget->previousSegment();
}

void
MatrixView::slotCurrentSegmentNext()
{
    if (m_matrixWidget)
        m_matrixWidget->nextSegment();
}

void
MatrixView::slotPreviewSelection()
{
    if (!getSelection())
        return;

    Composition &composition = m_document->getComposition();

    composition.setLoopMode(Composition::LoopOn);
    composition.setLoopStart(getSelection()->getStartTime());
    composition.setLoopEnd(getSelection()->getEndTime());
    emit m_document->loopChanged();
}

void
MatrixView::slotClearLoop()
{
    // ??? Not sure why there is a Move > Clear Loop.  The LoopRuler
    //     is available.  One has full control of looping from there.

    Composition &composition = m_document->getComposition();

    // Less destructive.  Just turn it off.
    composition.setLoopMode(Composition::LoopOff);
    emit m_document->loopChanged();
}

void
MatrixView::slotClearSelection()
{
    if (m_matrixWidget) m_matrixWidget->clearSelection();
}

void MatrixView::slotEscapePressed()
{
    // Esc switches us back to the select tool (see bug #1615)
    auto *toolAction = findAction("select");
    if (!toolAction->isChecked()) {
        toolAction->setChecked(true);
        slotSetSelectTool();
    }

    // ... and clears selection
    m_matrixWidget->clearSelection();
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
        EventContainer &ec =
            existingSelection->getSegmentEvents();
        for (EventContainer::iterator i =
                    ec.begin(); i != ec.end(); ++i) {
            if (dialog.keepEvent(*i)) {
                haveEvent = true;
                newSelection->addEvent(*i);
            }
        }

        if (haveEvent) setSelection(newSelection, false);
        else setSelection(nullptr, false);
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

    for (EventContainer::iterator i =
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
MatrixView::slotScrollToFollow()
{
    m_scrollToFollow = !m_scrollToFollow;
    m_matrixWidget->setScrollToFollowPlayback(m_scrollToFollow);
    m_document->getComposition().setEditorFollowPlayback(m_scrollToFollow);
}

void
MatrixView::slotLoop()
{
    RosegardenDocument::currentDocument->loopButton(
            findAction("loop")->isChecked());
}

void
MatrixView::slotLoopChanged()
{
    Composition &composition =
        RosegardenDocument::currentDocument->getComposition();

    findAction("loop")->setChecked(
            (composition.getLoopMode() != Composition::LoopOff));
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
    m_matrixWidget->showVelocityRuler();
    slotUpdateMenuStates();
}

void
MatrixView::slotTogglePitchbendRuler()
{
    m_matrixWidget->showPitchBendRuler();
    slotUpdateMenuStates();
}

void
MatrixView::slotAddControlRuler(QAction *action)
{
    m_matrixWidget->addControlRuler(action);
    slotUpdateMenuStates();
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

    TempoDialog tempoDlg(this, RosegardenDocument::currentDocument, false);

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

    TimeSignatureDialog *dialog = nullptr;
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
                         &RosegardenDocument::currentDocument->getComposition(),
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
        RG_WARNING << "Hint: selection is nullptr in slotTranpose()";
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
        RG_WARNING << "Hint: selection is nullptr in slotInvert()";
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
    QDesktopServices::openUrl(QUrl(
            "https://www.rosegardenmusic.com/wiki/donations"));
}

void
MatrixView::slotShowNames()
{
    const bool show = findAction("show_note_names")->isChecked();
    //RG_DEBUG << "show names:" << show;
    Preferences::setShowNoteNames(show);
    m_matrixWidget->getScene()->updateAll();
}

void
MatrixView::slotHighlight()
{
    const QObject *s = sender();
    QString name = s->objectName();
    if (name == "highlight_black_notes") {
        RG_DEBUG << "highlight black notes";
        QSettings settings;
        settings.beginGroup(MatrixViewConfigGroup);
        settings.setValue("highlight_type", MatrixScene::HT_BlackKeys);
        settings.endGroup();
    }
    if (name == "highlight_triads") {
        RG_DEBUG << "highlight triads";
        QSettings settings;
        settings.beginGroup(MatrixViewConfigGroup);
        settings.setValue("highlight_type", MatrixScene::HT_Triads);
        settings.endGroup();
    }
    m_matrixWidget->getScene()->updateAll();
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
    if (!cr)
        return;

    const ControlParameter *cp = cr->getControlParameter();
    if (!cp)
    {
        QMessageBox::information(
                this,
                tr("Rosegarden"),
                tr("Please select a control ruler first."));

        return;
    }

    insertControllerSequence(*cp);
}

void
MatrixView::
insertControllerSequence(const ControlParameter &controlParameter)
{
    EventSelection *selection = getSelection();

    // No selection?  Bail.
    if (!selection)
        return;

    const timeT startTime = selection->getStartTime();
    const timeT endTime = selection->getEndTime();

    // Times make no sense?  Bail.
    if (startTime >= endTime)
        return;

    PitchBendSequenceDialog dialog(
            this,  // parent
            getCurrentSegment(),
            controlParameter,
            startTime,
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
MatrixView::slotConstrainedMove()
{
    // toggle constrain
    bool constrain = Preferences::getMatrixConstrainNotes();
    Preferences::setMatrixConstrainNotes(! constrain);
    findAction("constrained_move")->setChecked(! constrain);
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
        emit stepByStepTargetRequested(nullptr);
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
    else return nullptr;
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

        EventContainer::iterator i =
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

        EventContainer::iterator i =
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
                            RosegardenDocument::currentDocument,
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
