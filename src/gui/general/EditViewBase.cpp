/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[EditViewBase]"
#define RG_NO_DEBUG_PRINT

#include "EditViewBase.h"

#include "document/RosegardenDocument.h"
#include "document/CommandHistory.h"
#include "gui/dialogs/ConfigureDialog.h"
#include "gui/dialogs/TimeDialog.h"
#include "base/Clipboard.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "misc/Debug.h"

#include <QAction>
#include <QStatusBar>
#include <QToolBar>


namespace Rosegarden
{


EditViewBase::EditViewBase(const std::vector<Segment *> &segments) :
    QMainWindow(nullptr),
    m_segments(segments)
{
    setAttribute(Qt::WA_DeleteOnClose);

    // Store so that we attach and detach from the same document.
    m_doc = RosegardenDocument::currentDocument;
    RosegardenDocument::currentDocument->attachEditView(this);

    connect(CommandHistory::getInstance(), &CommandHistory::commandExecuted,
            this, &EditViewBase::slotUpdateClipboardActionState);
}

EditViewBase::~EditViewBase()
{
    // Use m_doc to make sure we detach from the same document we attached to.
    m_doc->detachEditView(this);
}

void EditViewBase::readOptions()
{
    // ??? Trivial and confusing.  Inline into callers?

    QAction *a = findAction("options_show_statusbar");
    if (a)
        a->setChecked(!statusBar()->isHidden());
}

void EditViewBase::setCheckBoxState(const QString &actionName,
                                    const QString &toolbarName)
{
    // ??? This is called a lot, but it is trivial.  Inline into callers.
    //     Is the state saved to the .conf file?  If so, then this might
    //     be reducible to a single line once it is inlined.  Otherwise
    //     we could just hard-code the initial state and again, this is
    //     reduced to a single line.

    // Use isHidden() for visibility since ancestors may not be visible
    // since this is called during each view's constructor.
    const bool visible = !findToolbar(toolbarName)->isHidden();
    findAction(actionName)->setChecked(visible);
}


void EditViewBase::setupBaseActions(bool haveClipboard)
{
    // Actions all edit views will have

    createAction("options_show_statusbar", SLOT(slotToggleStatusBar()));
    createAction("options_configure", SLOT(slotConfigure()));

    createAction("file_save", SIGNAL(saveFile()));
    createAction("file_close", SLOT(slotCloseWindow()));

    if (haveClipboard) {
        createAction("edit_cut", SLOT(slotEditCut()));
        createAction("edit_copy", SLOT(slotEditCopy()));
        createAction("edit_paste", SLOT(slotEditPaste()));
    }

    createAction("open_in_matrix", SLOT(slotOpenInMatrix()));
    createAction("open_in_percussion_matrix", SLOT(slotOpenInPercussionMatrix()));
    createAction("open_in_notation", SLOT(slotOpenInNotation()));
    createAction("open_in_event_list", SLOT(slotOpenInEventList()));
    createAction("open_in_pitch_tracker", SLOT(slotOpenInPitchTracker()));
    createAction("set_segment_start", SLOT(slotSetSegmentStartTime()));
    createAction("set_segment_duration", SLOT(slotSetSegmentDuration()));
}

void EditViewBase::slotConfigure()
{
    ConfigureDialog *configDlg =
        new ConfigureDialog(RosegardenDocument::currentDocument, this);

    configDlg->show();
}

void
EditViewBase::slotOpenInNotation()
{
    emit openInNotation(m_segments);
}

void
EditViewBase::slotOpenInMatrix()
{
    emit openInMatrix(m_segments);
}

void
EditViewBase::slotOpenInPercussionMatrix()
{
    emit openInPercussionMatrix(m_segments);
}

void
EditViewBase::slotOpenInEventList()
{
    emit openInEventList(m_segments);
}

void
EditViewBase::slotOpenInPitchTracker()
{
    emit openInPitchTracker(m_segments);
}

void EditViewBase::slotCloseWindow()
{
    close();
}

void EditViewBase::slotToggleStatusBar()
{
    if (statusBar()->isVisible())
        statusBar()->hide();
    else
        statusBar()->show();
}

void EditViewBase::showStatusBarMessage(const QString &text)
{
    statusBar()->showMessage(text, 2000);
}

void
EditViewBase::slotUpdateClipboardActionState()
{
    if (Clipboard::mainClipboard()->isEmpty()) {
        RG_DEBUG << "EditViewBase::slotTestClipboard(): empty";
        leaveActionState("have_clipboard");
        leaveActionState("have_clipboard_single_segment");
    } else {
        RG_DEBUG << "EditViewBase::slotTestClipboard(): not empty";
        enterActionState("have_clipboard");
        if (Clipboard::mainClipboard()->isSingleSegment()) {
            enterActionState("have_clipboard_single_segment");
        } else {
            leaveActionState("have_clipboard_single_segment");
        }
    }
}

void
EditViewBase::slotToggleSolo()
{
    // Select the track for this segment.
    RosegardenDocument::currentDocument->getComposition().setSelectedTrack(
            getCurrentSegment()->getTrack());
    RosegardenDocument::currentDocument->getComposition().notifyTrackSelectionChanged(
            getCurrentSegment()->getTrack());
    // Calls RosegardenMainViewWidget::slotSelectTrackSegments().
    emit selectTrack(getCurrentSegment()->getTrack());
    // ??? Get rid of signal/slot.  Call directly.  It's faster and
    //     easier to understand.
    //RosegardenMainWindow::self()->getView()->slotSelectTrackSegments(
    //        getCurrentSegment()->getTrack());

    // Toggle solo on the selected track.
    // The "false" is ignored.  It was used for the checked state.
    // Calls RosegardenMainWindow::slotToggleSolo().
    // ??? Can't we just call the routine directly and void the signal/slot?
    //     Like we did for Track selection above.
    emit toggleSolo(false);
    // ??? Get rid of signal/slot.  Call directly.  It's faster and
    //     easier to understand.
    //RosegardenMainWindow::self()->slotToggleSolo(false);
}

void
EditViewBase::slotSetSegmentStartTime()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return;

