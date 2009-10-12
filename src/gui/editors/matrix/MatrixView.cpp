/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical matrix editor.
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

#include "MatrixWidget.h"
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
#include "commands/edit/RescaleCommand.h"
#include "commands/edit/RetrogradeCommand.h"
#include "commands/edit/RetrogradeInvertCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/segment/AddTempoChangeCommand.h"
#include "commands/segment/AddTimeSignatureAndNormalizeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"

#include "commands/matrix/MatrixInsertionCommand.h"

#include "gui/editors/notation/NotationStrings.h"
#include "gui/editors/notation/NotePixmapFactory.h"

#include "base/Quantizer.h"
#include "base/BasicQuantizer.h"
#include "base/LegatoQuantizer.h"
#include "base/BaseProperties.h"
#include "base/SnapGrid.h"
#include "base/Clipboard.h"
#include "base/AnalysisTypes.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/NotationTypes.h"

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
    
    setupActions();
    
    createGUI("matrix.rc");
     
    findToolbar("General Toolbar");

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    m_Thorn = settings.value("use_thorn_style", true).toBool();
    settings.endGroup();

    initActionsToolbar();
    initRulersToolbar();
    
    connect(m_matrixWidget, SIGNAL(editTriggerSegment(int)),
            this, SIGNAL(editTriggerSegment(int)));

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
        MATRIX_DEBUG << "initial state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked() << endl;
        if (toolAction->isChecked()) toolAction->toggle();
        MATRIX_DEBUG << "newer state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked() << endl;
        toolAction->trigger();
        MATRIX_DEBUG << "newest state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked() << endl;
    }

    m_matrixWidget->slotSetPlayTracking(m_tracking);

    updateWindowTitle();
    connect(m_document, SIGNAL(documentModified(bool)),
            this, SLOT(updateWindowTitle(bool)));

    // Set initial visibility ...
    bool view;

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
    
    readOptions(); // defined in EditViewBase
    
    settings.endGroup();

    // Restore window geometry
    settings.beginGroup(WindowGeometryConfigGroup);
    QString modeStr = (m_drumMode ? "Percussion_Matrix_View" : "Matrix_View");
    this->restoreGeometry(settings.value(modeStr).toByteArray());
    settings.endGroup();

    connect(m_matrixWidget, SIGNAL(segmentDeleted(Segment *)),
            this, SLOT(slotSegmentDeleted(Segment *)));
    connect(m_matrixWidget, SIGNAL(sceneDeleted()),
            this, SLOT(close()));
}


MatrixView::~MatrixView()
{
    RG_DEBUG << "MatrixView::~MatrixView()";
}

void
MatrixView::closeEvent(QCloseEvent *event)
{
    // Save window geometry
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    QString modeStr = (m_drumMode ? "Percussion_Matrix_View" : "Matrix_View");
    settings.setValue(modeStr, this->saveGeometry());
    settings.endGroup();

    QWidget::closeEvent(event);
}


