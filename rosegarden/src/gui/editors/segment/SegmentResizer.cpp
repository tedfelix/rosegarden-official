/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "SegmentResizer.h"

#include "base/Event.h"
#include <klocale.h>
#include "misc/Debug.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/Segment.h"
#include "base/Track.h"
#include "base/SnapGrid.h"
#include "commands/segment/AudioSegmentResizeFromStartCommand.h"
#include "commands/segment/AudioSegmentRescaleCommand.h"
#include "commands/segment/SegmentReconfigureCommand.h"
#include "commands/segment/SegmentResizeFromStartCommand.h"
#include "CompositionItemHelper.h"
#include "CompositionModel.h"
#include "CompositionView.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/general/BaseTool.h"
#include "gui/general/RosegardenCanvasView.h"
#include "gui/widgets/ProgressDialog.h"
#include "SegmentTool.h"
#include <kcommand.h>
#include <kmessagebox.h>
#include <qcursor.h>
#include <qevent.h>
#include <qpoint.h>
#include <qrect.h>
#include <qstring.h>


namespace Rosegarden
{

SegmentResizer::SegmentResizer(CompositionView *c, RosegardenGUIDoc *d,
                               int edgeThreshold)
        : SegmentTool(c, d),
        m_edgeThreshold(edgeThreshold)
{
    RG_DEBUG << "SegmentResizer()\n";
}

void SegmentResizer::ready()
{
    m_canvas->viewport()->setCursor(Qt::sizeHorCursor);
    connect(m_canvas, SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotCanvasScrolled(int, int)));
    setBasicContextHelp();
}

void SegmentResizer::stow()
{
    disconnect(m_canvas, SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotCanvasScrolled(int, int)));
}

void SegmentResizer::slotCanvasScrolled(int newX, int newY)
{
    QMouseEvent tmpEvent(QEvent::MouseMove,
                         m_canvas->viewport()->mapFromGlobal(QCursor::pos()) + QPoint(newX, newY),
                         Qt::NoButton, Qt::NoButton);
    handleMouseMove(&tmpEvent);
}

void SegmentResizer::handleMouseButtonPress(QMouseEvent *e)
{
    RG_DEBUG << "SegmentResizer::handleMouseButtonPress" << endl;
    m_canvas->getModel()->clearSelected();

    CompositionItem item = m_canvas->getFirstItemAt(e->pos());

    if (item) {
        RG_DEBUG << "SegmentResizer::handleMouseButtonPress - got item" << endl;
        setCurrentItem(item);

        // Are we resizing from start or end?
        if (item->rect().x() + item->rect().width() / 2 > e->pos().x()) {
            m_resizeStart = true;
        } else {
            m_resizeStart = false;
        }

        m_canvas->getModel()->startChange(item, m_resizeStart ? CompositionModel::ChangeResizeFromStart : CompositionModel::ChangeResizeFromEnd);

    }
}

void SegmentResizer::handleMouseButtonRelease(QMouseEvent *e)
{
    RG_DEBUG << "SegmentResizer::handleMouseButtonRelease" << endl;

    bool rescale = (e->state() & Qt::ControlButton);

    if (m_currentItem) {

        Segment* segment = CompositionItemHelper::getSegment(m_currentItem);

        // We only want to snap the end that we were actually resizing.

        timeT oldStartTime, oldEndTime;
        
        oldStartTime = segment->getStartTime();
        oldEndTime = segment->getEndMarkerTime();

        timeT newStartTime, newEndTime;

        if (m_resizeStart) {
            newStartTime = CompositionItemHelper::getStartTime
                           (m_currentItem, m_canvas->grid());
            newEndTime = oldEndTime;
        } else {
            newEndTime = CompositionItemHelper::getEndTime
                         (m_currentItem, m_canvas->grid());
            newStartTime = oldStartTime;
        }

        if (changeMade()) {

            if (m_resizeStart && (newStartTime < newEndTime)) {

                //!!! deal with rescale

                if (segment->getType() == Segment::Audio) {
                    addCommandToHistory(new AudioSegmentResizeFromStartCommand(segment, newStartTime));
                } else {
                    addCommandToHistory(new SegmentResizeFromStartCommand(segment, newStartTime));
                }

            } else if (rescale) {

                if (segment->getType() == Segment::Audio) {

                    //!!! too much stuff to be here

                    float ratio = float(newEndTime - newStartTime) /
                        float(oldEndTime - oldStartTime);

                    AudioSegmentRescaleCommand *command =
                        new AudioSegmentRescaleCommand(m_doc, segment, ratio);

                    ProgressDialog progressDlg
                        (i18n("Rescaling audio file..."), 100, 0);
                    progressDlg.setAutoClose(false);
                    progressDlg.setAutoReset(false);
                    progressDlg.show();

                    command->connectProgressDialog(&progressDlg);
                    
                    addCommandToHistory(command);

                    progressDlg.setLabel(i18n("Generating audio preview..."));

                    command->disconnectProgressDialog(&progressDlg);

                    connect(&m_doc->getAudioFileManager(), SIGNAL(setProgress(int)),
                            progressDlg.progressBar(), SLOT(setValue(int)));
                    connect(&progressDlg, SIGNAL(cancelClicked()),
                            &m_doc->getAudioFileManager(), SLOT(slotStopPreview()));

                    int fid = command->getNewAudioFileId();
                    if (fid >= 0) {
                        RosegardenGUIApp::self()->slotAddAudioFile(fid);
                        m_doc->getAudioFileManager().generatePreview(fid);
                    }

                } else {
                    
                    //!!! handle non-audio rescale

                }

            } else {

                SegmentReconfigureCommand *command =
                    new SegmentReconfigureCommand("Resize Segment");

                int trackPos = CompositionItemHelper::getTrackPos(m_currentItem, m_canvas->grid());

                Composition &comp = m_doc->getComposition();
                Track *track = comp.getTrackByPosition(trackPos);

                command->addSegment(segment,
                                    newStartTime,
                                    newEndTime,
                                    track->getId());
                addCommandToHistory(command);
            }
        }
    }

    m_canvas->getModel()->endChange();
    m_canvas->updateContents();
    setChangeMade(false);
    m_currentItem = CompositionItem();
    setBasicContextHelp();
}

