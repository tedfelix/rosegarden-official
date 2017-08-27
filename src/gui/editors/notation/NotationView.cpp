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

#define RG_MODULE_STRING "[NotationView]"

#include "NotationView.h"

#include "NotationWidget.h"
#include "NotationScene.h"
#include "NotationCommandRegistry.h"
#include "NoteStyleFactory.h"
#include "NoteFontFactory.h"
#include "NotationStrings.h"
#include "NoteRestInserter.h"
#include "NotationSelector.h"
#include "HeadersGroup.h"
#include "NotationHLayout.h"
#include "NotationStaff.h"
#include "NotationElement.h"
#include "NotePixmapFactory.h"

#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"

#include "misc/ConfigGroups.h"
#include "misc/Debug.h"

#include "base/AnalysisTypes.h"
#include "base/BaseProperties.h"
#include "base/Clipboard.h"
#include "base/CompositionTimeSliceAdapter.h"
#include "base/Controllable.h"
#include "base/Device.h"
#include "base/Event.h"
#include "base/Instrument.h"
#include "base/MidiDevice.h"
#include "base/MidiTypes.h"
#include "base/NotationQuantizer.h"
#include "base/NotationRules.h"
#include "base/NotationTypes.h"
#include "base/Selection.h"
#include "base/SoftSynthDevice.h"
#include "base/Studio.h"
#include "base/TriggerSegment.h"
#include "base/parameterpattern/ParameterPattern.h"

#include "commands/edit/CopyCommand.h"
#include "commands/edit/CutCommand.h"
#include "commands/edit/CutAndCloseCommand.h"
#include "commands/edit/EraseCommand.h"
#include "commands/edit/PasteEventsCommand.h"
#include "commands/edit/InsertTriggerNoteCommand.h"
#include "commands/edit/SetTriggerCommand.h"
#include "commands/edit/ClearTriggersCommand.h"
#include "commands/edit/ChangeVelocityCommand.h"
#include "commands/edit/RescaleCommand.h"
#include "commands/edit/TransposeCommand.h"
#include "commands/edit/InvertCommand.h"
#include "commands/edit/RetrogradeCommand.h"
#include "commands/edit/RetrogradeInvertCommand.h"
#include "commands/edit/MoveCommand.h"
#include "commands/edit/EventQuantizeCommand.h"
#include "commands/edit/SetLyricsCommand.h"
#include "commands/edit/EventEditCommand.h"
#include "commands/edit/CollapseNotesCommand.h"
#include "commands/edit/AddDotCommand.h"
#include "commands/edit/SetNoteTypeCommand.h"
#include "commands/edit/SelectAddEvenNotesCommand.h"
#include "commands/edit/MaskTriggerCommand.h"
#include "commands/edit/PlaceControllersCommand.h"

#include "commands/notation/AdoptSegmentCommand.h"
#include "commands/notation/InterpretCommand.h"
#include "commands/notation/ClefInsertionCommand.h"
#include "commands/notation/GeneratedRegionInsertionCommand.h"
#include "commands/notation/KeyInsertionCommand.h"
#include "commands/notation/MultiKeyInsertionCommand.h"
#include "commands/notation/SustainInsertionCommand.h"
#include "commands/notation/TupletCommand.h"
#include "commands/notation/TextInsertionCommand.h"
#include "commands/notation/KeyInsertionCommand.h"
#include "commands/notation/EraseEventCommand.h"
#include "commands/notation/NormalizeRestsCommand.h"
#include "commands/notation/CycleSlashesCommand.h"

#include "commands/segment/AddTempoChangeCommand.h"
#include "commands/segment/AddTimeSignatureAndNormalizeCommand.h"
#include "commands/segment/AddTimeSignatureCommand.h"
#include "commands/segment/AddLayerCommand.h"
#include "commands/segment/CutToTriggerSegmentCommand.h"
#include "commands/segment/SegmentTransposeCommand.h"
#include "commands/segment/SegmentSyncCommand.h"

#include "gui/dialogs/PasteNotationDialog.h"
#include "gui/dialogs/InterpretDialog.h"
#include "gui/dialogs/MakeOrnamentDialog.h"
#include "gui/dialogs/UseOrnamentDialog.h"
#include "gui/dialogs/ClefDialog.h"
#include "gui/dialogs/GeneratedRegionDialog.h"
#include "gui/dialogs/LilyPondOptionsDialog.h"
#include "gui/dialogs/SelectDialog.h"
#include "gui/dialogs/EventFilterDialog.h"
#include "gui/dialogs/EventParameterDialog.h"
#include "gui/dialogs/PitchBendSequenceDialog.h"
#include "gui/dialogs/KeySignatureDialog.h"
#include "gui/dialogs/IntervalDialog.h"
#include "gui/dialogs/TupletDialog.h"
#include "gui/dialogs/InsertTupletDialog.h"
#include "gui/dialogs/RescaleDialog.h"
#include "gui/dialogs/TempoDialog.h"
#include "gui/dialogs/TimeSignatureDialog.h"
#include "gui/dialogs/QuantizeDialog.h"
#include "gui/dialogs/LyricEditDialog.h"
#include "gui/dialogs/AboutDialog.h"
#include "gui/dialogs/EventEditDialog.h"
#include "gui/dialogs/TextEventDialog.h"
#include "gui/dialogs/SimpleEventEditDialog.h"
#include "gui/dialogs/ConfigureDialog.h"

#include "gui/dialogs/CheckForParallelsDialog.h"

#include "gui/general/EditTempoController.h"
#include "gui/general/IconLoader.h"
#include "gui/general/LilyPondProcessor.h"
#include "gui/general/PresetHandlerDialog.h"
#include "gui/general/ClefIndex.h"
#include "gui/general/ThornStyle.h"
#include "gui/rulers/ControlRulerWidget.h"
#include "gui/widgets/TmpStatusMsg.h"

#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"

#include "gui/editors/parameters/TrackParameterBox.h"

#include "document/io/LilyPondExporter.h"

#include <QWidget>
#include <QAction>
#include <QActionGroup>
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QTemporaryFile>
#include <QToolBar>
#include <QInputDialog>
#include <QStatusBar>
#include <QToolButton>
#include <QUrl>
#include <QDesktopServices>

#include <algorithm>
#include <set>

#define CALL_MEMBER_FN(OBJECT,PTRTOMEMBER)  ((OBJECT).*(PTRTOMEMBER))

namespace Rosegarden
{

using namespace Accidentals;

NotationView::NotationView(RosegardenDocument *doc,
                           std::vector<Segment *> segments,
                           QWidget *parent) :
    EditViewBase(doc, segments, parent),
    m_document(doc),
    m_durationMode(InsertingRests),
    m_durationPressed(0),
    m_accidentalPressed(0),
    m_selectionCounter(0),
    m_insertModeLabel(0),
    m_annotationsLabel(0),
    m_lilyPondDirectivesLabel(0),
    m_currentNotePixmap(0),
    m_hoveredOverNoteName(0),
    m_hoveredOverAbsoluteTime(0),
    m_fontCombo(0),
    m_fontSizeCombo(0),
    m_spacingCombo(0),
    m_segments(segments)
{
    m_notationWidget = new NotationWidget();
    setCentralWidget(m_notationWidget);

    m_notationWidget->suspendLayoutUpdates();

    setWidgetSegments();

    // connect the editElement signal from NotationSelector, relayed through
    // NotationWidget to be acted upon here in NotationView
    connect(m_notationWidget, SIGNAL(editElement(NotationStaff *, NotationElement *, bool)),
            this, SLOT(slotEditElement(NotationStaff *, NotationElement *, bool)));

    // Many actions are created here
    m_commandRegistry = new NotationCommandRegistry(this);

    setupActions();
    createMenusAndToolbars("notation.rc");
    slotUpdateMenuStates();
    slotTestClipboard();

    setWindowIcon(IconLoader().loadPixmap("window-notation"));

    connect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
            this, SLOT(slotUpdateMenuStates()));

    connect(m_notationWidget->getScene(), SIGNAL(selectionChanged()),
            this, SLOT(slotUpdateMenuStates()));

    //Initialize NoteRestInserter and DurationToolbar
    initializeNoteRestInserter();

    // Determine default action stolen from MatrixView.cpp
    // Toggle the desired tool off and then trigger it on again, to
    // make sure its signal is called at least once (as would not
    // happen if the tool was on by default otherwise)
    QAction *toolAction = 0;
    if (!m_notationWidget->segmentsContainNotes()) {
        toolAction = findAction("draw");
    } else {
        toolAction = findAction("select");
    }
    if (toolAction) {
        NOTATION_DEBUG << "initial state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked();
        if (toolAction->isChecked()) toolAction->toggle();
        NOTATION_DEBUG << "newer state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked();
        toolAction->trigger();
        NOTATION_DEBUG << "newest state for action '" << toolAction->objectName() << "' is " << toolAction->isChecked();
    }

    // Set display configuration
    bool visible;
    QSettings settings;

    m_Thorn = ThornStyle::isEnabled();

    settings.beginGroup(NotationViewConfigGroup);

    // Set font size for single or multiple staffs (hopefully this happens
    // before we've drawn anything, so there's no penalty changing it)
    m_fontSize = NoteFontFactory::getDefaultSize(m_fontName);
    if (m_notationWidget->getScene()->getVisibleStaffCount() > 1) {
        m_fontSize = settings.value("multistaffnotesize", 6).toInt();
        //RG_DEBUG << "setting multi staff size to " << size;
    } else {
        m_fontSize = settings.value("singlestaffnotesize", 8).toInt();
        //RG_DEBUG << "setting single staff size to " << size;
    }
    m_notationWidget->slotSetFontSize(m_fontSize);

    //Update Font Size pulldown menu
    QString action = QString("note_font_size_%1").arg(m_fontSize);
    findAction(action)->setChecked(true);


    // Set initial notation layout mode
    int layoutMode = settings.value("layoutmode", 0).toInt();
    switch (layoutMode) {
        case 0 :
            findAction("linear_mode")->setChecked(true);
            findAction("continuous_page_mode")->setChecked(false);
            findAction("multi_page_mode")->setChecked(false);
            slotLinearMode();
        break;
        case 1 :
            findAction("linear_mode")->setChecked(false);
            findAction("continuous_page_mode")->setChecked(true);
            findAction("multi_page_mode")->setChecked(false);
            slotContinuousPageMode();
        break;
        case 2 : 
            findAction("linear_mode")->setChecked(false);
            findAction("continuous_page_mode")->setChecked(false);
            findAction("multi_page_mode")->setChecked(true);
            slotMultiPageMode(); 
        break;
    }

    // Set initial visibility of chord name ruler, ...
    visible = settings.value("Chords ruler shown",
                          findAction("show_chords_ruler")->isChecked()
                         ).toBool();
    findAction("show_chords_ruler")->setChecked(visible);
    m_notationWidget->setChordNameRulerVisible(visible);

    // ... raw note ruler, ...
    visible = settings.value("Raw note ruler shown",
                          findAction("show_raw_note_ruler")->isChecked()
                         ).toBool();
    findAction("show_raw_note_ruler")->setChecked(visible);
    m_notationWidget->setRawNoteRulerVisible(visible);

    // ... and tempo ruler.
    visible = settings.value("Tempo ruler shown",
                          findAction("show_tempo_ruler")->isChecked()
                         ).toBool();
    findAction("show_tempo_ruler")->setChecked(visible);
    m_notationWidget->setTempoRulerVisible(visible);

    settings.endGroup();

    if (segments.size() > 1) {
        enterActionState("have_multiple_staffs");
    } else {
        leaveActionState("have_multiple_staffs");
    }

    // We never start with any adopted segments so we don't need to
    // test for this.
    leaveActionState("focus_adopted_segment"); 

    initLayoutToolbar();
    initRulersToolbar();
    initStatusBar();

    slotUpdateWindowTitle();
    connect(m_document, SIGNAL(documentModified(bool)),
            this, SLOT(slotUpdateWindowTitle(bool)));

    connect(m_notationWidget, SIGNAL(showContextHelp(const QString &)),
            this, SLOT(slotShowContextHelp(const QString &)));

    // Restore window geometry and toolbar/dock state
    settings.beginGroup(WindowGeometryConfigGroup);
    this->restoreGeometry(settings.value("Notation_View_Geometry").toByteArray());
    this->restoreState(settings.value("Notation_View_State").toByteArray());
    settings.endGroup();

    connect(m_notationWidget, SIGNAL(sceneNeedsRebuilding()),
            this, SLOT(slotRegenerateScene()));

    // Set the rewind and fast-forward buttons for auto-repeat.
    enableAutoRepeat("Transport Toolbar", "playback_pointer_back_bar");
    enableAutoRepeat("Transport Toolbar", "playback_pointer_forward_bar");
    enableAutoRepeat("Transport Toolbar", "cursor_back");
    enableAutoRepeat("Transport Toolbar", "cursor_forward");

    m_notationWidget->resumeLayoutUpdates();

    // Connection to update the "Show staff headers" check box in the menu
    // (Must be done before setting the initial visibility of the headers)
    connect(m_notationWidget, SIGNAL(headersVisibilityChanged(bool)),
            this,  SLOT(slotCheckShowHeadersMenu(bool)));

    // Set initial visibility of staff headers.
    // (Could not be done earlier because both view size and headers size are
    //  needed to know what should be done when the "show when needed" option
    //  is selected).
    settings.beginGroup(NotationViewConfigGroup);
    switch (settings.value("shownotationheader",
                           HeadersGroup::DefaultShowMode).toInt()) {
      case HeadersGroup::ShowNever :
          m_notationWidget->setHeadersVisible(false);
          break;
      case HeadersGroup::ShowWhenNeeded :
          m_notationWidget->setHeadersVisibleIfNeeded();
          break;
      case HeadersGroup::ShowAlways :
          m_notationWidget->setHeadersVisible(true);
          break;
      default :
          RG_WARNING << "NotationView: settings.value(\"shownotationheader\") "
                    << "returned an unexpected value. This is a bug.";
    }
    settings.endGroup();

    // Show the pointer as soon as notation editor opens
    m_notationWidget->slotUpdatePointerPosition(false);
    
    readOptions();
}

NotationView::~NotationView()
{
    NOTATION_DEBUG << "Deleting notation view";
    m_notationWidget->clearAll();
    
    // I own the m_adoptedSegments segments.
    for (SegmentVector::iterator it = m_adoptedSegments.begin();
         it != m_adoptedSegments.end(); ++it) {
        delete (*it);
    }
    
    delete m_commandRegistry;
}

bool
NotationView::hasSegment(Segment * seg) const
{
    for (std::vector<Segment *>::const_iterator it = m_segments.begin(); it != m_segments.end(); ++it) {
        if ((*it) == seg) return true;
    }
    return false;
}

void
NotationView::closeEvent(QCloseEvent *event)
{
    // Save window geometry and toolbar/dock state
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    RG_DEBUG << "storing window geometry for notation view";
    settings.setValue("Notation_View_Geometry", this->saveGeometry());
    settings.setValue("Notation_View_State", this->saveState());
    settings.endGroup();

    QWidget::closeEvent(event);
}

// Adopt a segment that doesn't live in Composition.  Take ownership
// of s; it is caller's responsibility to guarantee that s is not
// owned by something else.
void
NotationView::
adoptSegment(Segment *s)
{
    m_adoptedSegments.push_back(s);
    // We have at least 2 staffs.
    enterActionState("have_multiple_staffs");
    slotRegenerateScene();
    slotUpdateMenuStates();
}

// Unadopt a segment that we adopted earlier.  If s was not adopted
// earlier, do nothing.
void
NotationView::
unadoptSegment(Segment *s)
{
    SegmentVector::iterator found = findAdopted(s);

    if (found != m_adoptedSegments.end()) {
        m_adoptedSegments.erase(found);
        if (m_adoptedSegments.size() + m_segments.size() == 1)
            { leaveActionState("have_multiple_staffs"); }
        slotRegenerateScene();
        slotUpdateMenuStates();
    }
}

NotationView::SegmentVector::iterator
NotationView::
findAdopted(Segment *s)
{
    return
        std::find(m_adoptedSegments.begin(), m_adoptedSegments.end(), s);
}


// Set NotationWidget's segments.
void
NotationView::setWidgetSegments(void)
{
    SegmentVector allSegments = m_segments;
    allSegments.insert(allSegments.end(),
                       m_adoptedSegments.begin(),
                       m_adoptedSegments.end());
    m_notationWidget->setSegments(m_document, allSegments);
    // Reconnect because there's a new scene.
    connect(m_notationWidget->getScene(), SIGNAL(selectionChanged()),
            this, SLOT(slotUpdateMenuStates()));
}