void
MatrixView::updateWindowTitle(bool m)
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

    setIcon(IconLoader().loadPixmap("window-matrix"));
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

    createAction("pitch_bend_sequence", SLOT(slotPitchBendSequence()));    
    
    createAction("show_inst_parameters", SLOT(slotDockParametersBack()));
    createAction("show_chords_ruler", SLOT(slotToggleChordsRuler()));
    createAction("show_tempo_ruler", SLOT(slotToggleTempoRuler()));
    
    createAction("toggle_velocity_ruler", SLOT(slotToggleVelocityRuler()));
    createAction("toggle_pitchbend_ruler", SLOT(slotTogglePitchbendRuler()));
    createAction("add_control_ruler", SLOT(slotAddControlRuler()));
    
    // code just here temporarily as proof of concept
    QMenu *addControlRulerMenu = new QMenu;
    addControlRulerMenu->addAction("Glee!");
    addControlRulerMenu->addAction("Piffles!");
    addControlRulerMenu->addAction("Nee!");

    /*Controllable *c =
        dynamic_cast<MidiDevice *>(getCurrentDevice());
    if (!c) {
        c = dynamic_cast<SoftSynthDevice *>(getCurrentDevice());
        if (!c)
            return ;
    }

    const ControlList &list = c->getControlParameters();

    int i = 0;
    QString itemStr;

    for (ControlList::const_iterator it = list.begin();
            it != list.end(); ++it) {
        if (it->getType() == Controller::EventType) {
            QString hexValue;
            hexValue.sprintf("(0x%x)", it->getControllerValue());

            itemStr = tr("%1 Controller %2 %3")
                            .arg(strtoqstr(it->getName()))
                            .arg(it->getControllerValue())
                            .arg(hexValue);

        } else if (it->getType() == PitchBend::EventType)
            itemStr = tr("Pitch Bend");
        else
            itemStr = tr("Unsupported Event Type");

//                      addControlRulerMenu->addItem(itemStr, i++);
            addControlRulerMenu->addAction(itemStr); i++;   //@@@
    }

    connect(addControlRulerMenu, SIGNAL(activated(int)),
            SLOT(slotAddControlRuler(int)));*/

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

    //!!! fix: some of these may not be working
    createAction("show_tools_toolbar", SLOT(slot()));
    createAction("options_show_toolbar", SLOT(slot()));
    createAction("options_show_statusbar", SLOT(slot()));
    createAction("options_configure_keybinding", SLOT(slot()));
    createAction("options_configure_toolbars", SLOT(slot()));
    //createAction("options_configure", SLOT(slot()));        //!!! fix: memory access error
    createAction("insert_control_ruler_item", SLOT(slot()));
    createAction("erase_control_ruler_item", SLOT(slot()));
    createAction("clear_control_ruler_item", SLOT(slot()));
    createAction("start_control_line_item", SLOT(slot()));
    createAction("draw_property_line", SLOT(slot()));    
    createAction("select_all_properties", SLOT(slot()));

    createAction("help_contents", SLOT(slot()));
    createAction("tutorial", SLOT(slot()));
    createAction("guidelines", SLOT(slot()));
    
    createAction("language_switch", SLOT(slot()));
    createAction("help_online", SLOT(slot()));
    createAction("help_about_app", SLOT(slotHelpAbout()));
//     createAction("help_about_qt", SLOT(slot()));
    
    
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
    
    
    
    // actions formerly located in EditView
    // --
    
    //createInsertPitchActionMenu();
    
    createAction("add_tempo_change", SLOT(slotAddTempo()));
    createAction("add_time_signature", SLOT(slotAddTimeSignature()));
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
//     createAction("jog_left", SLOT(slotJogLeft()));
//     createAction("jog_right", SLOT(slotJogRight()));
    
}


