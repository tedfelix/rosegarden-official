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
#include "NoteRestInserter.h"
#include "ClefInserter.h"
#include "TextInserter.h"
#include "GuitarChordInserter.h"
#include "SymbolInserter.h"
#include "NotationMouseEvent.h"
#include "NotationSelector.h"
#include "NotationEraser.h"
#include "StaffLayout.h"
#include "HeadersGroup.h"

#include "base/RulerScale.h"

#include "document/RosegardenDocument.h"

#include "gui/application/RosegardenMainWindow.h"

#include "gui/widgets/Panner.h"
#include "gui/widgets/Panned.h"
#include "gui/general/IconLoader.h"

#include "gui/rulers/StandardRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/RawNoteRuler.h"

#include "misc/Debug.h"
#include "misc/Strings.h"

#include <QGraphicsView>
#include <QGridLayout>
#include <QScrollBar>
#include <QTimer>
#include <QGraphicsProxyWidget>
#include <QToolTip>
#include <QToolButton>

namespace Rosegarden
{

NotationWidget::NotationWidget() :
    m_document(0),
    m_view(0),
    m_scene(0),
    m_playTracking(true),
    m_hZoomFactor(1),
    m_vZoomFactor(1),
    m_referenceScale(0),
    m_topStandardRuler(0),
    m_bottomStandardRuler(0),
    m_tempoRuler(0),
    m_chordNameRuler(0),
    m_rawNoteRuler(0),
    m_headersGroup(0),
    m_headersView(0),
    m_headersScene(0),
    m_headersButtons(0),
    m_headersLastY(0),
    m_layout(0),
    m_linearMode(true),
    m_tempoRulerIsVisible(false),
    m_rawNoteRulerIsVisible(false),
    m_chordNameRulerIsVisible(false),
    m_headersAreVisible(false),
    m_chordMode(false),
    m_tripletMode(false),
    m_graceMode(false)
{
    m_layout = new QGridLayout;
    setLayout(m_layout);

    // Remove thick black lines beetween rulers and staves
    m_layout->setSpacing(0);

    // Remove black margins around the notation
    //m_layout->setContentsMargins(0, 0, 0, 0);
    
    m_view = new Panned;
    m_view->setBackgroundBrush(Qt::white);
    m_view->setRenderHints(QPainter::Antialiasing |
                           QPainter::TextAntialiasing |
                           QPainter::SmoothPixmapTransform);
    m_view->setBackgroundBrush(QBrush(IconLoader().loadPixmap("bg-paper-grey")));
    m_layout->addWidget(m_view, PANNED_ROW, MAIN_COL, 1, 1);

    m_hpanner = new Panner;
    m_hpanner->setMaximumHeight(80);
    m_hpanner->setBackgroundBrush(Qt::white);
    m_hpanner->setRenderHints(0);
    m_layout->addWidget(m_hpanner, PANNER_ROW, HEADER_COL, 1, 2);

    m_headersView = new Panned;
    m_headersView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_headersView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_layout->addWidget(m_headersView, PANNED_ROW, HEADER_COL, 1, 1);


    // Rulers being not defined still, they can't be added to m_layout.
    // This will be done in setSegments().

    // Move the scroll bar from m_view to NotationWidget
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_layout->addWidget(m_view->horizontalScrollBar(),
                        HSLIDER_ROW, HEADER_COL, 1, 2);


    // Create the headers close button
    //
    // NOTE: I tried to style this QToolButton to resemble the parameter area
    // close button, but I could never get it to come out sensibly as a square
    // button with a reasonably sized X icon in it.  I tried all kinds of wild
    // variations.
    //
    // In the end, I took the way Yves had solved this problem and just replaced
    // his white X icon with a screen capture of the button I wanted to copy.
    // It doesn't hover correctly, but it doesn't look too bad, and seems as
    // close as I'm going to get to what I wanted.
    QToolButton *headersCloseButton = new QToolButton;
    headersCloseButton->setIcon(IconLoader().loadPixmap("header-close-button"));
    headersCloseButton->setIconSize(QSize(14, 14));
    connect(headersCloseButton, SIGNAL(clicked(bool)),
            this, SLOT(slotCloseHeaders()));

    // Insert the button in a layout to push it on the right
    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(20);
    buttonsLayout->addWidget(headersCloseButton);

    // Put the layout inside a widget and the widget above the headers
    m_headersButtons = new QWidget;
    m_headersButtons->setLayout(buttonsLayout);
    m_layout->addWidget(m_headersButtons, TOPRULER_ROW, HEADER_COL, 1, 1);


    // Hide or show the horizontal scroll bar when needed
    connect(m_view->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)),
            this, SLOT(slotHScrollBarRangeChanged(int, int)));

    connect(m_view, SIGNAL(pannedRectChanged(QRectF)),
            m_hpanner, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannedRectChanged(QRectF)),
            m_view, SLOT(slotSetPannedRect(QRectF)));

    connect(m_hpanner, SIGNAL(pannerChanged(QRectF)),
             this, SLOT(slotAdjustHeadersVerticalPos(QRectF)));

    connect(m_view, SIGNAL(pannedContentsScrolled()),
            this, SLOT(slotHScroll()));

    connect(m_hpanner, SIGNAL(zoomIn()),
            this, SLOT(slotZoomInFromPanner()));

    connect(m_hpanner, SIGNAL(zoomOut()),
            this, SLOT(slotZoomOutFromPanner()));

    connect(m_headersView, SIGNAL(wheelEventReceived(QWheelEvent *)),
            m_view, SLOT(slotEmulateWheelEvent(QWheelEvent *)));

    connect(this, SIGNAL(adjustNeeded(bool)),
            this, SLOT(slotAdjustHeadersHorizontalPos(bool)),
            Qt::QueuedConnection);

    m_toolBox = new NotationToolBox(this);

    //!!! 
    NoteRestInserter *noteRestInserter = dynamic_cast<NoteRestInserter *>
        (m_toolBox->getTool(NoteRestInserter::ToolName));
    m_currentTool = noteRestInserter;
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
    if (m_document) {
        disconnect(m_document, SIGNAL(pointerPositionChanged(timeT)),
                   this, SLOT(slotPointerPositionChanged(timeT)));
    }

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

    connect(m_scene, SIGNAL(segmentDeleted(Segment *)),
            this, SIGNAL(segmentDeleted(Segment *)));

    connect(m_scene, SIGNAL(sceneDeleted()),
            this, SIGNAL(sceneDeleted()));

    m_view->setScene(m_scene);

    m_toolBox->setScene(m_scene);

    m_hpanner->setScene(m_scene);
    m_hpanner->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);

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

    m_rawNoteRuler = new RawNoteRuler(m_referenceScale,
                                      segments[0],
                                      0.0,
                                      20);  // why not 24 as other rulers ?

    if (m_headersGroup) disconnect(m_headersGroup, SIGNAL(headersResized(int)),
                                   this, SLOT(slotHeadersResized(int)));
    m_headersGroup = new HeadersGroup(m_document);
    m_headersGroup->setTracks(this, m_scene);

    m_layout->addWidget(m_topStandardRuler, TOPRULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_bottomStandardRuler, BOTTOMRULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_tempoRuler, TEMPORULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_chordNameRuler, CHORDNAMERULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_rawNoteRuler, RAWNOTERULER_ROW, MAIN_COL, 1, 1);

    connect(m_topStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));
    connect(m_bottomStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));
    
    connect(m_document, SIGNAL(pointerPositionChanged(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));

    m_topStandardRuler->connectRulerToDocPointer(document);
    m_bottomStandardRuler->connectRulerToDocPointer(document);

    m_tempoRuler->connectSignals();

    m_chordNameRuler->setReady();

    m_headersGroup->setFixedSize(m_headersGroup->sizeHint());
    m_headersView->setFixedWidth(m_headersGroup->sizeHint().width());

    delete m_headersScene;
    m_headersScene = new QGraphicsScene();
    QGraphicsProxyWidget *headersProxy = m_headersScene->addWidget(m_headersGroup);
    m_headersView->setScene(m_headersScene);
    m_headersView->centerOn(headersProxy);

    m_headersView->setMinimumHeight(0);

    // If headers scene and notation scene don't have the same height
    // one may shift from the other when scrolling vertically
    QRectF viewRect = m_scene->sceneRect();
    QRectF headersRect = m_headersScene->sceneRect();
    headersRect.setHeight(viewRect.height());
    m_headersScene->setSceneRect(headersRect);

    connect(m_headersGroup, SIGNAL(headersResized(int)),
            this, SLOT(slotHeadersResized(int)));


    //!!! attempt to scroll either to the start or to the current
    //!!! pointer position, and to the top of the staff... however,
    //!!! this doesn't work... why not?
    m_view->ensureVisible(QRectF(0, 0, 1, 1), 0, 0);
    slotPointerPositionChanged(m_document->getComposition().getPosition());
}

