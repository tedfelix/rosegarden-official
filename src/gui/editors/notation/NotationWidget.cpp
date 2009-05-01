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

#include "NotationWidget.h"

#include "NotationScene.h"
#include "NotationToolBox.h"
#include "NoteInserter.h"
#include "NotationMouseEvent.h"

#include <QGraphicsView>
#include <QGridLayout>
#include <QScrollBar>

#include "base/RulerScale.h"

#include "document/RosegardenDocument.h"

#include "gui/widgets/Panner.h"
#include "gui/widgets/Panned.h"

#include "misc/Debug.h"
#include "misc/Strings.h"

namespace Rosegarden
{

NotationWidget::NotationWidget() :
    m_document(0),
    m_view(0),
    m_scene(0),
    m_hZoomFactor(1),
    m_vZoomFactor(1),
    m_referenceScale(0)
{
    QGridLayout *layout = new QGridLayout;
    setLayout(layout);

    m_view = new Panned;
    m_view->setBackgroundBrush(Qt::white);
    layout->addWidget(m_view, 0, 0);

    m_hpanner = new Panner;
    m_hpanner->setMaximumHeight(80);
    m_hpanner->setBackgroundBrush(Qt::white);
//    m_hpanner->setRenderHints(QPainter::TextAntialiasing |
//                              QPainter::SmoothPixmapTransform);
    layout->addWidget(m_hpanner, 1, 0);

    connect(m_view, SIGNAL(pannedRectChanged(QRectF)),
            m_hpanner, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannedRectChanged(QRectF)),
            m_view, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(zoomIn()),
            this, SLOT(slotZoomInFromPanner()));

    connect(m_hpanner, SIGNAL(zoomOut()),
            this, SLOT(slotZoomOutFromPanner()));

    m_toolBox = new NotationToolBox(this);
    
    //!!! 
    NoteInserter *noteInserter = dynamic_cast<NoteInserter *>
        (m_toolBox->getTool(NoteInserter::ToolName));
    noteInserter->slotSetNote(Note::Crotchet);
    noteInserter->slotSetDots(0);
    m_currentTool = noteInserter;
    m_currentTool->ready();
}

NotationWidget::~NotationWidget()
{
    delete m_scene;
}

void
NotationWidget::setSegments(RosegardenDocument *document,
                            std::vector<Segment *> segments)
{
    m_document = document;

    delete m_referenceScale;

    delete m_scene;
    m_scene = new NotationScene();
    m_scene->setNotationWidget(this);
    m_scene->setStaffs(document, segments);

    m_referenceScale = new ZoomableRulerScale(m_scene->getRulerScale());

    connect(m_scene, SIGNAL(mousePressed(const NotationMouseEvent *)),
            this, SLOT(slotDispatchMousePress(const NotationMouseEvent *)));

    connect(m_scene, SIGNAL(mouseMoved(const NotationMouseEvent *)),
            this, SLOT(slotDispatchMouseMove(const NotationMouseEvent *)));

    connect(m_scene, SIGNAL(mouseReleased(const NotationMouseEvent *)),
            this, SLOT(slotDispatchMouseRelease(const NotationMouseEvent *)));

    connect(m_scene, SIGNAL(mouseDoubleClicked(const NotationMouseEvent *)),
            this, SLOT(slotDispatchMouseDoubleClick(const NotationMouseEvent *)));

    m_view->setScene(m_scene);

    m_toolBox->setScene(m_scene);

    m_hpanner->setScene(m_scene);
    m_hpanner->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void
NotationWidget::setCanvasCursor(QCursor c)
{
    if (m_view) m_view->setCursor(c);
}

Segment *
NotationWidget::getCurrentSegment()
{
    if (!m_scene) return 0;
    return m_scene->getCurrentSegment();
}

void
NotationWidget::slotDispatchMousePress(const NotationMouseEvent *e)
{
    if (e->buttons & Qt::LeftButton) {
        if (e->modifiers & Qt::ControlModifier) {
            if (m_scene) m_scene->slotSetInsertCursorPosition(e->time, true, true); //!!!
            return;
        }
    }

    if (!m_currentTool) return;

    //!!! todo: handle equivalents of NotationView::slotXXXItemPressed

    if (e->buttons & Qt::LeftButton) {
        m_currentTool->handleLeftButtonPress(e);
    } else if (e->buttons & Qt::MidButton) {
        m_currentTool->handleMidButtonPress(e);
    } else if (e->buttons & Qt::RightButton) {
        m_currentTool->handleRightButtonPress(e);
    }
}

void
NotationWidget::slotDispatchMouseMove(const NotationMouseEvent *e)
{
    if (!m_currentTool) return;
    NotationTool::FollowMode mode = m_currentTool->handleMouseMove(e);
    
    /*!!!
if (getCanvasView()->isTimeForSmoothScroll()) {

            if (follow & RosegardenCanvasView::FollowHorizontal) {
                getCanvasView()->slotScrollHorizSmallSteps(e->x());
            }

            if (follow & RosegardenCanvasView::FollowVertical) {
                getCanvasView()->slotScrollVertSmallSteps(e->y());
            }

        }
    }
    */
}

void
NotationWidget::slotDispatchMouseRelease(const NotationMouseEvent *e)
{
    if (!m_currentTool) return;
    m_currentTool->handleMouseRelease(e);
}

void
NotationWidget::slotDispatchMouseDoubleClick(const NotationMouseEvent *e)
{
    if (!m_currentTool) return;
    m_currentTool->handleMouseDoubleClick(e);
}

EventSelection *
NotationWidget::getSelection() const
{
    if (m_scene) return m_scene->getSelection();
    else return 0;
}

void
NotationWidget::setSelection(EventSelection *selection, bool preview)
{
    if (m_scene) m_scene->setSelection(selection, preview);
}

void
NotationWidget::slotZoomInFromPanner() 
{
    m_hZoomFactor /= 1.1;
    m_vZoomFactor /= 1.1;
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
}

void
NotationWidget::slotZoomOutFromPanner() 
{
    m_hZoomFactor *= 1.1;
    m_vZoomFactor *= 1.1;
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
}

}

#include "NotationWidget.moc"