void
NotationView::setupActions()
{
    //setup actions common to all views.
    setupBaseActions(true);

    //"file" MenuBar menu
    // "file_save"
    // Created in EditViewBase::setupActions() via creatAction()

    createAction("file_print_lilypond", SLOT(slotPrintLilyPond()));
    createAction("file_preview_lilypond", SLOT(slotPreviewLilyPond()));

    // "file_close"
    // Created in EditViewBase::setupActions() via creatAction()

    // "edit" MenuBar menu
    // "edit_undo"
    // Created in EditViewBase::setupActions() via creatAction()

    // "edit_redo"
    // Created in EditViewBase::setupActions() via creatAction()

    createAction("cut_and_close", SLOT(slotEditCutAndClose()));
    createAction("general_paste", SLOT(slotEditGeneralPaste()));
    createAction("delete", SLOT(slotEditDelete()));
    createAction("move_events_up_staff",
                 SLOT(slotMoveEventsUpStaff()));
    createAction("general_move_events_up_staff",
                 SLOT(slotMoveEventsUpStaffInteractive()));
    createAction("move_events_down_staff",
                 SLOT(slotMoveEventsDownStaff()));
    createAction("general_move_events_down_staff",
                 SLOT(slotMoveEventsDownStaffInteractive()));
    createAction("select_from_start", SLOT(slotEditSelectFromStart()));
    createAction("select_to_end", SLOT(slotEditSelectToEnd()));
    createAction("select_whole_staff", SLOT(slotEditSelectWholeStaff()));
    createAction("clear_selection", SLOT(slotClearSelection()));
    createAction("search_select", SLOT(slotSearchSelect()));
    createAction("filter_selection", SLOT(slotFilterSelection()));
    createAction("select_evenly_spaced_notes", SLOT(slotSelectEvenlySpacedNotes()));
    createAction("expression_sequence", SLOT(slotExpressionSequence()));    
    createAction("pitch_bend_sequence", SLOT(slotPitchBendSequence()));    
    createAction("controller_sequence", SLOT(slotControllerSequence()));    

    
    //"view" MenuBar menu
    // "note_font_actionmenu" subMenu
    // Custom Code. Coded below.

    // "note_font_size_actionmenu" subMenu
    // Custom Code. Coded below.
    
    // "stretch_actionmenu" subMenu
    // Custom Code. Coded below.
    // Code deactivated.

    // "proportion_actionmenu" subMenu
    // Custom Code. Coded below.
    // Code deactivated.

    // "layout" submenu 
    createAction("add_layer", SLOT(slotAddLayer()));
    createAction("magic_layer", SLOT(slotMagicLayer()));
    createAction("linear_mode", SLOT(slotLinearMode()));
    createAction("continuous_page_mode", SLOT(slotContinuousPageMode()));
    createAction("multi_page_mode", SLOT(slotMultiPageMode()));

    createAction("lyric_editor", SLOT(slotEditLyrics()));
    createAction("show_track_headers", SLOT(slotShowHeadersGroup()));

    //"document" Menubar menu
    createAction("add_tempo", SLOT(slotAddTempo()));
    createAction("add_time_signature", SLOT(slotAddTimeSignature()));
    createAction("check_for_parallels", SLOT(slotCheckForParallels()));

    //"segment" Menubar menu
    // "open-with" subMenu
    // Created in EditViewBase::setupActions() via creatAction()

    createAction("add_clef", SLOT(slotEditAddClef()));
    //uncomment this when we implement linked segment transposition
    //createAction("add_clef_this_link_only", SLOT(slotEditAddClefLinkOnly()));
    createAction("add_key_signature", SLOT(slotEditAddKeySignature()));
    createAction("add_sustain_down", SLOT(slotEditAddSustainDown()));
    createAction("add_sustain_up", SLOT(slotEditAddSustainUp()));

    // "set_segment_start"
    // Created in EditViewBase::setupActions() via creatAction()
    
    // "set_segment_duration"
    // Created in EditViewBase::setupActions() via creatAction()

    createAction("transpose_segment", SLOT(slotEditTranspose()));
    createAction("switch_preset", SLOT(slotEditSwitchPreset()));
    //createAction("create_anacrusis", SLOT(slotCreateAnacrusis()));

    //"Notes" Menubar menu

    // "Marks" subMenu
    //Created in Constructor via NotationCommandRegistry()
    //with AddMarkCommand::registerCommand()
    //with RemoveMarksCommand::registerCommand()

    // "ornaments" subMenu
    createAction("use_ornament", SLOT(slotUseOrnament()));
    createAction("make_ornament", SLOT(slotMakeOrnament()));
    createAction("remove_ornament", SLOT(slotRemoveOrnament()));
    createAction("edit_ornament_inline", SLOT(slotEditOrnamentInline()));
    createAction("show_ornament_expansion", SLOT(slotShowOrnamentExpansion()));
    createAction("mask_ornament", SLOT(slotMaskOrnament()));
    createAction("unmask_ornament", SLOT(slotUnmaskOrnament()));

    // "Fingering" subMenu
    // Created in Constructor via NotationCommandRegistry()
    // with AddFingeringMarkCommand::registerCommand()
    // with RemoveFingeringMarksCommand::registerCommand()

    // "Slashes" subMenu
    // Created in Constructor via NotationCommandRegistry()
    // with AddSlashesCommand::registerCommand()

    // "note_style_actionmenu" subMenu
    // Created in Constructor via NotationCommandRegistry()
    // with ChangeStyleCommand::registerCommand()
    // actionCreate really should be a custon code. Oh, well.


    // "Respell" subMenu
    // Created in Constructor via NotationCommandRegistry()
    // with RespellCommand::registerCommand()
    
    // "stem_up"
    // Created in Constructor via NotationCommandRegistry()
    // with ChangeStemsCommand::registerCommand()
    
    // "stem_down"
    // Created in Constructor via NotationCommandRegistry()
    // with ChangeStemsCommand::registerCommand()
    
    // "restore_stems"
    // Created in Constructor via NotationCommandRegistry()
    // with RestoreStemsCommand::registerCommand();

    //"Phrase" Menubar menu
    // "make_chord"
    // Created in Constructor via NotationCommandRegistry()
    // with MakeChordCommand::registerCommand();

    // "beam"
    // Created in Constructor via NotationCommandRegistry()
    // with BeamCommand::registerCommand();

    // "auto_beam"
    // Created in Constructor via NotationCommandRegistry()
    // with AutoBeamCommand::registerCommand();

    // "break_group"
    // Created in Constructor via NotationCommandRegistry()
    // with BreakCommand::registerCommand();

    // "remove_indications"

    createAction("simple_tuplet", SLOT(slotGroupSimpleTuplet()));
    createAction("tuplet", SLOT(slotGroupGeneralTuplet()));

    //Where is "break_tuplet" created?
    //"slur" & "phrasing_slur" are created in AddIndicationCommand
    
    //"Slurs" subMenu
    //where are "restore_slurs", "slurs_above", "slurs_below" created?

    //Where are "tie_notes", "untie_notes", created?

    //"Ties" subMenu
    //"restore_ties", "ties_above", & "ties_below" created?
    
    //"crescendo" & "decrescendo" are created in AddIndicationCommand

    //"octaves" subMenu
    //All are created in AddIndicationCommand

    //Actions first appear in "Adjust" Menubar menu

    //"rests" subMenu
    createAction("normalize_rests", SLOT(slotTransformsNormalizeRests()));
    //Where is "collapse_rests_aggresively" created?

    //"transform_notes" subMenu
    createAction("collapse_notes", SLOT(slotTransformsCollapseNotes()));
    //Where are "make_notes_viable" & "de_counterpoint" created?

    //Quantitize subMenu
    createAction("quantize", SLOT(slotTransformsQuantize()));
    //Where are "fix_quantization" & "remove_quantization" created?

    createAction("interpret", SLOT(slotTransformsInterpret()));

    //"Rescale" subMenu
    createAction("halve_durations", SLOT(slotHalveDurations()));
    createAction("double_durations", SLOT(slotDoubleDurations()));
    createAction("rescale", SLOT(slotRescale()));

    //"Transpose" subMenu
    createAction("transpose_up", SLOT(slotTransposeUp()));
    createAction("transpose_down", SLOT(slotTransposeDown()));
    createAction("transpose_up_octave", SLOT(slotTransposeUpOctave()));
    createAction("transpose_down_octave", SLOT(slotTransposeDownOctave()));
    createAction("general_transpose", SLOT(slotTranspose()));
    createAction("general_diatonic_transpose", SLOT(slotDiatonicTranspose()));

    //"Convert" subMenu
    createAction("invert", SLOT(slotInvert()));
    createAction("retrograde", SLOT(slotRetrograde()));
    createAction("retrograde_invert", SLOT(slotRetrogradeInvert()));

    //"velocities" subMenu
    createAction("velocity_up", SLOT(slotVelocityUp()));
    createAction("velocity_down", SLOT(slotVelocityDown()));
    createAction("set_velocities", SLOT(slotSetVelocities()));

    //"fine_positioning" subMenu
    //Where are "fine_position_restore", "fine_position_left",
    //"fine_position_right", "fine_position_up" & 
    //"fine_position_down" created?

    //"fine_timing" subMenu
    createAction("jog_left", SLOT(slotJogLeft()));
    createAction("jog_right", SLOT(slotJogRight()));

    //"visibility" subMenu
    //Where are "make_invisible" & "make_visible" created?

    //"controllers" Menubar menu
    createAction("copy_controllers",  SLOT(slotEditCopyControllers()));
    createAction("cut_controllers",   SLOT(slotEditCutControllers()));
    createAction("set_controllers",   SLOT(slotSetControllers()));
    createAction("place_controllers", SLOT(slotPlaceControllers()));

    //Actions first appear in "Tools" Menubar menu
    createAction("select", SLOT(slotSetSelectTool()));
    createAction("selectnoties", SLOT(slotSetSelectNoTiesTool()));
    createAction("erase", SLOT(slotSetEraseTool()));
    createAction("draw", SLOT(slotSetNoteRestInserter()));

    // These actions do as their names imply, and in this case, the toggle will
    // call one or the other of these
    // These rely on .rc script keeping the right state visible
    createAction("switch_to_rests", SLOT(slotSwitchToRests()));
    createAction("switch_to_notes", SLOT(slotSwitchToNotes()));

    // These actions always just pass straight to the toggle.
    // These rely on .rc script keeping the right state visible
    createAction("switch_dots_on", SLOT(slotToggleDot()));
    createAction("switch_dots_off", SLOT(slotToggleDot()));

    // Menu uses now "Switch to Notes", "Switch to Rests" and "Durations".
    createAction("duration_breve", SLOT(slotNoteAction()));
    createAction("duration_semibreve", SLOT(slotNoteAction()));
    createAction("duration_minim", SLOT(slotNoteAction()));
    createAction("duration_crotchet", SLOT(slotNoteAction()));
    createAction("duration_quaver", SLOT(slotNoteAction()));
    createAction("duration_semiquaver", SLOT(slotNoteAction()));
    createAction("duration_demisemi", SLOT(slotNoteAction()));
    createAction("duration_hemidemisemi", SLOT(slotNoteAction()));
    createAction("duration_dotted_breve", SLOT(slotNoteAction()));
    createAction("duration_dotted_semibreve", SLOT(slotNoteAction()));
    createAction("duration_dotted_minim", SLOT(slotNoteAction()));
    createAction("duration_dotted_crotchet", SLOT(slotNoteAction()));
    createAction("duration_dotted_quaver", SLOT(slotNoteAction()));
    createAction("duration_dotted_semiquaver", SLOT(slotNoteAction()));
    createAction("duration_dotted_demisemi", SLOT(slotNoteAction()));
    createAction("duration_dotted_hemidemisemi", SLOT(slotNoteAction()));

    // since we can't create toolbars with disabled icons, and to avoid having
    // to draw a lot of fancy icons for disabled durations, we have this dummy
    // filler to keep spacing the same across all toolbars, and there have to
    // two of them
    // Without this handler, the action takes up no space on the toolbar.
    createAction("dummy_1", SLOT(slotDummy1()));

    //"NoteTool" subMenu
    //NEED to create action methods
    createAction("breve", SLOT(slotNoteAction()));
    createAction("semibreve", SLOT(slotNoteAction()));
    createAction("minim", SLOT(slotNoteAction()));
    createAction("crotchet", SLOT(slotNoteAction()));
    createAction("quaver", SLOT(slotNoteAction()));
    createAction("semiquaver", SLOT(slotNoteAction()));
    createAction("demisemi", SLOT(slotNoteAction()));
    createAction("hemidemisemi", SLOT(slotNoteAction()));
    createAction("dotted_breve", SLOT(slotNoteAction()));
    createAction("dotted_semibreve", SLOT(slotNoteAction()));
    createAction("dotted_minim", SLOT(slotNoteAction()));
    createAction("dotted_crotchet", SLOT(slotNoteAction()));
    createAction("dotted_quaver", SLOT(slotNoteAction()));
    createAction("dotted_semiquaver", SLOT(slotNoteAction()));
    createAction("dotted_demisemi", SLOT(slotNoteAction()));
    createAction("dotted_hemidemisemi", SLOT(slotNoteAction()));

    //"RestTool" subMenu
    //NEED to create action methods
    createAction("rest_breve", SLOT(slotNoteAction()));
    createAction("rest_semibreve", SLOT(slotNoteAction()));
    createAction("rest_minim", SLOT(slotNoteAction()));
    createAction("rest_crotchet", SLOT(slotNoteAction()));
    createAction("rest_quaver", SLOT(slotNoteAction()));
    createAction("rest_semiquaver", SLOT(slotNoteAction()));
    createAction("rest_demisemi", SLOT(slotNoteAction()));
    createAction("rest_hemidemisemi", SLOT(slotNoteAction()));
    createAction("rest_dotted_breve", SLOT(slotNoteAction()));
    createAction("rest_dotted_semibreve", SLOT(slotNoteAction()));
    createAction("rest_dotted_minim", SLOT(slotNoteAction()));
    createAction("rest_dotted_crotchet", SLOT(slotNoteAction()));
    createAction("rest_dotted_quaver", SLOT(slotNoteAction()));
    createAction("rest_dotted_semiquaver", SLOT(slotNoteAction()));
    createAction("rest_dotted_demisemi", SLOT(slotNoteAction()));
    createAction("rest_dotted_hemidemisemi", SLOT(slotNoteAction()));

    //"Accidentals" submenu
    createAction("no_accidental", SLOT(slotNoAccidental()));
    createAction("follow_accidental", SLOT(slotFollowAccidental()));
    createAction("sharp_accidental", SLOT(slotSharp()));
    createAction("flat_accidental", SLOT(slotFlat()));
    createAction("natural_accidental", SLOT(slotNatural()));
    createAction("double_sharp_accidental", SLOT(slotDoubleSharp()));
    createAction("double_flat_accidental", SLOT(slotDoubleFlat()));
    
    //JAS "Clefs" subMenu
    createAction("treble_clef", SLOT(slotClefAction()));
    createAction("alto_clef", SLOT(slotClefAction()));
    createAction("tenor_clef", SLOT(slotClefAction()));
    createAction("bass_clef", SLOT(slotClefAction()));

    createAction("text", SLOT(slotText()));
    createAction("guitarchord", SLOT(slotGuitarChord()));

    // "Symbols" (sub)Menu
    createAction("add_segno", SLOT(slotSymbolAction()));
    createAction("add_coda", SLOT(slotSymbolAction()));
    createAction("add_breath", SLOT(slotSymbolAction()));

    //JAS "Move" subMenu
    createAction("extend_selection_backward", SLOT(slotExtendSelectionBackward()));
    createAction("extend_selection_forward", SLOT(slotExtendSelectionForward()));
    createAction("preview_selection", SLOT(slotPreviewSelection()));
    createAction("clear_loop", SLOT(slotClearLoop()));

    createAction("cursor_up_staff", SLOT(slotCurrentStaffUp()));
    createAction("cursor_down_staff", SLOT(slotCurrentStaffDown()));
    createAction("cursor_prior_segment", SLOT(slotCurrentSegmentPrior()));
    createAction("cursor_next_segment", SLOT(slotCurrentSegmentNext()));
    createAction("unadopt_segment", SLOT(slotUnadoptSegment()));

    //"Transport" subMenu
    createAction("play", SIGNAL(play()));
    createAction("stop", SIGNAL(stop()));
    //Step Backward/Forward are protected signals
    // so the pitch tracker (our derrived class) can see them
    // Because they're protected, we'll connect them here.
    createAction("cursor_back", SIGNAL(stepBackward()));
    connect(this, SIGNAL(stepBackward()),
            this, SLOT(slotStepBackward()));
    createAction("cursor_forward", SIGNAL(stepForward()));
    connect(this, SIGNAL(stepForward()),
            this, SLOT(slotStepForward()));
    createAction("playback_pointer_back_bar", SIGNAL(rewindPlayback()));
    createAction("playback_pointer_forward_bar", SIGNAL(fastForwardPlayback()));
    createAction("playback_pointer_start", SIGNAL(rewindPlaybackToBeginning()));
    createAction("playback_pointer_end", SIGNAL(fastForwardPlaybackToEnd()));
    createAction("toggle_solo", SLOT(slotToggleSolo()));
    createAction("toggle_tracking", SLOT(slotToggleTracking()));
    createAction("panic", SIGNAL(panic()));

    //"insert_note_actionmenu" coded below. 

    createAction("chord_mode", SLOT(slotUpdateInsertModeStatus()));
    createAction("triplet_mode", SLOT(slotUpdateInsertModeStatusTriplet()));
    createAction("tuplet_mode", SLOT(slotUpdateInsertModeStatusTuplet()));
    createAction("grace_mode", SLOT(slotUpdateInsertModeStatus()));
    createAction("toggle_step_by_step", SLOT(slotToggleStepByStep()));

    /// YG: Only for debug
     createAction("dump_staves", SLOT(slotDebugDump()));
     createAction("dump_bardata", SLOT(slotBarDataDump()));

    createAction("manual", SLOT(slotHelp()));
    createAction("tutorial", SLOT(slotTutorial()));
    createAction("guidelines", SLOT(slotBugGuidelines()));
    createAction("help_about_app", SLOT(slotHelpAbout()));
    createAction("help_about_qt", SLOT(slotHelpAboutQt()));
    createAction("donate", SLOT(slotDonate()));

    createAction("toggle_velocity_ruler", SLOT(slotToggleVelocityRuler()));
    createAction("toggle_pitchbend_ruler", SLOT(slotTogglePitchbendRuler()));
    createAction("add_control_ruler", "");

    createAction("cycle_slashes", SLOT(slotCycleSlashes()));

    createAction("interpret_activate", SLOT(slotInterpretActivate()));
    // I don't think you can use <qt> in tooltips in .rc files, and this one
    // wants formatting, so I pulled it out here.
    findAction("interpret_activate")->setToolTip(tr("<qt><p>Apply the interpretations selected on this toolbar to the selection.</p><p>If there is no selection, interpretations apply to the entire segment automatically.</p></qt>"));

    // These have to connect to something, so connect to the dummy slot:
    createAction("interpret_text_dynamics", SLOT(slotDummy1()));
    createAction("interpret_hairpins", SLOT(slotDummy1()));
    createAction("interpret_slurs", SLOT(slotDummy1()));
    createAction("interpret_beats", SLOT(slotDummy1()));

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

    connect(m_notationWidget, SIGNAL(hoveredOverNoteChanged(const QString&)),
            SLOT (slotHoveredOverNoteChanged(const QString&)));

    findAction("add_control_ruler")->setMenu(addControlRulerMenu);

    //Actions first appear in "settings" Menubar menu
    //"toolbars" subMenu
    createAction("options_show_toolbar", SLOT(slotToggleGeneralToolBar()));
    createAction("show_tools_toolbar", SLOT(slotToggleToolsToolBar()));
    createAction("show_duration_toolbar", SLOT(slotToggleDurationToolBar()));
    createAction("show_accidentals_toolbar", SLOT(slotToggleAccidentalsToolBar()));
    createAction("show_clefs_toolbar", SLOT(slotToggleClefsToolBar()));
    createAction("show_marks_toolbar", SLOT(slotToggleMarksToolBar()));
    createAction("show_group_toolbar", SLOT(slotToggleGroupToolBar()));
    createAction("show_symbol_toolbar", SLOT(slotToggleSymbolsToolBar()));
    createAction("show_transport_toolbar", SLOT(slotToggleTransportToolBar()));
    createAction("show_layout_toolbar", SLOT(slotToggleLayoutToolBar()));
    createAction("show_layer_toolbar", SLOT(slotToggleLayerToolBar()));
    createAction("show_rulers_toolbar", SLOT(slotToggleRulersToolBar()));
    createAction("show_interpret_toolbar", SLOT(slotToggleInterpretToolBar()));

    //"rulers" subMenu
    createAction("show_chords_ruler", SLOT(slotToggleChordsRuler()));
    createAction("show_raw_note_ruler", SLOT(slotToggleRawNoteRuler()));
    createAction("show_tempo_ruler", SLOT(slotToggleTempoRuler()));

    //createAction("show_annotations", SLOT(slotToggleAnnotations()));
    //createAction("show_lilypond_directives", SLOT(slotToggleLilyPondDirectives()));

    createAction("extend_selection_backward_bar", SLOT(slotExtendSelectionBackwardBar()));
    createAction("extend_selection_forward_bar", SLOT(slotExtendSelectionForwardBar()));
    //!!! not here yet createAction("move_selection_left", SLOT(slotMoveSelectionLeft()));
    //&&& NB Play has two shortcuts (Enter and Ctrl+Return) -- need to
    // ensure both get carried across somehow
    createAction("add_dot", SLOT(slotAddDot()));
    createAction("add_notation_dot", SLOT(slotAddDotNotationOnly()));

    //set duration of notes by CTRL+<number>
    createAction("set_note_type_doublewhole",SLOT(slotSetNoteType()));
    createAction("set_note_type_whole",SLOT(slotSetNoteType()));
    createAction("set_note_type_half",SLOT(slotSetNoteType()));
    createAction("set_note_type_quarter",SLOT(slotSetNoteType()));
    createAction("set_note_type_eighth",SLOT(slotSetNoteType()));
    createAction("set_note_type_sixteenth",SLOT(slotSetNoteType()));
    createAction("set_note_type_thirtysecond",SLOT(slotSetNoteType()));
    createAction("set_note_type_sixtyfourth",SLOT(slotSetNoteType()));

    //set duration of notes by CTRL+ALT+<number>
    createAction("set_note_type_notation_doublewhole",SLOT(slotSetNoteTypeNotationOnly()));
    createAction("set_note_type_notation_whole",SLOT(slotSetNoteTypeNotationOnly()));
    createAction("set_note_type_notation_half",SLOT(slotSetNoteTypeNotationOnly()));
    createAction("set_note_type_notation_quarter",SLOT(slotSetNoteTypeNotationOnly()));
    createAction("set_note_type_notation_eighth",SLOT(slotSetNoteTypeNotationOnly()));
    createAction("set_note_type_notation_sixteenth",SLOT(slotSetNoteTypeNotationOnly()));
    createAction("set_note_type_notation_thirtysecond",SLOT(slotSetNoteTypeNotationOnly()));
    createAction("set_note_type_notation_sixtyfourth",SLOT(slotSetNoteTypeNotationOnly()));

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
    createAction(QString("insert_rest"),SLOT(slotInsertRestFromAction()));

    std::set<QString> fs(NoteFontFactory::getFontNames());
    std::vector<QString> f;
    for (std::set<QString>::const_iterator i = fs.begin(); i != fs.end(); ++i) {
        f.push_back(*i);
    }
    std::sort(f.begin(), f.end());

    // Custom Note Font Menu creator
    QMenu *fontActionMenu = new QMenu(tr("Note &Font"), this); 
    fontActionMenu->setObjectName("note_font_actionmenu");

    QActionGroup *ag = new QActionGroup(this);

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    m_fontName = settings.value("notefont", NoteFontFactory::getDefaultFontName()).toString();

    for (std::vector<QString>::iterator i = f.begin(); i != f.end(); ++i) {

        QString fontQName(*i);

        m_availableFontNames.push_back(fontQName);

        QAction *a = createAction("note_font_" + fontQName,
                                  SLOT(slotChangeFontFromAction()));

        ag->addAction(a);

        a->setText(fontQName);
        a->setCheckable(true);
        a->setChecked(*i == m_fontName);

        fontActionMenu->addAction(a);    
    }

    QMenu *fontSizeActionMenu = new QMenu(tr("Si&ze"), this);
    fontSizeActionMenu->setObjectName("note_font_size_actionmenu");
    ag = new QActionGroup(this);

    m_availableFontSizes = NoteFontFactory::getScreenSizes(m_fontName);

    for (unsigned int i = 0; i < m_availableFontSizes.size(); ++i) {

        QString actionName = QString("note_font_size_%1").arg(m_availableFontSizes[i]);

        QAction *sizeAction = createAction(actionName,
                                SLOT(slotChangeFontSizeFromAction()));
        sizeAction->setText(tr("%n pixel(s)", "", m_availableFontSizes[i]));
        sizeAction->setCheckable(true);
        ag->addAction(sizeAction);

        sizeAction->setChecked(m_availableFontSizes[i] == m_fontSize);
        fontSizeActionMenu->addAction(sizeAction);
    }

    QMenu *spacingActionMenu = new QMenu(tr("S&pacing"), this);
    spacingActionMenu->setObjectName("stretch_actionmenu");

    m_spacing = settings.value("spacing", m_notationWidget->getScene()->getHSpacing()).toInt();
    m_notationWidget->getScene()->setHSpacing(m_spacing);
    m_availableSpacings = NotationHLayout::getAvailableSpacings();

    ag = new QActionGroup(this);

    for (std::vector<int>::iterator i = m_availableSpacings.begin();
         i != m_availableSpacings.end(); ++i) {

        QAction *a = createAction(QString("spacing_%1").arg(*i),
                                  SLOT(slotChangeSpacingFromAction()));

        ag->addAction(a);
        a->setText(QString("%1%").arg(*i));
        a->setCheckable(true);
        a->setChecked(*i == m_spacing);

        spacingActionMenu->addAction(a);
    }

    // no more duration factor controls

    settings.endGroup();

    // connect up the segment changer signals
    connect(m_notationWidget, SIGNAL(currentSegmentNext()),
            SLOT(slotCurrentSegmentNext()));
    connect(m_notationWidget, SIGNAL(currentSegmentPrior()),
            SLOT(slotCurrentSegmentPrior()));
}

