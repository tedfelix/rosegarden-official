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
#include <QStackedLayout>
#include <QWidget>
#include <QPushButton>

#include "document/RosegardenDocument.h"

#include "gui/application/RosegardenMainWindow.h"
#include "gui/application/TransportStatus.h"

#include "gui/seqmanager/SequenceManager.h"

#include "gui/widgets/Panner.h"
#include "gui/widgets/Panned.h"
#include "gui/widgets/Thumbwheel.h"

#include "gui/rulers/PitchRuler.h"
#include "gui/rulers/PercussionPitchRuler.h"

#include "gui/rulers/ControllerEventsRuler.h"
#include "gui/rulers/ControlRulerWidget.h"
#include "gui/rulers/StandardRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/rulers/ChordNameRuler.h"

#include "gui/general/IconLoader.h"

#include "misc/Debug.h"
#include "misc/Strings.h"

#include "base/Composition.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/RulerScale.h"
#include "base/PropertyName.h"
#include "base/BaseProperties.h"

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
    m_lastZoomWasHV(true),
    m_pitchRuler(0),
    m_pianoView(0),
    m_pianoScene(0),
    m_localMapping(0),
    m_topStandardRuler(0),
    m_bottomStandardRuler(0),
    m_tempoRuler(0),
    m_chordNameRuler(0),
    m_layout(0)
{
    m_layout = new QGridLayout;
    setLayout(m_layout);

    // Remove thick black lines beetween rulers and matrix
    m_layout->setSpacing(0);

    // Remove black margins around the matrix
    //m_layout->setContentsMargins(0, 0, 0, 0);

    m_view = new Panned;
    m_view->setBackgroundBrush(Qt::white);
    m_layout->addWidget(m_view, PANNED_ROW, MAIN_COL, 1, 1);

    m_pianoView = new Panned;
    m_pianoView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pianoView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_layout->addWidget(m_pianoView, PANNED_ROW, HEADER_COL, 1, 1);

    m_controlsWidget = new ControlRulerWidget;
    m_layout->addWidget(m_controlsWidget, CONTROLS_ROW, MAIN_COL, 1, 1);

    // the panner along with zoom controls in one strip at one grid location
    QWidget *panner = new QWidget;
    QHBoxLayout *pannerLayout = new QHBoxLayout;
    pannerLayout->setContentsMargins(0, 0, 0, 0);
    pannerLayout->setSpacing(0);
    panner->setLayout(pannerLayout);

    m_hpanner = new Panner;
// EXPERIMENTAL: make the Panner 10 px taller so there is more room for wheels
//    m_hpanner->setMaximumHeight(50);
    m_hpanner->setMaximumHeight(60);
    m_hpanner->setBackgroundBrush(Qt::white);
    m_hpanner->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);

    pannerLayout->addWidget(m_hpanner);

    // row, col, row span, col span
    QFrame *controls = new QFrame;

    QGridLayout *controlsLayout = new QGridLayout;
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controls->setLayout(controlsLayout);

    m_HVzoom = new Thumbwheel(Qt::Vertical);
//    m_HVzoom->setFixedSize(QSize(30, 30));
    m_HVzoom->setFixedSize(QSize(40, 40));
    m_HVzoom->setToolTip(tr("Zoom"));

    // +/- 20 clicks seems to be the reasonable limit
    m_HVzoom->setMinimumValue(-20);
    m_HVzoom->setMaximumValue(20);
    m_HVzoom->setDefaultValue(0);
    m_lastHVzoomValue = m_HVzoom->getValue();
    controlsLayout->addWidget(m_HVzoom, 0, 0, Qt::AlignCenter);

    connect(m_HVzoom, SIGNAL(valueChanged(int)), this,
            SLOT(slotPrimaryThumbwheelMoved(int)));

    m_Hzoom = new Thumbwheel(Qt::Horizontal);
//    m_Hzoom->setFixedSize(QSize(40, 16));
    m_Hzoom->setFixedSize(QSize(50, 16));
    m_Hzoom->setToolTip(tr("Horizontal Zoom"));

    // not sure how the ruler scales work, but experimentation shows we never
    // want to go below 1, and 100 is a few clicks beyond reason for horizontal,
    // 50 clicks for vertical
    m_Hzoom->setMinimumValue(1);
    m_Hzoom->setMaximumValue(100);
    m_Hzoom->setDefaultValue(10); //!!! this isn't quite right
    controlsLayout->addWidget(m_Hzoom, 1, 0);
    connect(m_Hzoom, SIGNAL(valueChanged(int)), this,
            SLOT(slotHorizontalThumbwheelMoved(int)));

    m_Vzoom = new Thumbwheel(Qt::Vertical);
