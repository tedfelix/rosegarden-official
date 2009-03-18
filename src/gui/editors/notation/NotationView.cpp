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

#include "NotationView.h"

#include "document/RosegardenGUIDoc.h"
#include "document/CommandHistory.h"
#include "document/ConfigGroups.h"
#include "NotationWidget.h"
#include "NotationCommandRegistry.h"
#include "NoteFontFactory.h"
#include "base/Clipboard.h"
#include "base/Selection.h"

#include "commands/edit/CopyCommand.h"
#include "commands/edit/CutCommand.h"
#include "commands/edit/CutAndCloseCommand.h"
#include "commands/edit/EraseCommand.h"
#include "commands/edit/PasteEventsCommand.h"

#include "gui/dialogs/PasteNotationDialog.h"

#include <QWidget>
#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>

#include <algorithm>

namespace Rosegarden
{

NewNotationView::NewNotationView(RosegardenGUIDoc *doc,
                                 std::vector<Segment *> segments,
                                 QWidget *parent) :
    EditViewBase(doc, segments, parent),
    m_document(doc)
{
    m_notationWidget = new NotationWidget();
    setCentralWidget(m_notationWidget);
    m_notationWidget->setSegments(doc, segments);

    m_commandRegistry = new NotationCommandRegistry(this);

    setupActions();

    createGUI("notation.rc");
}

NewNotationView::~NewNotationView()
{
}

void
NewNotationView::setupActions()
{
    EditViewBase::setupActions(true);

    createAction("file_print", SLOT(slotFilePrint()));
    createAction("file_print_preview", SLOT(slotFilePrintPreview()));
    createAction("file_print_lilypond", SLOT(slotPrintLilyPond()));
    createAction("file_preview_lilypond", SLOT(slotPreviewLilyPond()));
    createAction("show_track_headers", SLOT(slotShowHeadersGroup()));
    createAction("no_accidental", SLOT(slotNoAccidental()));
    createAction("follow_accidental", SLOT(slotFollowAccidental()));
    createAction("sharp_accidental", SLOT(slotSharp()));
    createAction("flat_accidental", SLOT(slotFlat()));
    createAction("natural_accidental", SLOT(slotNatural()));
    createAction("double_sharp_accidental", SLOT(slotDoubleSharp()));
    createAction("double_flat_accidental", SLOT(slotDoubleFlat()));
    createAction("treble_clef", SLOT(slotTrebleClef()));
    createAction("alto_clef", SLOT(slotAltoClef()));
    createAction("tenor_clef", SLOT(slotTenorClef()));
    createAction("bass_clef", SLOT(slotBassClef()));
    createAction("text", SLOT(slotText()));
    createAction("guitarchord", SLOT(slotGuitarChord()));
    createAction("lilypond_directive", SLOT(slotLilyPondDirective()));
    createAction("erase", SLOT(slotEraseSelected()));
    createAction("select", SLOT(slotSelectSelected()));
    createAction("toggle_step_by_step", SLOT(slotToggleStepByStep()));
    createAction("select_from_start", SLOT(slotEditSelectFromStart()));
    createAction("select_to_end", SLOT(slotEditSelectToEnd()));
    createAction("select_whole_staff", SLOT(slotEditSelectWholeStaff()));
    createAction("cut_and_close", SLOT(slotEditCutAndClose()));
    createAction("general_paste", SLOT(slotEditGeneralPaste()));
    createAction("delete", SLOT(slotEditDelete()));
    createAction("move_events_up_staff", SLOT(slotMoveEventsUpStaff()));
    createAction("move_events_down_staff", SLOT(slotMoveEventsDownStaff()));
    createAction("linear_mode", SLOT(slotLinearMode()));
    createAction("continuous_page_mode", SLOT(slotContinuousPageMode()));
    createAction("multi_page_mode", SLOT(slotMultiPageMode()));
    createAction("show_chords_ruler", SLOT(slotToggleChordsRuler()));
    createAction("show_raw_note_ruler", SLOT(slotToggleRawNoteRuler()));
    createAction("show_tempo_ruler", SLOT(slotToggleTempoRuler()));
    createAction("show_annotations", SLOT(slotToggleAnnotations()));
    createAction("show_lilypond_directives", SLOT(slotToggleLilyPondDirectives()));
    createAction("lyric_editor", SLOT(slotEditLyrics()));
    createAction("simple_tuplet", SLOT(slotGroupSimpleTuplet()));
    createAction("tuplet", SLOT(slotGroupGeneralTuplet()));
    createAction("triplet_mode", SLOT(slotUpdateInsertModeStatus()));
    createAction("chord_mode", SLOT(slotUpdateInsertModeStatus()));
    createAction("grace_mode", SLOT(slotUpdateInsertModeStatus()));
    createAction("normalize_rests", SLOT(slotTransformsNormalizeRests()));
    createAction("collapse_notes", SLOT(slotTransformsCollapseNotes()));
    createAction("quantize", SLOT(slotTransformsQuantize()));
    createAction("interpret", SLOT(slotTransformsInterpret()));
    createAction("debug_dump", SLOT(slotDebugDump()));
    createAction("make_ornament", SLOT(slotMakeOrnament()));
    createAction("use_ornament", SLOT(slotUseOrnament()));
    createAction("remove_ornament", SLOT(slotRemoveOrnament()));
    createAction("add_clef", SLOT(slotEditAddClef()));
    createAction("add_key_signature", SLOT(slotEditAddKeySignature()));
    createAction("add_sustain_down", SLOT(slotEditAddSustainDown()));
    createAction("add_sustain_up", SLOT(slotEditAddSustainUp()));
    createAction("transpose_segment", SLOT(slotEditTranspose()));
    createAction("switch_preset", SLOT(slotEditSwitchPreset()));
    createAction("show_tools_toolbar", SLOT(slotToggleToolsToolBar()));
    createAction("show_notes_toolbar", SLOT(slotToggleNotesToolBar()));
    createAction("show_rests_toolbar", SLOT(slotToggleRestsToolBar()));
    createAction("show_accidentals_toolbar", SLOT(slotToggleAccidentalsToolBar()));
    createAction("show_clefs_toolbar", SLOT(slotToggleClefsToolBar()));
    createAction("show_marks_toolbar", SLOT(slotToggleMarksToolBar()));
    createAction("show_group_toolbar", SLOT(slotToggleGroupToolBar()));
    createAction("show_layout_toolbar", SLOT(slotToggleLayoutToolBar()));
    createAction("show_transport_toolbar", SLOT(slotToggleTransportToolBar()));
    createAction("show_meta_toolbar", SLOT(slotToggleMetaToolBar()));
    createAction("cursor_back", SLOT(slotStepBackward()));
    createAction("cursor_forward", SLOT(slotStepForward()));
    createAction("cursor_back_bar", SLOT(slotJumpBackward()));
    createAction("cursor_forward_bar", SLOT(slotJumpForward()));
    createAction("extend_selection_backward", SLOT(slotExtendSelectionBackward()));
    createAction("extend_selection_forward", SLOT(slotExtendSelectionForward()));
    createAction("extend_selection_backward_bar", SLOT(slotExtendSelectionBackwardBar()));
    createAction("extend_selection_forward_bar", SLOT(slotExtendSelectionForwardBar()));
    //!!! not here yet createAction("move_selection_left", SLOT(slotMoveSelectionLeft()));
    createAction("cursor_start", SLOT(slotJumpToStart()));
    createAction("cursor_end", SLOT(slotJumpToEnd()));
    createAction("cursor_up_staff", SLOT(slotCurrentStaffUp()));
    createAction("cursor_down_staff", SLOT(slotCurrentStaffDown()));
    createAction("cursor_prior_segment", SLOT(slotCurrentSegmentPrior()));
    createAction("cursor_next_segment", SLOT(slotCurrentSegmentNext()));
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
    createAction("velocity_up", SLOT(slotVelocityUp()));
    createAction("velocity_down", SLOT(slotVelocityDown()));
    createAction("set_velocities", SLOT(slotSetVelocities()));
    createAction("toggle_dot", SLOT(slotToggleDot()));
    createAction("add_dot", SLOT(slotAddDot()));
    createAction("add_notation_dot", SLOT(slotAddDotNotationOnly()));

    
    std::set<QString> fs(NoteFontFactory::getFontNames());
    std::vector<QString> f(fs.begin(), fs.end());
    std::sort(f.begin(), f.end());

    QMenu *fontActionMenu = new QMenu(tr("Note &Font"), this); 
    fontActionMenu->setObjectName("note_font_actionmenu");

    QActionGroup *ag = new QActionGroup(this);

    for (std::vector<QString>::iterator i = f.begin(); i != f.end(); ++i) {

        QString fontQName(*i);

        QAction *a = createAction("note_font_" + fontQName,
                                  SLOT(slotChangeFontFromAction()));

        ag->addAction(a);

        a->setText(fontQName);
        a->setCheckable(true);
//!!!        a->setChecked(*i == m_fontName);

        fontActionMenu->addAction(a);
    }

    //&&& add fontActionMenu to the appropriate super-menu

//!!!    QMenu *fontSizeActionMenu = new QMenu(tr("Si&ze"), this);
//    fontSizeActionMenu->setObjectName("note_font_size_actionmenu");

//    setupFontSizeMenu();

    //&&& add m_fontSizeActionMenu to the appropriate super-menu

/*!!!
    QMenu *spacingActionMenu = new QMenu(tr("S&pacing"), this);
    spacingActionMenu->setObjectName("stretch_actionmenu");

    int defaultSpacing = m_hlayout->getSpacing();
    std::vector<int> spacings = NotationHLayout::getAvailableSpacings();

    ag = new QActionGroup(this);

    for (std::vector<int>::iterator i = spacings.begin();
         i != spacings.end(); ++i) {

        QAction *a = createAction(QString("spacing_%1").arg(*i),
                                  SLOT(slotChangeSpacingFromAction()));

        ag->addAction(a);
        a->setText(QString("%1%").arg(*i));
        a->setCheckable(true);
        a->setChecked(*i == defaultSpacing);

        spacingActionMenu->addAction(a);
    }

    //&&& add spacingActionMenu to the appropriate super-menu


    QMenu *proportionActionMenu = new QMenu(tr("Du&ration Factor"), this);
    proportionActionMenu->setObjectName("proportion_actionmenu");

    int defaultProportion = m_hlayout->getProportion();
    std::vector<int> proportions = NotationHLayout::getAvailableProportions();

    ag = new QActionGroup(this);

    for (std::vector<int>::iterator i = proportions.begin();
         i != proportions.end(); ++i) {

        QString name = QString("%1%").arg(*i);
        if (*i == 0) name = tr("None");

        QAction *a = createAction(QString("proportion_%1").arg(*i),
                                  SLOT(slotChangeSpacingFromAction()));

        ag->addAction(a);
        a->setText(name);
        a->setCheckable(true);
        a->setChecked(*i == defaultProportion);

        proportionActionMenu->addAction(a);
    }
*/
    //&&& add proportionActionMenu to the appropriate super-menu

#ifdef NOT_JUST_NOW //!!!


//!!!
// I believe this one was never actually used:
//    KActionMenu *ornamentActionMenu =
//        new KActionMenu(tr("Use Ornament"), this, "ornament_actionmenu");


    ag = new QActionGroup(this);

    for (NoteActionDataMap::iterator actionDataIter = m_noteActionDataMap->begin();
         actionDataIter != m_noteActionDataMap->end();
         ++actionDataIter) {

        NoteActionData noteActionData = **actionDataIter;

        QAction *a = createAction(noteActionData.actionName,
                                  SLOT(slotNoteAction()));
        
        ag->addAction(a);
        a->setCheckable(true);

        if (noteActionData.noteType == Note::Crotchet &&
            noteActionData.dots == 0 && !noteActionData.rest) {
            m_selectDefaultNote = a;
        }
    }

#endif
    //!!! NoteChangeActionData also never used, I think

}

Segment *
NewNotationView::getCurrentSegment()
{
    if (m_notationWidget) return m_notationWidget->getCurrentSegment();
    else return 0;
}

EventSelection *
NewNotationView::getSelection() const
{
    if (m_notationWidget) return m_notationWidget->getSelection();
    else return 0;
}

void
NewNotationView::setSelection(EventSelection *selection, bool preview)
{
    if (m_notationWidget) m_notationWidget->setSelection(selection, preview);
}

void
NewNotationView::slotEditCut()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CutCommand(*selection, m_document->getClipboard()));
}