void
NotationWidget::setCanvasCursor(QCursor c)
{
    if (m_view) m_view->viewport()->setCursor(c);
}

Segment *
NotationWidget::getCurrentSegment()
{
    if (!m_scene) return 0;
    return m_scene->getCurrentSegment();
}

bool
NotationWidget::segmentsContainNotes() const
{
    if (!m_scene) return false;
    return m_scene->segmentsContainNotes();
}

void
NotationWidget::locatePanner(bool tall)
{
    m_layout->removeWidget(m_hpanner);
    if (tall) {
        m_hpanner->setMaximumHeight(QWIDGETSIZE_MAX);
        m_hpanner->setMaximumWidth(80);
        m_layout->addWidget(m_hpanner, PANNED_ROW, VPANNER_COL);
    } else {
        m_hpanner->setMaximumHeight(80);
        m_hpanner->setMaximumWidth(QWIDGETSIZE_MAX);
        m_layout->addWidget(m_hpanner, PANNER_ROW, HEADER_COL, 1, 2);
    }
}

void
NotationWidget::slotSetLinearMode()
{
    if (!m_scene) return;
    if (m_scene->getPageMode() == StaffLayout::ContinuousPageMode) {
        locatePanner(false);
    }
    m_scene->setPageMode(StaffLayout::LinearMode);
    m_linearMode = true;
    hideOrShowRulers();
}