//    m_Vzoom->setFixedSize(QSize(16, 40));
    m_Vzoom->setFixedSize(QSize(16, 50));
    m_Vzoom->setToolTip(tr("Vertical Zoom"));
    m_Vzoom->setMinimumValue(1);
    m_Vzoom->setMaximumValue(50);
    m_Vzoom->setDefaultValue(10); //!!! this isn't quite right
    controlsLayout->addWidget(m_Vzoom, 0, 1, Qt::AlignRight);

    connect(m_Vzoom, SIGNAL(valueChanged(int)), this,
            SLOT(slotVerticalThumbwheelMoved(int)));

    // a blank QPushButton forced square looks better than the tool button did
    m_reset = new QPushButton;
    m_reset->setFixedSize(QSize(10, 10));
    m_reset->setToolTip(tr("Reset Zoom"));
    controlsLayout->addWidget(m_reset, 1, 1, Qt::AlignCenter);

    connect(m_reset, SIGNAL(clicked()), this, 
            SLOT(slotResetZoomClicked()));

    pannerLayout->addWidget(controls);

    m_layout->addWidget(panner, PANNER_ROW, MAIN_COL, 1, 1);

    // Rulers being not defined still, they can't be added to m_layout.
    // This will be done in setSegments().
    // Move the scroll bar from m_view to MatrixWidget
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_layout->addWidget(m_view->horizontalScrollBar(),
                        HSLIDER_ROW, MAIN_COL, 1, 1);

    // Hide or show the horizontal scroll bar when needed
    connect(m_view->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)),
            this, SLOT(slotHScrollBarRangeChanged(int, int)));

    connect(m_view, SIGNAL(pannedRectChanged(QRectF)),
            m_hpanner, SLOT(slotSetPannedRect(QRectF)));

    connect(m_view, SIGNAL(pannedRectChanged(QRectF)),
            m_pianoView, SLOT(slotSetPannedRect(QRectF)));

    connect(m_view, SIGNAL(pannedRectChanged(QRectF)),
            m_controlsWidget, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannedRectChanged(QRectF)),
            m_view, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannedRectChanged(QRectF)),
            m_pianoView, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannedRectChanged(QRectF)),
            m_controlsWidget, SLOT(slotSetPannedRect(QRectF)));

    connect(m_view, SIGNAL(pannedContentsScrolled()),
            this, SLOT(slotHScroll()));

    connect(m_hpanner, SIGNAL(zoomIn()),
            this, SLOT(slotSyncPannerZoomIn()));

    connect(m_hpanner, SIGNAL(zoomOut()),
            this, SLOT(slotSyncPannerZoomOut()));

    connect(m_pianoView, SIGNAL(wheelEventReceived(QWheelEvent *)),
            m_view, SLOT(slotEmulateWheelEvent(QWheelEvent *)));

    connect(m_controlsWidget, SIGNAL(dragScroll(timeT)),
            this, SLOT(slotEnsureTimeVisible(timeT)));
    
    m_toolBox = new MatrixToolBox(this);

    MatrixMover *matrixMoverTool = dynamic_cast <MatrixMover *> (m_toolBox->getTool(MatrixMover::ToolName));
    if (matrixMoverTool) {
        connect(matrixMoverTool, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
                m_controlsWidget, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));
    }

    MatrixVelocity *matrixVelocityTool = dynamic_cast <MatrixVelocity *> (m_toolBox->getTool(MatrixVelocity::ToolName));
    if (matrixVelocityTool) {
        connect(matrixVelocityTool, SIGNAL(hoveredOverNoteChanged()),
                m_controlsWidget, SLOT(slotHoveredOverNoteChanged()));
    }

    connect(this, SIGNAL(toolChanged(QString)),
            m_controlsWidget, SLOT(slotSetToolName(QString)));
}

MatrixWidget::~MatrixWidget()
{
    RG_DEBUG << "MatrixWidget::~MatrixWidget() - start";
    delete m_scene;
    delete m_pianoScene;
    delete m_localMapping;
    RG_DEBUG << "MatrixWidget::~MatrixWidget() - end";
}