void
NewNotationView::slotEditDelete()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand(new EraseCommand(*selection));
}

void
NewNotationView::slotEditCopy()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CopyCommand(*selection, m_document->getClipboard()));
}

void
NewNotationView::slotEditCutAndClose()
{
    EventSelection *selection = getSelection();
    if (!selection) return;
    CommandHistory::getInstance()->addCommand
        (new CutAndCloseCommand(*selection, m_document->getClipboard()));
}

static const QString RESTRICTED_PASTE_FAILED_DESCRIPTION = QObject::tr(
                      "The Restricted paste type requires enough empty " \
                      "space (containing only rests) at the paste position " \
                      "to hold all of the events to be pasted.\n" \
                      "Not enough space was found.\n" \
                      "If you want to paste anyway, consider using one of " \
                      "the other paste types from the \"Paste...\" option " \
                      "on the Edit menu.  You can also change the default " \
                      "paste type to something other than Restricted if " \
                      "you wish."
    );

void
NewNotationView::slotEditPaste()
{
    Clipboard *clipboard = m_document->getClipboard();

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

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
        settings.value("pastetype",
                       PasteEventsCommand::Restricted).toUInt();

    PasteEventsCommand *command = new PasteEventsCommand
        (*segment, clipboard, insertionTime, defaultType);

    if (!command->isPossible()) {
        QMessageBox::warning	//@@@ detailedError
            (this, RESTRICTED_PASTE_FAILED_DESCRIPTION, 
             tr("Couldn't paste at this point.") );
        delete command;
    } else {
        CommandHistory::getInstance()->addCommand(command);
        setSelection(new EventSelection(command->getPastedEvents()), false);
//!!!        slotSetInsertCursorPosition(endTime, true, false);
    }

    settings.endGroup();
}