void 
NotationView::slotUpdateMenuStates()
{
    //NOTATION_DEBUG << "NotationView::slotUpdateMenuStates";

    // 1. set selection-related states

    // Clear states first, then enter only those ones that apply
    // (so as to avoid ever clearing one after entering another, in
    // case the two overlap at all)
    leaveActionState("have_selection");
    leaveActionState("have_notes_in_selection");
    leaveActionState("have_rests_in_selection");
    leaveActionState("have_clefs_in_selection");
    leaveActionState("have_symbols_in_selection");
    leaveActionState("have_linked_segment");

    if (!m_notationWidget) return;

    EventSelection *selection = m_notationWidget->getSelection();

    if (selection) {

        //NOTATION_DEBUG << "NotationView::slotUpdateMenuStates: Have selection; it's " << selection << " covering range from " << selection->getStartTime() << " to " << selection->getEndTime() << " (" << selection->getSegmentEvents().size() << " events)";

        enterActionState("have_selection");
        if (selection->contains(Note::EventType)) {
            enterActionState("have_notes_in_selection");
        }
        if (selection->contains(Note::EventRestType)) {
            enterActionState("have_rests_in_selection");
        }
        if (selection->contains(Clef::EventType)) {
            enterActionState("have_clefs_in_selection");
        }
        if (selection->contains(Symbol::EventType)) {
            enterActionState("have_symbols_in_selection");
        }

    } else {
        //NOTATION_DEBUG << "Do not have a selection";
    }

    // 2. set inserter-related states
    NoteRestInserter *currentTool = dynamic_cast<NoteRestInserter *>(m_notationWidget->getCurrentTool());
    if (currentTool) {
        //NOTATION_DEBUG << "Have NoteRestInserter ";
        enterActionState("note_rest_tool_current");
        
    } else {
        //NOTATION_DEBUG << "Do not have NoteRestInserter ";
        leaveActionState("note_rest_tool_current");
    }

    if (m_selectionCounter) {
        if (selection && !selection->getSegmentEvents().empty()) {
            m_selectionCounter->setText(tr("  %n event(s) selected ", "",
                                           selection->getSegmentEvents().size()));
        } else {
            m_selectionCounter->setText(tr("  No selection "));
        }
    }

    // 3. linked segment specific states
    Segment *segment = getCurrentSegment();
    if (segment && segment->isLinked()) {
        enterActionState("have_linked_segment");
    }

    conformRulerSelectionState();
}

void
NotationView::
conformRulerSelectionState(void)
{
    ControlRulerWidget * cr = m_notationWidget->getControlsWidget();
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
NotationView::initLayoutToolbar()
{
    QToolBar *layoutToolbar = findToolbar("Layout Toolbar");

    if (!layoutToolbar) {
        RG_WARNING << "NotationView::initLayoutToolbar() : layout toolbar not found";
        return;
    }

    QLabel *label = new QLabel(tr("  Font:  "), layoutToolbar);
    layoutToolbar->addWidget(label);


    //
    // font combo
    //
    m_fontCombo = new QComboBox(layoutToolbar);
    m_fontCombo->setEditable(false);
    layoutToolbar->addWidget(m_fontCombo);

    bool foundFont = false;

    for (std::vector<QString>::const_iterator i = m_availableFontNames.begin(); i != m_availableFontNames.end(); ++i) {

        QString fontQName(*i);

        m_fontCombo->addItem(fontQName);
        if (fontQName.toLower() == m_fontName.toLower()) {
            m_fontCombo->setCurrentIndex(m_fontCombo->count() - 1);
            foundFont = true;
        }
    }

    if (!foundFont) {
        // don't annoy user with stupid internal warning dialog (except while
        // debugging)
        QMessageBox::warning (this, tr("Rosegarden"), tr("Unknown font \"%1\", using default")
                             .arg(m_fontName) );
        m_fontName = NoteFontFactory::getDefaultFontName();
    }

    connect(m_fontCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotFontComboChanged(int)));

    label = new QLabel(tr("  Size:  "), layoutToolbar);
    layoutToolbar->addWidget(label);

    QString value;

    //
    // font size combo
    //
    m_fontSizeCombo = new QComboBox(layoutToolbar);
    layoutToolbar->addWidget(m_fontSizeCombo);

    for (std::vector<int>::iterator i = m_availableFontSizes.begin(); i != m_availableFontSizes.end(); ++i) {
        value.setNum(*i);
        m_fontSizeCombo->addItem(value);
        if ((*i) == m_fontSize) {
            m_fontSizeCombo->setCurrentIndex(m_fontSizeCombo->count() - 1);
        }
    }

    connect(m_fontSizeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotSizeComboChanged(int)));

    label = new QLabel(tr("  Spacing:  "), layoutToolbar);

    layoutToolbar->addWidget(label);
    
    //
    // spacing combo
    //
    int spacing = m_notationWidget->getScene()->getHSpacing();
    m_availableSpacings = NotationHLayout::getAvailableSpacings();

    m_spacingCombo = new QComboBox(layoutToolbar);
    for (std::vector<int>::iterator i = m_availableSpacings.begin(); i != m_availableSpacings.end(); ++i) {

        value.setNum(*i);
        value += "%";
        m_spacingCombo->addItem(value);
        if ((*i) == spacing) {
            m_spacingCombo->setCurrentIndex(m_spacingCombo->count() - 1);
        }
    }

    connect(m_spacingCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(slotSpacingComboChanged(int)));

    layoutToolbar->addWidget(m_spacingCombo);
}

void
NotationView::initRulersToolbar()
{
    QToolBar *rulersToolbar = findToolbar("Rulers Toolbar");
    if (!rulersToolbar) {
        RG_WARNING << "NotationView::initRulersToolbar() - rulers toolbar not found!";
        return;
    }

    // set the "ruler n" tool button to pop up its menu instantly
    QToolButton *tb = dynamic_cast<QToolButton *>(findToolbar("Rulers Toolbar")->widgetForAction(findAction("add_control_ruler")));
    if (tb) {
        tb->setPopupMode(QToolButton::InstantPopup);
    }
}

void
NotationView::initStatusBar()
{
    QStatusBar* sb = statusBar();

    m_hoveredOverNoteName = new QLabel(sb);
    m_hoveredOverNoteName->setMinimumWidth(32);
    sb->addPermanentWidget(m_hoveredOverNoteName);

    m_hoveredOverAbsoluteTime = new QLabel(sb);
    m_hoveredOverAbsoluteTime->setMinimumWidth(160);
    sb->addPermanentWidget(m_hoveredOverAbsoluteTime);

    m_currentNotePixmap = new QLabel(sb);
    m_currentNotePixmap->setMinimumWidth(20);
    sb->addPermanentWidget(m_currentNotePixmap);

    m_insertModeLabel = new QLabel(sb);
    sb->addPermanentWidget(m_insertModeLabel);

    m_annotationsLabel = new QLabel(sb);
    sb->addPermanentWidget(m_annotationsLabel);

    m_lilyPondDirectivesLabel = new QLabel(sb);
    sb->addPermanentWidget(m_lilyPondDirectivesLabel);
    
    m_selectionCounter = new QLabel(sb);
    sb->addWidget(m_selectionCounter);

    sb->setContentsMargins(0, 0, 0, 0);
}

void
NotationView::slotShowContextHelp(const QString &help)
{
    statusBar()->showMessage(help, 10000);
}

void
NotationView::readOptions()
{
    setCheckBoxState("options_show_toolbar", "General Toolbar");
    setCheckBoxState("show_tools_toolbar", "Tools Toolbar");
    setCheckBoxState("show_accidentals_toolbar", "Accidentals Toolbar");
    setCheckBoxState("show_clefs_toolbar", "Clefs Toolbar");
    setCheckBoxState("show_marks_toolbar", "Marks Toolbar");
    setCheckBoxState("show_group_toolbar", "Group Toolbar");
    setCheckBoxState("show_symbol_toolbar", "Symbols Toolbar");
    setCheckBoxState("show_transport_toolbar", "Transport Toolbar");
    setCheckBoxState("show_layout_toolbar", "Layout Toolbar");
    setCheckBoxState("show_layer_toolbar", "Layer Toolbar");
    setCheckBoxState("show_rulers_toolbar", "Rulers Toolbar");
    setCheckBoxState("show_duration_toolbar", "Duration Toolbar");
    setCheckBoxState("show_interpret_toolbar", "Interpret Toolbar");
    // Suspected BUG.  We read the options, but when do we ever write them?  Not
    // in slotToggleNamedToolBar() I don't imagine.  How would it translate name
    // "Foo Toolbar" into action "show_foo_toolbar" reliably?  I bet it doesn't,
    // but haven't looked.  I have a vague feeling all of these toggle actions
    // have never maintained their persistence reliably, and suspect I know why.
    // To be investigated one day...
}

void
NotationView::setCurrentNotePixmap(QPixmap p)
{
    if (!m_currentNotePixmap) return;
    QPixmap ip = IconLoader().invert(p);
    if (ip.height() > 16) {
        ip = ip.scaledToHeight(16, Qt::SmoothTransformation);
    }
    m_currentNotePixmap->setPixmap(ip);
}

void
NotationView::setCurrentNotePixmapFrom(QAction *a)
{
    if (!a) return;
    //setCurrentNotePixmap(a->icon().pixmap());
    // QT3: You have to use one of the ctors that takes a QSize() argument now,
    // but I only have the vaguest idea what this code does.  I pulled 32x32 out
    // my ass to get the code compiling, but there's a 99.99% chance this is
    // wrong, and even if it's right, it's still wrong to write code this way.
    setCurrentNotePixmap(a->icon().pixmap(QSize(32,32)));
}

bool
NotationView::exportLilyPondFile(QString file, bool forPreview)
{
    QString caption = "", heading = "";
    if (forPreview) {
        caption = tr("LilyPond Preview Options");
        heading = tr("LilyPond preview options");
    }

    LilyPondOptionsDialog dialog(this, m_doc, caption, heading, true);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    RosegardenMainViewWidget * view = RosegardenMainWindow::self()->getView();

    LilyPondExporter e(m_doc, view->getSelection(), std::string(QFile::encodeName(file)), this);

    if (!e.write()) {
        QMessageBox::warning(this, tr("Rosegarden"), e.getMessage());
        return false;
    }

    return true;
}

void
NotationView::slotPrintLilyPond()
{
    TmpStatusMsg msg(tr("Printing with LilyPond..."), this);

    QString filename = getLilyPondTmpFilename();

    if (filename.isEmpty()) return;

    if (!exportLilyPondFile(filename, true)) {
        return ;
    }

    LilyPondProcessor *dialog = new LilyPondProcessor(this, LilyPondProcessor::Print, filename);

    dialog->exec();
}

void
NotationView::slotPreviewLilyPond()
{
    TmpStatusMsg msg(tr("Previewing with LilyPond..."), this);

    QString filename = getLilyPondTmpFilename();

    if (filename.isEmpty()) return;

    if (!exportLilyPondFile(filename, true)) {
        return ;
    }

    LilyPondProcessor *dialog = new LilyPondProcessor(this, LilyPondProcessor::Preview, filename);

    dialog->exec();
}

QString
NotationView::getLilyPondTmpFilename()
{
    QString mask = QString("%1/rosegarden_tmp_XXXXXX.ly").arg(QDir::tempPath());
    RG_DEBUG << "NotationView::getLilyPondTmpName() - using tmp file: " << qstrtostr(mask);

    QTemporaryFile *file = new QTemporaryFile(mask);
    file->setAutoRemove(true);
    if (!file->open()) {
        QMessageBox::warning(this, tr("Rosegarden"),
                                       tr("<qt><p>Failed to open a temporary file for LilyPond export.</p>"
                                          "<p>This probably means you have run out of disk space on <pre>%1</pre></p></qt>").
                                       arg(QDir::tempPath()));
        delete file;
        return QString();
    }
    QString filename = file->fileName(); // must call this before close()
    file->close(); // we just want the filename

    return filename;
}


void
NotationView::slotLinearMode()
{
    enterActionState("linear_mode");
    if (m_notationWidget) m_notationWidget->slotSetLinearMode();
}

void
NotationView::slotContinuousPageMode()
{
    leaveActionState("linear_mode");
    if (m_notationWidget) m_notationWidget->slotSetContinuousPageMode();
}

void
NotationView::slotMultiPageMode()
{
    leaveActionState("linear_mode");
    if (m_notationWidget) m_notationWidget->slotSetMultiPageMode();
}

void
NotationView::slotShowHeadersGroup()
{
    if (m_notationWidget) m_notationWidget->toggleHeadersView();
}

void
NotationView::slotChangeFontFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();
    if (name.left(10) == "note_font_") {
        name = name.right(name.length() - 10);
        if (m_notationWidget) m_notationWidget->slotSetFontName(name);
        for (uint i = 0; i < m_availableFontNames.size(); ++i) {
            if (m_availableFontNames[i] == name) {
                m_fontCombo->setCurrentIndex(i);
                break;
            }
        }
    } else {
        QMessageBox::warning
            (this, tr("Rosegarden"), tr("Unknown font action %1").arg(name));
    }
}

void
NotationView::slotChangeFontSizeFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    if (name.left(15) == "note_font_size_") {
        name = name.right(name.length() - 15);
        bool ok = false;
        int size = name.toInt(&ok);
        if (ok) {
            if (m_notationWidget) m_notationWidget->slotSetFontSize(size);
            for (uint i = 0; i < m_availableFontSizes.size(); ++i) {
                if (m_availableFontSizes[i] == size) {
                    m_fontSizeCombo->setCurrentIndex(i);
                    break;
                }
            }
            return;
        } 
    }
    QMessageBox::warning
        (this, tr("Rosegarden"), tr("Unknown font size action %1").arg(name));
}