void
MatrixWidget::setSegments(RosegardenDocument *document,
			  std::vector<Segment *> segments)
{
    if (m_document) {
        disconnect(m_document, SIGNAL(pointerPositionChanged(timeT)),
                   this, SLOT(slotPointerPositionChanged(timeT)));
    }

    m_document = document;

    delete m_scene;
    m_scene = new MatrixScene();
    m_scene->setMatrixWidget(this);
    m_scene->setSegments(document, segments);

    m_referenceScale = m_scene->getReferenceScale();

    connect(m_scene, SIGNAL(mousePressed(const MatrixMouseEvent *)),
            this, SLOT(slotDispatchMousePress(const MatrixMouseEvent *)));

    connect(m_scene, SIGNAL(mouseMoved(const MatrixMouseEvent *)),
            this, SLOT(slotDispatchMouseMove(const MatrixMouseEvent *)));

    connect(m_scene, SIGNAL(mouseReleased(const MatrixMouseEvent *)),
            this, SLOT(slotDispatchMouseRelease(const MatrixMouseEvent *)));

    connect(m_scene, SIGNAL(mouseDoubleClicked(const MatrixMouseEvent *)),
            this, SLOT(slotDispatchMouseDoubleClick(const MatrixMouseEvent *)));

    connect(m_scene, SIGNAL(segmentDeleted(Segment *)),
            this, SIGNAL(segmentDeleted(Segment *)));

    connect(m_scene, SIGNAL(sceneDeleted()),
            this, SIGNAL(sceneDeleted()));

    m_view->setScene(m_scene);

    m_toolBox->setScene(m_scene);

    m_hpanner->setScene(m_scene);

    Composition &comp = document->getComposition();

    Track *track =
        comp.getTrackById(segments[0]->getTrack());

    Instrument *instr = document->getStudio().
                        getInstrumentById(track->getInstrument());

    const MidiKeyMapping *mapping = 0;

    if (instr) {
        mapping = instr->getKeyMapping();
        if (mapping) {
            RG_DEBUG << "MatrixView: Instrument has key mapping: "
                     << mapping->getName() << endl;
            m_localMapping = new MidiKeyMapping(*mapping);
            m_localMapping->extend();
        } else {
            RG_DEBUG << "MatrixView: Instrument has no key mapping\n";
        }
    }

    if (mapping && !m_localMapping->getMap().empty()) {
        m_pitchRuler = new PercussionPitchRuler(0, m_localMapping,
                                                m_scene->getYResolution());
    } else {
        m_pitchRuler = new PianoKeyboard(0);
    }

    m_pitchRuler->setFixedSize(m_pitchRuler->sizeHint());
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width() + 4);
    //@@@ The 4 pixels have been added empirically in line above to
    //    show the pitch ruler completely. (The pitch ruler contents was
    //    horizontally moving with Alt + wheel)

    delete m_pianoScene;
    m_pianoScene = new QGraphicsScene();
    QGraphicsProxyWidget *pianoKbd = m_pianoScene->addWidget(m_pitchRuler);
    m_pianoView->setScene(m_pianoScene);
    m_pianoView->centerOn(pianoKbd);

    // If piano scene and matrix scene don't have the same height
    // one may shift from the other when scrolling vertically
    QRectF viewRect = m_scene->sceneRect();
    QRectF pianoRect = m_pianoScene->sceneRect();
    pianoRect.setHeight(viewRect.height());
    m_pianoScene->setSceneRect(pianoRect);

    m_controlsWidget->setSegments(document, segments);
    m_controlsWidget->setScene(m_scene);

    // For some reason this doesn't work in the constructor - not looked in detail
    connect(m_scene, SIGNAL(selectionChanged(EventSelection *)),
            m_controlsWidget, SLOT(slotSelectionChanged(EventSelection *)));

    m_topStandardRuler = new StandardRuler(document,
                                           m_referenceScale, 0, 25,
                                           false);

    m_bottomStandardRuler = new StandardRuler(document,
                                               m_referenceScale, 0, 25,
                                               true);

    m_tempoRuler = new TempoRuler(m_referenceScale,
                                  document,
                                  RosegardenMainWindow::self(),
                                  0.0,    // xorigin
                                  24,     // height
                                  true);  // small

    m_chordNameRuler = new ChordNameRuler(m_referenceScale,
                                          document,
                                          segments,
                                          0.0,     // xorigin
                                          24);     // height

    m_layout->addWidget(m_topStandardRuler, TOPRULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_bottomStandardRuler, BOTTOMRULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_tempoRuler, TEMPORULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_chordNameRuler, CHORDNAMERULER_ROW, MAIN_COL, 1, 1);

    m_topStandardRuler->setSnapGrid(m_scene->getSnapGrid());
    m_bottomStandardRuler->setSnapGrid(m_scene->getSnapGrid());

    m_topStandardRuler->connectRulerToDocPointer(document);
    m_bottomStandardRuler->connectRulerToDocPointer(document);

    connect(m_topStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));
    connect(m_bottomStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));

    connect(m_document, SIGNAL(pointerPositionChanged(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));

    m_tempoRuler->connectSignals();

    m_chordNameRuler->setReady();
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
    slotHScroll();
}

