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

#include "MatrixWidget.h"

#include "MatrixScene.h"
#include "MatrixToolBox.h"
#include "MatrixTool.h"
#include "MatrixSelector.h"
#include "MatrixPainter.h"
#include "MatrixEraser.h"
#include "MatrixMover.h"
#include "MatrixResizer.h"
#include "MatrixVelocity.h"
#include "MatrixMouseEvent.h"
#include "PianoKeyboard.h"

#include <QGraphicsView>
#include <QGridLayout>
#include <QScrollBar>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QWheelEvent>

#include "document/RosegardenDocument.h"

#include "gui/widgets/Panner.h"
#include "gui/widgets/Panned.h"

#include "gui/rulers/PitchRuler.h"

#include "misc/Debug.h"
#include "misc/Strings.h"

#include "base/RulerScale.h"

namespace Rosegarden
{

MatrixWidget::MatrixWidget(bool drumMode) :
    m_document(0),
    m_view(0),
    m_scene(0),
    m_toolBox(0),
    m_currentTool(0),
    m_drumMode(drumMode),
    m_playTracking(true),
    m_hZoomFactor(1.0),
    m_vZoomFactor(1.0),
    m_currentVelocity(100),
    m_referenceScale(0),
    m_inMove(false),
    m_pitchRuler(0),
    m_pianoView(0)
{
    QGridLayout *layout = new QGridLayout;
    setLayout(layout);

    m_view = new Panned;
    m_view->setRenderHints(QPainter::Antialiasing);
    m_view->setBackgroundBrush(Qt::white);
    layout->addWidget(m_view, 0, 1, 1, 1);

    m_pitchRuler = new PianoKeyboard(0);
    m_pitchRuler->setFixedSize(m_pitchRuler->sizeHint());

    m_pianoView = new Panned();
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width());

    layout->addWidget(m_pianoView, 0, 0, 1, 1);

    m_hpanner = new Panner;
    m_hpanner->setMaximumHeight(50);
    m_hpanner->setRenderHints(QPainter::Antialiasing);
    m_hpanner->setBackgroundBrush(Qt::white);
    layout->addWidget(m_hpanner, 1, 0, 1, 2);

    m_pianoScene = new QGraphicsScene();
    QGraphicsProxyWidget *pianoKbd = m_pianoScene->addWidget(m_pitchRuler);
    m_pianoView->setScene(m_pianoScene);
    m_pianoView->centerOn(pianoKbd);

    connect(m_view, SIGNAL(pannedRectChanged(QRectF)),
            m_hpanner, SLOT(slotSetPannedRect(QRectF)));

    connect(m_view, SIGNAL(pannedRectChanged(QRectF)),
            m_pianoView, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannedRectChanged(QRectF)),
            m_view, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannedRectChanged(QRectF)),
            m_pianoView, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(zoomIn()),
            this, SLOT(slotZoomInFromPanner()));

    connect(m_hpanner, SIGNAL(zoomOut()),
            this, SLOT(slotZoomOutFromPanner()));

    connect(m_pianoView, SIGNAL(wheelEventReceived(QWheelEvent *)),
            m_view, SLOT(slotEmulateWheelEvent(QWheelEvent *)));

    m_toolBox = new MatrixToolBox(this);
}

MatrixWidget::~MatrixWidget()
{
    delete m_scene;
}

void
MatrixWidget::setSegments(RosegardenDocument *document,
			  std::vector<Segment *> segments)
{
    m_document = document;

    delete m_referenceScale;

    delete m_scene;
    m_scene = new MatrixScene();
    m_scene->setMatrixWidget(this);
    m_scene->setSegments(document, segments);

    m_referenceScale = new ZoomableRulerScale(m_scene->getRulerScale());

    connect(m_scene, SIGNAL(mousePressed(const MatrixMouseEvent *)),
            this, SLOT(slotDispatchMousePress(const MatrixMouseEvent *)));

    connect(m_scene, SIGNAL(mouseMoved(const MatrixMouseEvent *)),
            this, SLOT(slotDispatchMouseMove(const MatrixMouseEvent *)));

    connect(m_scene, SIGNAL(mouseReleased(const MatrixMouseEvent *)),
            this, SLOT(slotDispatchMouseRelease(const MatrixMouseEvent *)));

    connect(m_scene, SIGNAL(mouseDoubleClicked(const MatrixMouseEvent *)),
            this, SLOT(slotDispatchMouseDoubleClick(const MatrixMouseEvent *)));

    m_view->setScene(m_scene);

    m_toolBox->setScene(m_scene);

    m_hpanner->setScene(m_scene);

    // If piano scene and matrix scene don't have the same height
    // one may shift from the other when scrolling vertically
    QRectF viewRect = m_scene->sceneRect();
    QRectF pianoRect = m_pianoScene->sceneRect();
    pianoRect.setHeight(viewRect.height());
    m_pianoScene->setSceneRect(pianoRect);
}

bool
MatrixWidget::segmentsContainNotes() const
{
    if (!m_scene) return false;
    return m_scene->segmentsContainNotes();
}

void
MatrixWidget::setHorizontalZoomFactor(double factor)
{
    m_hZoomFactor = factor;
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
}

double
MatrixWidget::getHorizontalZoomFactor() const
{
    return m_hZoomFactor;
}

void
MatrixWidget::slotZoomInFromPanner() 
{
    m_hZoomFactor /= 1.1;
    m_vZoomFactor /= 1.1;
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
    m_pianoView->resetMatrix();
    m_pianoView->scale(m_vZoomFactor, m_vZoomFactor);
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width() * m_vZoomFactor);
}