void
MatrixView::initActionsToolbar()
{
    MATRIX_DEBUG << "MatrixView::initActionsToolbar" << endl;

    QToolBar *actionsToolbar = findToolbar("Actions Toolbar");
//    QToolBar *actionsToolbar = m_actionsToolBar;
    //actionsToolbar->setLayout(new QHBoxLayout(actionsToolbar));

    if (!actionsToolbar) {
        MATRIX_DEBUG << "MatrixView::initActionsToolbar - "
        << "tool bar not found" << endl;
        return ;
    }

    // There's some way to do this kind of thing with states or properties or
    // something, but I couldn't ever get it to work.  So, again, I'll just use
    // another hacky hard coded internal stylesheet.
    //
    QString comboStyle("QComboBox::enabled,QComboBox{ border: 1px solid #AAAAAA; border-radius: 3px; padding: 0 5px 0 5px; min-width: 2em; color: #000000; } QComboBox::enabled:hover, QComboBox:hover, QComboBox::drop-down:hover { background-color: #CCDFFF; } QComboBox::!editable, QComboBox::drop-down:!editable { background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #EEEEEE, stop:1 #DDDDDD); } QComboBox::!editable:on, QComboBox::drop-down:editable:on, { background-color: qlineargradient(spread:pad, x1:0, y1:1, x2:0, y2:0, stop:0 #E0E0E0, stop:1 #EEEEEE); } QComboBox::on { padding-top: 3px; padding-left: 4px; } QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 15px; } QComboBox::down-arrow { image: url(:pixmaps/style/arrow-down-small.png); } QComboBox::down-arrow:on { top: 1px; left: 1px; } QComboBox QAbstractItemView { border-image: url(:pixmaps/style/combo-dropdown.png) 1; selection-background-color: #80AFFF; selection-color: #FFFFFF; color: #000000; }");

    // The SnapGrid combo and Snap To... menu items
    //
    QLabel *sLabel = new QLabel(tr(" Grid: "), actionsToolbar);
    sLabel->setIndent(10);
    actionsToolbar->addWidget(sLabel);
    sLabel->setObjectName("Humbug");

    QPixmap noMap = NotePixmapFactory::makeToolbarPixmap("menu-no-note");

    m_snapGridCombo = new QComboBox(actionsToolbar);
    if (m_Thorn) m_snapGridCombo->setStyleSheet(comboStyle);
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
    vlabel->setObjectName("Humbug");
    actionsToolbar->addWidget(vlabel);

    m_velocityCombo = new QComboBox(actionsToolbar);
    if (m_Thorn) m_velocityCombo->setStyleSheet(comboStyle);
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
    qLabel->setObjectName("Humbug");
    actionsToolbar->addWidget(qLabel);

    m_quantizeCombo = new QComboBox(actionsToolbar);
    if (m_Thorn) m_quantizeCombo->setStyleSheet(comboStyle);
    actionsToolbar->addWidget(m_quantizeCombo);

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
MatrixView::initRulersToolbar()
{
    QToolBar *rulersToolbar = findToolbar("Rulers Toolbar");
    if (!rulersToolbar) {
        std::cerr << "MatrixView::initRulersToolbar() - rulers toolbar not found!" << std::endl;
        return;
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
    MATRIX_DEBUG << "MatrixView::slotSetSelectTool" << endl;
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
            MATRIX_DEBUG << "Warning: MatrixView::slotSetSnapFromAction: unrecognised action " << name << endl;
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
        (new CutCommand(*selection, m_document->getClipboard()));
}

void
MatrixView::slotEditCopy()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CopyCommand(*selection, m_document->getClipboard()));
//    emit usedSelection();//!!!
}

void
MatrixView::slotEditPaste()
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
MatrixView::slotSetVelocitiesToCurrent()
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
}

void
MatrixView::slotTogglePitchbendRuler()
{
    m_matrixWidget->slotTogglePitchbendRuler();
}

void
MatrixView::slotAddControlRuler()
{
    m_matrixWidget->slotAddControlRuler();
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
    if (!selection) std::cout << "Hint: selection is NULL in slotTranpose() " << std::endl;
    if (!selection) return;

    QSettings settings;
    settings.beginGroup(MatrixViewConfigGroup);

    int dialogDefault = settings.value("lasttransposition", 0).toInt() ;

    bool ok = false;
    int semitones = QInputDialog::getInteger
            (tr("Transpose"),
             tr("By number of semitones: "),
                dialogDefault, -127, 127, 1, &ok, this);
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
        std::cout << "Transposing changing keys is not currently supported on selections" << std::endl;
    }
    else
    {
    // Transpose within key
        //std::cout << "Transposing semitones, steps: " << semitones << ", " << steps << std::endl;
        CommandHistory::getInstance()->addCommand(new TransposeCommand
                (semitones, steps, *selection));
    }
}

