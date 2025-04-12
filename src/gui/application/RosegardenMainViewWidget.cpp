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

#define RG_MODULE_STRING "[RosegardenMainViewWidget]"
#define RG_NO_DEBUG_PRINT

#include "RosegardenMainViewWidget.h"

#include "sound/Midi.h"
#include "sound/SoundDriver.h"
#include "gui/editors/segment/TrackButtons.h"
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "gui/application/TransportStatus.h"
#include "base/AudioLevel.h"
#include "base/Composition.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "base/RealTime.h"
#include "base/RulerScale.h"
#include "base/Segment.h"
#include "base/Selection.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "commands/segment/AudioSegmentAutoSplitCommand.h"
#include "commands/segment/AudioSegmentInsertCommand.h"
#include "commands/segment/SegmentSingleRepeatToCopyCommand.h"
#include "document/CommandHistory.h"
#include "document/RosegardenDocument.h"
#include "RosegardenApplication.h"
#include "gui/configuration/GeneralConfigurationPage.h"
#include "gui/configuration/AudioConfigurationPage.h"
#include "gui/dialogs/AudioSplitDialog.h"
#include "gui/dialogs/AudioManagerDialog.h"
#include "gui/dialogs/DocumentConfigureDialog.h"
#include "gui/editors/eventlist/EventListEditor.h"
#include "gui/editors/matrix/MatrixView.h"
#include "gui/editors/notation/NotationView.h"
#include "gui/editors/parameters/InstrumentParameterBox.h"
#include "gui/editors/parameters/RosegardenParameterArea.h"
#include "gui/editors/parameters/SegmentParameterBox.h"
#include "gui/editors/parameters/TrackParameterBox.h"
#include "gui/editors/pitchtracker/PitchTrackerView.h"
#include "gui/editors/segment/compositionview/CompositionView.h"
#include "gui/editors/segment/compositionview/SegmentSelector.h"
#include "gui/editors/segment/TrackEditor.h"
#include "gui/seqmanager/SequenceManager.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/LoopRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/rulers/StandardRuler.h"
#include "gui/studio/StudioControl.h"
#include "RosegardenMainWindow.h"
#include "SetWaitCursor.h"
#include "sound/AudioFile.h"
#include "sound/AudioFileManager.h"
#include "sound/MappedEvent.h"
#include "sound/SequencerDataBlock.h"
#include "document/Command.h"

#include <QApplication>
#include <QSettings>
#include <QMessageBox>
#include <QProcess>
#include <QProgressDialog>
#include <QApplication>
#include <QCursor>
#include <QDialog>
#include <QFileInfo>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QTextStream>
#include <QScrollBar>

#include "gui/editors/parameters/MIDIInstrumentParameterPanel.h"

