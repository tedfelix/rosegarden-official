/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SegmentResizer]"

#include "SegmentResizer.h"

#include "base/Event.h"
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Track.h"
#include "base/SnapGrid.h"
#include "commands/segment/AudioSegmentResizeFromStartCommand.h"
#include "commands/segment/AudioSegmentRescaleCommand.h"
#include "commands/segment/SegmentRescaleCommand.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "commands/segment/SegmentResizeFromStartCommand.h"
#include "commands/segment/SegmentLinkToCopyCommand.h"
#include "CompositionItemHelper.h"
#include "CompositionModelImpl.h"
#include "CompositionView.h"
#include "document/RosegardenDocument.h"
#include "gui/general/BaseTool.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/general/RosegardenScrollView.h"
#include "gui/widgets/ProgressDialog.h"
#include "SegmentTool.h"
#include "document/Command.h"
#include <QMessageBox>
#include <QCursor>
#include <QEvent>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QMouseEvent>


namespace Rosegarden
{


const QString SegmentResizer::ToolName = "segmentresizer";

SegmentResizer::SegmentResizer(CompositionView *c, RosegardenDocument *d,
                               int edgeThreshold)
        : SegmentTool(c, d),
        m_edgeThreshold(edgeThreshold)
{
    RG_DEBUG << "SegmentResizer()\n";
}

void SegmentResizer::ready()
{
    m_canvas->viewport()->setCursor(Qt::SizeHorCursor);
    setBasicContextHelp(false);
}

void SegmentResizer::stow()
{
}

void SegmentResizer::mousePressEvent(QMouseEvent *e)
{
    RG_DEBUG << "SegmentResizer::mousePressEvent" << endl;

    // Let the baseclass have a go.
    SegmentTool::mousePressEvent(e);

    // We only care about the left mouse button.
    if (e->button() != Qt::LeftButton)
        return;

    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    CompositionItemPtr item = m_canvas->getModel()->getSegmentAt(pos);

    if (item) {
        RG_DEBUG << "SegmentResizer::mousePressEvent - got item" << endl;
        setCurrentIndex(item);

        // Are we resizing from start or end?
        if (item->rect().x() + item->rect().width() / 2 > pos.x()) {
            m_resizeStart = true;
        } else {
            m_resizeStart = false;
        }

        m_canvas->getModel()->startChange(item, 
            m_resizeStart ? CompositionModelImpl::ChangeResizeFromStart :
                            CompositionModelImpl::ChangeResizeFromEnd);

        setSnapTime(e, SnapGrid::SnapToBeat);
    }
}

void SegmentResizer::mouseReleaseEvent(QMouseEvent *e)
{
    //RG_DEBUG << "mouseReleaseEvent()";

    // We only care about the left mouse button.
    if (e->button() != Qt::LeftButton)
        return;

    // No need to propagate.
    e->accept();

    bool rescale = (e->modifiers() & Qt::ControlModifier);

    if (m_currentIndex) {

        Segment* segment = m_currentIndex->getSegment();

        // We only want to snap the end that we were actually resizing.

        timeT oldStartTime, oldEndTime;
        
        oldStartTime = segment->getStartTime();
        oldEndTime = segment->getEndMarkerTime(false);

        timeT newStartTime, newEndTime;

        if (m_resizeStart) {
            newStartTime = m_currentIndex->getStartTime(m_canvas->grid());
            newEndTime = oldEndTime;
        } else {
            newEndTime = CompositionItemHelper::getEndTime
                         (m_currentIndex, m_canvas->grid());
            newStartTime = oldStartTime;
        }

        // If something has changed
        if (newStartTime != oldStartTime  ||  newEndTime != oldEndTime) {
                
            if (newStartTime > newEndTime) std::swap(newStartTime, newEndTime);

            if (rescale) {

                if (segment->getType() == Segment::Audio) {

                    try {
                        m_doc->getAudioFileManager().testAudioPath();
                    } catch (AudioFileManager::BadAudioPathException) {
                        if (QMessageBox::warning(0, tr("Warning"), //tr("Set audio file path"), 
                                 tr("The audio file path does not exist or is not writable.\nYou must set the audio file path to a valid directory in Document Properties before rescaling an audio file.\nWould you like to set it now?"),
                                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel
                                ) 
                                == QMessageBox::Yes ) {
                            RosegardenMainWindow::self()->slotOpenAudioPathSettings();
                        }
                    }

                    float ratio = float(newEndTime - newStartTime) /
                        float(oldEndTime - oldStartTime);

                    AudioSegmentRescaleCommand *command =
                        new AudioSegmentRescaleCommand(m_doc, segment, ratio,
                                                       newStartTime, newEndTime);

                    //cc 20150508: avoid dereferencing self-deleted
                    //progress dialog after user has closed it, by
                    //using a QPointer
                    QPointer<ProgressDialog> progressDlg = new ProgressDialog(
                            tr("Rescaling audio file..."), (QWidget*)parent());
                    command->connectProgressDialog(progressDlg);
                    
                    addCommandToHistory(command);

                    if (progressDlg) {
                        command->disconnectProgressDialog(progressDlg);
                        progressDlg->close();
                    }
                    
                    progressDlg = new ProgressDialog(tr("Generating audio preview..."),
                                                     (QWidget*)parent());

                    connect(&m_doc->getAudioFileManager(), SIGNAL(setValue(int)),
                            progressDlg, SLOT(setValue(int)));
                    // Removed since ProgressDialog::cancelClicked() does not exist.
                    //connect(progressDlg, SIGNAL(cancelClicked()),
                    //        &m_doc->getAudioFileManager(), SLOT(slotStopPreview()));

                    int fid = command->getNewAudioFileId();
                    if (fid >= 0) {
                        RosegardenMainWindow::self()->slotAddAudioFile(fid);
                        m_doc->getAudioFileManager().generatePreview(fid);
                    }

                    if (progressDlg) progressDlg->close();
                
                } else {
                    
                    SegmentRescaleCommand *command =
                        new SegmentRescaleCommand(segment,
                                                  newEndTime - newStartTime,
                                                  oldEndTime - oldStartTime,
                                                  newStartTime);
                    addCommandToHistory(command);
                }
            } else {

                if (m_resizeStart) {

                    if (segment->getType() == Segment::Audio) {
                        addCommandToHistory(new AudioSegmentResizeFromStartCommand
                                            (segment, newStartTime));
                    } else {
                        SegmentLinkToCopyCommand* unlinkCmd = 
                                         new SegmentLinkToCopyCommand(segment);
                        SegmentResizeFromStartCommand* resizeCmd = 
                        new SegmentResizeFromStartCommand(segment, newStartTime);
                        
                        MacroCommand* command = new MacroCommand(
                            SegmentResizeFromStartCommand::getGlobalName());
                       
                        command->addCommand(unlinkCmd);
                        command->addCommand(resizeCmd);
                        
                        addCommandToHistory(command);
                    }

                } else {

                    Composition &comp = m_doc->getComposition();

                    SegmentReconfigureCommand *command =
                        new SegmentReconfigureCommand(tr("Resize Segment"), &comp);

                    int trackPos = CompositionItemHelper::getTrackPos
                        (m_currentIndex, m_canvas->grid());

                    Track *track = comp.getTrackByPosition(trackPos);

                    command->addSegment(segment,
                                        newStartTime,
                                        newEndTime,
                                        track->getId());
                    addCommandToHistory(command);
                }
            }
        }
    }

    m_canvas->getModel()->endChange();
//     m_canvas->updateContents();
    m_canvas->update();
    
    setChangeMade(false);
    m_currentIndex = CompositionItemPtr();
    setBasicContextHelp();
}

int SegmentResizer::mouseMoveEvent(QMouseEvent *e)
{
    //     RG_DEBUG << "SegmentResizer::mouseMoveEvent" << endl;

    // No need to propagate.
    e->accept();

    QPoint pos = m_canvas->viewportToContents(e->pos());

    bool rescale = (e->modifiers() & Qt::ControlModifier);

    if (!m_currentIndex) {
        setBasicContextHelp(rescale);
        return RosegardenScrollView::NoFollow;
    }

    if (rescale) {
        // If shift isn't being held down
        if ((e->modifiers() & Qt::ShiftModifier) == 0) {
            setContextHelp(tr("Hold Shift to avoid snapping to beat grid"));
        } else {
            clearContextHelp();
        }
    } else {
        // If shift isn't being held down
        if ((e->modifiers() & Qt::ShiftModifier) == 0) {
            setContextHelp(tr("Hold Shift to avoid snapping to beat grid; hold Ctrl as well to rescale contents"));
        } else {
            setContextHelp(tr("Hold Ctrl to rescale contents"));
        }
    }

    Segment* segment = m_currentIndex->getSegment();

    // Don't allow Audio segments to resize yet
    //
    /*!!!
        if (segment->getType() == Segment::Audio)
        {
            m_currentIndex = CompositionItemPtr();
            QMessageBox::information(m_canvas,
                    tr("You can't yet resize an audio segment!"));
            return RosegardenScrollView::NoFollow;
        }
    */

    QRect oldRect = m_currentIndex->rect();

    setSnapTime(e, SnapGrid::SnapToBeat);

    // Convert X coord to time
    timeT time = m_canvas->grid().snapX(pos.x());

    // Get the "snap size" of the grid at the current X coord.  It can change
    // with certain snap modes and different time signatures.
    // ??? rename getSnapTime() -> getSnapTimeForX()
    timeT snapSize = m_canvas->grid().getSnapTime(double(pos.x()));

    // If snap to grid is off
    if (snapSize == 0) {
        // Use the shortest note duration.
        snapSize = Note(Note::Shortest).getDuration();
    }

    if (m_resizeStart) {

        timeT itemEndTime = segment->getEndMarkerTime();

        timeT duration = itemEndTime - time;

        //         RG_DEBUG << "SegmentResizer::mouseMoveEvent() resize start : duration = "
        //                  << duration << " - snap = " << snapSize
        //                  << " - itemEndTime : " << itemEndTime
        //                  << " - time : " << time
        //                  << endl;

        timeT newStartTime = time;

        if (duration < snapSize) {
        
            // Make sure the segment can never be smaller than the snap size.
            newStartTime = itemEndTime - snapSize;

        }

        // Change the size of the segment on the canvas.
        m_currentIndex->setStartTime(newStartTime, m_canvas->grid());

    } else { // resize end

        timeT itemStartTime = segment->getStartTime();

        timeT duration = time - itemStartTime;

        timeT newEndTime = time;

        //         RG_DEBUG << "SegmentResizer::mouseMoveEvent() resize end : duration = "
        //                  << duration << " - snap = " << snapSize
        //                  << " - itemStartTime : " << itemStartTime
        //                  << " - time : " << time
        //                  << endl;

        if (duration < snapSize) {

            // Make sure the segment can't be resized smaller than the snap
            // size.
            newEndTime = itemStartTime + snapSize;

        }

        // Change the size of the segment on the canvas.
        CompositionItemHelper::setEndTime(m_currentIndex,
                                          newEndTime,
                                          m_canvas->grid());
    }

    // Redraw the canvas
    m_canvas->slotAllNeedRefresh(m_currentIndex->rect() | oldRect);

    return RosegardenScrollView::FollowHorizontal;
}

bool SegmentResizer::cursorIsCloseEnoughToEdge(CompositionItemPtr p, const QPoint &coord,
        int edgeThreshold, bool &start)
{
    if (abs(p->rect().x() + p->rect().width() - coord.x()) < edgeThreshold) {
        start = false;
        return true;
    } else if (abs(p->rect().x() - coord.x()) < edgeThreshold) {
        start = true;
        return true;
    } else {
        return false;
    }
}

void SegmentResizer::setBasicContextHelp(bool ctrlPressed)
{
    if (ctrlPressed) {
        setContextHelp(tr("Click and drag to resize a segment; hold Ctrl as well to rescale its contents"));
    } else {
        setContextHelp(tr("Click and drag to rescale segment"));
    }        
}    


}
#include "SegmentResizer.moc"