void MatrixView::slotTransposeUp()
{
    EventSelection *selection = getSelection();
    if (!selection) std::cout << "Hint: selection is NULL in slotTranposeUp() " << std::endl;
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
    std::cout << "slotInvert() called" << std::endl;
    
    EventSelection *selection = getSelection();
    if (!selection) std::cout << "Hint: selection is NULL in slotInvert() " << std::endl;
    if (!selection) return ;
    
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
MatrixView::slotHelpAbout()
{
    new AboutDialog(this);
}

void
MatrixView::slotStepBackward()
{
    timeT time = getInsertionTime();
    m_document->slotSetPointerPosition(getSnapGrid()->snapTime
                                       (time - 1, SnapGrid::SnapLeft));
}

void
MatrixView::slotStepForward()
{
    timeT time = getInsertionTime();
    m_document->slotSetPointerPosition(getSnapGrid()->snapTime
                                       (time + 1, SnapGrid::SnapRight));
}

void
MatrixView::slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn)
{
    // hjj:
    // The default insertion mode is implemented equivalently in
    // matrixviewslots.cpp:
    //  - proceed if notes do not overlap
    //  - make the chord if notes do overlap, and do not proceed

    velocity = 42; // shut up warning; one day it would be nice to use velocity for something

    static int numberOfNotesOn = 0;
    static time_t lastInsertionTime = 0;
    if (!noteOn) {
        numberOfNotesOn--;
        return ;
    }

    QAction *action = findAction("toggle_step_by_step");

    if (!action) {
        MATRIX_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
        return ;
    }
    if (!action->isChecked())
        return ;

//    if (m_inPaintEvent) {
//        m_pendingInsertableNotes.push_back(std::pair<int, int>(pitch, velocity));
//        return ;
//    }

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

    MATRIX_DEBUG << "Inserting note at pitch " << pitch << endl;

    Event modelEvent(Note::EventType, 0, 1);
    modelEvent.set<Int>(BaseProperties::PITCH, pitch);
    static timeT insertionTime(getInsertionTime());
    if (insertionTime >= segment->getEndMarkerTime()) {
        MATRIX_DEBUG << "WARNING: off end of segment" << endl;
        return ;
    }
    time_t now;
    time (&now);
    double elapsed = difftime(now, lastInsertionTime);
    time (&lastInsertionTime);

    if (numberOfNotesOn <= 0 || elapsed > 10.0 ) {
        numberOfNotesOn = 0;
        insertionTime = getInsertionTime();
    }
    numberOfNotesOn++;
    timeT endTime(insertionTime + getSnapGrid()->getSnapTime(insertionTime));

    if (endTime <= insertionTime) {
        static bool showingError = false;
        if (showingError)
            return ;
        showingError = true;
        /* was sorry */ QMessageBox::warning(this, "", tr("Can't insert note: No grid duration selected"));
        showingError = false;
        return ;
    }

    MatrixInsertionCommand* command = 
        new MatrixInsertionCommand(*segment, insertionTime,
                                   endTime, &modelEvent);

    CommandHistory::getInstance()->addCommand(command);

    m_document->slotSetPointerPosition(endTime);

//    if (!isInChordMode()) {
//        slotSetInsertCursorPosition(endTime);
//    }
}

void
MatrixView::slotInsertableNoteOnReceived(int pitch, int velocity)
{
    MATRIX_DEBUG << "MatrixView::slotInsertableNoteOnReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, true);
}

void
MatrixView::slotInsertableNoteOffReceived(int pitch, int velocity)
{
    MATRIX_DEBUG << "MatrixView::slotInsertableNoteOffReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, false);
}

void
MatrixView::slotPitchBendSequence()
{
    timeT startTime=0;
    timeT endTime=0;

    if (getSelection()) {
        startTime=getSelection()->getStartTime();
        endTime=getSelection()->getEndTime();
    } else {
        startTime = getInsertionTime();
    }

    PitchBendSequenceDialog dialog(this, getCurrentSegment(), startTime,
                                   endTime);
    dialog.exec();
}