void
NotationView::slotChangeSpacingFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    if (name.left(8) == "spacing_") {
        name = name.right(name.length() - 8);
        bool ok = false;
        int spacing = name.toInt(&ok);
        if (ok) {
            if (m_notationWidget) m_notationWidget->getScene()->setHSpacing(spacing);
            for (uint i = 0; i < m_availableSpacings.size(); ++i) {
                if (m_availableSpacings[i] == spacing) {
                    m_spacingCombo->setCurrentIndex(i);
                    break;
                }
            }
            return;
        } 
    }
    QMessageBox::warning
        (this, tr("Rosegarden"), tr("Unknown spacing action %1").arg(name));
}

Segment *
NotationView::getCurrentSegment()
{
    if (m_notationWidget) return m_notationWidget->getCurrentSegment();
    else return 0;
}

EventSelection *
NotationView::getSelection() const
{
    if (m_notationWidget) return m_notationWidget->getSelection();
    else return 0;
}

void
NotationView::setSelection(EventSelection *selection, bool preview)
{
    if (m_notationWidget) m_notationWidget->setSelection(selection, preview);
}

timeT
NotationView::getInsertionTime() const
{
    if (m_notationWidget) return m_notationWidget->getInsertionTime();
    else return 0;
}

void
NotationView::slotEditCut()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CutCommand(*selection, getClipboard()));
}

void
NotationView::slotEditDelete()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand(new EraseCommand(*selection));
}

void
NotationView::slotEditCopy()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CopyCommand(*selection, getClipboard()));
}

void
NotationView::slotEditCutAndClose()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CutAndCloseCommand(*selection, getClipboard()));
}

void
NotationView::slotEditPaste()
{
    Clipboard *clipboard = getClipboard();

    if (clipboard->isEmpty()) return;
    if (!clipboard->isSingleSegment()) {
        slotStatusHelpMsg(tr("Can't paste multiple Segments into one"));
        return;
    }

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    // Paste at cursor position
    //
    timeT insertionTime = getInsertionTime();
    timeT endTime = insertionTime +
        (clipboard->getSingleSegment()->getEndTime() -
         clipboard->getSingleSegment()->getStartTime());

    PasteEventsCommand::PasteType defaultType =
        PasteNotationDialog::getSavedPasteType();

    PasteEventsCommand *command = new PasteEventsCommand
        (*segment, clipboard, insertionTime, defaultType);

    if (!command->isPossible()) {
        // NOTES: To get a reasonable presentation of the standard and detailed
        // text, we have to build up our own QMessageBox
        //
        // The old RESTRICTED_PASTE_DESCRIPTION was removed because it was
        // impossible to get the translation, which had to be done in the
        // QObject::tr() context, to work in this context here.  Qt is really
        // quirky that way.  Instead, I'm just block copying all of this now
        // that I've reworked it.  Is this copy you're looking at the original,
        // or the copy?  Only I know for sure, and I'll never tell!  Bwa haha!
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Rosegarden"));
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("Couldn't paste at this point."));
        if (defaultType == PasteEventsCommand::Restricted) {
            msgBox.setInformativeText(tr("<qt><p>The Restricted paste type requires enough empty space (containing only rests) at the paste position to hold all of the events to be pasted.</p><p>Not enough space was found.</p><p>If you want to paste anyway, consider using one of the other paste types from the <b>Paste...</b> option on the Edit menu.  You can also change the default paste type to something other than Restricted if you wish.</p></qt>"));
        }
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.exec();
        delete command;
    } else {
        CommandHistory::getInstance()->addCommand(command);
        setSelection(command->getSubsequentSelection(), false);
//!!!        slotSetInsertCursorPosition(endTime, true, false);
        m_document->slotSetPointerPosition(endTime);
    }
}

void
NotationView::slotEditGeneralPaste()
{
    Clipboard *clipboard = getClipboard();

    if (clipboard->isEmpty()) {
        slotStatusHelpMsg(tr("Clipboard is empty"));
        return ;
    }

    slotStatusHelpMsg(tr("Inserting clipboard contents..."));

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    PasteNotationDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {

        PasteEventsCommand::PasteType type = dialog.getPasteType();

        timeT insertionTime = getInsertionTime();
        timeT endTime = insertionTime +
            (clipboard->getSingleSegment()->getEndTime() -
             clipboard->getSingleSegment()->getStartTime());

        PasteEventsCommand *command = new PasteEventsCommand
            (*segment, clipboard, insertionTime, type);

        if (!command->isPossible()) {
            // NOTES: To get a reasonable presentation of the standard and detailed
            // text, we have to build up our own QMessageBox
            //
            // The old RESTRICTED_PASTE_DESCRIPTION was removed because it was
            // impossible to get the translation, which had to be done in the
            // QObject::tr() context, to work in this context here.  Qt is really
            // quirky that way.  Instead, I'm just block copying all of this now
            // that I've reworked it.  Is this copy you're looking at the original,
            // or the copy?  Only I know for sure, and I'll never tell!  Bwa haha!
            QMessageBox msgBox;
            msgBox.setWindowTitle(tr("Rosegarden"));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText(tr("Couldn't paste at this point."));
            if (type == PasteEventsCommand::Restricted) {
                msgBox.setInformativeText(tr("<qt><p>The Restricted paste type requires enough empty space (containing only rests) at the paste position to hold all of the events to be pasted.</p><p>Not enough space was found.</p><p>If you want to paste anyway, consider using one of the other paste types from the <b>Paste...</b> option on the Edit menu.  You can also change the default paste type to something other than Restricted if you wish.</p></qt>"));
            }
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.exec();
            delete command;
        } else {
            CommandHistory::getInstance()->addCommand(command);
            setSelection(new EventSelection(*segment, insertionTime, endTime),
                         false);
//!!!            slotSetInsertCursorPosition(endTime, true, false);
            m_document->slotSetPointerPosition(endTime);
        }
    }
}

void
NotationView::slotPreviewSelection()
{
    if (!getSelection())
        return ;

    getDocument()->slotSetLoop(getSelection()->getStartTime(),
                               getSelection()->getEndTime());
}

void
NotationView::slotClearSelection()
{
    // Actually we don't clear the selection immediately: if we're
    // using some tool other than the select tool, then the first
    // press switches us back to the select tool.

    NotationSelector *selector = dynamic_cast<NotationSelector *>(m_notationWidget->getCurrentTool());

    if (!selector) {
        slotSetSelectTool();
    } else {
        setSelection(0, false);
    }
}

void
NotationView::slotEditSelectFromStart()
{
    timeT t = getInsertionTime();
    Segment *segment = getCurrentSegment();
    setSelection(new EventSelection(*segment,
                                    segment->getStartTime(),
                                    t),
                 false);
}

void
NotationView::slotEditSelectToEnd()
{
    timeT t = getInsertionTime();
    Segment *segment = getCurrentSegment();
    setSelection(new EventSelection(*segment,
                                    t,
                                    segment->getEndMarkerTime()),
                 false);
}

void
NotationView::slotEditSelectWholeStaff()
{
    Segment *segment = getCurrentSegment();
    setSelection(new EventSelection(*segment,
                                    segment->getStartTime(),
                                    segment->getEndMarkerTime()),
                 false);
}

void
NotationView::slotSearchSelect()
{
    NOTATION_DEBUG << "NotationView::slotSearchSelect";

    SelectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        NOTATION_DEBUG << "slotSearchSelect- accepted";
    }
}

void
NotationView::slotFilterSelection()
{
    NOTATION_DEBUG << "NotationView::slotFilterSelection";

    Segment *segment = getCurrentSegment();
    EventSelection *existingSelection = getSelection();
    if (!segment || !existingSelection)
        return ;

    EventFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        NOTATION_DEBUG << "slotFilterSelection- accepted";

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

        if (haveEvent) {
            setSelection(newSelection, false);
        } else {
            setSelection(0, false);
        }
    }
}

// Launch SelectAddEvenNotesCommand
void
NotationView::slotSelectEvenlySpacedNotes()
{
    if (!getSelection()) { return; }

    EventSelection *eventSelection = getSelection();
    if (eventSelection->getSegmentEvents().size() < 2) { return; }
    BasicCommand *command = new
        SelectAddEvenNotesCommand(SelectAddEvenNotesCommand::findBeatEvents(eventSelection),
                           &eventSelection->getSegment());

    CommandHistory::getInstance()->addCommand(command);
    setSelection(command->getSubsequentSelection(), false);    
}

void
NotationView::slotVelocityUp()
{
    if (!getSelection())
        return ;
    TmpStatusMsg msg(tr("Raising velocities..."), this);

    CommandHistory::getInstance()->addCommand(new ChangeVelocityCommand(
            10, *getSelection()));
}

void
NotationView::slotVelocityDown()
{
    if (!getSelection())
        return ;
    TmpStatusMsg msg(tr("Lowering velocities..."), this);

    CommandHistory::getInstance()->addCommand(new ChangeVelocityCommand(
            -10, *getSelection()));
}

void
NotationView::slotSetVelocities()
{
    ParameterPattern::
        setVelocities(this, getSelection());
}

void
NotationView::slotEditCopyControllers()
{
    ControlRulerWidget *cr = m_notationWidget->getControlsWidget();
    EventSelection *selection = cr->getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CopyCommand(*selection, getClipboard()));
}

void
NotationView::slotEditCutControllers()
{
    ControlRulerWidget *cr = m_notationWidget->getControlsWidget();
    EventSelection *selection = cr->getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CutCommand(*selection, getClipboard()));
}

void
NotationView::slotSetControllers()
{
    ControlRulerWidget * cr = m_notationWidget->getControlsWidget();
    ParameterPattern::
        setProperties(this, cr->getSituation(),
                      &ParameterPattern::VelocityPatterns);
}

void
NotationView::slotPlaceControllers()
{
    EventSelection *selection = getSelection();
    if (!selection) { return; }
    
    ControlRulerWidget *cr = m_notationWidget->getControlsWidget();
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
NotationView::slotClearLoop()
{
    getDocument()->slotSetLoop(0, 0);
}

void
NotationView::slotCurrentStaffUp()
{
    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;
    timeT targetTime = m_doc->getComposition().getPosition();
    NotationStaff *staff = scene->getStaffAbove(targetTime);
    if (!staff) return;
    setCurrentStaff(staff);
}

void
NotationView::slotCurrentStaffDown()
{
    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;
    timeT targetTime = m_doc->getComposition().getPosition();
    NotationStaff *staff = scene->getStaffBelow(targetTime);
    if (!staff) return;
    setCurrentStaff(staff);
}

void
NotationView::slotCurrentSegmentPrior()
{
    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;
    NotationStaff *staff = scene->getPriorStaffOnTrack();
    if (!staff) return;
    setCurrentStaff(staff);
    slotEditSelectWholeStaff();
}

void
NotationView::slotCurrentSegmentNext()
{
    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;
    NotationStaff *staff = scene->getNextStaffOnTrack();
    if (!staff) return;
    setCurrentStaff(staff);
    slotEditSelectWholeStaff();
}

void
NotationView::
setCurrentStaff(NotationStaff *staff)
{
    if (!staff) return;
    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;

    if (findAdopted(&staff->getSegment()) != m_adoptedSegments.end())
        { enterActionState("focus_adopted_segment"); }
    else
        { leaveActionState("focus_adopted_segment"); }

    scene->setCurrentStaff(staff);
    m_notationWidget->slotUpdatePointerPosition(true);
    // m_notationWidget->
    //     slotPointerPositionChanged(m_document->getComposition().getPosition(),
    //                                true);
}

void
NotationView::slotToggleGeneralToolBar()
{
    toggleNamedToolBar("General Toolbar");
}

void
NotationView::slotToggleToolsToolBar()
{
    toggleNamedToolBar("Tools Toolbar");
}

void
NotationView::slotToggleDurationToolBar()
{
    toggleNamedToolBar("Duration Toolbar");
}

void
NotationView::slotToggleInterpretToolBar()
{
    toggleNamedToolBar("Interpret Toolbar");
}

void
NotationView::slotToggleAccidentalsToolBar()
{
    toggleNamedToolBar("Accidentals Toolbar");
}

void
NotationView::slotToggleClefsToolBar()
{
    toggleNamedToolBar("Clefs Toolbar");
}

void
NotationView::slotToggleMarksToolBar()
{
    toggleNamedToolBar("Marks Toolbar");
}

void
NotationView::slotToggleGroupToolBar()
{
    toggleNamedToolBar("Group Toolbar");
}

void
NotationView::slotToggleSymbolsToolBar()
{
    toggleNamedToolBar("Symbols Toolbar");
}

void
NotationView::slotToggleLayoutToolBar()
{
    toggleNamedToolBar("Layout Toolbar");
}

void
NotationView::slotToggleRulersToolBar()
{
    toggleNamedToolBar("Rulers Toolbar");
}

void
NotationView::slotToggleTransportToolBar()
{
    toggleNamedToolBar("Transport Toolbar");
}

void
NotationView::slotToggleLayerToolBar()
{
    toggleNamedToolBar("Layer Toolbar");
}

void
NotationView::toggleNamedToolBar(const QString& toolBarName, bool* force)
{
//     QToolBar *namedToolBar = toolBar(toolBarName);
    QToolBar *namedToolBar = findChild<QToolBar*>(toolBarName);

    if (!namedToolBar) {
        NOTATION_DEBUG << "NotationView::toggleNamedToolBar() : toolBar "
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

//     setSettingsDirty();    //&&& not required ?

}

void
NotationView::slotSetSelectTool()
{
    if (m_notationWidget) m_notationWidget->slotSetSelectTool();
    slotUpdateMenuStates();
}    

void
NotationView::slotSetSelectNoTiesTool()
{
    if (m_notationWidget) m_notationWidget->slotSetSelectNoTiesTool();
    slotUpdateMenuStates();
}    

void
NotationView::slotSetEraseTool()
{
    if (m_notationWidget) m_notationWidget->slotSetEraseTool();
    slotUpdateMenuStates();
}    

void
NotationView::slotSetNoteRestInserter()
{
    NOTATION_DEBUG << "NotationView::slotSetNoteRestInserter : entered. ";

    if (m_notationWidget) m_notationWidget->slotSetNoteRestInserter();

    //Must ensure it is set since may be called from multiple actions. 
    findAction("draw")->setChecked(true);
    slotUpdateMenuStates();
}    

void
NotationView::slotSwitchToNotes()
{
    NOTATION_DEBUG << "NotationView::slotSwitchToNotes : entered. ";

    QString actionName = "";
    NoteRestInserter *currentInserter = NULL;
    if (m_notationWidget) {
        currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());

        if (!currentInserter) {
            // Switch to NoteRestInserter
            slotSetNoteRestInserter();
            NOTATION_DEBUG << "NotationView::slotSwitchToNotes() : " 
                    << "NoteRestInserter not current. Attempted to  switch. ";
            //Try again to see if tool is set.
            currentInserter = dynamic_cast<NoteRestInserter *>
                    (m_notationWidget->getCurrentTool());
            if (!currentInserter) {
                NOTATION_DEBUG << "NotationView::slotSwitchToNotes() : expected"
                        << " NoteRestInserter as current tool & "
                        << "could not switch to it.  Silent exit.";
                return;
            }
        }

        Note::Type unitType = currentInserter->getCurrentNote()
            .getNoteType();
        int dots = (currentInserter->getCurrentNote().getDots() ? 1 : 0);
        actionName = NotationStrings::getReferenceName(Note(unitType,dots));
        actionName.replace(QRegExp("-"), "_");

        m_notationWidget->slotSetNoteInserter();
    }

    //Must set duration_ shortcuts to false to fix bug when in rest mode
    // and a duration shortcut key is pressed (or selected from dur. menu).
    findAction(QString("duration_%1").arg(actionName))->setChecked(false);
    QAction *currentAction = findAction(actionName);
    currentAction->setChecked(true);
    
    // This code and last line above used to maintain exclusive state
    // of the Duration Toolbar so we can reactivate the NoteRestInserter
    // even when from a pressed button on the bar.
    
    // Now un-select previous pressed button pressed
    if (currentAction != m_durationPressed) {
        m_durationPressed->setChecked(false);
        m_durationPressed = currentAction;
    }

    morphDurationMonobar();

    slotUpdateMenuStates();
}

void
NotationView::slotSwitchToRests()
{
    NOTATION_DEBUG << "NotationView::slotSwitchToRests : entered. ";

    QString actionName = "";
    NoteRestInserter *currentInserter = NULL;
    if (m_notationWidget) {
        currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());

        if (!currentInserter) {
            // Switch to NoteRestInserter
            slotSetNoteRestInserter();
            NOTATION_DEBUG << "NotationView::slotSwitchToRests() : " 
                    << "NoteRestInserter not current. Attempted to  switch. ";
            
            //Try again to see if tool is set.
            currentInserter = dynamic_cast<NoteRestInserter *>
                    (m_notationWidget->getCurrentTool());
            if (!currentInserter) {
                NOTATION_DEBUG << "NotationView::slotSwitchToRests() : expected"
                        << " NoteRestInserter as current tool & "
                        << "could not switch to it.  Silent exit.";
                return;
            }
        }

        Note::Type unitType = currentInserter->getCurrentNote()
            .getNoteType();
        int dots = (currentInserter->getCurrentNote().getDots() ? 1 : 0);
        actionName = NotationStrings::getReferenceName(Note(unitType,dots));
        actionName.replace(QRegExp("-"), "_");

        m_notationWidget->slotSetRestInserter();
    }

    //Must set duration_ shortcuts to false to fix bug when in rest mode
    // and a duration shortcut key is pressed (or selected from dur. menu).
    findAction(QString("duration_%1").arg(actionName))->setChecked(false);
    findAction(QString("rest_%1").arg(actionName))->setChecked(true);


    //Must set duration_ shortcuts to false to fix bug when in rest mode
    // and a duration shortcut key is pressed (or selected from dur. menu).
    findAction(QString("duration_%1").arg(actionName))->setChecked(false);
    QAction *currentAction = findAction(QString("rest_%1").arg(actionName));
    currentAction->setChecked(true);
    
    // This code and last line above used to maintain exclusive state
    // of the Duration Toolbar so we can reactivate the NoteRestInserter
    // even when from a pressed button on the bar.
    
    // Now un-select previous pressed button pressed
    if (currentAction != m_durationPressed) {
        m_durationPressed->setChecked(false);
        m_durationPressed = currentAction;
    }

    morphDurationMonobar();

    slotUpdateMenuStates();
}

