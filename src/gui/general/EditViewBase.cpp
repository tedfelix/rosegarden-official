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

#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/RosegardenMainViewWidget.h"
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

void EditViewBase::setupBaseActions(bool haveClipboard)
{
    // Actions all edit views will have

    QAction *showStatusBar = createAction(
            "options_show_statusbar", SLOT(slotToggleStatusBar()));
    showStatusBar->setChecked(!statusBar()->isHidden());

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
    Composition &composition =
            RosegardenDocument::currentDocument->getComposition();

    const TrackId trackID = getCurrentSegment()->getTrack();

    // Select the track for this segment.
    composition.setSelectedTrack(trackID);
    composition.notifyTrackSelectionChanged(trackID);
    RosegardenMainWindow::self()->getView()->slotSelectTrackSegments(trackID);

    // Toggle solo on the selected track.
    // The "false" is ignored.  It was used for the checked state.
    RosegardenMainWindow::self()->slotToggleSolo(false);

    // Make sure the toggle button stays in sync with the actual
    // toggle state.
    updateSoloButton();
}

void
EditViewBase::updateSoloButton()
{
    // ??? Still not working like we need it to.
    //     Test cases that are failing:
    //       - Notation: Clicking on a staff to make it current.  This
    //         does not update the solo button.  There may be other cases.
    //         Seems like the call to updateSoloButton() should originate
    //         further down.  NotationWidget or NotationScene maybe.
    //       - Matrix: There is no appropriate place to call updateSoloButton()
    //         in MatrixView.  The call would need to originate further down.
    //         MatrixWidget or MatrixScene maybe.

    const TrackId trackID = getCurrentSegment()->getTrack();

    QAction *toggleSoloAction = findAction("toggle_solo");
    if (toggleSoloAction) {
        Track *track = RosegardenDocument::currentDocument->getComposition().
                getTrackById(trackID);
        if (track)
            toggleSoloAction->setChecked(track->isSolo());
    }
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

        QString segLabel = strtoqstr(m_segments[0]->getLabel());
        if (!segLabel.isEmpty())
            segLabel = tr("Segment \"%1\" ").arg(segLabel);

        QString trkLabel = strtoqstr(track->getLabel());
        if (trkLabel.isEmpty()  ||  trkLabel == tr("<untitled>"))
            trkLabel = " ";
        else
            trkLabel = QString(" \"%1\" ").arg(trkLabel);

        title = tr("%1%2 - %3Track%4#%5 - %6").
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
        title = tr("%1%2 - %3 Segments - %4").
                arg(modified).
                arg(doc->getTitle()).
                arg(segmentCount).
                arg(editorName);
    }

    return title;
}


}