/*

void MatrixView::slotJogLeft()
{
    if (!selection)
        return ;
    TmpStatusMsg msg(tr("Jogging left..."), this);

    RG_DEBUG << "MatrixView::slotJogLeft" << endl;

    CommandHistory::getInstance()->addCommand(
        new MoveCommand(*getCurrentSegment(),
                        -Note(Note::Demisemiquaver).getDuration(),
                        false,  // don't use matrix timings
                        *selection));
}

void MatrixView::slotJogRight()
{
    if (!selection)
        return ;
    TmpStatusMsg msg(tr("Jogging right..."), this);

    RG_DEBUG << "MatrixView::slotJogRight" << endl;

    CommandHistory::getInstance()->addCommand(
        new MoveCommand(*getCurrentSegment(),
                        Note(Note::Demisemiquaver).getDuration(),
                        false,  // don't use matrix timings
                        *selection));
}
*/

// --
// end of code formerly located in EditView.cpp

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
        MATRIX_DEBUG << "WARNING: off end of segment" << endl;
        return ;
    }
    ::Rosegarden::Key key = segment->getKeyAtTime(time);
    Clef clef = segment->getClefAtTime(time);

    try {

        pitch = getPitchFromNoteInsertAction(name, accidental, clef, key);

    } catch (...) {

        QMessageBox::warning(this, "", tr("Unknown note insert action %1").arg(name));
        return ;
    }

//    TmpStatusMsg msg(tr("Inserting note"), this);

    MATRIX_DEBUG << "Inserting note at pitch " << pitch << endl;

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

        int modify = 0;
        int octave = 0;

        if (name.right(5) == "_high") {

            octave = 1;
            name = name.left(name.length() - 5);

        } else if (name.right(4) == "_low") {

            octave = -1;
            name = name.left(name.length() - 4);
        }

        if (name.right(6) == "_sharp") {

            modify = 1;
            accidental = Sharp;
            name = name.left(name.length() - 6);

        } else if (name.right(5) == "_flat") {

            modify = -1;
            accidental = Flat;
            name = name.left(name.length() - 5);
        }

        int scalePitch = name.toInt();

        if (scalePitch < 0 || scalePitch > 7) {
            NOTATION_DEBUG << "MatrixView::getPitchFromNoteInsertAction: pitch "
            << scalePitch << " out of range, using 0" << endl;
            scalePitch = 0;
        }

        Pitch clefPitch(clef.getAxisHeight(), clef, key, NoAccidental);

        int pitchOctave = clefPitch.getOctave() + octave;

        std::cerr << "MatrixView::getPitchFromNoteInsertAction:"
                  << " key = " << key.getName() 
                  << ", clef = " << clef.getClefType() 
                  << ", octaveoffset = " << clef.getOctaveOffset() << std::endl;
        std::cerr << "MatrixView::getPitchFromNoteInsertAction: octave = " << pitchOctave << std::endl;

        // We want still to make sure that when (i) octave = 0,
        //  (ii) one of the noteInScale = 0..6 is
        //  (iii) at the same heightOnStaff than the heightOnStaff of the key.
        int lowestNoteInScale = 0;
        Pitch lowestPitch(lowestNoteInScale, clefPitch.getOctave(), key, NoAccidental);

        int heightToAdjust = (clefPitch.getHeightOnStaff(clef, key) - lowestPitch.getHeightOnStaff(clef, key));
        for (; heightToAdjust < 0; heightToAdjust += 7) pitchOctave++;
        for (; heightToAdjust > 6; heightToAdjust -= 7) pitchOctave--;

        std::cerr << "MatrixView::getPitchFromNoteInsertAction: octave = " << pitchOctave << " (adjusted)" << std::endl;

        Pitch pitch(scalePitch, pitchOctave, key, accidental);
        return pitch.getPerformancePitch();

    } else {

        throw Exception("Not an insert action",
                        __FILE__, __LINE__);
    }
}


}
#include "MatrixView.moc"