void
NotationView::morphDurationMonobar()
{
    NOTATION_DEBUG << "NotationView::morphDurationMonobar : entered. ";

    NoteRestInserter *currentInserter = 0; 
    if (m_notationWidget) {
        currentInserter = dynamic_cast<NoteRestInserter *>
        (m_notationWidget->getCurrentTool());
    }

    if (!currentInserter)
    {
        // Morph called when NoteRestInserter not set as current tool
        NOTATION_DEBUG << "NotationView::morphNotationToolbar() : expected"
               << " NoteRestInserter.  Silent Exit." 
              ;
        return;

    }
    // Retrieve duration and dot values
    int dots = currentInserter->getCurrentNote().getDots();
    Note::Type note = currentInserter->getCurrentNote().getNoteType();

    // Determine duration tooolbar mode
    DurationMonobarModeType newMode = InsertingNotes;
    if (currentInserter->isaRestInserter()) {
        newMode = (dots ? InsertingDottedRests : InsertingRests);
    } else {
        newMode = (dots ? InsertingDottedNotes : InsertingNotes);
    }
    
    //Convert to English for debug purposes.        
    std::string modeStr;
    switch (newMode) {

    case InsertingNotes: modeStr = "Notes Toolbar"; break;
    case InsertingDottedNotes: modeStr = "Dotted Notes Toolbar"; break;
    case InsertingRests: modeStr = "Rests Toolbar"; break;
    case InsertingDottedRests: modeStr = "Dotted Rests Toolbar"; break;
    default: modeStr = "WTF?  This won't be pretty.";

    }
    NOTATION_DEBUG << "NotationView::morphDurationMonobar: morphing to "
        << modeStr;

    if (newMode == m_durationMode && note != Note::Shortest && dots) {
        NOTATION_DEBUG << "NotationView::morphDurationMonobar: new "
            << "mode and last mode are the same.  exit wothout morphing."
           ;
        return;
    }
    
    // Turn off current state (or last state--depending on perspective.)
    switch (m_durationMode) {

    case InsertingNotes:
        leaveActionState("note_0_dot_mode");
        break;

    case InsertingDottedNotes:
        leaveActionState("note_1_dot_mode");
        break;

    case InsertingRests:
        leaveActionState("rest_0_dot_mode");
        break;

    case InsertingDottedRests:
        leaveActionState("rest_1_dot_mode");
        break;

    default:
        NOTATION_DEBUG << "NotationView::morphDurationMonobar:  None of "
            << "The standard four modes were selected for m_durationMode. "
            << "How did that happen?";
    }

    // transfer new mode to member for next recall.
    m_durationMode = newMode;
    
    // Now morph to new state.
    switch (newMode) {

    case InsertingNotes:
        enterActionState("note_0_dot_mode");
        break;

    case InsertingDottedNotes:
        enterActionState("note_1_dot_mode");
        break;

    case InsertingRests:
        enterActionState("rest_0_dot_mode");
        break;

    case InsertingDottedRests:
        enterActionState("rest_1_dot_mode");
        break;
    default:
        NOTATION_DEBUG << "NotationView::morphDurationMonobar:  None of "
            << "The standard four modes were selected for newMode. "
            << "How did that happen?";
    }

    // This code to manage shortest dotted note selection.
    // Disable the shortcut in the menu for shortest duration.
    if (note == Note::Shortest && !dots) {
        NOTATION_DEBUG << "NotationView::morphDurationMonobar:  shortest "
            << "note / no dots.  disable off +. action";
        QAction *switchDots = findAction("switch_dots_on");
        switchDots->setEnabled(false);
    }
}

void
NotationView::initializeNoteRestInserter()
{     
    // Set Default Duration based on Time Signature denominator.
    // The default unitType is taken from the denominator of the time signature:
    //   e.g. 4/4 -> 1/4, 6/8 -> 1/8, 2/2 -> 1/2.
    TimeSignature sig = getDocument()->getComposition().getTimeSignatureAt(getInsertionTime());
    Note::Type unitType = sig.getUnit();

    QString actionName = NotationStrings::getReferenceName(Note(unitType,0));
    actionName.replace(QRegExp("-"), "_");

    //Initialize Duration Toolbar (hide all buttons)   
    leaveActionState("note_0_dot_mode");
    leaveActionState("note_1_dot_mode");
    leaveActionState("rest_0_dot_mode");
    leaveActionState("rest_1_dot_mode");
    
    //Change exclusive settings so we can retrigger Duration Toolbar
    //actions when button needed is pressed.
    //exclusive state maintianed via slotSwitchToRests() / slotSwitchToNotes().
    findGroup("duration_toolbar")->setExclusive(false);


    // Initialize the m_durationPressed so we don't have to null check elswhere.
    m_durationPressed = findAction(QString("duration_%1").arg(actionName));

    // Counting on a InsertingRests to be stored in NoteRestInserter::
    // m_durationMode which it was passed in the constructor.  This will
    // ensure morphDurationMonobar always fires correctly since
    // a duration_ shortcut is always tied to the note palette.
    m_durationPressed->trigger();

    //Change exclusive settings so we can retrigger Accidental Toolbar
    //actions when button needed is pressed.
    //exclusive state maintianed via manageAccidentalAction().
    findGroup("accidentals")->setExclusive(false);

    // Initialize the m_durationPressed so we don't have to null check elswhere.
    m_accidentalPressed = findAction("no_accidental");
}

int
NotationView::getPitchFromNoteInsertAction(QString name,
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
            RG_WARNING << "NotationView::getPitchFromNoteInsertAction: pitch "
                      << scalePitch << " out of range, using 0";
            scalePitch = 0;
        }

        Pitch clefPitch(clef.getAxisHeight(), clef, key, NoAccidental);

        int clefOctave = clefPitch.getOctave();
        int pitchOctave = clefOctave + octave;

        NOTATION_DEBUG << "NotationView::getPitchFromNoteInsertAction:"
                       << " key = " << key.getName() 
                       << ", clef = " << clef.getClefType() 
                       << ", octaveoffset = " << clef.getOctaveOffset()
                      ;
        NOTATION_DEBUG << "NotationView::getPitchFromNoteInsertAction: octave = "
                       << pitchOctave;
        
        // Rewrite to fix bug #2997303 :
        //
        // We want to make sure that
        //    (i) The lowest note in scale (with octave = -1) is drawn below
        //        the staff
        //    (ii) The highest note in scale (with octave = +1) is drawn above
        //         the staff
        //
        // Let lnh be the height on staff of this lowest note and let hnh be
        // the height on staff of this highest note.
        //    (iii) hnh = lnh + 7 + 7 + 6 = lnh + 20
        //
        // (iv) One way to have (i) and (ii) verified is to make the middle
        // of lnh and hnh, i.e. (lnh + hnh) / 2, as near as possible of
        // the middle of the staff, i.e. 4.
        //    
        // (iii) and (iv) result in lnh being as near as possible of -6,
        //    i.e.  -10 < lnh < -2.

        int lowestNoteInScale = 0;
        Pitch lowestPitch(lowestNoteInScale, clefOctave - 1, key, NoAccidental);

        int lnh = lowestPitch.getHeightOnStaff(clef, key);
        for (; lnh < -9; lnh += 7) pitchOctave++;
        for (; lnh > -3; lnh -= 7) pitchOctave--;

        NOTATION_DEBUG << "NotationView::getPitchFromNoteInsertAction: octave = "
                       << pitchOctave << " (adjusted)";
        
        Pitch pitch(scalePitch, pitchOctave, key, accidental);
        return pitch.getPerformancePitch();

    } else {

        throw Exception("Not an insert action",
                        __FILE__, __LINE__);
    }
}

void
NotationView::slotExpressionSequence()
{
    insertControllerSequence(ControlParameter::getExpression());
}

void
NotationView::slotPitchBendSequence()
{
    insertControllerSequence(ControlParameter::getPitchBend());
}

void
NotationView::slotControllerSequence()
{
    ControlRulerWidget *cr = m_notationWidget->getControlsWidget();
    // ??? We should pop up a message: No ruler is visible/selected.
    if (!cr) { return; }
    
    const ControlParameter *cp = cr->getControlParameter();
    // ??? We should pop up a message: Selected ruler is not a controller ruler.
    if (!cp) { return; }

    insertControllerSequence(*cp);
}

void
NotationView::
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
NotationView::slotInsertNoteFromAction()
{
    const QObject *s = sender();
    QString name = s->objectName();

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    NoteRestInserter *currentInserter = 0;
    if(m_notationWidget) {
        currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());

        if(!currentInserter) {
            //set the NoteRestInserter as current
            slotSetNoteRestInserter();
            //re-fetch the current tool for analysis
            currentInserter = dynamic_cast<NoteRestInserter *>
                (m_notationWidget->getCurrentTool());
        }
    
        if (currentInserter) {
            if (currentInserter->isaRestInserter()) {
                slotSwitchToNotes();
            }
            int pitch = 0;
            Accidental accidental = Accidentals::NoAccidental;

            timeT insertionTime = getInsertionTime();
            Rosegarden::Key key = segment->getKeyAtTime(insertionTime);
            Clef clef = segment->getClefAtTime(insertionTime);

            try {

                RG_DEBUG << "NotationView::slotInsertNoteFromAction: time = "
                    << insertionTime << ", key = " << key.getName()
                    << ", clef = " << clef.getClefType() << ", octaveoffset = "
                    << clef.getOctaveOffset();

                pitch = getPitchFromNoteInsertAction(name, accidental, clef, key);

            } catch (...) {

                QMessageBox::warning
                    (this, tr("Rosegarden"),  tr("Unknown note insert action %1").arg(name));
                return ;
            }

            TmpStatusMsg msg(tr("Inserting note"), this);

            NOTATION_DEBUG << "Inserting note at pitch " << pitch;
            currentInserter->insertNote(*segment, insertionTime,
                pitch, accidental, 100); // Velocity hard coded for now.
        }
    }
}

void
NotationView::slotInsertRestFromAction()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;
    
    NoteRestInserter *currentInserter = 0;
    if(m_notationWidget) {
        currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());

        if(!currentInserter) {
            //set the NoteRestInserter as current
            slotSetNoteRestInserter();
            //re-fetch the current tool for analysis
            currentInserter = dynamic_cast<NoteRestInserter *>
                (m_notationWidget->getCurrentTool());
        }
    
        if (currentInserter) {
            if (!currentInserter->isaRestInserter()) {
                slotSwitchToRests();
            }
           timeT insertionTime = getInsertionTime();

           currentInserter->insertNote(*segment, insertionTime,
               0, Accidentals::NoAccidental, true);
        }
    }
}

void
NotationView::slotToggleDot()
{
    NOTATION_DEBUG << "NotationView::slotToggleDot : entered. ";
    NoteRestInserter *currentInserter = NULL;
    if (m_notationWidget) {
        currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());
        if (!currentInserter) {
            // Switch to NoteRestInserter
            slotSetNoteRestInserter();
            NOTATION_DEBUG << "NotationView::slotToggleDot() : " 
                    << "NoteRestInserter not current. Attempted to  switch. ";
            
            //Try again to see if tool is set.
            currentInserter = dynamic_cast<NoteRestInserter *>
                    (m_notationWidget->getCurrentTool());
            if (!currentInserter) {

                NOTATION_DEBUG << "NotationView::slotToggleDot() : expected"
                        << " NoteRestInserter as current tool & "
                        << "could not switch to it.  Silent exit.";
                return;
            }
        }
        Note note = currentInserter->getCurrentNote();

        Note::Type noteType = note.getNoteType();
        int noteDots = (note.getDots() ? 0 : 1); // Toggle the dot state
        
        if (noteDots && noteType == Note::Shortest)
        {
            // This might have been invoked via a keboard shortcut or other
            // toggling the +. button when the shortest note was pressed.
            // RG does not render dotted versions of its shortest duration
            // and rounds it up to the next duration.
            // Following RG's lead on this makes the inteface feel off since
            // This moves the toggle to the next longest duration without
            // switching the pallete to dots.
            // So just leave the duration alone and don't toggle the dot
            // in this case.
            noteDots = 0;
        }

        QString actionName(NotationStrings::getReferenceName(Note(noteType,noteDots)));
        actionName.replace(QRegExp("-"), "_");

        m_notationWidget->slotSetInsertedNote(noteType, noteDots);
        if (currentInserter->isaRestInserter()) {
            slotSwitchToRests();
        } else {
            slotSwitchToNotes();
        }
    }
}

void
NotationView::slotNoteAction()
{
    NOTATION_DEBUG << "NotationView::slotNoteAction : entered. ";

    QObject *s = sender();
    QAction *a = dynamic_cast<QAction *>(s);
    QString name = s->objectName();
    QString noteToolbarName;

    //Set defaults for duration_ shortcut calls
    bool rest = false;  
    int dots = 0;

    if (m_notationWidget) {
        NoteRestInserter *currentTool = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());
        if (!currentTool) {
            //Must select NoteRestInserter tool as current Tool
            slotSetNoteRestInserter();
            //Now re-fetch the current tool for analysis.
            currentTool = dynamic_cast<NoteRestInserter *>(m_notationWidget
                ->getCurrentTool());
        }
        if (name.startsWith("duration_")) {
            name = name.replace("duration_", "");
            NOTATION_DEBUG << "NotationView::slotNoteAction : "
                << "Duration shortcut called.";
            //duration shortcut called from keyboard or menu.
            //Must switch to insert Notes mode.

        } else if (currentTool->isaRestInserter()) {
            NOTATION_DEBUG << "NotationView::slotNoteAction : "
                << "Have rest inserter.";
            if (name.startsWith("rest_")) {
                name = name.replace("rest_", "");
            }
            rest = true;
        } else {
            NOTATION_DEBUG << "NotationView::slotNoteAction : "
                << "Have note inserter.";
        }
    }

    if (name.startsWith("dotted_")) {
        dots = 1;
        name = name.replace("dotted_", "");
    }

    Note::Type type = NotationStrings::getNoteForName(name).getNoteType();
    if (m_notationWidget) {
        m_notationWidget->slotSetInsertedNote(type, dots);
        if (rest) {
            slotSwitchToRests();
        } else {
            slotSwitchToNotes();
        }
    }

    setCurrentNotePixmapFrom(a);
}

void
NotationView::slotDummy1()
{
    // Empty function required to appease Qt.
}

void
NotationView::manageAccidentalAction(QString actionName)
{
     NOTATION_DEBUG << "NotationView::manageAccidentalAction: enter. "
         << "actionName = " << actionName << ".";

    // Manage exclusive group setting since group->isExclusive() == false.
    QAction *currentAction = findAction(actionName);
    // Force the current button to be pressed
    currentAction->setChecked(true);
    if (m_accidentalPressed != currentAction) {
        m_accidentalPressed->setChecked(false);
        m_accidentalPressed = currentAction;
    }

    // Set The Note / Rest Inserter Tool as curretn tool if needed.
    if (m_notationWidget) {
        NoteRestInserter *currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());
        if (!currentInserter) {
            slotSetNoteRestInserter();
            
            // re-fetch tool for analysis.
            currentInserter = dynamic_cast<NoteRestInserter *>
            (m_notationWidget->getCurrentTool());
        }
        if (currentInserter->isaRestInserter()) {
            slotSwitchToNotes();
        }
    }
    
}

void
NotationView::slotNoAccidental()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(NoAccidental, false);
}

void
NotationView::slotFollowAccidental()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(NoAccidental, true);
}

void
NotationView::slotSharp()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(Sharp, false);
}

void
NotationView::slotFlat()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(Flat, false);
}

void
NotationView::slotNatural()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(Natural, false);
}

void
NotationView::slotDoubleSharp()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(DoubleSharp, false);
}

void
NotationView::slotDoubleFlat()
{
    QObject *s = sender();
    QString name = s->objectName();
    
    manageAccidentalAction(name);
    
    if (m_notationWidget) m_notationWidget->slotSetAccidental(DoubleFlat, false);
}

void
NotationView::slotClefAction()
{
    QObject *s = sender();
    QAction *a = dynamic_cast<QAction *>(s);
    QString n = s->objectName();

    Clef type = Clef::Treble;

    if (n == "treble_clef") type = Clef::Treble;
    else if (n == "alto_clef") type = Clef::Alto;
    else if (n == "tenor_clef") type = Clef::Tenor;
    else if (n == "bass_clef") type = Clef::Bass;

    setCurrentNotePixmapFrom(a);

    if (!m_notationWidget) return;
    m_notationWidget->slotSetClefInserter();
    m_notationWidget->slotSetInsertedClef(type);
    slotUpdateMenuStates();
}

void
NotationView::slotText()
{
    QObject *s = sender();
    setCurrentNotePixmapFrom(dynamic_cast<QAction *>(s));

    if (!m_notationWidget) return;
    m_notationWidget->slotSetTextInserter();
    slotUpdateMenuStates();
}

void
NotationView::slotGuitarChord()
{
    QObject *s = sender();
    setCurrentNotePixmapFrom(dynamic_cast<QAction *>(s));

    if (!m_notationWidget) return;
    m_notationWidget->slotSetGuitarChordInserter();
    slotUpdateMenuStates();
}

void
NotationView::slotTransformsQuantize()
{
    EventSelection *selection = getSelection();
    if (!selection) return;

    QuantizeDialog dialog(this, true);

    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand
             (new EventQuantizeCommand
              (*selection,
               dialog.getQuantizer()));
    }
}

void
NotationView::slotTransformsInterpret()
{
    EventSelection *selection = getSelection();
    if (!selection) return;

    InterpretDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand
            (new InterpretCommand
             (*selection,
              getDocument()->getComposition().getNotationQuantizer(),
              dialog.getInterpretations()));
    }
}

void
NotationView::slotMakeOrnament()
{
    if (!getSelection())
        return ;

    EventSelection::eventcontainer &ec =
        getSelection()->getSegmentEvents();

    int basePitch = -1;
    int baseVelocity = -1;

    NoteStyle *style = NoteStyleFactory::getStyle(NoteStyleFactory::DefaultStyle);

    for (EventSelection::eventcontainer::iterator i =
             ec.begin(); i != ec.end(); ++i) {
        if ((*i)->isa(Note::EventType)) {
            if ((*i)->has(BaseProperties::PITCH)) {
                basePitch = (*i)->get<Int>(BaseProperties::PITCH);
                style = NoteStyleFactory::getStyleForEvent(*i);
                if (baseVelocity != -1) break;
            }
            if ((*i)->has(BaseProperties::VELOCITY)) {
                baseVelocity = (*i)->get<Int>(BaseProperties::VELOCITY);
                if (basePitch != -1) break;
            }
        }
    }

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    timeT absTime = getSelection()->getStartTime();

    Track *track =
        segment->getComposition()->getTrackById(segment->getTrack());
    QString name;
    int barNo = segment->getComposition()->getBarNumber(absTime);
    if (track) {
        name = QString(tr("Ornament track %1 bar %2").arg(track->getPosition() + 1).arg(barNo + 1));
    } else {
        name = QString(tr("Ornament bar %1").arg(barNo + 1));
    }

    MakeOrnamentDialog dialog(this, name, basePitch);
    if (dialog.exec() != QDialog::Accepted)
        return ;

    name = dialog.getName();
    basePitch = dialog.getBasePitch();

    CommandHistory::getInstance()->
        addCommand(new CutToTriggerSegmentCommand
                   (getSelection(), getDocument()->getComposition(),
                    name, basePitch, baseVelocity,
                    style->getName(), true,
                    BaseProperties::TRIGGER_SEGMENT_ADJUST_NONE,
                    Marks::NoMark));
}