void
NotationWidget::slotSetContinuousPageMode()
{
    if (!m_scene) return;
    if (m_scene->getPageMode() == StaffLayout::ContinuousPageMode) return;
    locatePanner(true);
    m_scene->setPageMode(StaffLayout::ContinuousPageMode);
    m_linearMode = false;
    hideOrShowRulers();
}

void
NotationWidget::slotSetMultiPageMode()
{
    if (!m_scene) return;
    if (m_scene->getPageMode() == StaffLayout::ContinuousPageMode) {
        locatePanner(false);
    }
    m_scene->setPageMode(StaffLayout::MultiPageMode);
    m_linearMode = false;
    hideOrShowRulers();
}

void
NotationWidget::slotSetFontName(QString name)
{
    if (m_scene) m_scene->setFontName(name);
}

void
NotationWidget::slotSetFontSize(int size)
{
    if (m_scene) m_scene->setFontSize(size);
}

NotationTool *
NotationWidget::getCurrentTool() const
{
    return m_currentTool;
}

void
NotationWidget::slotSetTool(QString name)
{
    NotationTool *tool = dynamic_cast<NotationTool *>(m_toolBox->getTool(name));
    if (!tool) return;
    if (m_currentTool) m_currentTool->stow();
    m_currentTool = tool;
    m_currentTool->ready();
}

void
NotationWidget::slotSetEraseTool()
{
    slotSetTool(NotationEraser::ToolName);
}

void
NotationWidget::slotSetSelectTool()
{
    slotSetTool(NotationSelector::ToolName);
}

void
NotationWidget::slotSetNoteRestInserter()
{
    slotSetTool(NoteRestInserter::ToolName);
}

void
NotationWidget::slotSetNoteInserter()
{
    NoteRestInserter *noteRestInserter = dynamic_cast<NoteRestInserter *>
        (m_toolBox->getTool(NoteRestInserter::ToolName));
    noteRestInserter->setToRestInserter(false); // set to insert notes.

    slotSetTool(NoteRestInserter::ToolName);
}

void
NotationWidget::slotSetRestInserter()
{
    NoteRestInserter *noteRestInserter = dynamic_cast<NoteRestInserter *>
        (m_toolBox->getTool(NoteRestInserter::ToolName));
    noteRestInserter->setToRestInserter(true); // set to insert notes.

    slotSetTool(NoteRestInserter::ToolName);
}

void
NotationWidget::slotSetInsertedNote(Note::Type type, int dots)
{
    NoteRestInserter *ni = dynamic_cast<NoteRestInserter *>(m_currentTool);
    if (ni) {
        
        ni->slotSetNote(type);
        ni->slotSetDots(dots);
        return;
    }
}