void
NewNotationView::slotEditGeneralPaste()
{
    Clipboard *clipboard = getDocument()->getClipboard();

    if (clipboard->isEmpty()) {
        slotStatusHelpMsg(tr("Clipboard is empty"));
        return ;
    }

    slotStatusHelpMsg(tr("Inserting clipboard contents..."));

    Segment *segment = getCurrentSegment();
    if (!segment) return;

    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);

    PasteEventsCommand::PasteType defaultType = (PasteEventsCommand::PasteType)
        settings.value("pastetype",
                       PasteEventsCommand::Restricted).toUInt();
    
    PasteNotationDialog dialog(this, defaultType);

    if (dialog.exec() == QDialog::Accepted) {

        PasteEventsCommand::PasteType type = dialog.getPasteType();
        if (dialog.setAsDefault()) {
            //###settings.beginGroup( NotationViewConfigGroup );
            settings.setValue("pastetype", type);
        }

        timeT insertionTime = getInsertionTime();
        timeT endTime = insertionTime +
            (clipboard->getSingleSegment()->getEndTime() -
             clipboard->getSingleSegment()->getStartTime());

        PasteEventsCommand *command = new PasteEventsCommand
            (*segment, clipboard, insertionTime, type);

        if (!command->isPossible()) {
            QMessageBox::warning	//detailedError
                (this, RESTRICTED_PASTE_FAILED_DESCRIPTION, 
                 tr("Couldn't paste at this point.") );
            delete command;
        } else {
            CommandHistory::getInstance()->addCommand(command);
            setSelection(new EventSelection(*segment, insertionTime, endTime),
                         false);
//!!!            slotSetInsertCursorPosition(endTime, true, false);
        }
    }

    settings.endGroup();
}

}

#include "NotationView.moc"