void
NotationView::slotUseOrnament()
{
    // Take an existing note and match an ornament to it.

    if (!getSelection())
        return ;

    UseOrnamentDialog dialog(this, &getDocument()->getComposition());
    if (dialog.exec() != QDialog::Accepted)
        return ;

    CommandHistory::getInstance()->addCommand(
            new SetTriggerCommand(*getSelection(),
                                  dialog.getId(),
                                  true,
                                  dialog.getRetune(),
                                  dialog.getTimeAdjust(),
                                  dialog.getMark(),
                                  tr("Use Ornament")));
}

void
NotationView::slotRemoveOrnament()
{
    if (!getSelection())
        return ;

    CommandHistory::getInstance()->addCommand(
            new ClearTriggersCommand(*getSelection(),
                                     tr("Remove Ornaments")));
}

void
NotationView::slotEditOrnamentInline()
{
    ForAllSelection(&NotationView::EditOrnamentInline);
}

void
NotationView::slotShowOrnamentExpansion()
{
    ForAllSelection(&NotationView::ShowOrnamentExpansion);
}

void
NotationView::EditOrnamentInline(Event *trigger, Segment *containing)
{
    TriggerSegmentRec *rec =
        getDocument()->getComposition().getTriggerSegmentRec(trigger);
    
    if (!rec) { return; }
    Segment *link = rec->makeLinkedSegment(trigger, containing);

    // makeLinkedSegment can return NULL, eg if ornament was squashed.
    if (!link) { return; }

    link->setParticipation(Segment::editableClone);
    // The same track the host segment had
    link->setTrack(containing->getTrack());
    // Give it a composition so it doesn't get into trouble.
    link->setComposition(&getDocument()->getComposition());

    // Adopt it into the view.
    CommandHistory::getInstance()->addCommand
        (new AdoptSegmentCommand
         (tr("Edit ornament inline"), *this, link, true));
}


void
NotationView::ShowOrnamentExpansion(Event *trigger, Segment *containing)
{
    TriggerSegmentRec *rec =
        getDocument()->getComposition().getTriggerSegmentRec(trigger);
    if (!rec) { return; }
    Instrument *instrument = getDocument()->getInstrument(containing);

    Segment *s =
        rec->makeExpansion(trigger, containing, instrument);

    if (!s) { return; }

    s->setParticipation(Segment::readOnly);
    s->setGreyOut();
    // The same track the host segment had
    s->setTrack(containing->getTrack());
    s->setComposition(&getDocument()->getComposition());
    s->normalizeRests(s->getStartTime(), s->getEndTime());

    // Adopt it into the view.
    CommandHistory::getInstance()->addCommand
        (new AdoptSegmentCommand
         (tr("Show ornament expansion"), *this, s, true));
}

void
NotationView::
ForAllSelection(opOnEvent op)
{
    EventSelection *selection = getSelection();
    if (!selection) { return; }

    EventSelection::eventcontainer ec =
        selection->getSegmentEvents();

    for (EventSelection::eventcontainer::iterator i = ec.begin();
         i != ec.end();
         ++i) {
        CALL_MEMBER_FN(*this,op)(*i, getCurrentSegment());
    }
}

void
NotationView::
slotUnadoptSegment()
{
    // unadoptSegment checks this too, but we check now so that (a) we
    // don't have a did-nothing command on the history, and (b)
    // because undoing that command would be very wrong.
    SegmentVector::iterator found = findAdopted(getCurrentSegment());

    if (found == m_adoptedSegments.end()) { return; }    

    CommandHistory::getInstance()->addCommand
        (new AdoptSegmentCommand
         (tr("Unadopt Segment"), *this, *found, false));
}

void
NotationView::slotMaskOrnament()
{
    if (!getSelection())
        { return; }

    CommandHistory::getInstance()->addCommand
        (new MaskTriggerCommand(*getSelection(), false));
}

void
NotationView::slotUnmaskOrnament()
{
    if (!getSelection())
        { return; }

    CommandHistory::getInstance()->addCommand
        (new MaskTriggerCommand(*getSelection(), true));
}

void
NotationView::slotEditAddClef()
{
    Segment *segment = getCurrentSegment();
    timeT insertionTime = getInsertionTime();
    static Clef lastClef = segment->getClefAtTime(insertionTime);

    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;

    // Fix bug #2997311 : don't use a NotePixmapFactory in selection mode
    // to draw inside the dialog
    NotePixmapFactory npf = *scene->getNotePixmapFactory();
    npf.setSelected(false);

    ClefDialog dialog(this, &npf, lastClef);

    if (dialog.exec() == QDialog::Accepted) {

        ClefDialog::ConversionType conversion = dialog.getConversionType();

        bool shouldChangeOctave = (conversion != ClefDialog::NoConversion);
        bool shouldTranspose = (conversion == ClefDialog::Transpose);

        CommandHistory::getInstance()->addCommand(
                new ClefInsertionCommand(*segment,
                                         insertionTime,
                                         dialog.getClef(),
                                         shouldChangeOctave,
                                         shouldTranspose));

        lastClef = dialog.getClef();
    } 
}

void
NotationView::slotEditAddClefLinkOnly()
{
    Segment *segment = getCurrentSegment();
    if (!segment->isLinked()) {
        return;
    }
    timeT insertionTime = getInsertionTime();
    static Clef lastClef = segment->getClefAtTime(insertionTime);

    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;

    NotePixmapFactory npf = *scene->getNotePixmapFactory();
    npf.setSelected(false);

    ClefDialog dialog(this, &npf, lastClef);

    if (dialog.exec() == QDialog::Accepted) {

        ClefDialog::ConversionType conversion = dialog.getConversionType();

        bool shouldChangeOctave = (conversion != ClefDialog::NoConversion);
        bool shouldTranspose = (conversion == ClefDialog::Transpose);

        CommandHistory::getInstance()->addCommand(
                new ClefLinkInsertionCommand(*segment,
                                            insertionTime,
                                            dialog.getClef(),
                                            shouldChangeOctave,
                                            shouldTranspose));

        lastClef = dialog.getClef();
    } 
}

void
NotationView::slotEditAddKeySignature()
{
    Segment *segment = getCurrentSegment();
    timeT insertionTime = getInsertionTime();
    Clef clef = segment->getClefAtTime(insertionTime);
    Key key = AnalysisHelper::guessKeyForSegment(insertionTime, segment);

    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;

    // Fix bug #2997311 : don't use a NotePixmapFactory in selection mode
    // to draw inside the dialog
    NotePixmapFactory npf = *scene->getNotePixmapFactory();
    npf.setSelected(false);

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
NotationView::slotEditAddSustain(bool down)
{
    Segment *segment = getCurrentSegment();
    timeT insertionTime = getInsertionTime();

    Studio *studio = &getDocument()->getStudio();
    Track *track = segment->getComposition()->getTrackById(segment->getTrack());

    if (track) {

        Instrument *instrument = studio->getInstrumentById
            (track->getInstrument());
        if (instrument) {
            MidiDevice *device = dynamic_cast<MidiDevice *>
                (instrument->getDevice());
            if (device) {
                for (ControlList::const_iterator i =
                         device->getControlParameters().begin();
                     i != device->getControlParameters().end(); ++i) {

                    if (i->getType() == Controller::EventType &&
                        (i->getName() == "Sustain" ||
                         strtoqstr(i->getName()) == tr("Sustain"))) {

                        CommandHistory::getInstance()->addCommand(
                                new SustainInsertionCommand(*segment, insertionTime, down,
                                                            i->getControllerValue()));
                        return ;
                    }
                }
            } else if (instrument->getDevice() &&
                       instrument->getDevice()->getType() == Device::SoftSynth) {
                CommandHistory::getInstance()->addCommand(
                        new SustainInsertionCommand(*segment, insertionTime, down, 64));
                return;
            }
        }
    }

    QMessageBox::warning(this, tr("Rosegarden"), tr("There is no sustain controller defined for this device.\nPlease ensure the device is configured correctly in the Manage MIDI Devices dialog in the main window."));
}

void
NotationView::slotEditAddSustainDown()
{
    slotEditAddSustain(true);
}

void
NotationView::slotEditAddSustainUp()
{
    slotEditAddSustain(false);
}

void
NotationView::slotEditTranspose()
{
    IntervalDialog intervalDialog(this, true, true);
    int ok = intervalDialog.exec();
    
    int semitones = intervalDialog.getChromaticDistance();
    int steps = intervalDialog.getDiatonicDistance();

    if (!ok || (semitones == 0 && steps == 0)) return;

    // TODO combine commands into one 
    for (size_t i = 0; i < m_segments.size(); i++)
    {
        CommandHistory::getInstance()->addCommand(new SegmentTransposeCommand(
                *(m_segments[i]), 
                intervalDialog.getChangeKey(), steps, semitones, 
                intervalDialog.getTransposeSegmentBack()));
    }
}

void
NotationView::slotEditSwitchPreset()
{
    PresetHandlerDialog dialog(this, true);
    
    if (dialog.exec() != QDialog::Accepted) return;
    
    if (dialog.getConvertAllSegments()) {
        // get all segments for this track and convert them.
        Composition& comp = getDocument()->getComposition();
        TrackId selectedTrack = getCurrentSegment()->getTrack();

        // satisfy #1885251 the way that seems most reasonble to me at the
        // moment, only changing track parameters when acting on all segments on
        // this track from the notation view 
        //
        //!!! This won't be undoable, and I'm not sure if that's seriously
        // wrong, or just mildly wrong, but I'm betting somebody will tell me
        // about it if this was inappropriate
        Track *track = comp.getTrackById(selectedTrack);
        track->setPresetLabel( qstrtostr(dialog.getName()) );
        track->setClef(dialog.getClef());
        track->setTranspose(dialog.getTranspose());
        track->setLowestPlayable(dialog.getLowRange());
        track->setHighestPlayable(dialog.getHighRange());

        CommandHistory::getInstance()->addCommand(new SegmentSyncCommand(
                            comp.getSegments(), selectedTrack,
                            dialog.getTranspose(), 
                            dialog.getLowRange(), 
                            dialog.getHighRange(),
                            clefIndexToClef(dialog.getClef())));

        comp.notifyTrackChanged(track);

    } else {
        CommandHistory::getInstance()->addCommand(new SegmentSyncCommand(
                            m_segments, 
                            dialog.getTranspose(), 
                            dialog.getLowRange(), 
                            dialog.getHighRange(),
                            clefIndexToClef(dialog.getClef())));
    }

    m_doc->slotDocumentModified();
}

void
NotationView::slotToggleChordsRuler()
{
    bool visible = findAction("show_chords_ruler")->isChecked();

    m_notationWidget->setChordNameRulerVisible(visible);

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);
    settings.setValue("Chords ruler shown", visible);
    settings.endGroup();
}

void
NotationView::slotToggleTempoRuler()
{
    bool visible = findAction("show_tempo_ruler")->isChecked();

    m_notationWidget->setTempoRulerVisible(visible);

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);
    settings.setValue("Tempo ruler shown", visible);
    settings.endGroup();
}

void
NotationView::slotAddTempo()
{
    const timeT insertionTime = getInsertionTime();
    EditTempoController::self()->editTempo(this, insertionTime);
}

void
NotationView::slotAddTimeSignature()
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

// check composition for parallels

void
NotationView::slotCheckForParallels()
{
    qDebug() << "check for parallels...";

    Segment *segment = getCurrentSegment();

    if (!segment) return ;

    Composition *composition = segment->getComposition();

    CheckForParallelsDialog *dialog = 0;

    dialog = new CheckForParallelsDialog(this, m_document, m_notationWidget->getScene(), composition);

    dialog->show();
}

void
NotationView::slotToggleRawNoteRuler()
{
    bool visible = findAction("show_raw_note_ruler")->isChecked();

    m_notationWidget->setRawNoteRulerVisible(visible);

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);
    settings.setValue("Raw note ruler shown", visible);
    settings.endGroup();
}

void
NotationView::slotToggleTracking()
{
    if (m_notationWidget) m_notationWidget->slotTogglePlayTracking();
}

void
NotationView::slotRegenerateScene()
{
    NOTATION_DEBUG << "NotationView::slotRegenerateScene: "
                   << m_notationWidget->getScene()->getSegmentsDeleted()->size()
                   << " segments deleted";

    // The scene is going to be deleted then restored.  To continue
    // processing at best is useless and at the worst may cause a
    // crash.  This call could replace the multiple calls in
    // NotationScene.
    disconnect(CommandHistory::getInstance(), SIGNAL(commandExecuted()),
               m_notationWidget->getScene(), SLOT(slotCommandExecuted()));
    
    // Look for segments to be removed from vector
    std::vector<Segment *> * segmentDeleted =
        m_notationWidget->getScene()->getSegmentsDeleted();

    // If there is no such segment regenerate the notation widget directly
    if (segmentDeleted->size() != 0) {
        
        // else look if there is something to display still
        if (m_notationWidget->getScene()->isSceneEmpty()) {
            // All segments have been removed : don't regenerate anything
            // but close the editor.
            NOTATION_DEBUG << "NotationView::slotSceneDeleted";

            close();
            return;
        }

        // then remove the deleted segments
        for (std::vector<Segment *>::iterator isd = segmentDeleted->begin();
            isd != segmentDeleted->end(); ++isd) {
            for (std::vector<Segment *>::iterator i = m_segments.begin();
                i != m_segments.end(); ++i) {
                if (*isd == *i) {
                    m_segments.erase(i);
                    NOTATION_DEBUG << "NotationView::slotRegenerateScene:"
                                    " Erased segment from vector, have "
                                << m_segments.size() << " segment(s) remaining"
                               ;
                    break;
                }
            }
        }
    }

    // Fix bug #2960243:
    // When a segment is deleted : remove the selection rect 
    NotationTool * tool =  m_notationWidget->getCurrentTool();
    QString toolName;
    if (tool) {
        toolName = tool->getToolName();
        tool->stow();
    }
    
    // remember zoom factors
    double hZoomFactor = m_notationWidget->getHorizontalZoomFactor();
    double vZoomFactor = m_notationWidget->getVerticalZoomFactor();
    
    // TODO: remember scene position
    
    // regenerate the whole notation widget .
    setWidgetSegments();
    
    // restore size and spacing of notation police
    m_notationWidget->slotSetFontName(m_fontName);
    m_notationWidget->slotSetFontSize(m_fontSize);
    m_notationWidget->getScene()->setHSpacing(m_spacing);

    // restore zoom factors
    m_notationWidget->setVerticalZoomFactor(vZoomFactor);
    m_notationWidget->setHorizontalZoomFactor(hZoomFactor);
    
    // TODO: restore scene position
    
    // and restore the current tool if any
    if (tool) m_notationWidget->slotSetTool(toolName);
}

void
NotationView::slotUpdateWindowTitle(bool m)
{
    QString indicator = (m ? "*" : "");

    if (m_segments.empty()) return;

    // Scene may be empty and the editor is about to be closed,
    // but this info doesn't propagate to view still.
    // (Because signals used to trig slotUpdateWindowTitle() _are not queued_
    //  but signal used to trig slotRegenerateScene() _is queued_).
    // In such a case, don't do anything (to avoid a crash).
    if (m_notationWidget->getScene()->isSceneEmpty()) return;

    if (m_segments.size() == 1) {

        TrackId trackId = m_segments[0]->getTrack();
        Track *track =
            m_segments[0]->getComposition()->getTrackById(trackId);

        int trackPosition = -1;
        if (track)
            trackPosition = track->getPosition();
        //    RG_DEBUG << std::endl << std::endl << "DEBUG TITLE BAR: " << getDocument()->getTitle() << std::endl << std::endl << std::endl;
        setWindowTitle(tr("%1%2 - Segment Track #%3 - Notation")
                      .arg(indicator)
                      .arg(getDocument()->getTitle())
                      .arg(trackPosition + 1));

    } else if (m_segments.size() == getDocument()->getComposition().getNbSegments()) {

        setWindowTitle(tr("%1%2 - All Segments - Notation")
                      .arg(indicator)
                      .arg(getDocument()->getTitle()));

    } else {

        setWindowTitle(tr("%1%2 - %n Segment(s) - Notation", "", m_segments.size())
                    .arg(indicator)
                    .arg(getDocument()->getTitle()));

    }
}

void 
NotationView::slotGroupSimpleTuplet()
{
    slotGroupTuplet(true);
}

void 
NotationView::slotGroupGeneralTuplet()
{
    slotGroupTuplet(false);
}

void 
NotationView::slotGroupTuplet(bool simple)
{
    timeT t = 0;
    timeT unit = 0;
    int tupled = 2;
    int untupled = 3;
    Segment *segment = 0;
    bool hasTimingAlready = false;
    EventSelection *selection = getSelection();

    if (selection) {
        t = selection->getStartTime();

        timeT duration = selection->getTotalDuration();
        Note::Type unitType = Note::getNearestNote(duration / 3, 0)
                                                   .getNoteType();
        unit = Note(unitType).getDuration();

        if (!simple) {
            TupletDialog dialog(this, unitType, duration);
            if (dialog.exec() != QDialog::Accepted)
                return ;
            unit = Note(dialog.getUnitType()).getDuration();
            tupled = dialog.getTupledCount();
            untupled = dialog.getUntupledCount();
            hasTimingAlready = dialog.hasTimingAlready();
        }

        segment = &selection->getSegment();

    } else {

        t = getInsertionTime();

        NoteRestInserter *currentInserter = dynamic_cast<NoteRestInserter *> (m_notationWidget->getCurrentTool());

        Note::Type unitType;

// Should fix this too (maybe go fetch the NoteRestTool and check its duration).
        if (currentInserter) {
            unitType = currentInserter->getCurrentNote().getNoteType();
        } else {
            unitType = Note::Quaver;
        }

        unit = Note(unitType).getDuration();

        if (!simple) {
            TupletDialog dialog(this, unitType);
            if (dialog.exec() != QDialog::Accepted)
                return ;
            unit = Note(dialog.getUnitType()).getDuration();
            tupled = dialog.getTupledCount();
            untupled = dialog.getUntupledCount();
            hasTimingAlready = dialog.hasTimingAlready();
        }

        segment = getCurrentSegment();
    }

    CommandHistory::getInstance()->addCommand(new TupletCommand
                                              (*segment, t, unit, untupled, 
                                              tupled, hasTimingAlready));

    if (!hasTimingAlready) {
//        slotSetInsertCursorPosition(t + (unit * tupled), true, false);
        m_document->slotSetPointerPosition(t + (unit * tupled));
    }
}