void
MatrixWidget::setVerticalZoomFactor(double factor)
{
    m_vZoomFactor = factor;
    if (m_referenceScale) m_referenceScale->setYZoomFactor(m_vZoomFactor);
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
// I don't think we need a slotVScroll
//    slotVScroll();
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
    QMatrix m;
    m.scale(m_hZoomFactor, m_vZoomFactor);
    m_view->setMatrix(m);
    m_pianoView->setMatrix(m);
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width() * m_vZoomFactor);
    slotHScroll();
}

void
MatrixWidget::slotZoomOutFromPanner()
{
    m_hZoomFactor *= 1.1;
    m_vZoomFactor *= 1.1;
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    QMatrix m;
    m.scale(m_hZoomFactor, m_vZoomFactor);
    m_view->setMatrix(m);
    m_pianoView->setMatrix(m);
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width() * m_vZoomFactor);
    slotHScroll();
}

void
MatrixWidget::slotHScroll()
{
    // Get time of the window left
    QPointF topLeft = m_view->mapToScene(0, 0);

    // Apply zoom correction
    int x = topLeft.x() * m_hZoomFactor;

    // Scroll rulers accordingly
    // ( -2 : to fix a small offset between view and rulers)
    m_topStandardRuler->slotScrollHoriz(x - 2);
    m_bottomStandardRuler->slotScrollHoriz(x - 2);
    m_tempoRuler->slotScrollHoriz(x - 2);
    m_chordNameRuler->slotScrollHoriz(x - 2);
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
    } else if ((e->buttons & Qt::MidButton) || (e->buttons & Qt::LeftButton & Qt::RightButton)) {
        m_currentTool->handleMidButtonPress(e);
    } else if (e->buttons & Qt::RightButton) {
        m_currentTool->handleRightButtonPress(e);
    }
}

void
MatrixWidget::slotDispatchMouseMove(const MatrixMouseEvent *e)
{
    m_pitchRuler->drawHoverNote(e->pitch);
    m_pianoView->update();   // Needed to remove black trailers left by
                             // hover note at hight zoom levels

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
MatrixWidget::slotEnsureTimeVisible(timeT t)
{
    m_inMove = true;
    QPointF pos = m_view->mapToScene(0,m_view->height()/2);
    pos.setX(m_scene->getRulerScale()->getXForTime(t));
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
    if (m_view) m_view->viewport()->setCursor(c);
}

void
MatrixWidget::slotSetTool(QString name)
{
    MatrixTool *tool = dynamic_cast<MatrixTool *>(m_toolBox->getTool(name));
    if (!tool) return;
    if (m_currentTool) m_currentTool->stow();
    m_currentTool = tool;
    m_currentTool->ready();
    emit toolChanged(name);
}

void
MatrixWidget::slotSetPaintTool()
{
    MATRIX_DEBUG << "slotSetPaintTool" << endl;
    
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
    MATRIX_DEBUG << "slotSetSelectTool" << endl;
    
    slotSetTool(MatrixSelector::ToolName);
    MatrixSelector *selector = dynamic_cast<MatrixSelector *>(m_currentTool);
    if (selector) {
        MATRIX_DEBUG << "slotSetSelectTool: selector successfully set" << endl;
    
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
        m_view->slotEnsurePositionPointerInView(true);
    }
}

void
MatrixWidget::slotToggleVelocityRuler()
{
    m_controlsWidget->slotTogglePropertyRuler(BaseProperties::VELOCITY);
}

void
MatrixWidget::slotTogglePitchbendRuler()
{
    m_controlsWidget->slotToggleControlRuler("PitchBend");
}

void
MatrixWidget::slotAddControlRuler()
{
    m_controlsWidget->slotAddRuler();
}

void
MatrixWidget::slotHScrollBarRangeChanged(int min, int max)
{
    if (max > min) {
        m_view->horizontalScrollBar()->show();
    } else {
        m_view->horizontalScrollBar()->hide();
    }
}

void
MatrixWidget::slotPointerPositionChanged(timeT t)
{
    QObject *s = sender();
    bool fromDocument = (s == m_document);

    if (!m_scene) return;

    double sceneX = m_scene->getRulerScale()->getXForTime(t);

    // Never move the pointer outside the scene (else the scene will grow)
    double x1 = m_scene->sceneRect().x();
    double x2 = x1 + m_scene->sceneRect().width();

    if ((sceneX < x1) || (sceneX > x2)) {
        m_view->slotHidePositionPointer();
        m_hpanner->slotHidePositionPointer();
    } else {
        m_view->slotShowPositionPointer(sceneX);
        m_hpanner->slotShowPositionPointer(sceneX);
    }

    if (getPlayTracking() || !fromDocument) {
        m_view->slotEnsurePositionPointerInView(fromDocument);
    }
}

void
MatrixWidget::setTempoRulerVisible(bool visible)
{
    if (visible) m_tempoRuler->show();
    else m_tempoRuler->hide();
}

void
MatrixWidget::setChordNameRulerVisible(bool visible)
{
    if (visible) m_chordNameRuler->show();
    else m_chordNameRuler->hide();
}

void
MatrixWidget::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);
    slotHScroll();
}