void
NotationWidget::slotSetAccidental(Accidental accidental, bool follow)
{
    // You don't have to be in note insertion mode to change the accidental
    NoteRestInserter *ni = dynamic_cast<NoteRestInserter *>
        (m_toolBox->getTool(NoteRestInserter::ToolName));
    if (ni) {
        ni->slotSetAccidental(accidental, follow);
        return;
    }
}

void
NotationWidget::slotSetClefInserter()
{
    slotSetTool(ClefInserter::ToolName);
}

void
NotationWidget::slotSetInsertedClef(Clef type)
{
    ClefInserter *ci = dynamic_cast<ClefInserter *>(m_currentTool);
    if (ci) ci->slotSetClef(type);
}

void
NotationWidget::slotSetTextInserter()
{
    slotSetTool(TextInserter::ToolName);
}

void
NotationWidget::slotSetGuitarChordInserter()
{
    slotSetTool(GuitarChordInserter::ToolName);
}

void
NotationWidget::slotSetPlayTracking(bool tracking)
{
    m_playTracking = tracking;
    if (m_playTracking) {
        m_view->slotEnsurePositionPointerInView(true);
    }
}

void
NotationWidget::slotTogglePlayTracking()
{
    slotSetPlayTracking(!m_playTracking);
}

void
NotationWidget::slotPointerPositionChanged(timeT t)
{
    QObject *s = sender();
    bool fromDocument = (s == m_document);

    NOTATION_DEBUG << "NotationWidget::slotPointerPositionChanged to " << t << endl;

    if (!m_scene) return;

    QLineF p = m_scene->snapTimeToStaffPosition(t);
    if (p == QLineF()) return;

    //!!! p will also contain sensible Y (although not 100% sensible yet)
    double sceneX = p.x1();
    double sceneY = std::min(p.y1(), p.y2());
    double height = fabsf(p.y2() - p.y1());

    // Never move the pointer outside the scene (else the scene will grow)
    double x1 = m_scene->sceneRect().x();
    double x2 = x1 + m_scene->sceneRect().width();

    if ((sceneX < x1) || (sceneX > x2)) {
        m_view->slotHidePositionPointer();
        m_hpanner->slotHidePositionPointer();
    } else {
        m_view->slotShowPositionPointer(QPointF(sceneX, sceneY), height);
        m_hpanner->slotShowPositionPointer(QPointF(sceneX, sceneY), height);
    }

    if (getPlayTracking() || !fromDocument) {
        m_view->slotEnsurePositionPointerInView(fromDocument);
    }
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
    
    if (mode != NotationTool::NoFollow) {
        m_lastMouseMoveScenePos = QPointF(e->sceneX, e->sceneY);
        slotEnsureLastMouseMoveVisible();
        QTimer::singleShot(100, this, SLOT(slotEnsureLastMouseMoveVisible()));
    }

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
NotationWidget::slotEnsureLastMouseMoveVisible()
{
    if (m_inMove) return;
    m_inMove = true;
    QPointF pos = m_lastMouseMoveScenePos;
    if (m_scene) m_scene->constrainToSegmentArea(pos);
    m_view->ensureVisible(QRectF(pos, pos));
    m_inMove = false;
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

timeT
NotationWidget::getInsertionTime() const
{
    if (m_scene) return m_scene->getInsertionTime();
    else return 0;
}

void
NotationWidget::slotZoomInFromPanner() 
{
    m_hZoomFactor /= 1.1;
    m_vZoomFactor /= 1.1;
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    QMatrix m;
    m.scale(m_hZoomFactor, m_vZoomFactor);
    m_view->setMatrix(m);
    m_headersView->setMatrix(m);
    m_headersView->setFixedWidth(m_headersGroup->sizeHint().width()
                                                         * m_hZoomFactor);
    slotHScroll();
}

void
NotationWidget::slotZoomOutFromPanner() 
{
    m_hZoomFactor *= 1.1;
    m_vZoomFactor *= 1.1;
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    QMatrix m;
    m.scale(m_hZoomFactor, m_vZoomFactor);
    m_view->setMatrix(m);
    m_headersView->setMatrix(m);
    m_headersView->setFixedWidth(m_headersGroup->sizeHint().width()
                                                         * m_hZoomFactor);
    slotHScroll();
}

void
NotationWidget::slotAdjustHeadersHorizontalPos(bool last)
{
// Sometimes, after a zoom change, the headers are no more horizontally
// aligned with the headers view.
// The following code is an attempt to reposition the headers in the view.
// Actually it doesn't succeed always (ie. with stormy-riders).

// Workaround :
//   - 1) The old method adjustHeadersHorizontalPos() is changed into a slot
//        called when the new signal adjustNeeded() is emitted.
//        This slot is connected with a Qt::QueuedConnection type connection
//        to delay as much as possible its execution.
//    -2) The headers refresh problem occurs each time the x0 (or xinit)
//        value defined below is <= 0 : When such a situation occurs, the slot
//        is calls itself again. Usually, the second call works.
//        The slot arg. "last" has been added to avoid an infinite
//        loop if x0 is never > 0.
//
// I don't like this code which is really a workaround. Just now I don't
// know the true cause of the problem. (When zoom factor is decreased, x0 is
// > 0 sometimes and < 0 some other times : why ?)
// This slot always works when it is called after some delay, but it fails
// sometimes when it is called without any delay.
//
//  Maybe another solution would be to use a timer to call the slot.
//  But what should be the delay ? Should it depend on the machine where
//  RG run ? Or on the Qt version ?
//  The previous solution seems better.

//std::cerr << "\nXproxy0=" << m_headersProxy->scenePos().x() << "\n";

    double xinit;

    double x = xinit = m_headersView->mapToScene(0, 0).x();
//std::cerr << " x0=" << x << "\n";

    // First trial
    if ((x > 1) || (x < -1)) {
        QRectF view = m_headersView->sceneRect();
        view.moveLeft(0.0);
        m_headersView->setSceneRect(view);
        x = m_headersView->mapToScene(0, 0).x();
    }
//std::cerr << "x1=" << x << "\n";

    // Second trial. Why isn't the first iteration always sufficient ?
    // Number of iterations is limited to 3.
    int n = 1;
    while ((x > 1) || (x < -1)) {
//std::cerr << "n=" << n << " xt2=" << x << "\n";
        QRectF view = m_headersView->sceneRect();
        view.translate(-x, 0);
        m_headersView->setSceneRect(view);
        x = m_headersView->mapToScene(0, 0).x();
        if (n++ > 3) break;
    }

//std::cerr << "x2=" << x << "\n";

    // Third trial.
    // If precedent trial doesn't succeed, try again with a coefficient...
    // Number of iterations is limited to 6.    int m = 1;
    int m = 1;
    while ((x > 1) || (x < -1)) {
//std::cerr << "m=" << m << " xt3=" << x << "\n";
        QRectF view = m_headersView->sceneRect();
        view.translate(-x * 0.477, 0);
        m_headersView->setSceneRect(view);
        x = m_headersView->mapToScene(0, 0).x();
        if (m++ > 6) break;
    }

//std::cerr << "x3=" << x << "\n";

    // Probably totally useless here.
    m_headersView->update();

    // Now, sometimes, although x is null or almost null, the headers are
    // not fully visible !!??

//std::cerr << "Xproxy1=" << m_headersProxy->scenePos().x() << "\n";

    // Call again the current slot if we have some reason to think it
    // did not succeed and if it has been called in the current context
    // only once.
    // (See comment at the beginning of the slotAdjustHeadersHorizontalPos.)
    if (!last && xinit < 0.001) emit adjustNeeded(true);
}

void
NotationWidget::slotAdjustHeadersVerticalPos(QRectF r)
{
    r.setX(0);
    r.setWidth(m_headersView->sceneRect().width());

    // Misalignment between staffs and headers depends of the vertical
    // scrolling direction.
    // This is a hack : The symptoms are fixed (more or less) but still the
    //                  cause of the problem is unknown.
    double y = r.y();
    double delta = y > m_headersLastY ? - 2 : - 4;
    m_headersLastY = y;

    QRectF vr = m_view->mapToScene(m_view->rect()).boundingRect();
    r.setY(vr.y() + delta);
    r.setHeight(vr.height());

    m_headersView->setSceneRect(r);
}

double
NotationWidget::getViewLeftX()
{
    return m_view->mapToScene(0, 0).x();
}

int
NotationWidget::getNotationViewWidth()
{
    return m_view->width();
}

double
NotationWidget::getNotationSceneHeight()
{
    return m_scene->height();
}

void
NotationWidget::slotHScroll()
{
    // Get time of the window left
    QPointF topLeft = m_view->mapToScene(0, 0);
    double xs = topLeft.x();

    // Apply zoom correction (Offset of 20 found empirically : probably
    // some improvments are needed ...)
    int x = (xs - 20) * m_hZoomFactor;

    // Scroll rulers accordingly
    m_topStandardRuler->slotScrollHoriz(x);
    m_bottomStandardRuler->slotScrollHoriz(x);
    m_tempoRuler->slotScrollHoriz(x);
    m_chordNameRuler->slotScrollHoriz(x);
    m_rawNoteRuler->slotScrollHoriz(x);

    // Update staff headers
    m_headersGroup->slotUpdateAllHeaders(xs);

    emit adjustNeeded(false);
}

void
NotationWidget::slotHScrollBarRangeChanged(int min, int max)
{
    if (max > min) {
        m_view->horizontalScrollBar()->show(); 
    } else {
        m_view->horizontalScrollBar()->hide();
    }
}

void
NotationWidget::setTempoRulerVisible(bool visible)
{
    if (visible && m_linearMode) m_tempoRuler->show();
    else m_tempoRuler->hide();
    m_tempoRulerIsVisible = visible;
}

void
NotationWidget::setChordNameRulerVisible(bool visible)
{
    if (visible && m_linearMode) m_chordNameRuler->show();
    else m_chordNameRuler->hide();
    m_chordNameRulerIsVisible = visible;
}

void
NotationWidget::setRawNoteRulerVisible(bool visible)
{
    if (visible && m_linearMode) m_rawNoteRuler->show();
    else m_rawNoteRuler->hide();
    m_rawNoteRulerIsVisible = visible;
}

void
NotationWidget::setHeadersVisible(bool visible)
{
    if (visible && m_linearMode) {
        m_headersView->show();
        m_headersButtons->show();
    } else {
        m_headersView->hide();
        m_headersButtons->hide();
    }
    m_headersAreVisible = visible;
}

void
NotationWidget::toggleHeadersView()
{
    m_headersAreVisible = !m_headersAreVisible;
    if (m_headersAreVisible && m_linearMode) {
        m_headersView->show();
        m_headersButtons->show();
    } else {
        m_headersView->hide();
        m_headersButtons->hide();
    }
}

void
NotationWidget::slotCloseHeaders()
{
    setHeadersVisible(false);
}

void
NotationWidget::hideOrShowRulers()
{
    if (m_linearMode) {
        if (m_tempoRulerIsVisible) m_tempoRuler->show();
        if (m_rawNoteRulerIsVisible) m_rawNoteRuler->show();
        if (m_chordNameRulerIsVisible) m_chordNameRuler->show();
        if (m_headersAreVisible) m_headersView->show();
        m_bottomStandardRuler->show();
        m_topStandardRuler->show();
    } else {
        if (m_tempoRulerIsVisible) m_tempoRuler->hide();
        if (m_rawNoteRulerIsVisible) m_rawNoteRuler->hide();
        if (m_chordNameRulerIsVisible) m_chordNameRuler->hide();
        if (m_headersAreVisible) m_headersView->hide();
        m_bottomStandardRuler->hide();
        m_topStandardRuler->hide();
    }
}

void
NotationWidget::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);
    slotHScroll();
}

void
NotationWidget::slotShowHeaderToolTip(QString toolTipText)
{
    QToolTip::showText(QCursor::pos(), toolTipText, this);
}

void
NotationWidget::slotHeadersResized(int width)
{
    // Set headers view width to accomodate headers width.   
    m_headersView->setFixedWidth(
        m_headersGroup->sizeHint().width() * m_hZoomFactor);
}

void
NotationWidget::slotSetSymbolInserter()
{
    slotSetTool(SymbolInserter::ToolName);
}

void
NotationWidget::slotSetInsertedSymbol(Symbol type)
{
    SymbolInserter *ci = dynamic_cast<SymbolInserter *>(m_currentTool);
    if (ci) ci->slotSetSymbol(type);
}

void
NotationWidget::setPointerPosition(timeT t)
{
    m_document->slotSetPointerPosition(t);
}

}

#include "NotationWidget.moc"