void
MatrixWidget::slotZoomOutFromPanner() 
{
    m_hZoomFactor *= 1.1;
    m_vZoomFactor *= 1.1;
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
    m_pianoView->resetMatrix();
    m_pianoView->scale(m_vZoomFactor, m_vZoomFactor);
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width() * m_vZoomFactor);
}

EventSelection *
MatrixWidget::getSelection() const
{
    if (!m_scene) return 0;
    return m_scene->getSelection();
}

void
MatrixWidget::setSelection(EventSelection *s, bool preview)
{
    if (!m_scene) return;
    m_scene->setSelection(s, preview);
}    

const SnapGrid *
MatrixWidget::getSnapGrid() const
{
    if (!m_scene) return 0;
    return m_scene->getSnapGrid();
}

void
MatrixWidget::slotSetSnap(timeT t)
{
    if (!m_scene) return;
    m_scene->setSnap(t);
}    

void
MatrixWidget::slotSelectAll()
{
    if (!m_scene) return;
    m_scene->selectAll();
}

void
MatrixWidget::slotClearSelection()
{
    // Actually we don't clear the selection immediately: if we're
    // using some tool other than the select tool, then the first
    // press switches us back to the select tool.

    MatrixSelector *selector = dynamic_cast<MatrixSelector *>(m_currentTool);
    
    if (!selector) {
        slotSetSelectTool();
    } else {
        setSelection(0, false);
    }
}

Segment *
MatrixWidget::getCurrentSegment()
{
    if (!m_scene) return 0;
    return m_scene->getCurrentSegment();
}

void
MatrixWidget::slotDispatchMousePress(const MatrixMouseEvent *e)
{
    if (!m_currentTool) return;

    if (e->buttons & Qt::LeftButton) {
        m_currentTool->handleLeftButtonPress(e);
    } else if (e->buttons & Qt::MidButton) {
        m_currentTool->handleMidButtonPress(e);
    } else if (e->buttons & Qt::RightButton) {
        m_currentTool->handleRightButtonPress(e);
    }
}

void
MatrixWidget::slotDispatchMouseMove(const MatrixMouseEvent *e)
{
    m_pitchRuler->drawHoverNote(e->pitch);
    m_pianoView->update();

    if (!m_currentTool) return;

    if (m_inMove) {
        m_lastMouseMoveScenePos = QPointF(e->sceneX, e->sceneY);
        m_inMove = false;
        return;
    }

    MatrixTool::FollowMode mode = m_currentTool->handleMouseMove(e);

    if (mode != MatrixTool::NoFollow) {
        m_lastMouseMoveScenePos = QPointF(e->sceneX, e->sceneY);
        m_inMove = true;
        slotEnsureLastMouseMoveVisible();
        QTimer::singleShot(100, this, SLOT(slotEnsureLastMouseMoveVisible()));
        m_inMove = false;
    }
}

void
MatrixWidget::slotEnsureLastMouseMoveVisible()
{
    m_inMove = true;
    QPointF pos = m_lastMouseMoveScenePos;
    if (m_scene) m_scene->constrainToSegmentArea(pos);
    m_view->ensureVisible(QRectF(pos, pos));
    m_inMove = false;
}    

void
MatrixWidget::slotDispatchMouseRelease(const MatrixMouseEvent *e)
{
    if (!m_currentTool) return;
    m_currentTool->handleMouseRelease(e);
    QPointF pos(e->sceneX, e->sceneY);
    if (m_scene) m_scene->constrainToSegmentArea(pos);
    m_view->ensureVisible(QRectF(pos, pos));
}

void
MatrixWidget::slotDispatchMouseDoubleClick(const MatrixMouseEvent *e)
{
    if (!m_currentTool) return;
    m_currentTool->handleMouseDoubleClick(e);
}

void
MatrixWidget::setCanvasCursor(QCursor c)
{
    if (m_view) m_view->setCursor(c);
}

void
MatrixWidget::slotSetTool(QString name)
{
    MatrixTool *tool = dynamic_cast<MatrixTool *>(m_toolBox->getTool(name));
    if (!tool) return;
    if (m_currentTool) m_currentTool->stow();
    m_currentTool = tool;
    m_currentTool->ready();
}

void
MatrixWidget::slotSetPaintTool()
{
    slotSetTool(MatrixPainter::ToolName);
}

void
MatrixWidget::slotSetEraseTool()
{
    slotSetTool(MatrixEraser::ToolName);
}

void
MatrixWidget::slotSetSelectTool()
{
    slotSetTool(MatrixSelector::ToolName);
    MatrixSelector *selector = dynamic_cast<MatrixSelector *>(m_currentTool);
    if (selector) {
        connect(selector, SIGNAL(editTriggerSegment(int)),
                this, SIGNAL(editTriggerSegment(int)));
    }
}

void
MatrixWidget::slotSetMoveTool()
{
    slotSetTool(MatrixMover::ToolName);
}

void
MatrixWidget::slotSetResizeTool()
{
    slotSetTool(MatrixResizer::ToolName);
}

void
MatrixWidget::slotSetVelocityTool()
{
    slotSetTool(MatrixVelocity::ToolName);
}

void
MatrixWidget::slotSetPlayTracking(bool tracking)
{
    m_playTracking = tracking;
    if (m_playTracking) {
        if (m_scene) m_scene->ensurePointerVisible();
    }
}

}

#include "MatrixWidget.moc"