void
NotationView::slotUpdateInsertModeStatusTriplet()
{
    if (isInTripletMode()) {
      m_notationWidget->setTupletMode(true);
      m_notationWidget->setTupledCount();
      m_notationWidget->setUntupledCount();
      (findAction("tuplet_mode"))->setChecked(false);
    } else m_notationWidget->setTupletMode(false);
    slotUpdateInsertModeStatus();
}

void
NotationView::slotUpdateInsertModeStatusTuplet()
{
    if (isInTupletMode()) {
      m_notationWidget->setTupletMode(true);
      InsertTupletDialog dialog(this, m_notationWidget->getUntupledCount(),  m_notationWidget->getTupledCount());
      if (dialog.exec() == QDialog::Accepted) {
        m_notationWidget->setTupledCount(dialog.getTupledCount());
        m_notationWidget->setUntupledCount(dialog.getUntupledCount());
      }
      (findAction("triplet_mode"))->setChecked(false);
    } else m_notationWidget->setTupletMode(false);
    slotUpdateInsertModeStatus();
}

void
NotationView::slotUpdateInsertModeStatus()
{
    QString tripletMessage = tr("Tuplet");
    QString chordMessage = tr("Chord");
    QString graceMessage = tr("Grace");
    QString message;

    m_notationWidget->setChordMode(isInChordMode());
    m_notationWidget->setGraceMode(isInGraceMode());

    if (isInTripletMode()||isInTupletMode()) {
        message = tr("%1 %2").arg(message).arg(tripletMessage);
    }

    if (isInChordMode()) {
        message = tr("%1 %2").arg(message).arg(chordMessage);
    }

    if (isInGraceMode()) {
        message = tr("%1 %2").arg(message).arg(graceMessage);
    }

    m_insertModeLabel->setText(message);
}

bool
NotationView::isInChordMode()
{
    QAction* tac = findAction("chord_mode");
    return tac->isChecked();
}

bool
NotationView::isInTripletMode()
{
    QAction* tac = findAction("triplet_mode");
    return tac->isChecked();
}

bool
NotationView::isInTupletMode()
{
    QAction* tac = findAction("tuplet_mode");
    return tac->isChecked();
}

bool
NotationView::isInGraceMode()
{
    QAction* tac = findAction("grace_mode");
    return tac->isChecked();
}

void
NotationView::slotSymbolAction()
{
    QObject *s = sender();
    setCurrentNotePixmapFrom(dynamic_cast<QAction *>(s));
    QString n = s->objectName();

    Symbol type = Symbol::Segno;

    if (n == "add_segno") type = Symbol::Segno;
    else if (n == "add_coda") type = Symbol::Coda;
    else if (n == "add_breath") type = Symbol::Breath;

    if (!m_notationWidget) return;
    m_notationWidget->slotSetSymbolInserter();
    m_notationWidget->slotSetInsertedSymbol(type);
    slotUpdateMenuStates();
}

void
NotationView::slotHalveDurations()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new RescaleCommand(*getSelection(),
                                              getSelection()->getTotalDuration() / 2,
                                              false));
}

void
NotationView::slotDoubleDurations()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new RescaleCommand(*getSelection(),
                                              getSelection()->getTotalDuration() * 2,
                                              false));
}

void
NotationView::slotRescale()
{
    if (!getSelection()) return ;

    RescaleDialog dialog
    (this,
     &getDocument()->getComposition(),
     getSelection()->getStartTime(),
     getSelection()->getEndTime() -
         getSelection()->getStartTime(),
     1,
     true,
     true);

    if (dialog.exec() == QDialog::Accepted) {
        CommandHistory::getInstance()->addCommand(new RescaleCommand
                                                  (*getSelection(),
                                                  dialog.getNewDuration(),
                                                  dialog.shouldCloseGap()));
    }
}

void
NotationView::slotTransposeUp()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new TransposeCommand
                                              (1, *getSelection()));
}

void
NotationView::slotTransposeDown()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new TransposeCommand
                                              ( -1, *getSelection()));
}

void
NotationView::slotTransposeUpOctave()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new TransposeCommand
                                              (12, *getSelection()));
}

void
NotationView::slotTransposeDownOctave()
{
    if (!getSelection()) return ;

    CommandHistory::getInstance()->addCommand(new TransposeCommand
                                              ( -12, *getSelection()));
}

void
NotationView::slotTranspose()
{
    EventSelection *selection = getSelection();
    if (!selection) {
        RG_WARNING << "Hint: selection is NULL in slotTranpose()";
        return;
    }

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    int dialogDefault = settings.value("lasttransposition", 0).toInt() ;

    bool ok = false;
    int semitones = QInputDialog::getInt
            (this, tr("Transpose"),
             tr("By number of semitones: "),
                dialogDefault, -127, 127, 1, &ok);
    if (!ok || semitones == 0) return;

    settings.setValue("lasttransposition", semitones);

    CommandHistory::getInstance()->addCommand(new TransposeCommand
            (semitones, *selection));

    settings.endGroup();
}

void
NotationView::slotDiatonicTranspose()
{
    if (!getSelection()) return ;

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

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
                //RG_DEBUG << "Transposing semitones, steps: " << semitones << ", " << steps;
        CommandHistory::getInstance()->addCommand(new TransposeCommand
                                                  (semitones, steps,
                                                  *getSelection()));
    }
}

void
NotationView::slotInvert()
{
    if (!getSelection()) return;

    int semitones = 0;

    CommandHistory::getInstance()->addCommand(new InvertCommand
                                              (semitones, *getSelection()));
}

void
NotationView::slotRetrograde()
{
    if (!getSelection()) return;

    int semitones = 0;

    CommandHistory::getInstance()->addCommand(new RetrogradeCommand
                                              (semitones, *getSelection()));
}

void
NotationView::slotRetrogradeInvert()
{
    if (!getSelection()) return;

    int semitones = 0;

    CommandHistory::getInstance()->addCommand(new RetrogradeInvertCommand
                                              (semitones, *getSelection()));
}

void
NotationView::slotJogLeft()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;

    RG_DEBUG << "NotationView::slotJogLeft";

    bool useNotationTimings = true;

    CommandHistory::getInstance()->addCommand(new MoveCommand
                                              (*getCurrentSegment(),
                                              -Note(Note::Demisemiquaver).getDuration(),
                                              useNotationTimings,
                                              *selection));
}

void
NotationView::slotJogRight()
{
    EventSelection *selection = getSelection();
    if (!selection) return ;

    RG_DEBUG << "NotationView::slotJogRight"<< endl;

    bool useNotationTimings = true;

    CommandHistory::getInstance()->addCommand(new MoveCommand
                                              (*getCurrentSegment(),
                                              Note(Note::Demisemiquaver).getDuration(),
                                              useNotationTimings,
                                              *selection));
}

bool
NotationView::
isShowable(Event *e)
{
    if (e->isa(PitchBend::EventType)) { return false; }
    if (e->isa(Controller::EventType)) { return false; }
    return true;
}

void
NotationView::slotStepBackward()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;

    timeT time = getInsertionTime();
    Segment::iterator i = segment->findTime(time);

    while (i != segment->begin() &&
           (i == segment->end() ||
            (*i)->getNotationAbsoluteTime() >= time ||
            !isShowable(*i)))
        { --i; }

    if (i != segment->end()){
        m_document->slotSetPointerPosition((*i)->getNotationAbsoluteTime());
    }
}

void
NotationView::slotStepForward()
{
    Segment *segment = getCurrentSegment();
    if (!segment) return;

    timeT time = getInsertionTime();
    Segment::iterator i = segment->findTime(time);

    while (i != segment->end() &&
           ((*i)->getNotationAbsoluteTime() <= time ||
            !isShowable(*i)))
        { ++i; }

    if (i == segment->end()){
        m_document->slotSetPointerPosition(segment->getEndMarkerTime());
    } else {
        m_document->slotSetPointerPosition((*i)->getNotationAbsoluteTime());
    }
}

void
NotationView::slotInsertableNoteOnReceived(int pitch, int velocity)
{
    NOTATION_DEBUG << "NotationView::slotInsertableNoteOnReceived: " << pitch;
    slotInsertableNoteEventReceived(pitch, velocity, true);
}

void
NotationView::slotInsertableNoteOffReceived(int pitch, int velocity)
{
    NOTATION_DEBUG << "NotationView::slotInsertableNoteOffReceived: " << pitch;
    slotInsertableNoteEventReceived(pitch, velocity, false);
}

void
//!!! shut up compiler warning about unused 'velocity' but left original intact
// because it would be a good thing to make use of velocity one day
//NotationView::slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn)
NotationView::slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn)
{

    // find step recording action in menu (it ought to exist!)
    QAction *action = findAction("toggle_step_by_step");
    if (!action) {
        NOTATION_DEBUG << "WARNING: No toggle_step_by_step action";
        return ;
    }

    // return if we're not in step recording (step by step) mode
    if (!action->isChecked()) return ;

    // return if this window is not active, to avoid a window that is out of
    // sight and out of mind filling rapidly with unexpected gibberish which has
    // to be undone one event at a time
    //
    // NOTE: This prevents using step recording mode with an external app like
    // VMPK.  I can't think of any other way to avoid the far greater problem of
    // having to undo 15,000 unexpected events you didn't intend to record in an
    // out of focus view that got left in step record mode.
    if (!isActiveWindow()) return;

    Segment *segment = getCurrentSegment();

    NoteRestInserter *noteInserter = dynamic_cast<NoteRestInserter *>
                                     (m_notationWidget->getCurrentTool());
    if (!noteInserter) {
        // The old behavior was totally evil, so let's try failing silently and
        // see how obnoxious it is for the user to discover the hard way that
        // nothing is happening, because the wrong tool is selected.
        return;
    }

//    if (m_inPaintEvent) {
//        NOTATION_DEBUG << "NotationView::slotInsertableNoteEventReceived: in paint event already";
//        if (noteOn) {
//            m_pendingInsertableNotes.push_back(std::pair<int, int>(pitch, velocity));
//        }
//        return ;
//    }

    // If the segment is transposed, we want to take that into
    // account.  But the note has already been played back to the user
    // at its untransposed pitch, because that's done by the MIDI THRU
    // code in the sequencer which has no way to know whether a note
    // was intended for step recording.  So rather than adjust the
    // pitch for playback according to the transpose setting, we have
    // to adjust the stored pitch in the opposite direction.

    pitch -= segment->getTranspose();

    //    TmpStatusMsg msg(tr("Inserting note"), this);

    // We need to ensure that multiple notes hit at once come out as
    // chords, without imposing the interpretation that overlapping
    // notes are always chords and without getting too involved with
    // the actual absolute times of the notes (this is still step
    // editing, not proper recording).

    // First, if we're in chord mode, there's no problem.

    static int numberOfNotesOn = 0;
    static timeT insertionTime = getInsertionTime();
    static time_t lastInsertionTime = 0;

    if (isInChordMode()) {
        if (!noteOn)
            return ;
        NOTATION_DEBUG << "Inserting note in chord at pitch " << pitch;
        noteInserter->insertNote(*segment, getInsertionTime(), pitch,
                                 Accidentals::NoAccidental, velocity,
                                 true);

    } else {

        if (!noteOn) {
            numberOfNotesOn--;
        } else if (noteOn) {
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

            noteInserter->insertNote(*segment, insertionTime, pitch,
                                     Accidentals::NoAccidental, velocity,
                                     true);
        }
    }
}

void
NotationView::slotEditLyrics()
{
    Segment *segment = getCurrentSegment();
    int oldVerseCount = segment->getVerseCount();

    LyricEditDialog dialog(this, m_segments, segment);

    if (dialog.exec() == QDialog::Accepted) {

        // User may have change segment from inside the dialog
        // (see ticket #1547)
        segment = dialog.getSegment();

        MacroCommand *macro = new MacroCommand
            (SetLyricsCommand::getGlobalName());

        for (int i = 0; i < dialog.getVerseCount(); ++i) {
            SetLyricsCommand *command = new SetLyricsCommand
                (segment, i, dialog.getLyricData(i));
            macro->addCommand(command);
        }
        for (int i = dialog.getVerseCount(); i < oldVerseCount; ++i) {
            // (hjj) verse count decreased, delete extra verses.
            SetLyricsCommand *command = new SetLyricsCommand
                (segment, i, QString(""));
            macro->addCommand(command);
        }

        CommandHistory::getInstance()->addCommand(macro);
    }
}


void
NotationView::slotHoveredOverNoteChanged(const QString &noteName)
{
    m_hoveredOverNoteName->setText(QString(" ") + noteName);
}

void
NotationView::slotHoveredOverAbsoluteTimeChanged(unsigned int time)
{
    timeT t = time;
    RealTime rt =
        getDocument()->getComposition().getElapsedRealTime(t);
    long ms = rt.msec();

    int bar, beat, fraction, remainder;
    getDocument()->getComposition().getMusicalTimeForAbsoluteTime
        (t, bar, beat, fraction, remainder);

    //    QString message;
    //    QString format("%ld (%ld.%03lds)");
    //    format = tr("Time: %1").arg(format);
    //    message.sprintf(format, t, rt.sec, ms);

    QString message = tr("Time: %1 (%2.%3s)")
         .arg(QString("%1-%2-%3-%4")
             .arg(QString("%1").arg(bar + 1).rightJustified(3, '0'))
             .arg(QString("%1").arg(beat).rightJustified(2, '0'))
             .arg(QString("%1").arg(fraction).rightJustified(2, '0'))
             .arg(QString("%1").arg(remainder).rightJustified(2, '0')))
         .arg(rt.sec)
         .arg(QString("%1").arg(ms).rightJustified(3, '0'));

    m_hoveredOverAbsoluteTime->setText(message);
}

void
NotationView::slotFontComboChanged(int index)
{
    QString name = m_availableFontNames[index];
    if (m_notationWidget) m_notationWidget->slotSetFontName(name);
    m_fontName = name;
    QString action = QString("note_font_%1").arg(name);
    findAction(action)->setChecked(true);
}

void
NotationView::slotSizeComboChanged(int index)
{
    int size = m_availableFontSizes[index];
    if (m_notationWidget) m_notationWidget->slotSetFontSize(size);
    m_fontSize = size;
    QString action = QString("note_font_size_%1").arg(size);
    findAction(action)->setChecked(true);
}

void
NotationView::slotSpacingComboChanged(int index)
{
    int spacing = m_availableSpacings[index];
    if (m_notationWidget) m_notationWidget->getScene()->setHSpacing(spacing);
    m_spacing = spacing;
    QString action = QString("spacing_%1").arg(spacing);
    findAction(action)->setChecked(true);
}

void
NotationView::slotToggleVelocityRuler()
{
    m_notationWidget->slotToggleVelocityRuler();
    conformRulerSelectionState();
}

void
NotationView::slotTogglePitchbendRuler()
{
    m_notationWidget->slotTogglePitchbendRuler();
    conformRulerSelectionState();
}

void
NotationView::slotAddControlRuler(QAction *action)
{
    NOTATION_DEBUG << "NotationView::slotAddControlRuler(" << action << ")" << endl;
    m_notationWidget->slotAddControlRuler(action);
    conformRulerSelectionState();
}

Device *
NotationView::getCurrentDevice()
{
    if (m_notationWidget) return m_notationWidget->getCurrentDevice();
    else return 0;
}

void
NotationView::slotHelp()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:manual-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:notation-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

void
NotationView::slotTutorial()
{
    QString tutorialURL = tr("http://www.rosegardenmusic.com/tutorials/en/chapter-0.html");
    QDesktopServices::openUrl(QUrl(tutorialURL));
}

void
NotationView::slotBugGuidelines()
{
    QString glURL = tr("http://rosegarden.sourceforge.net/tutorial/bug-guidelines.html");
     QDesktopServices::openUrl(QUrl(glURL));
}

void
NotationView::slotHelpAbout()
{
    new AboutDialog(this);
}

void
NotationView::slotHelpAboutQt()
{
    QMessageBox::aboutQt(this, tr("Rosegarden"));
}

void
NotationView::slotDonate()
{
    QString url("https://sourceforge.net/project/project_donations.php?group_id=4932");
    QDesktopServices::openUrl(QUrl(url));
}

void
NotationView::slotToggleStepByStep()
{
    QAction *action = findAction("toggle_step_by_step");

    if (!action) {
        NOTATION_DEBUG << "WARNING: No toggle_step_by_step action";
        return ;
    }
    if (action->isChecked()) {
        emit stepByStepTargetRequested(this);
    } else {
        emit stepByStepTargetRequested(0);
    }
}

void
NotationView::slotStepByStepTargetRequested(QObject *obj)
{
    QAction *action = findAction("toggle_step_by_step");

    if (!action) {
        MATRIX_DEBUG << "WARNING: No toggle_step_by_step action";
        return ;
    }
    action->setChecked(obj == this);
}

void
NotationView::slotMoveEventsUpStaffInteractive(void)
{ generalMoveEventsToStaff(true, true); }

void
NotationView::slotMoveEventsDownStaffInteractive(void)
{ generalMoveEventsToStaff(false, true); }

void
NotationView::slotMoveEventsUpStaff(void)
{ generalMoveEventsToStaff(true, false); }

void
NotationView::slotMoveEventsDownStaff(void)
{ generalMoveEventsToStaff(false, false); }

// Move the selected events to another staff
// @param upStaff
// if true, move them to the staff above this one, otherwise to the
// staff below.
// @param useDialog
// Whether to use a dialog, otherwise use default values and no
// interaction.
void
NotationView::generalMoveEventsToStaff(bool upStaff, bool useDialog)
{
    EventSelection *selection = getSelection();
    if (!selection) return;

    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;
    timeT targetTime = selection->getStartTime();

    PasteEventsCommand::PasteType type;

    if (useDialog) {
        PasteNotationDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted) { return; } 
        type = dialog.getPasteType();
    } else {
        type = PasteEventsCommand::NoteOverlay;
    }

    NotationStaff *target_staff =
        upStaff ?
        scene->getStaffAbove(targetTime) :
        scene->getStaffBelow(targetTime);
    QString commandName = 
        upStaff ?
        tr("Move Events to Staff Above") :
        tr("Move Events to Staff Below");
    
    if (!target_staff) return;

    Segment *segment = &target_staff->getSegment();

    MacroCommand *command = new MacroCommand(commandName);

    timeT insertionTime = selection->getStartTime();

    Clipboard *c = new Clipboard;
    CopyCommand *cc = new CopyCommand(*selection, c);
    cc->execute();

    command->addCommand(new EraseCommand(*selection));

    command->addCommand(new PasteEventsCommand
                        (*segment, c, insertionTime,
                         type));
    
    CommandHistory::getInstance()->addCommand(command);

    delete c;
}