int SegmentResizer::handleMouseMove(QMouseEvent *e)
{
    //     RG_DEBUG << "SegmentResizer::handleMouseMove" << endl;

    if (!m_currentItem) {
        setBasicContextHelp();
        return RosegardenCanvasView::NoFollow;
    }

    bool rescale = (e->state() & Qt::ControlButton);

    if (rescale) {
        setContextHelp(i18n("Hold Shift to avoid snapping to beat grid"));
    } else {
        setContextHelp(i18n("Hold Shift to avoid snapping to beat grid; hold Ctrl as well to rescale contents"));
    }

    Segment* segment = CompositionItemHelper::getSegment(m_currentItem);

    // Don't allow Audio segments to resize yet
    //
    /*!!!
        if (segment->getType() == Segment::Audio)
        {
            m_currentItem = CompositionItem();
            KMessageBox::information(m_canvas,
                    i18n("You can't yet resize an audio segment!"));
            return RosegardenCanvasView::NoFollow;
        }
    */

    QRect oldRect = m_currentItem->rect();

    m_canvas->setSnapGrain(true);

    timeT time = m_canvas->grid().snapX(e->pos().x());
    timeT snap = m_canvas->grid().getSnapTime(double(e->pos().x()));
    if (snap == 0)
        snap = Note(Note::Shortest).getDuration();

    // We only want to snap the end that we were actually resizing.

    timeT itemStartTime, itemEndTime;

    if (m_resizeStart) {
        itemStartTime = CompositionItemHelper::getStartTime
                        (m_currentItem, m_canvas->grid());
        itemEndTime = segment->getEndMarkerTime();
    } else {
        itemEndTime = CompositionItemHelper::getEndTime
                      (m_currentItem, m_canvas->grid());
        itemStartTime = segment->getStartTime();
    }

    timeT duration = 0;

    if (m_resizeStart) {

        duration = itemEndTime - time;
        //         RG_DEBUG << "SegmentResizer::handleMouseMove() resize start : duration = "
        //                  << duration << " - snap = " << snap
        //                  << " - itemEndTime : " << itemEndTime
        //                  << " - time : " << time
        //                  << endl;

        timeT newStartTime = 0;

        if ((duration > 0 && duration < snap) ||
                (duration < 0 && duration > -snap)) {

            newStartTime = itemEndTime - (duration < 0 ? -snap : snap);

        } else {

            newStartTime = itemEndTime - duration;

        }

        CompositionItemHelper::setStartTime(m_currentItem,
                                            newStartTime,
                                            m_canvas->grid());
    } else { // resize end

        duration = time - itemStartTime;

        timeT newEndTime = 0;

        //         RG_DEBUG << "SegmentResizer::handleMouseMove() resize end : duration = "
        //                  << duration << " - snap = " << snap
        //                  << " - itemEndTime : " << itemEndTime
        //                  << " - time : " << time
        //                  << endl;

        if ((duration > 0 && duration < snap) ||
                (duration < 0 && duration > -snap)) {

            newEndTime = (duration < 0 ? -snap : snap) + itemStartTime;

        } else {

            newEndTime = duration + itemStartTime;

        }

        CompositionItemHelper::setEndTime(m_currentItem,
                                          newEndTime,
                                          m_canvas->grid());
    }

    if (duration != 0)
        setChangeMade(true);

    m_canvas->slotUpdateSegmentsDrawBuffer(m_currentItem->rect() | oldRect);

    return RosegardenCanvasView::FollowHorizontal;
}

bool SegmentResizer::cursorIsCloseEnoughToEdge(const CompositionItem& p, const QPoint &coord,
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

void SegmentResizer::setBasicContextHelp()
{
    setContextHelp(i18n("Click and drag to resize a segment; hold Ctrl as well to rescale its contents"));
}    

const QString SegmentResizer::ToolName  = "segmentresizer";

}
#include "SegmentResizer.moc"