    TimeDialog dialog(this, tr("Segment Start Time"),
                      &RosegardenDocument::currentDocument->getComposition(),
                      segment->getStartTime(), false);

    if (dialog.exec() == QDialog::Accepted) {

        SegmentReconfigureCommand *command = new SegmentReconfigureCommand(
                tr("Set Segment Start Time"),
                &RosegardenDocument::currentDocument->getComposition());

        command->addSegment(
                segment,
                dialog.getTime(),
                segment->getEndMarkerTime() - segment->getStartTime() +
                        dialog.getTime(),
                segment->getTrack());

        CommandHistory::getInstance()->addCommand(command);
    }
}

void
EditViewBase::slotSetSegmentDuration()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return;

    TimeDialog dialog(this, tr("Segment Duration"),
                      &RosegardenDocument::currentDocument->getComposition(),
                      segment->getStartTime(),
                      segment->getEndMarkerTime() - segment->getStartTime(),
                      Note(Note::Shortest).getDuration(),
                      false);  // constrainToCompositionDuration

    if (dialog.exec() == QDialog::Accepted) {

        SegmentReconfigureCommand *command = new SegmentReconfigureCommand(
                tr("Set Segment Duration"),
                &RosegardenDocument::currentDocument->getComposition());

        command->addSegment(
                segment,
                segment->getStartTime(),  // newStartTime
                segment->getStartTime() + dialog.getTime(),  // newEndMarkerTime
                segment->getTrack());  // newTrack

        CommandHistory::getInstance()->addCommand(command);
    }
}

QString
EditViewBase::getTitle(const QString &editorName)
{
    const RosegardenDocument *doc = RosegardenDocument::currentDocument;

    const QString modified = (doc->isModified() ? "*" : "");

    QString title;

    const size_t segmentCount = m_segments.size();

    // Format the titlebar text based on the number of Segments.
    if (segmentCount == 1) {
        // For one Segment, provide Segment and Track info.

        const TrackId trackId = m_segments[0]->getTrack();
        const Track *track =
                m_segments[0]->getComposition()->getTrackById(trackId);
        if (!track)
            return "";

        const int trackPosition = track->getPosition();

        // ??? Empty Segment name results in "...Segment Track..." which
        //     looks bad.  Move "Segment" up here and eliminate it if
        //     there is no Segment label.
        QString segLabel = strtoqstr(m_segments[0]->getLabel());
        if (segLabel.isEmpty())
            segLabel = " ";
        else
            segLabel = QString(" \"%1\" ").arg(segLabel);

        QString trkLabel = strtoqstr(track->getLabel());
        if (trkLabel.isEmpty()  ||  trkLabel == tr("<untitled>"))
            trkLabel = " ";
        else
            trkLabel = QString(" \"%1\" ").arg(trkLabel);

        title = tr("%1%2 - Segment%3Track%4#%5 - %6").
                arg(modified).
                arg(doc->getTitle()).
                arg(segLabel).
                arg(trkLabel).
                arg(trackPosition + 1).
                arg(editorName);

    } else if (segmentCount == doc->getComposition().getNbSegments()) {
        // All Segments.
        title = tr("%1%2 - All Segments - %3").
                arg(modified).
                arg(doc->getTitle()).
                arg(editorName);
    } else {
        // More than one Segment, but not all.
        // ??? No need for "(s)".  This is always more than one.  "Segments".
        title = tr("%1%2 - %3 Segment(s) - %4").
                arg(modified).
                arg(doc->getTitle()).
                arg(segmentCount).
                arg(editorName);
    }

    return title;
}


}