void
MatrixWidget::slotHorizontalThumbwheelMoved(int v)
{
    std::cout << "horizontal wheel V == " << v << std::endl;
    setHorizontalZoomFactor(v);
}

void
MatrixWidget::slotVerticalThumbwheelMoved(int v)
{
    std::cout << "vertical wheel V == " << v << std::endl;
    setVerticalZoomFactor(v);
}

void
MatrixWidget::slotPrimaryThumbwheelMoved(int v)
{
    std::cout << "primary wheel debug (before): v == " << v << " m_lastHVzoomValue == " << m_lastHVzoomValue
              << " m_hZoomFactor == " << m_hZoomFactor << " m_vZoomFactor == " << m_vZoomFactor  << std::endl;

    // little bit of kludge work to deal with value manipulations that are
    // outside of the constraints imposed by the primary zoom wheel itself
    if (v < -20) v = -20;
    if (v > 20) v = 20;
    if (m_lastHVzoomValue < -20) m_lastHVzoomValue = -20;
    if (m_lastHVzoomValue > 20) m_lastHVzoomValue = 20;

    // When dragging the wheel up and down instead of mouse wheeling it, it
    // steps according to its speed.  I don't see a sure way (and after all
    // there are no docs!) to make sure dragging results in a smooth 1:1
    // relationship when compared with mouse wheeling, and we are just hijacking
    // slotZoomInFromPanner() here, so we will look at the number of steps
    // between the old value and the last one, and call the slot that many times
    // in order to enforce the 1:1 relationship.
    int steps = v - m_lastHVzoomValue;
    if (steps < 0) steps *= -1;

    for (int i = 0; i < steps; ++i) {
        if (v < m_lastHVzoomValue) slotZoomInFromPanner();
        else if (v > m_lastHVzoomValue) slotZoomOutFromPanner();
    }

    m_lastHVzoomValue = v;
    std::cout << "primary wheel debug (after): v == " << v << " m_lastHVzoomValue == " << m_lastHVzoomValue
              << " m_hZoomFactor == " << m_hZoomFactor << " m_vZoomFactor == " << m_vZoomFactor  << std::endl;
}

void
MatrixWidget::slotResetZoomClicked()
{
    std::cout << "reset clicked" << std::endl;
    // just taking a potshot at this one, let's see if this works
    m_hZoomFactor = 1.0;
    m_vZoomFactor = 1.0;
    if (m_referenceScale) {
        m_referenceScale->setXZoomFactor(m_hZoomFactor);
        m_referenceScale->setYZoomFactor(m_vZoomFactor);
    }
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
    slotHScroll();

    // scale factor 1.0 = 100% zoom
    m_Hzoom->setValue(1);
    m_Vzoom->setValue(1);
    m_HVzoom->setValue(0);
}

void
MatrixWidget::slotSyncPannerZoomIn()
{
    int v = m_lastHVzoomValue - 1;

    m_HVzoom->setValue(v);
    slotPrimaryThumbwheelMoved(v);
}

void
MatrixWidget::slotSyncPannerZoomOut()
{
    int v = m_lastHVzoomValue + 1;

    m_HVzoom->setValue(v);
    slotPrimaryThumbwheelMoved(v);
}


}

#include "MatrixWidget.moc"