namespace Rosegarden
{

// Use this to define the basic unit of the main segment canvas size.
//
// This apparently arbitrary figure is what we think is an
// appropriate width in pixels for a 4/4 bar.  Beware of making it
// too narrow, as shorter bars will be proportionally smaller --
// the visual difference between 2/4 and 4/4 is perhaps greater
// than it sounds.
//
static double barWidth44 = 100.0;

// This is the maximum number of matrix, event view or percussion
// matrix editors to open in a single operation (not the maximum that
// can be open at a time -- there isn't one)
//
static int maxEditorsToOpen = 8;

RosegardenMainViewWidget::RosegardenMainViewWidget(bool showTrackLabels,
                                                   SegmentParameterBox* segmentParameterBox,
                                                   InstrumentParameterBox* instrumentParameterBox,
                                                   TrackParameterBox* trackParameterBox,
                                                   RosegardenParameterArea* parameterArea,
                                                   RosegardenMainWindow *parent)
        : QWidget(parent),
        m_rulerScale(nullptr),
        m_trackEditor(nullptr),
        m_segmentParameterBox(segmentParameterBox),
        m_instrumentParameterBox(instrumentParameterBox),
        m_trackParameterBox(trackParameterBox)
{
    setObjectName("View");
    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    Composition *comp = &doc->getComposition();
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(parameterArea);

    double unitsPerPixel =
        TimeSignature(4, 4).getBarDuration() / barWidth44;
    m_rulerScale = new SimpleRulerScale(comp, 0, unitsPerPixel);

    // Construct the trackEditor first so we can then
    // query it for placement information
    //
    m_trackEditor = new TrackEditor(doc, this, m_rulerScale, showTrackLabels);

    layout->addWidget(m_trackEditor);
    setLayout(layout);

    connect(m_trackEditor->getCompositionView(),
            &CompositionView::editSegment,
            this, &RosegardenMainViewWidget::slotEditSegment);

    //connect(m_trackEditor->getCompositionView(),
    //        SIGNAL(editSegmentNotation(Segment *)),
    //        SLOT(slotEditSegmentNotation(Segment *)));

    //connect(m_trackEditor->getCompositionView(),
    //        SIGNAL(editSegmentPitchView(Segment *)),
    //        SLOT(slotEditSegmentPitchView(Segment *)));

    //connect(m_trackEditor->getCompositionView(),
    //        SIGNAL(editSegmentMatrix(Segment *)),
    //        SLOT(slotEditSegmentMatrix(Segment *)));

    //connect(m_trackEditor->getCompositionView(),
    //        SIGNAL(editSegmentAudio(Segment *)),
    //        SLOT(slotEditSegmentAudio(Segment *)));

    //connect(m_trackEditor->getCompositionView(),
    //        SIGNAL(audioSegmentAutoSplit(Segment *)),
    //        SLOT(slotSegmentAutoSplit(Segment *)));

    //connect(m_trackEditor->getCompositionView(),
    //        SIGNAL(editSegmentEventList(Segment *)),
    //        SLOT(slotEditSegmentEventList(Segment *)));

    connect(m_trackEditor->getCompositionView(),
            &CompositionView::editRepeat,
            this, &RosegardenMainViewWidget::slotEditRepeat);

    connect(m_trackEditor->getCompositionView(),
            &CompositionView::setPointerPosition,
            doc, &RosegardenDocument::slotSetPointerPosition);

    connect(m_trackEditor, &TrackEditor::droppedDocument,
            parent, &RosegardenMainWindow::slotOpenDroppedURL);

    connect(m_trackEditor,
            &TrackEditor::droppedAudio,
            this,
            &RosegardenMainViewWidget::slotDroppedAudio);

    connect(m_trackEditor,
            &TrackEditor::droppedNewAudio,
            this,
            &RosegardenMainViewWidget::slotDroppedNewAudio);

    connect(m_trackParameterBox,
            &TrackParameterBox::instrumentSelected,
            m_trackEditor->getTrackButtons(),
            &TrackButtons::slotTPBInstrumentSelected);

    connect(&ExternalController::self(),
                &ExternalController::externalControllerRMVW,
            this, &RosegardenMainViewWidget::slotExternalController);

    connect(CommandHistory::getInstance(),
            &CommandHistory::commandExecuted,
            m_trackEditor->getCompositionView(),
            &CompositionView::slotUpdateAll);
}

RosegardenMainViewWidget::~RosegardenMainViewWidget()
{
    RG_DEBUG << "dtor";

    delete m_rulerScale;
}

void RosegardenMainViewWidget::selectTool(QString toolName)
{
    m_trackEditor->getCompositionView()->setTool(toolName);
}

bool
RosegardenMainViewWidget::haveSelection()
{
    return m_trackEditor->getCompositionView()->haveSelection();
}

SegmentSelection
RosegardenMainViewWidget::getSelection()
{
    return m_trackEditor->getCompositionView()->getSelectedSegments();
}

void RosegardenMainViewWidget::updateSelectedSegments()
{
    m_trackEditor->getCompositionView()->updateSelectedSegments();
}

void RosegardenMainViewWidget::slotEditSegment(Segment *segment)
{
    Segment::SegmentType type = Segment::Internal;

    if (segment) {
        type = segment->getType();
    } else {
        if (haveSelection()) {

            bool haveType = false;

            SegmentSelection selection = getSelection();
            for (SegmentSelection::iterator i = selection.begin();
                    i != selection.end(); ++i) {

                Segment::SegmentType myType = (*i)->getType();
                if (haveType) {
                    if (myType != type) {
                         QMessageBox::warning(this, tr("Rosegarden"), tr("Selection must contain only audio or non-audio segments"));
                        return ;
                    }
                } else {
                    type = myType;
                    haveType = true;
                    segment = *i;
                }
            }
        } else
            return ;
    }

    if (type == Segment::Audio) {
        slotEditSegmentAudio(segment);
    } else {

        QSettings settings;
        settings.beginGroup( GeneralOptionsConfigGroup );

        GeneralConfigurationPage::DoubleClickClient
        client =
            (GeneralConfigurationPage::DoubleClickClient)
            ( settings.value("doubleclickclient",
                                          (unsigned int) GeneralConfigurationPage::NotationView).toUInt());

        if (client == GeneralConfigurationPage::MatrixView) {

            bool isPercussion = false;
            Track *track = RosegardenDocument::currentDocument->getComposition().getTrackById
                (segment->getTrack());
            if (track) {
                InstrumentId iid = track->getInstrument();
                Instrument *instrument =
                    RosegardenDocument::currentDocument->getStudio().getInstrumentById(iid);
                if (instrument && instrument->isPercussion()) isPercussion = true;
            }

            if (isPercussion) {
                slotEditSegmentPercussionMatrix(segment);
            } else {
                slotEditSegmentMatrix(segment);
            }

        } else if (client == GeneralConfigurationPage::EventListEditor) {
            slotEditSegmentEventList(segment);
        } else {
            slotEditSegmentNotation(segment);
        }
        settings.endGroup();
    }
}

void RosegardenMainViewWidget::slotEditRepeat(Segment *segment,
                                       timeT time)
{
    SegmentSingleRepeatToCopyCommand *command =
        new SegmentSingleRepeatToCopyCommand(segment, time);
    slotAddCommandToHistory(command);
}

void RosegardenMainViewWidget::slotEditSegmentNotation(Segment *p)
{
    SetWaitCursor waitCursor;
    std::vector<Segment *> segmentsToEdit;

    RG_DEBUG << "slotEditSegmentNotation(): p is " << p;

    // The logic here is: If we're calling for this operation to
    // happen on a particular segment, then open that segment and if
    // it's part of a selection open all other selected segments too.
    // If we're not calling for any particular segment, then open all
    // selected segments if there are any.

    if (haveSelection()) {

        SegmentSelection selection = getSelection();

        if (!p || (selection.find(p) != selection.end())) {
            for (SegmentSelection::iterator i = selection.begin();
                    i != selection.end(); ++i) {
                if ((*i)->getType() != Segment::Audio) {
                    segmentsToEdit.push_back(*i);
                }
            }
        } else {
            if (p->getType() != Segment::Audio) {
                segmentsToEdit.push_back(p);
            }
        }

    } else if (p) {
        if (p->getType() != Segment::Audio) {
            segmentsToEdit.push_back(p);
        }
    } else {
        return ;
    }

    if (segmentsToEdit.empty()) {
         QMessageBox::warning(this, tr("Rosegarden"), tr("No non-audio segments selected"));
        return ;
    }

    slotEditSegmentsNotation(segmentsToEdit);
}

void RosegardenMainViewWidget::slotEditSegmentsNotation(const std::vector<Segment *>& segmentsToEdit)
{
    createNotationView(segmentsToEdit);
}

void
RosegardenMainViewWidget::createNotationView(const std::vector<Segment *>& segmentsToEdit)
{
    NotationView *notationView =
        new NotationView(RosegardenDocument::currentDocument, segmentsToEdit);

    connect(notationView, &NotationView::play,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotPlay);
    connect(notationView, &NotationView::stop,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotStop);
    connect(notationView, &NotationView::fastForwardPlayback,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotFastforward);
    connect(notationView, &NotationView::rewindPlayback,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotRewind);
    connect(notationView, &NotationView::fastForwardPlaybackToEnd,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotFastForwardToEnd);
    connect(notationView, &NotationView::rewindPlaybackToBeginning,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotRewindToBeginning);
    connect(notationView, &NotationView::panic,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotPanic);

    connect(notationView, &EditViewBase::saveFile,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotFileSave);
    connect(notationView, &NotationView::openInNotation,
            this, &RosegardenMainViewWidget::slotEditSegmentsNotation);
    connect(notationView, &NotationView::openInMatrix,
            this, &RosegardenMainViewWidget::slotEditSegmentsMatrix);
    connect(notationView, &NotationView::openInPercussionMatrix,
            this, &RosegardenMainViewWidget::slotEditSegmentsPercussionMatrix);
    connect(notationView, &NotationView::openInEventList,
            this, &RosegardenMainViewWidget::slotEditSegmentsEventList);
    connect(notationView, &NotationView::openInPitchTracker,
            this, &RosegardenMainViewWidget::slotEditSegmentsPitchTracker);
    connect(notationView, &NotationView::editTriggerSegment,
            this, &RosegardenMainViewWidget::slotEditTriggerSegment);
    // No such signal comes from NotationView
    //connect(notationView, SIGNAL(staffLabelChanged(TrackId, QString)),
    //        this, SLOT(slotChangeTrackLabel(TrackId, QString)));

    SequenceManager *sM = RosegardenDocument::currentDocument->getSequenceManager();

    connect(sM, SIGNAL(insertableNoteOnReceived(int, int)),
            notationView, SLOT(slotInsertableNoteOnReceived(int, int)));
    connect(sM, SIGNAL(insertableNoteOffReceived(int, int)),
            notationView, SLOT(slotInsertableNoteOffReceived(int, int)));

    connect(notationView, &NotationView::stepByStepTargetRequested,
            this, &RosegardenMainViewWidget::stepByStepTargetRequested);
    connect(this, SIGNAL(stepByStepTargetRequested(QObject *)),
            notationView, SLOT(slotStepByStepTargetRequested(QObject *)));

    // Encourage the notation view window to open to the same
    // interval as the current segment view.  Since scrollToTime is
    // commented out (what bug?), it made no sense to leave the
    // support code in.
//     if (m_trackEditor->getCompositionView()->horizontalScrollBar()->value() > 1) { // don't scroll unless we need to
//         // first find the time at the center of the visible segment canvas
//         int centerX = (int)(m_trackEditor->getCompositionView()->contentsX() +
//                             m_trackEditor->getCompositionView()->visibleWidth() / 2);
//         timeT centerSegmentView = m_trackEditor->getRulerScale()->getTimeForX(centerX);
//         // then scroll the notation view to that time, "localized" for the current segment
// //!!!        notationView->scrollToTime(centerSegmentView);
// //!!!        notationView->updateView();
//     }
}

// mostly copied from slotEditSegmentNotationView, but some changes
// marked with CMT
void RosegardenMainViewWidget::slotEditSegmentPitchTracker(Segment *p)
{

    SetWaitCursor waitCursor;
    std::vector<Segment *> segmentsToEdit;

    RG_DEBUG << "slotEditSegmentPitchTracker(): p is " << p;

    // The logic here is: If we're calling for this operation to
    // happen on a particular segment, then open that segment and if
    // it's part of a selection open all other selected segments too.
    // If we're not calling for any particular segment, then open all
    // selected segments if there are any.

    if (haveSelection()) {

        SegmentSelection selection = getSelection();

        if (!p || (selection.find(p) != selection.end())) {
            for (SegmentSelection::iterator i = selection.begin();
                    i != selection.end(); ++i) {
                if ((*i)->getType() != Segment::Audio) {
                    segmentsToEdit.push_back(*i);
                }
            }
        } else {
            if (p->getType() != Segment::Audio) {
                segmentsToEdit.push_back(p);
            }
        }

    } else if (p) {
        if (p->getType() != Segment::Audio) {
            segmentsToEdit.push_back(p);
        }
    } else {
        return ;
    }

    if (segmentsToEdit.empty()) {
        /* was sorry */ QMessageBox::warning(this, "", tr("No non-audio segments selected"));
        return ;
    }


    // addition by CMT
    if (segmentsToEdit.size() > 1) {
        QMessageBox::warning(this, "", tr("Pitch Tracker can only contain 1 segment."));
        return ;
    }
    //!!! not necessary?  NotationView doesn't do this.  -gp
//    if (Segment::Audio == segmentsToEdit[0]->getType()) {
//        QMessageBox::warning(this, "", tr("Pitch Tracker needs a non-audio track."));
//        return ;
//    }

    slotEditSegmentsPitchTracker(segmentsToEdit);
}

void RosegardenMainViewWidget::slotEditSegmentsPitchTracker(const std::vector<Segment *>& segmentsToEdit)
{
    PitchTrackerView *view = createPitchTrackerView(segmentsToEdit);
    if (view) {
        if (view->getJackConnected()) {
            view->show();
        } else {
            delete view;
        }
    }
}

PitchTrackerView *
RosegardenMainViewWidget::createPitchTrackerView(const std::vector<Segment *>& segmentsToEdit)
{
    PitchTrackerView *pitchTrackerView =
        new PitchTrackerView(RosegardenDocument::currentDocument, segmentsToEdit);

    connect(pitchTrackerView, &NotationView::play,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotPlay);
    connect(pitchTrackerView, &NotationView::stop,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotStop);
    connect(pitchTrackerView, &NotationView::fastForwardPlayback,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotFastforward);
    connect(pitchTrackerView, &NotationView::rewindPlayback,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotRewind);
    connect(pitchTrackerView, &NotationView::fastForwardPlaybackToEnd,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotFastForwardToEnd);
    connect(pitchTrackerView, &NotationView::rewindPlaybackToBeginning,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotRewindToBeginning);
    connect(pitchTrackerView, &NotationView::panic,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotPanic);

    connect(pitchTrackerView, &EditViewBase::saveFile,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotFileSave);
//  This probably is obsolete in Thorn.
//    connect(pitchTrackerView, SIGNAL(jumpPlaybackTo(timeT)),
//            RosegardenDocument::currentDocument, SLOT(slotSetPointerPosition(timeT)));
    connect(pitchTrackerView, &PitchTrackerView::openInNotation,
            this, &RosegardenMainViewWidget::slotEditSegmentsNotation);
    connect(pitchTrackerView, &PitchTrackerView::openInMatrix,
            this, &RosegardenMainViewWidget::slotEditSegmentsMatrix);
    connect(pitchTrackerView, &PitchTrackerView::openInPercussionMatrix,
            this, &RosegardenMainViewWidget::slotEditSegmentsPercussionMatrix);
    connect(pitchTrackerView, &PitchTrackerView::openInEventList,
            this, &RosegardenMainViewWidget::slotEditSegmentsEventList);
    connect(pitchTrackerView, &PitchTrackerView::openInPitchTracker,
            this, &RosegardenMainViewWidget::slotEditSegmentsPitchTracker);
    connect(pitchTrackerView, &NotationView::editTriggerSegment,
            this, &RosegardenMainViewWidget::slotEditTriggerSegment);
    // No such signal comes from PitchTrackerView
    //connect(pitchTrackerView, SIGNAL(staffLabelChanged(TrackId, QString)),
    //        this, SLOT(slotChangeTrackLabel(TrackId, QString)));

    SequenceManager *sM = RosegardenDocument::currentDocument->getSequenceManager();

    connect(sM, SIGNAL(insertableNoteOnReceived(int, int)),
            pitchTrackerView, SLOT(slotInsertableNoteOnReceived(int, int)));
    connect(sM, SIGNAL(insertableNoteOffReceived(int, int)),
            pitchTrackerView, SLOT(slotInsertableNoteOffReceived(int, int)));

    connect(pitchTrackerView, &NotationView::stepByStepTargetRequested,
            this, &RosegardenMainViewWidget::stepByStepTargetRequested);
    connect(this, SIGNAL(stepByStepTargetRequested(QObject *)),
            pitchTrackerView, SLOT(slotStepByStepTargetRequested(QObject *)));

    // Encourage the notation view window to open to the same
    // interval as the current segment view.  Since scrollToTime is
    // commented out (what bug?), it made no sense to leave the
    // support code in.
//     if (m_trackEditor->getCompositionView()->horizontalScrollBar()->value() > 1) { // don't scroll unless we need to
//         // first find the time at the center of the visible segment canvas
//         int centerX = (int)(m_trackEditor->getCompositionView()->contentsX() +
//                             m_trackEditor->getCompositionView()->visibleWidth() / 2);
//         timeT centerSegmentView = m_trackEditor->getRulerScale()->getTimeForX(centerX);
//         // then scroll the notation view to that time, "localized" for the current segment
// //!!!        pitchTrackerView->scrollToTime(centerSegmentView);
// //!!!        pitchTrackerView->updateView();
//     }

    return pitchTrackerView;
}

void RosegardenMainViewWidget::slotEditSegmentMatrix(Segment* p)
{
    SetWaitCursor waitCursor;

    std::vector<Segment *> segmentsToEdit;

    if (haveSelection()) {

        SegmentSelection selection = getSelection();

        if (!p || (selection.find(p) != selection.end())) {
            for (SegmentSelection::iterator i = selection.begin();
                    i != selection.end(); ++i) {
                if ((*i)->getType() != Segment::Audio) {
                    segmentsToEdit.push_back(*i);
                }
            }
        } else {
            if (p->getType() != Segment::Audio) {
                segmentsToEdit.push_back(p);
            }
        }

    } else if (p) {
        if (p->getType() != Segment::Audio) {
            segmentsToEdit.push_back(p);
        }
    } else {
        return ;
    }

/*!!!
    // unlike notation, if we're calling for this on a particular
    // segment we don't open all the other selected segments as well
    // (fine in notation because they're in a single window)

    if (p) {
        if (p->getType() != Segment::Audio) {
            segmentsToEdit.push_back(p);
        }
    } else {
        int count = 0;
        SegmentSelection selection = getSelection();
        for (SegmentSelection::iterator i = selection.begin();
                i != selection.end(); ++i) {
            if ((*i)->getType() != Segment::Audio) {
                slotEditSegmentMatrix(*i);
                if (++count == maxEditorsToOpen)
                    break;
            }
        }
        return ;
    }
*/
    if (segmentsToEdit.empty()) {
         QMessageBox::warning(this, tr("Rosegarden"), tr("No non-audio segments selected"));
        return ;
    }

    slotEditSegmentsMatrix(segmentsToEdit);
}

void RosegardenMainViewWidget::slotEditSegmentPercussionMatrix(Segment* p)
{
    SetWaitCursor waitCursor;

    std::vector<Segment *> segmentsToEdit;

    if (haveSelection()) {

        SegmentSelection selection = getSelection();

        if (!p || (selection.find(p) != selection.end())) {
            for (SegmentSelection::iterator i = selection.begin();
                    i != selection.end(); ++i) {
                if ((*i)->getType() != Segment::Audio) {
                    segmentsToEdit.push_back(*i);
                }
            }
        } else {
            if (p->getType() != Segment::Audio) {
                segmentsToEdit.push_back(p);
            }
        }

    } else if (p) {
        if (p->getType() != Segment::Audio) {
            segmentsToEdit.push_back(p);
        }
    } else {
        return ;
    }

    if (segmentsToEdit.empty()) {
         QMessageBox::warning(this, tr("Rosegarden"), tr("No non-audio segments selected"));
        return ;
    }

    slotEditSegmentsPercussionMatrix(segmentsToEdit);
}

void RosegardenMainViewWidget::slotEditSegmentsMatrix(const std::vector<Segment *>& segmentsToEdit)
{
    createMatrixView(segmentsToEdit, false);
}

void RosegardenMainViewWidget::slotEditSegmentsPercussionMatrix(const std::vector<Segment *>& segmentsToEdit)
{
    createMatrixView(segmentsToEdit, true);
}

void
RosegardenMainViewWidget::createMatrixView(const std::vector<Segment *>& segmentsToEdit, bool drumMode)
{
    MatrixView *matrixView = new MatrixView(RosegardenDocument::currentDocument,
                                                  segmentsToEdit,
                                                  drumMode);

    connect(matrixView, &MatrixView::play,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotPlay);
    connect(matrixView, &MatrixView::stop,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotStop);
    connect(matrixView, &MatrixView::fastForwardPlayback,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotFastforward);
    connect(matrixView, &MatrixView::rewindPlayback,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotRewind);
    connect(matrixView, &MatrixView::fastForwardPlaybackToEnd,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotFastForwardToEnd);
    connect(matrixView, &MatrixView::rewindPlaybackToBeginning,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotRewindToBeginning);
    connect(matrixView, &MatrixView::panic,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotPanic);

    connect(matrixView, &EditViewBase::saveFile,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotFileSave);
    connect(matrixView, &MatrixView::openInNotation,
            this, &RosegardenMainViewWidget::slotEditSegmentsNotation);
    connect(matrixView, &MatrixView::openInMatrix,
            this, &RosegardenMainViewWidget::slotEditSegmentsMatrix);
    connect(matrixView, &MatrixView::openInPercussionMatrix,
            this, &RosegardenMainViewWidget::slotEditSegmentsPercussionMatrix);
    connect(matrixView, &MatrixView::openInEventList,
            this, &RosegardenMainViewWidget::slotEditSegmentsEventList);
    connect(matrixView, &MatrixView::editTriggerSegment,
            this, &RosegardenMainViewWidget::slotEditTriggerSegment);
    connect(matrixView, &MatrixView::openInPitchTracker,
            this, &RosegardenMainViewWidget::slotEditSegmentsPitchTracker);

    SequenceManager *sM = RosegardenDocument::currentDocument->getSequenceManager();

    connect(sM, SIGNAL(insertableNoteOnReceived(int, int)),
            matrixView, SLOT(slotInsertableNoteOnReceived(int, int)));
    connect(sM, SIGNAL(insertableNoteOffReceived(int, int)),
            matrixView, SLOT(slotInsertableNoteOffReceived(int, int)));

    connect(matrixView, &MatrixView::stepByStepTargetRequested,
            this, &RosegardenMainViewWidget::stepByStepTargetRequested);
    connect(this, SIGNAL(stepByStepTargetRequested(QObject *)),
            matrixView, SLOT(slotStepByStepTargetRequested(QObject *)));

    // Encourage the matrix view window to open to the same
    // interval as the current segment view.   Since scrollToTime is
    // commented out (what bug?), it made no sense to leave the
    // support code in.
//     if (m_trackEditor->getCompositionView()->horizontalScrollBar()->value() > 1) { // don't scroll unless we need to
//         // first find the time at the center of the visible segment canvas
//         int centerX = (int)(m_trackEditor->getCompositionView()->contentsX());
//         // Seems to work better for matrix view to scroll to left side
//         // + m_trackEditor->getCompositionView()->visibleWidth() / 2);
//         timeT centerSegmentView = m_trackEditor->getRulerScale()->getTimeForX(centerX);
//         // then scroll the view to that time, "localized" for the current segment
// //!!!        matrixView->scrollToTime(centerSegmentView);
// //!!!        matrixView->updateView();
//     }

}

void RosegardenMainViewWidget::slotEditSegmentEventList(Segment *p)
{
    SetWaitCursor waitCursor;

    std::vector<Segment *> segmentsToEdit;

    // unlike notation, if we're calling for this on a particular
    // segment we don't open all the other selected segments as well
    // (fine in notation because they're in a single window)

    if (p) {
        if (p->getType() != Segment::Audio) {
            segmentsToEdit.push_back(p);
        }
    } else {
        int count = 0;
        SegmentSelection selection = getSelection();
        for (SegmentSelection::iterator i = selection.begin();
                i != selection.end(); ++i) {
            if ((*i)->getType() != Segment::Audio) {
                slotEditSegmentEventList(*i);
                if (++count == maxEditorsToOpen)
                    break;
            }
        }
        return ;
    }

    if (segmentsToEdit.empty()) {
         QMessageBox::warning(this, tr("Rosegarden"), tr("No non-audio segments selected"));
        return ;
    }

    slotEditSegmentsEventList(segmentsToEdit);
}

void RosegardenMainViewWidget::slotEditSegmentsEventList(
        const std::vector<Segment *> &segmentsToEdit)
{
    int count = 0;
    for (std::vector<Segment *>::const_iterator segmentIter =
             segmentsToEdit.begin();
         segmentIter != segmentsToEdit.end();
         ++segmentIter) {
        EventListEditor *view = createEventView(*segmentIter);
        if (view) {
            // ??? Why does it start out hidden?
            view->show();
            if (++count == maxEditorsToOpen)
                break;
        }
    }
}

void RosegardenMainViewWidget::slotEditTriggerSegment(int id)
{
    RG_DEBUG << "slotEditTriggerSegment(): caught editTriggerSegment signal";

    SetWaitCursor waitCursor;

    std::vector<Segment *> segmentsToEdit;

    Segment *s = RosegardenDocument::currentDocument->getComposition().getTriggerSegment(id);

    if (s) {
        segmentsToEdit.push_back(s);
    } else {
        RG_WARNING << "slotEditTriggerSegment(): caught id: " << id << " and must not have been valid?";
        return ;
    }

    slotEditSegmentsEventList(segmentsToEdit);
}

/* unused
void RosegardenMainViewWidget::slotSegmentAutoSplit(Segment *segment)
{
    AudioSplitDialog aSD(this, segment, RosegardenDocument::currentDocument);

    if (aSD.exec() == QDialog::Accepted) {
        Command *command =
            new AudioSegmentAutoSplitCommand(RosegardenDocument::currentDocument,
                                             segment, aSD.getThreshold());
        slotAddCommandToHistory(command);
    }
}
*/

void RosegardenMainViewWidget::slotEditSegmentAudio(Segment *segment)
{
    RG_DEBUG << "slotEditSegmentAudio() - starting external audio editor";

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    QString application = settings.value("externalaudioeditor", "").toString();
    settings.endGroup();

    if (application == "") {
        application = AudioConfigurationPage::getBestAvailableAudioEditor();
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    QStringList splitCommand = application.split(" ", Qt::SkipEmptyParts);
#else
    QStringList splitCommand = application.split(" ", QString::SkipEmptyParts);
#endif

    if (splitCommand.size() == 0) {

        RG_WARNING << "slotEditSegmentAudio() - external editor \"" << application.data() << "\" not found";

         QMessageBox::warning(this, tr("Rosegarden"),
                           tr("You've not yet defined an audio editor for Rosegarden to use.\nSee Edit -> Preferences -> Audio."));

        return ;
    }

    QFileInfo *appInfo = new QFileInfo(splitCommand[0]);
    if (appInfo->exists() == false || appInfo->isExecutable() == false) {
        RG_WARNING << "slotEditSegmentAudio() - can't execute \"" << splitCommand[0] << "\"";
        return;
    }

    AudioFile *aF = RosegardenDocument::currentDocument->getAudioFileManager().
                    getAudioFile(segment->getAudioFileId());
    if (aF == nullptr) {
        RG_WARNING << "slotEditSegmentAudio() - can't find audio file";
        return ;
    }

    // wait cursor
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Setup the process
    //
    QProcess *process = new QProcess();
    splitCommand << aF->getAbsoluteFilePath();

    // Start it
    //
    process->start(splitCommand.takeFirst(), splitCommand);
    if (!process->waitForStarted()) {  //@@@ JAS Check here first for errors
        RG_WARNING << "slotEditSegmentAudio() - can't start external editor";
    }

    // restore cursor
    QApplication::restoreOverrideCursor();

}

void RosegardenMainViewWidget::setZoomSize(double size)
{
    double oldSize = m_rulerScale->getUnitsPerPixel();

    // For readability
    CompositionView *compositionView = m_trackEditor->getCompositionView();

    QScrollBar *horizScrollBar = compositionView->horizontalScrollBar();
    int halfWidth = lround(compositionView->viewport()->width() / 2.0);
    int oldHCenter = horizScrollBar->value() + halfWidth;

    // Set the new zoom factor
    m_rulerScale->setUnitsPerPixel(size);

#if 0
    double duration44 = TimeSignature(4, 4).getBarDuration();
    double xScale = duration44 / (size * barWidth44);
    RG_DEBUG << "setZoomSize():  xScale = " << xScale;
#endif

    // Redraw everything

    // Redraw the playback position pointer.
    // Move these lines to a new CompositionView::redrawPointer()?
    timeT pointerTime = RosegardenDocument::currentDocument->getComposition().getPosition();
    double pointerXPosition = compositionView->
        grid().getRulerScale()->getXForTime(pointerTime);
    compositionView->drawPointer(pointerXPosition);

    compositionView->deleteCachedPreviews();
    compositionView->slotUpdateSize();
    compositionView->slotUpdateAll();

    // At this point, the scroll bar's range has been updated.
    // We can now safely modify it.

    // Maintain the center of the view.
    // ??? See MatrixWidget and NotationWidget for a more extensive
    //   zoom/panner feature.
    horizScrollBar->setValue(
        (int)(oldHCenter * (oldSize / size)) - halfWidth);

    // ??? An alternate behavior is to have the zoom always center on the
    //   playback position pointer.  Might make this a user preference, or
    //   maybe when holding down "Shift" while zooming.
//    horizScrollBar->setValue(pointerXPosition - halfWidth);

    if (m_trackEditor->getTempoRuler())
        m_trackEditor->getTempoRuler()->repaint();

    if (m_trackEditor->getChordNameRuler())
        m_trackEditor->getChordNameRuler()->repaint();

    if (m_trackEditor->getTopStandardRuler())
        m_trackEditor->getTopStandardRuler()->repaint();

    if (m_trackEditor->getBottomStandardRuler())
        m_trackEditor->getBottomStandardRuler()->repaint();
}

void RosegardenMainViewWidget::slotSelectTrackSegments(int trackId)
{
    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Track *track = comp.getTrackById(trackId);

    if (track == nullptr)
        return ;

    SegmentSelection segments;

    if (QApplication::keyboardModifiers() != Qt::ShiftModifier) {

        // Shift key is not pressed :

        // Select all segments on the current track
        // (all the other segments will be deselected)
        for (Composition::iterator i =
                    RosegardenDocument::currentDocument->getComposition().begin();
                i != RosegardenDocument::currentDocument->getComposition().end(); ++i) {
            if (((int)(*i)->getTrack()) == trackId)
                segments.insert(*i);
        }

    } else {

        // Shift key is pressed :

        // Get the list of the currently selected segments
        segments = getSelection();

        // Segments on the current track will be added to or removed
        // from this list depending of the number of segments already
        // selected on this track.

        // Look for already selected segments on this track
        bool noSegSelected = true;
        for (Composition::iterator i =
                  RosegardenDocument::currentDocument->getComposition().begin();
              i != RosegardenDocument::currentDocument->getComposition().end(); ++i) {
            if (((int)(*i)->getTrack()) == trackId) {
                if (segments.count(*i)) {
                    // Segment *i is selected
                    noSegSelected = false;
                }
            }
        }

        if (!noSegSelected) {

            // Some segments are selected :
            // Deselect all selected segments on this track
            for (Composition::iterator i =
                     RosegardenDocument::currentDocument->getComposition().begin();
                 i != RosegardenDocument::currentDocument->getComposition().end(); ++i) {
                if (((int)(*i)->getTrack()) == trackId) {
                    if (segments.count(*i)) {
                        // Segment *i is selected
                        segments.erase(*i);
                    }
                }
            }


        } else {

            // There is no selected segment on this track :
            // Select all segments on this track
            for (Composition::iterator i =
                     RosegardenDocument::currentDocument->getComposition().begin();
                 i != RosegardenDocument::currentDocument->getComposition().end(); ++i) {
                if (((int)(*i)->getTrack()) == trackId) {
                    segments.insert(*i);
                }
            }
        }

    }


    // This is now handled via Composition::notifyTrackSelectionChanged()
    //m_trackEditor->getTrackButtons()->selectTrack(track->getPosition());

    // Make sure the track is visible.
    CompositionView *compositionView = m_trackEditor->getCompositionView();
    compositionView->makeTrackPosVisible(track->getPosition());

    // Store the selected Track in the Composition
    //
    comp.setSelectedTrack(trackId);


    slotPropagateSegmentSelection(segments);

    // inform
    emit segmentsSelected(segments);
}

void RosegardenMainViewWidget::slotPropagateSegmentSelection(const SegmentSelection &segments)
{
    // Send this signal to the GUI to activate the correct tool
    // on the toolbar so that we have a SegmentSelector object
    // to write the Segments into
    //
    if (!segments.empty()) {
        emit activateTool(SegmentSelector::ToolName());
    }

    // Send the segment list even if it's empty as we
    // use that to clear any current selection
    //
    m_trackEditor->getCompositionView()->selectSegments(segments);

    if (!segments.empty()) {
        emit stateChange("have_selection", true);
        if (!hasNonAudioSegment(segments)) {
            emit stateChange("audio_segment_selected", true);
        }
    } else {
        emit stateChange("have_selection", false);
    }
}

void RosegardenMainViewWidget::slotSelectAllSegments()
{
    SegmentSelection segments;

    Composition &comp = RosegardenDocument::currentDocument->getComposition();

    // For each Segment in the Composition
    for (Composition::iterator i = comp.begin(); i != comp.end(); ++i) {
        // Add this Segment to segments.
        segments.insert(*i);
    }

    // Send this signal to the GUI to activate the correct tool
    // on the toolbar so that we have a SegmentSelector object
    // to write the Segments into
    //
    if (!segments.empty()) {
        emit activateTool(SegmentSelector::ToolName());
    }

    // Send the segment list even if it's empty as we
    // use that to clear any current selection
    //
    m_trackEditor->getCompositionView()->selectSegments(segments);

    //!!! similarly, how to set no selected track?
    //comp.setSelectedTrack(trackId);

    if (!segments.empty()) {
        emit stateChange("have_selection", true);
        if (!hasNonAudioSegment(segments)) {
            emit stateChange("audio_segment_selected", true);
        }
    } else {
        emit stateChange("have_selection", false);
    }

    // inform
    //!!! inform what? is this signal actually used?
    emit segmentsSelected(segments);
}

void
RosegardenMainViewWidget::updateMeters()
{
    const int unknownState = 0, oldState = 1, newState = 2;

    typedef std::map<InstrumentId, int> StateMap;
    static StateMap states;
    static StateMap recStates;

    typedef std::map<InstrumentId, LevelInfo> LevelMap;
    static LevelMap levels;
    static LevelMap recLevels;

    for (StateMap::iterator i = states.begin(); i != states.end(); ++i) {
        i->second = unknownState;
    }
    for (StateMap::iterator i = recStates.begin(); i != recStates.end(); ++i) {
        i->second = unknownState;
    }

    for (Composition::TrackMap::iterator i =
             RosegardenDocument::currentDocument->getComposition().getTracks().begin();
         i != RosegardenDocument::currentDocument->getComposition().getTracks().end(); ++i) {

        Track *track = i->second;
        if (!track) continue;

        InstrumentId instrumentId = track->getInstrument();

        if (states[instrumentId] == unknownState) {
            bool isNew =
                SequencerDataBlock::getInstance()->getInstrumentLevel
                (instrumentId, levels[instrumentId]);
            states[instrumentId] = (isNew ? newState : oldState);
        }

        if (recStates[instrumentId] == unknownState) {
            bool isNew =
                SequencerDataBlock::getInstance()->getInstrumentRecordLevel
                (instrumentId, recLevels[instrumentId]);
            recStates[instrumentId] = (isNew ? newState : oldState);
        }

        if (states[instrumentId] == oldState &&
            recStates[instrumentId] == oldState)
            continue;

        Instrument *instrument =
            RosegardenDocument::currentDocument->getStudio().getInstrumentById(instrumentId);
        if (!instrument) continue;

        // This records the level of this instrument, not necessarily
        // caused by notes on this particular track.
        LevelInfo &info = levels[instrumentId];
        LevelInfo &recInfo = recLevels[instrumentId];

        if (instrument->getType() == Instrument::Audio ||
            instrument->getType() == Instrument::SoftSynth) {

            float dBleft = AudioLevel::DB_FLOOR;
            float dBright = AudioLevel::DB_FLOOR;
            float recDBleft = AudioLevel::DB_FLOOR;
            float recDBright = AudioLevel::DB_FLOOR;

            bool toSet = false;

            if (states[instrumentId] == newState &&
                (RosegardenDocument::currentDocument->getSequenceManager()->getTransportStatus()
                 != STOPPED)) {

                if (info.level != 0 || info.levelRight != 0) {
                    dBleft = AudioLevel::fader_to_dB
                             (info.level, 127, AudioLevel::LongFader);
                    dBright = AudioLevel::fader_to_dB
                              (info.levelRight, 127, AudioLevel::LongFader);
                }
                toSet = true;
                m_trackEditor->getTrackButtons()->slotSetTrackMeter
                ((info.level + info.levelRight) / 254.0, track->getPosition());
            }

            if (recStates[instrumentId] == newState &&
                instrument->getType() == Instrument::Audio &&
                (RosegardenDocument::currentDocument->getSequenceManager()->getTransportStatus()
                 != PLAYING)) {

                if (recInfo.level != 0 || recInfo.levelRight != 0) {
                    recDBleft = AudioLevel::fader_to_dB
                                (recInfo.level, 127, AudioLevel::LongFader);
                    recDBright = AudioLevel::fader_to_dB
                                 (recInfo.levelRight, 127, AudioLevel::LongFader);
                }
                toSet = true;
            }

            if (toSet) {
                RosegardenDocument *doc = RosegardenDocument::currentDocument;
                Composition &comp = doc->getComposition();

                InstrumentId selectedInstrumentId = comp.getSelectedInstrumentId();

                Instrument *selectedInstrument = nullptr;

                if (selectedInstrumentId != NoInstrument) {
                    // ??? Performance: LINEAR SEARCH
                    selectedInstrument =
                            doc->getStudio().getInstrumentById(selectedInstrumentId);
                }

                if (selectedInstrument  &&
                    instrument->getId() == selectedInstrument->getId()) {

                    m_instrumentParameterBox->setAudioMeter(
                            dBleft, dBright, recDBleft, recDBright);

                }
            }

        } else {
            // Not audio or softsynth
            if (info.level == 0)
                continue;

            if (RosegardenDocument::currentDocument->getSequenceManager()->getTransportStatus()
                != STOPPED) {

                // The information in 'info' is specific for this instrument, not
                //  for this track.
                //m_trackEditor->getTrackButtons()->slotSetTrackMeter
                //	(info.level / 127.0, track->getPosition());
                m_trackEditor->getTrackButtons()->slotSetMetersByInstrument
                	(info.level / 127.0, instrumentId);
            }
        }
    }

    for (StateMap::iterator i = states.begin(); i != states.end(); ++i) {
        if (i->second == newState) {
            emit instrumentLevelsChanged(i->first, levels[i->first]);
        }
    }
}

void
RosegardenMainViewWidget::updateMonitorMeters()
{
    RosegardenDocument *doc = RosegardenDocument::currentDocument;
    Composition &comp = doc->getComposition();

    InstrumentId selectedInstrumentId = comp.getSelectedInstrumentId();

    Instrument *selectedInstrument = nullptr;

    if (selectedInstrumentId != NoInstrument) {
        // ??? Performance: LINEAR SEARCH
        selectedInstrument =
                doc->getStudio().getInstrumentById(selectedInstrumentId);
    }

    if (!selectedInstrument ||
        (selectedInstrument->getType() != Instrument::Audio)) {
        return;
    }

    LevelInfo level;
    if (!SequencerDataBlock::getInstance()->
        getInstrumentRecordLevel(selectedInstrument->getId(), level)) {
        return;
    }

    float dBleft = AudioLevel::fader_to_dB
                   (level.level, 127, AudioLevel::LongFader);
    float dBright = AudioLevel::fader_to_dB
                    (level.levelRight, 127, AudioLevel::LongFader);

    m_instrumentParameterBox->setAudioMeter
    (AudioLevel::DB_FLOOR, AudioLevel::DB_FLOOR,
     dBleft, dBright);
}

void
RosegardenMainViewWidget::slotSelectedSegments(const SegmentSelection &segments)
{
    if (!segments.empty()) {
        emit stateChange("have_selection", true);
        if (!hasNonAudioSegment(segments))
            emit stateChange("audio_segment_selected", true);
    } else {
        emit stateChange("have_selection", false);
    }

    emit segmentsSelected(segments);
}

void RosegardenMainViewWidget::slotShowRulers(bool v)
{
    if (v) {
        m_trackEditor->getTopStandardRuler()->getLoopRuler()->show();
        m_trackEditor->getBottomStandardRuler()->getLoopRuler()->show();
    } else {
        m_trackEditor->getTopStandardRuler()->getLoopRuler()->hide();
        m_trackEditor->getBottomStandardRuler()->getLoopRuler()->hide();
    }
}

void RosegardenMainViewWidget::slotShowTempoRuler(bool v)
{
    if (v) {
        m_trackEditor->getTempoRuler()->show();
    } else {
        m_trackEditor->getTempoRuler()->hide();
    }
}

void RosegardenMainViewWidget::slotShowChordNameRuler(bool v)
{
    if (v) {
        m_trackEditor->getChordNameRuler()->setStudio(&RosegardenDocument::currentDocument->getStudio());
        m_trackEditor->getChordNameRuler()->show();
    } else {
        m_trackEditor->getChordNameRuler()->hide();
    }
}

void RosegardenMainViewWidget::slotShowPreviews(bool v)
{
    m_trackEditor->getCompositionView()->setShowPreviews(v);
    m_trackEditor->getCompositionView()->slotUpdateAll();
}

void RosegardenMainViewWidget::slotShowSegmentLabels(bool v)
{
    m_trackEditor->getCompositionView()->setShowSegmentLabels(v);
    m_trackEditor->getCompositionView()->slotUpdateAll();
}

void RosegardenMainViewWidget::addTrack(
        InstrumentId instrument, int position)
{
    m_trackEditor->addTrack(instrument, position);
}

void RosegardenMainViewWidget::slotDeleteTracks(
    std::vector<TrackId> tracks)
{
    RG_DEBUG << "slotDeleteTracks() - deleting " << tracks.size() << " tracks";

    m_trackEditor->deleteTracks(tracks);
}

void
RosegardenMainViewWidget::slotAddCommandToHistory(Command *command)
{
    CommandHistory::getInstance()->addCommand(command);
}

#if 0
void
RosegardenMainViewWidget::slotChangeTrackLabel(TrackId id,
                                        QString label)
{
    m_trackEditor->getTrackButtons()->changeTrackName(id, label);
}
#endif

void
RosegardenMainViewWidget::slotAddAudioSegment(AudioFileId audioId,
                                       TrackId trackId,
                                       timeT position,
                                       const RealTime &startTime,
                                       const RealTime &endTime)
{
    AudioSegmentInsertCommand *command =
        new AudioSegmentInsertCommand(RosegardenDocument::currentDocument,
                                      trackId,
                                      position,
                                      audioId,
                                      startTime,
                                      endTime);
    slotAddCommandToHistory(command);

    Segment *newSegment = command->getNewSegment();
    if (newSegment) {
        SegmentSelection selection;
        selection.insert(newSegment);
        slotPropagateSegmentSelection(selection);
        emit segmentsSelected(selection);
    }
}

/* unused
void
RosegardenMainViewWidget::slotAddAudioSegmentCurrentPosition(AudioFileId audioFileId,
        const RealTime &startTime,
        const RealTime &endTime)
{
    RG_DEBUG << "slotAddAudioSegmentCurrentPosition(...) - slot firing as ordered, sir!";

    Composition &comp = RosegardenDocument::currentDocument->getComposition();

    AudioSegmentInsertCommand *command =
        new AudioSegmentInsertCommand(RosegardenDocument::currentDocument,
                                      comp.getSelectedTrack(),
                                      comp.getPosition(),
                                      audioFileId,
                                      startTime,
                                      endTime);
    slotAddCommandToHistory(command);

    Segment *newSegment = command->getNewSegment();
    if (newSegment) {
        SegmentSelection selection;
        selection.insert(newSegment);
        slotPropagateSegmentSelection(selection);
        emit segmentsSelected(selection);
    }
}
*/

void
RosegardenMainViewWidget::slotAddAudioSegmentDefaultPosition(AudioFileId audioFileId,
        const RealTime &startTime,
        const RealTime &endTime)
{
    RG_DEBUG << "slotAddAudioSegmentDefaultPosition()...";

    // Add at current track if it's an audio track, otherwise at first
    // empty audio track if there is one, otherwise at first audio track.
    // This behaviour should be of no interest to proficient users (who
    // should have selected the right track already, or be using drag-
    // and-drop) but it should save beginners from inserting an audio
    // segment and being quite unable to work out why it won't play

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Studio &studio = RosegardenDocument::currentDocument->getStudio();

    TrackId currentTrackId = comp.getSelectedTrack();
    Track *track = comp.getTrackById(currentTrackId);

    if (track) {
        InstrumentId ii = track->getInstrument();
        Instrument *instrument = studio.getInstrumentById(ii);

        if (instrument &&
                instrument->getType() == Instrument::Audio) {
            slotAddAudioSegment(audioFileId, currentTrackId,
                                comp.getPosition(), startTime, endTime);
            return ;
        }
    }

    // current track is not an audio track, find a more suitable one

    TrackId bestSoFar = currentTrackId;

    for (Composition::TrackMap::const_iterator
            ti = comp.getTracks().begin();
            ti != comp.getTracks().end(); ++ti) {

        InstrumentId ii = ti->second->getInstrument();
        Instrument *instrument = studio.getInstrumentById(ii);

        if (instrument &&
                instrument->getType() == Instrument::Audio) {

            if (bestSoFar == currentTrackId)
                bestSoFar = ti->first;
            bool haveSegment = false;

            for (SegmentMultiSet::const_iterator si =
                        comp.getSegments().begin();
                    si != comp.getSegments().end(); ++si) {
                if ((*si)->getTrack() == ti->first) {
                    // there's a segment on this track
                    haveSegment = true;
                    break;
                }
            }

            if (!haveSegment) { // perfect
                slotAddAudioSegment(audioFileId, ti->first,
                                    comp.getPosition(), startTime, endTime);
                return ;
            }
        }
    }

    slotAddAudioSegment(audioFileId, bestSoFar,
                        comp.getPosition(), startTime, endTime);
    return ;
}

void
RosegardenMainViewWidget::slotDroppedNewAudio(QString audioDesc)
{
    // If audio is not OK
    if (RosegardenDocument::currentDocument->getSequenceManager()  &&
        !(RosegardenDocument::currentDocument->getSequenceManager()->getSoundDriverStatus() &
          AUDIO_OK)) {

#ifdef HAVE_LIBJACK
        QMessageBox::warning(this, tr("Rosegarden"),
            tr("Cannot add dropped file.  JACK audio server is not available."));
#else
        QMessageBox::warning(this, tr("Rosegarden"),
            tr("Cannot add dropped file.  This version of rosegarden was not built with audio support."));
#endif

        return;
    }

    QTextStream s(&audioDesc, QIODevice::ReadOnly);

    QString url = s.readLine();

    int trackId;
    s >> trackId;

    timeT time;
    s >> time;

    RG_DEBUG << "slotDroppedNewAudio(): url " << url << ", trackId " << trackId << ", time " << time;

    // If we don't have a valid path for audio files, bail.
    if (!RosegardenMainWindow::self()->testAudioPath(tr("importing an audio file that needs to be converted or resampled")))
        return;

    // Progress Dialog
    // Note: The label text and range will be set later as needed.
    QProgressDialog progressDialog(
            tr("Adding audio file..."),  // labelText
            tr("Cancel"),  // cancelButtonText
            0, 100,  // min, max
            RosegardenMainWindow::self());  // parent
    progressDialog.setWindowTitle(tr("Rosegarden"));
    progressDialog.setWindowModality(Qt::WindowModal);
    // Don't want to auto close since this is a multi-step
    // process.  Any of the steps may set progress to 100.  We
    // will close anyway when this object goes out of scope.
    progressDialog.setAutoClose(false);
    // Just force the progress dialog up.
    // Both Qt4 and Qt5 have bugs related to delayed showing of progress
    // dialogs.  In Qt4, the dialog sometimes won't show.  In Qt5, KDE
    // based distros might lock up.  See Bug #1546.
    progressDialog.show();

    AudioFileManager &aFM = RosegardenDocument::currentDocument->getAudioFileManager();

    aFM.setProgressDialog(&progressDialog);

    // Flush the event queue.
    qApp->processEvents(QEventLoop::AllEvents);

    AudioFileId audioFileId = 0;

    QUrl qurl(url);

    int sampleRate = 0;
    if (RosegardenDocument::currentDocument->getSequenceManager())
        sampleRate = RosegardenDocument::currentDocument->getSequenceManager()->getSampleRate();

    try {
        audioFileId = aFM.importURL(qurl, sampleRate);
    } catch (const AudioFileManager::BadAudioPathException &e) {
        QString errorString = tr("Can't add dropped file. ") + strtoqstr(e.getMessage());
        QMessageBox::warning(this, tr("Rosegarden"), errorString);
        return ;
    } catch (const SoundFile::BadSoundFileException &e) {
        QString errorString = tr("Can't add dropped file. ") + strtoqstr(e.getMessage());
        QMessageBox::warning(this, tr("Rosegarden"), errorString);
        return;
    }

    try {
        aFM.generatePreview(audioFileId);
    } catch (const Exception &e) {
        QString message = strtoqstr(e.getMessage()) + "\n\n" +
                          tr("Try copying this file to a directory where you have write permission and re-add it");
        QMessageBox::information(this, tr("Rosegarden"), message);
        return;
    }

    // add the file at the sequencer
    emit addAudioFile(audioFileId);

    // Now fetch file details
    AudioFile *aF = aFM.getAudioFile(audioFileId);

    if (aF) {
        RG_DEBUG << "slotDroppedNewAudio(file = " << url << ", trackid = " << trackId << ", time = " << time;

        // Add the audio segment to the Composition.
        slotAddAudioSegment(audioFileId, trackId, time,
                            RealTime(0, 0), aF->getLength());
    }
}

void
RosegardenMainViewWidget::slotDroppedAudio(QString audioDesc)
{
    QTextStream s(&audioDesc, QIODevice::ReadOnly);

    AudioFileId audioFileId;
    TrackId trackId;
    timeT position;
    RealTime startTime, endTime;

    // read the audio info
    s >> audioFileId;
    s >> trackId;
    s >> position;
    s >> startTime.sec;
    s >> startTime.nsec;
    s >> endTime.sec;
    s >> endTime.nsec;

    RG_DEBUG << "slotDroppedAudio("
             //<< audioDesc
             << ") : audioFileId = " << audioFileId
             << " - trackId = " << trackId
             << " - position = " << position
             << " - startTime.sec = " << startTime.sec
             << " - startTime.nsec = " << startTime.nsec
             << " - endTime.sec = " << endTime.sec
             << " - endTime.nsec = " << endTime.nsec;

    slotAddAudioSegment(audioFileId, trackId, position, startTime, endTime);
}

/* unused
void
RosegardenMainViewWidget::slotSetRecord(InstrumentId id, bool value)
{
    RG_DEBUG << "slotSetRecord() - id = " << id << ",value = " << value;
*//*
        // IPB
        //
        m_instrumentParameterBox->setRecord(value);
    */
    /* #ifdef NOT_DEFINED
    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Composition::TrackMap &tracks = comp.getTracks();
    Composition::TrackMap::iterator it;

    for (it = tracks.begin(); it != tracks.end(); ++it) {
        if (comp.getSelectedTrack() == (*it).second->getId()) {
            //!!! MTR            m_trackEditor->getTrackButtons()->
            //                setRecordTrack((*it).second->getPosition());
        }
    }
#endif
    // Studio &studio = RosegardenDocument::currentDocument->getStudio();
    // Instrument *instr = studio.getInstrumentById(id);
}
*/

/* unused
void
RosegardenMainViewWidget::slotSetSolo(InstrumentId id, bool value)
{
    RG_DEBUG << "slotSetSolo() - " << "id = " << id << ",value = " << value;

    emit toggleSolo(value);
}
*/

/* unused
void
RosegardenMainViewWidget::slotUpdateRecordingSegment(Segment *segment,
        timeT )
{
    // We're only interested in this on the first call per recording segment,
    // when we possibly create a view for it
    static Segment *lastRecordingSegment = nullptr;

    if (segment == lastRecordingSegment)
        return ;
    lastRecordingSegment = segment;

    QSettings settings;
    settings.beginGroup( GeneralOptionsConfigGroup );

    int tracking = settings.value("recordtracking", 0).toUInt() ;
    settings.endGroup();

    if (tracking == 0)
        return ;

    RG_DEBUG << "slotUpdateRecordingSegment(): segment is " << segment << ", lastRecordingSegment is " << lastRecordingSegment << ", opening a new view";

    std::vector<Segment *> segments;
    segments.push_back(segment);

    createNotationView(segments);
}
*/

void
RosegardenMainViewWidget::slotSynchroniseWithComposition()
{
    // Track buttons
    //
    m_trackEditor->getTrackButtons()->slotSynchroniseWithComposition();
}

void
RosegardenMainViewWidget::slotExternalController(const MappedEvent *event)
{
    //RG_DEBUG << "slotExternalController()...";

    activateWindow();
    raise();

    //RG_DEBUG << "  Event is type: " << int(e->getType()) << ", channel " << int(e->getRecordedChannel()) << ", data1 " << int(e->getData1()) << ", data2 " << int(e->getData2());

    Composition &comp = RosegardenDocument::currentDocument->getComposition();
    Studio &studio = RosegardenDocument::currentDocument->getStudio();

    // Program Change?  Adjust Track PC to match.
    // ??? This is odd.  I don't think control surfaces typically provide
    //     a Program Change control.  That's more a keyboard thing.
    //     And if we handle this, we should also handle Bank Select.
    //     But we don't.  I suspect this can be removed.
    if (event->getType() == MappedEvent::MidiProgramChange) {
        const Track *track = comp.getTrackById(comp.getSelectedTrack());
        if (!track)
            return;

        Instrument *instrument =
                studio.getInstrumentById(track->getInstrument());
        if (!instrument)
            return;

        instrument->setProgramChange(event->getData1());
        instrument->sendChannelSetup();
        RosegardenDocument::currentDocument->slotDocumentModified();

        return;
    }

    if (event->getType() != MappedEvent::MidiController)
        return;

    // We have a CC coming in...

    const MidiByte controlNumber = event->getData1();
    const MidiByte value = event->getData2();

    // CONTROLLER_SELECT_TRACK
    if (controlNumber == 82) {
        const unsigned tracks = comp.getNbTracks();

        // Get the track to select based on CC value.
        Track *track = comp.getTrackByPosition(value * tracks / 128);
        if (!track)
            return;

        comp.setSelectedTrack(track->getId());
        comp.notifyTrackSelectionChanged(track->getId());

        slotSelectTrackSegments(track->getId());

        // Emit a documentModified(), but don't set the document
        // modified flag.  This refreshes the UI but doesn't make
        // the star appear in the titlebar.
        RosegardenDocument::currentDocument->emitDocumentModified();

        return;
    }

    const Track *track = comp.getTrackById(comp.getSelectedTrack());
    if (!track)
        return;

    Instrument *instrument = studio.getInstrumentById(track->getInstrument());
    if (!instrument)
        return;

    if (instrument->getType() == Instrument::Midi) {
        // If the incoming control number is valid for the Instrument...
        if (instrument->hasController(controlNumber)) {
            // Adjust the Instrument and update everyone.
            instrument->setControllerValue(controlNumber, value);
            Instrument::emitControlChange(instrument, controlNumber);
            RosegardenDocument::currentDocument->setModified();
        }

        return;
    }

    // We have an audio instrument or a softsynth...

    if (controlNumber == MIDI_CONTROLLER_VOLUME) {
        //RG_DEBUG << "  Setting volume for instrument " << instrument->getId() << " to " << value;

        const float dB = AudioLevel::fader_to_dB(
                value, 127, AudioLevel::ShortFader);

        instrument->setLevel(dB);
        Instrument::emitControlChange(instrument, MIDI_CONTROLLER_VOLUME);
        RosegardenDocument::currentDocument->setModified();

        return;
    }

    if (controlNumber == MIDI_CONTROLLER_PAN) {
        //RG_DEBUG << "  Setting pan for instrument " << instrument->getId() << " to " << value;

        instrument->setControllerValue(
                MIDI_CONTROLLER_PAN,
                AudioLevel::AudioPanI(value));
        Instrument::emitControlChange(instrument, MIDI_CONTROLLER_PAN);
        RosegardenDocument::currentDocument->setModified();

        return;
    }
}

void
RosegardenMainViewWidget::initChordNameRuler()
{
    getTrackEditor()->getChordNameRuler()->setReady();
}

EventListEditor *
RosegardenMainViewWidget::createEventView(Segment *segment)
{
    // EventListEditor expects a vector because of EditViewBase.
    std::vector<Segment *> segments;
    segments.push_back(segment);

    EventListEditor *eventView = new EventListEditor(
            RosegardenDocument::currentDocument, segments);

    connect(eventView, &EditViewBase::saveFile,
            RosegardenMainWindow::self(), &RosegardenMainWindow::slotFileSave);
    connect(eventView, &EditViewBase::openInNotation,
            this, &RosegardenMainViewWidget::slotEditSegmentsNotation);
    connect(eventView, &EditViewBase::openInMatrix,
            this, &RosegardenMainViewWidget::slotEditSegmentsMatrix);
    connect(eventView, &EditViewBase::openInPercussionMatrix,
            this, &RosegardenMainViewWidget::slotEditSegmentsPercussionMatrix);
    connect(eventView, &EditViewBase::openInEventList,
            this, &RosegardenMainViewWidget::slotEditSegmentsEventList);
    connect(eventView, &EditViewBase::openInPitchTracker,
            this, &RosegardenMainViewWidget::slotEditSegmentsPitchTracker);
    connect(eventView, &EventListEditor::editTriggerSegment,
            this, &RosegardenMainViewWidget::slotEditTriggerSegment);

    return eventView;
}

bool RosegardenMainViewWidget::hasNonAudioSegment(const SegmentSelection &segments)
{
    for (SegmentSelection::const_iterator i = segments.begin();
         i != segments.end();
         ++i) {
        if ((*i)->getType() == Segment::Internal)
            return true;
    }
    return false;
}


}