void
NotationView::slotEditElement(NotationStaff *staff,
                              NotationElement *element,
                              bool advanced)
{
    NOTATION_DEBUG << "NotationView::slotEditElement()";

    NotationScene *scene = m_notationWidget->getScene();
    if (!scene) return;

    NotePixmapFactory *npf = scene->getNotePixmapFactory();

    if (advanced) {

        EventEditDialog dialog(this, *element->event(), true);

        if (dialog.exec() == QDialog::Accepted &&
            dialog.isModified()) {

            EventEditCommand *command = new EventEditCommand
                (staff->getSegment(),
                 element->event(),
                 dialog.getEvent());

            CommandHistory::getInstance()->addCommand(command);
        }

    } else if (element->event()->isa(Clef::EventType)) {

        try {
            ClefDialog dialog(this, npf,
                              Clef(*element->event()));

            if (dialog.exec() == QDialog::Accepted) {

                ClefDialog::ConversionType conversion = dialog.getConversionType();
                bool shouldChangeOctave = (conversion != ClefDialog::NoConversion);
                bool shouldTranspose = (conversion == ClefDialog::Transpose);
                CommandHistory::getInstance()->addCommand
                    (new ClefInsertionCommand
                     (staff->getSegment(), element->event()->getAbsoluteTime(),
                      dialog.getClef(), shouldChangeOctave, shouldTranspose));
            }
        } catch (Exception e) {
            RG_WARNING << e.getMessage();
        }

        return ;

    } else if (element->event()->isa(GeneratedRegion::EventType)) {

        try {
            GeneratedRegionDialog dialog(this, npf,
                                         GeneratedRegion(*element->event()),
                                         tr("Edit Generated region mark"));

            if (dialog.exec() == QDialog::Accepted) {
                GeneratedRegionInsertionCommand * command =
                    new GeneratedRegionInsertionCommand
                    (staff->getSegment(),
                     element->event()->getAbsoluteTime(),
                     dialog.getGeneratedRegion());
                
                MacroCommand *macroCommand = dialog.extractCommand();

                macroCommand->addCommand(new
                                         EraseEventCommand(staff->getSegment(),
                                                           element->event(),
                                                           false));
                macroCommand->addCommand(command);
                CommandHistory::getInstance()->addCommand(macroCommand);
                 
            } else {
                /* Still execute the command but without erase+insert,
                   because it may still contain legitimate commands
                   (eg to update tags). */
                MacroCommand *macroCommand = dialog.extractCommand();
                if (macroCommand->haveCommands()) {
                    macroCommand->setName(tr("Updated tags for aborted edit"));
                    CommandHistory::getInstance()->addCommand(macroCommand);
                }
            }
                
        } catch (Exception e) {
            RG_WARNING << e.getMessage();
        }

        return ;

    } else if (element->event()->isa(Rosegarden::Key::EventType)) {

        try {
            Clef clef(staff->getSegment().getClefAtTime
                      (element->event()->getAbsoluteTime()));
            KeySignatureDialog dialog
                (this, npf, clef, Rosegarden::Key(*element->event()),
                 false, true);

            if (dialog.exec() == QDialog::Accepted &&
                dialog.isValid()) {

                KeySignatureDialog::ConversionType conversion =
                    dialog.getConversionType();

                CommandHistory::getInstance()->addCommand
                    (new KeyInsertionCommand
                     (staff->getSegment(),
                      element->event()->getAbsoluteTime(), dialog.getKey(),
                      conversion == KeySignatureDialog::Convert,
                      conversion == KeySignatureDialog::Transpose,
                      dialog.shouldBeTransposed(),
              dialog.shouldIgnorePercussion()));
            }

        } catch (Exception e) {
            RG_WARNING << e.getMessage();
        }

        return ;

    } else if (element->event()->isa(Text::EventType)) {

        try {
            TextEventDialog dialog
                (this, npf, Text(*element->event()));
            
            if (dialog.exec() == QDialog::Accepted) {
                TextInsertionCommand *command = new TextInsertionCommand
                    (staff->getSegment(),
                     element->event()->getAbsoluteTime(),
                     dialog.getText());
                
                MacroCommand *macroCommand = new MacroCommand(tr("Edit Text Event"));
                
                macroCommand->addCommand(new EraseEventCommand(staff->getSegment(),
                                                               element->event(), false));
                macroCommand->addCommand(command);
                CommandHistory::getInstance()->addCommand(macroCommand);
            }
        } catch (Exception e) {
            RG_WARNING << e.getMessage();
        }

        return ;

    } else if (element->isNote() &&
               element->event()->has(BaseProperties::TRIGGER_SEGMENT_ID)) {

        int id = element->event()->get
            <Int>
            (BaseProperties::TRIGGER_SEGMENT_ID);
        
        emit editTriggerSegment(id); 
        return ;

    } else {

        SimpleEventEditDialog dialog(this, getDocument(), *element->event(), false);

        if (dialog.exec() == QDialog::Accepted &&
            dialog.isModified()) {

            EventEditCommand *command = new EventEditCommand
                (staff->getSegment(),
                 element->event(),
                 dialog.getEvent());

            CommandHistory::getInstance()->addCommand(command);
        }
    }
}

void
NotationView::slotTransformsNormalizeRests()
{
    EventSelection *selection = m_notationWidget->getSelection();

    if (!selection)
        return ;

    TmpStatusMsg msg(tr("Normalizing rests..."), this);

    CommandHistory::getInstance()->
            addCommand(new NormalizeRestsCommand(*selection));
}

void
NotationView::slotTransformsCollapseNotes()
{
    EventSelection *selection = m_notationWidget->getSelection();

    if (!selection)
        return ;
    TmpStatusMsg msg(tr("Collapsing notes..."), this);

    CommandHistory::getInstance()->
            addCommand(new CollapseNotesCommand(*selection));
}

void NotationView::extendSelectionHelper(bool forward, EventSelection *es, const std::vector<Event *> &eventVec, bool select)
{
    int moveCount = 0;
    int prevEventTime = 0;
    int prevSubOrdering = 0;
    for (unsigned int j = 0; j < eventVec.size(); ++j) {
        Event *event = eventVec[j];
        int count;
        if (select) { // select
            count = es->addEvent(event, true, forward);
        } else { // unselect
            count = es->removeEvent(event, true, forward);
        }
        if (prevEventTime != event->getAbsoluteTime() || prevSubOrdering != event->getSubOrdering()) {
            moveCount = qMax(moveCount, count);
        }
        prevEventTime = event->getAbsoluteTime();
        prevSubOrdering = event->getSubOrdering();
    }

    // #1519: increment cursor for every event selected/unselected
    for (int c = 1; c < moveCount; ++c) {
        if (forward)
            slotStepForward();
        else
            slotStepBackward();
    }
}

void
NotationView::slotExtendSelectionBackward()
{
    slotExtendSelectionBackward(false);
}

void
NotationView::slotExtendSelectionBackwardBar()
{
    slotExtendSelectionBackward(true);
}

void
NotationView::slotExtendSelectionBackward(bool bar)
{
    // Move the cursor left and toggle selection between oldTime and newTime

    timeT oldTime = getInsertionTime();

    if (bar) emit rewindPlayback();
    else slotStepBackward();

    timeT newTime = getInsertionTime();

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    NotationStaff *currentStaff = m_notationWidget->getScene()->getCurrentStaff();
    if (!currentStaff) return;

    ViewElementList *vel = currentStaff->getViewElementList();
    EventSelection *s = getSelection();
    EventSelection *es;
    if (s && &s->getSegment() == segment)
        es = new EventSelection(*s);
    else
        es = new EventSelection(*segment);

    ViewElementList::iterator extendFrom = vel->findTime(oldTime);
    if (extendFrom == vel->begin()) // shouldn't happen
        return;
    ViewElementList::iterator firstNote = extendFrom;
    --firstNote;
    const bool wasSelected = es->contains((*firstNote)->event());

    // store events in a separate data structure, to avoid
    // breaking iteration while removing events from the segment.
    std::vector<Event *> eventVec;

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
        Event *event = (*extendFrom)->event();
        if (event->isa(Note::EventType)) {
            eventVec.push_back(event);
        }
    }

    const bool forward = false;
    extendSelectionHelper(forward, es, eventVec, !wasSelected);

    setSelection(es, true);
}

void
NotationView::slotExtendSelectionForward()
{
    slotExtendSelectionForward(false);
}

void
NotationView::slotExtendSelectionForwardBar()
{
    slotExtendSelectionForward(true);
}

void
NotationView::slotExtendSelectionForward(bool bar)
{
    // Move the cursor right and toggle selection between oldTime and newTime

    timeT oldTime = getInsertionTime();

    if (bar) emit fastForwardPlayback();
    else slotStepForward();

    timeT newTime = getInsertionTime();

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    NotationStaff *currentStaff = m_notationWidget->getScene()->getCurrentStaff();
    if (!currentStaff) return;

    ViewElementList *vel = currentStaff->getViewElementList();
    EventSelection *s = getSelection();
    EventSelection *es;
    if (s && &s->getSegment() == segment)
        es = new EventSelection(*s);
    else
        es = new EventSelection(*segment);

    ViewElementList::iterator extendFrom = vel->findTime(oldTime);
    if (extendFrom == vel->end()) // shouldn't happen
        return;
    const bool wasSelected = es->contains((*extendFrom)->event());

    std::vector<Event *> eventVec;

    while (extendFrom != vel->end() &&
           (*extendFrom)->getViewAbsoluteTime() < newTime) {
        Event *event = (*extendFrom)->event();
        // Only grab notes for now, see slotExtendSelectionBackward()
        if (event->isa(Note::EventType)) {
            eventVec.push_back(event);
        }
        ++extendFrom;
    }

    const bool forward = true;
    extendSelectionHelper(forward, es, eventVec, !wasSelected);

    setSelection(es, true);
}


void
NotationView::slotAddDot()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    TmpStatusMsg msg(tr("Adding dot..."), this);
    CommandHistory::getInstance()->addCommand
            (new AddDotCommand(*selection, false));
}

void
NotationView::slotAddDotNotationOnly()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    TmpStatusMsg msg(tr("Adding dot..."), this);
    CommandHistory::getInstance()->addCommand
            (new AddDotCommand(*selection, true));
}


void
NotationView::slotSetNoteType()
{
    QObject *s = sender();
    QString name = s->objectName();
    int note=Note::WholeNote;

    EventSelection *selection = getSelection();
    if (!selection) return;
    TmpStatusMsg msg(tr("Set Note Type..."), this);

    if (name == "set_note_type_doublewhole") note=Note::DoubleWholeNote;
    else if (name == "set_note_type_whole") note=Note::WholeNote;
    else if (name == "set_note_type_half") note=Note::HalfNote;
    else if (name == "set_note_type_quarter") note=Note::QuarterNote;
    else if (name == "set_note_type_eighth") note=Note::EighthNote;
    else if (name == "set_note_type_sixteenth") note=Note::SixteenthNote;
    else if (name == "set_note_type_thirtysecond") note=Note::ThirtySecondNote;
    else if (name == "set_note_type_sixtyfourth") note=Note::SixtyFourthNote;

    CommandHistory::getInstance()->addCommand
            (new SetNoteTypeCommand(*selection, note, false));
}

void
NotationView::slotSetNoteTypeNotationOnly()
{
    QObject *s = sender();
    QString name = s->objectName();
    int note=Note::WholeNote;

    EventSelection *selection = getSelection();
    if (!selection) return;
    TmpStatusMsg msg(tr("Set Note Type notation only..."), this);

    if (name == "set_note_type_notation_doublewhole") note=Note::DoubleWholeNote;
    else if (name == "set_note_type_notation_whole") note=Note::WholeNote;
    else if (name == "set_note_type_notation_half") note=Note::HalfNote;
    else if (name == "set_note_type_notation_quarter") note=Note::QuarterNote;
    else if (name == "set_note_type_notation_eighth") note=Note::EighthNote;
    else if (name == "set_note_type_notation_sixteenth") note=Note::SixteenthNote;
    else if (name == "set_note_type_notation_thirtysecond") note=Note::ThirtySecondNote;
    else if (name == "set_note_type_notation_sixtyfourth") note=Note::SixtyFourthNote;

    CommandHistory::getInstance()->addCommand
            (new SetNoteTypeCommand(*selection, note, true));
}

void
NotationView::slotCycleSlashes()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    TmpStatusMsg msg(tr("Cycling slashes..."), this);
    CommandHistory::getInstance()->addCommand(new CycleSlashesCommand(*selection));
}

void
NotationView::slotAddLayer()
{
    // switch to the pencil, as we are about to create an empty segment for
    // editing
    //
    //!!! This also detours around at least three related but distinct crashes
    // in NotationSelector, although I do not fully fathom why this is so, and am
    // worried about memory leaks or other obnoxious gotchas waiting in the
    // wings.
    slotSetNoteRestInserter();

    AddLayerCommand *command = new AddLayerCommand(getCurrentSegment(), getDocument()->getComposition());
    CommandHistory::getInstance()->addCommand(command);

    // get the pointer to the segment we just created and add it to m_segments
    m_segments.push_back(command->getSegment());

    // re-invoke setSegments with the ammended m_segments
    setWidgetSegments();

    // try to make the new segment active immediately
    slotCurrentSegmentNext();

    // Undoing this goes kaboom bigtime.  What to do about that?  Make the
    // command's undo emit a signal or something?  The notation widget needs to
    // pick up the change and reboot itself again on the smaller set of
    // segments, or at least close gracefully instead of crashing, the way it
    // does when you undo the creation of a segment that was displayed in this
    // view.
    //
    // I suppose it would be most ideal to have some mechanism in place whereby
    // undoing segment creation in general triggered successive calls to
    // setSegments() until there was only one segment left, and then we'd blink
    // out of existence only after undoing that final one in a multi-segment
    // context.
}

void
NotationView::slotMagicLayer()
{
    // grab selection; else abort cleanly
    EventSelection *selection = getSelection();
    if (!selection) return;

    // switch to the pencil, as in slotAddLayer
    slotSetNoteRestInserter();

    MacroCommand *macro = new MacroCommand(tr("New Layer from Selection"));

    // make a new "layer" segment
    AddLayerCommand *command = new AddLayerCommand(getCurrentSegment(), getDocument()->getComposition());
    command->execute();

    // Not sure how to handle this:  We have to execute() here to get the
    // segment created for pasting, but if we do that, the command executes a
    // second time later on, and you end up with two new segments, one of which
    // is empty...  The only way I see to avoid that is to execute here, and
    // skip adding this to the MacroCommand, which means if you undo this
    // operation, it won't be clean.  The only alternative I can think of is to
    // add a series of commands to the stack, so one operation requires multiple
    // undos to reverse.  I don't like that either.  Meh.  
//    macro->addCommand(command);

    // get the new segment we just created; abort if there is no new segment or
    // if it is exactly the same as the current segment, which means new segment
    // creation failed
    Segment *newLayer = command->getSegment();
    if (!newLayer || newLayer == getCurrentSegment()) {
        RG_DEBUG << "NotationView::slotMagicLayer(): newLayer: " 
                 << newLayer
                 << " currentSegment: "
                 << getCurrentSegment()
                 << " aborting!" << endl;
        delete macro;
        return;
    }

    // cut the selected events from the parent segment
    timeT insertionTime = selection->getStartTime();

    Clipboard *c = new Clipboard;
    CopyCommand *cc = new CopyCommand(*selection, c);
    cc->execute();

    macro->addCommand(new EraseCommand(*selection));

    // use overlay paste to avoid checking for space; paste to new "layer" 
    PasteEventsCommand::PasteType type = PasteEventsCommand::NoteOverlay;
    macro->addCommand(new PasteEventsCommand(*newLayer, c, insertionTime, type));
    
    delete c;

    CommandHistory::getInstance()->addCommand(macro);

    // normalize rests to clean up the weird double whole rest problem
    newLayer->normalizeRests(newLayer->getStartTime(), newLayer->getEndTime());

    // get the pointer to the segment we just created and add it to m_segments
    m_segments.push_back(newLayer);

    // re-invoke setSegments with the ammended m_segments
    setWidgetSegments();

    // try to make the new segment active immediately
    slotCurrentSegmentNext();
}

void
NotationView::slotConfigure()
{
    ConfigureDialog *configDlg =  new ConfigureDialog(getDocument(), this);

    configDlg->setNotationPage();
    configDlg->show();
}

void
NotationView::slotCheckShowHeadersMenu(bool checked)
{
    findAction("show_track_headers")->setChecked(checked);
}

/// YG: Only for debug
void
NotationView::slotDebugDump()
{
    m_notationWidget->getScene()->dumpVectors();
}

/// YG: Only for debug
void
NotationView::slotBarDataDump()
{
    m_notationWidget->getScene()->dumpBarDataMap();
}

void
NotationView::slotInterpretActivate()
{
    // If there is a selection, run the interpretations against it in the usual
    // fashion.  If there is no selection, select the entire segment first.
    // (Will this work well in practice?  The last thing the user did will
    // probably leave a selection of one event.  Let's start here and refine as
    // the need becomes evident through testing.)
    EventSelection *selection = getSelection();

    // After placing a hairpin and likely other incidental actions, you can end
    // up with a valid selection that has a total duration of 0.  In this case,
    // treat it like there was no selection at all by just zeroing it out and
    // feeding it along to be replaced with a select all.
    if (selection) {
        if (selection->getTotalDuration() == 0) selection = 0;
    }

    // Selections aren't undoable anywhere else, so there's no point writing a
    // new command or making a MacroCommand.  Just call the slot as if the user
    // hit Ctrl+A and go.
    if (!selection) {
        slotEditSelectWholeStaff();
        selection = getSelection();
    }

    // Make sure it worked, just in case.
    if (!selection) return;

    // xor all the options together in the same fashion as the dialog
    int flags = 0;
    if (findAction("interpret_text_dynamics")->isChecked()) flags |= InterpretCommand::ApplyTextDynamics;
    if (findAction("interpret_hairpins")->isChecked()) flags |= InterpretCommand::ApplyHairpins;
    if (findAction("interpret_slurs")->isChecked()) flags |= InterpretCommand::Articulate;
    if (findAction("interpret_beats")->isChecked()) flags |= InterpretCommand::StressBeats;

    // Debug output just to make it possible to observe that all the checkable
    // buttons are working correctly:
    RG_DEBUG << "NotationView::slotInterpretActivate() using flags: "
             << (flags & InterpretCommand::ApplyTextDynamics ? "[TEXT]" : "[    ]")
             << (flags & InterpretCommand::ApplyHairpins ? "[HAIR]" : "[    ]")
             << (flags & InterpretCommand::Articulate ? "[SLUR]" : "[    ]")
             << (flags & InterpretCommand::StressBeats ? "[BEAT]" : "[    ]");

    // go straight to the command with the flags pulled from the toolbar as
    // though it were the dialog
    CommandHistory::getInstance()->addCommand(new InterpretCommand
         (*selection,
          getDocument()->getComposition().getNotationQuantizer(),
          flags));
}


} // end namespace Rosegarden

