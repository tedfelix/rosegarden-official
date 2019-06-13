/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MatrixWidget]"

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
#include "MatrixViewSegment.h"
#include "PianoKeyboard.h"

#include "document/RosegardenDocument.h"

#include "gui/application/RosegardenMainWindow.h"

#include "gui/widgets/Panner.h"
#include "gui/widgets/Panned.h"
#include "gui/widgets/Thumbwheel.h"

#include "gui/rulers/PitchRuler.h"
#include "gui/rulers/PercussionPitchRuler.h"

#include "gui/rulers/ControlRulerWidget.h"
#include "gui/rulers/StandardRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/LoopRuler.h"

#include "gui/general/ThornStyle.h"

#include "gui/studio/StudioControl.h"

#include "misc/Debug.h"

#include "base/Composition.h"
#include "base/Instrument.h"
//#include "base/MidiProgram.h"
#include "base/RulerScale.h"
#include "base/PropertyName.h"
#include "base/BaseProperties.h"
#include "base/Controllable.h"
#include "base/Studio.h"
//#include "base/InstrumentStaticSignals.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/SoftSynthDevice.h"
#include "base/ColourMap.h"
#include "base/Colour.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QGraphicsView>
#include <QGridLayout>
#include <QScrollBar>
#include <QTimer>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>
#include <QPushButton>


namespace Rosegarden
{


// Widgets vertical positions inside the main QGridLayout (m_layout).
enum {
    CHORDNAMERULER_ROW,
    TEMPORULER_ROW,
    TOPRULER_ROW,
    PANNED_ROW,  // Matrix
    BOTTOMRULER_ROW,
    CONTROLS_ROW,
    HSCROLLBAR_ROW,
    PANNER_ROW  // Navigation area
};

// Widgets horizontal positions inside the main QGridLayout (m_layout).
enum {
    HEADER_COL,  // Piano Ruler
    MAIN_COL,
};

MatrixWidget::MatrixWidget(bool drumMode) :
    m_document(nullptr),
    m_scene(nullptr),
    m_view(nullptr),
    m_playTracking(true),
    m_hZoomFactor(1.0),
    m_vZoomFactor(1.0),
    m_instrument(nullptr),
    m_localMapping(nullptr),
    m_pitchRuler(nullptr),
    m_pianoScene(nullptr),
    m_pianoView(nullptr),
    m_onlyKeyMapping(false),
    m_drumMode(drumMode),
    m_firstNote(0),
    m_lastNote(0),
    m_hoverNoteIsVisible(true),
    m_toolBox(nullptr),
    m_currentTool(nullptr),
    m_currentVelocity(100),
    m_lastZoomWasHV(true),
    m_lastH(0),
    m_lastV(0),
    m_chordNameRuler(nullptr),
    m_tempoRuler(nullptr),
    m_topStandardRuler(nullptr),
    m_bottomStandardRuler(nullptr),
    m_referenceScale(nullptr)
{
    //RG_DEBUG << "ctor";

    m_layout = new QGridLayout;
    setLayout(m_layout);

    // Remove thick black lines between rulers and matrix
    m_layout->setSpacing(0);

    // Remove black margins around the matrix
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_view = new Panned;
    m_view->setBackgroundBrush(Qt::white);
    m_view->setWheelZoomPan(true);
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

    // the segment changer roller
    m_changerWidget = new QWidget;
    m_changerWidget->setAutoFillBackground(true);
    QVBoxLayout *changerWidgetLayout = new QVBoxLayout;
    m_changerWidget->setLayout(changerWidgetLayout);

    bool useRed = true;
    m_segmentChanger = new Thumbwheel(Qt::Vertical, useRed);
    m_segmentChanger->setFixedWidth(18);
    m_segmentChanger->setMinimumValue(-120);
    m_segmentChanger->setMaximumValue(120);
    m_segmentChanger->setDefaultValue(0);
    m_segmentChanger->setShowScale(true);
    m_segmentChanger->setValue(60);
    m_segmentChanger->setSpeed(0.05);
    m_lastSegmentChangerValue = m_segmentChanger->getValue();
    connect(m_segmentChanger, &Thumbwheel::valueChanged, this,
            &MatrixWidget::slotSegmentChangerMoved);
    changerWidgetLayout->addWidget(m_segmentChanger);

    pannerLayout->addWidget(m_changerWidget);

    // the panner
    m_panner = new Panner;
    m_panner->setMaximumHeight(60);
    m_panner->setBackgroundBrush(Qt::white);
    m_panner->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);

    pannerLayout->addWidget(m_panner);

    // row, col, row span, col span
    QFrame *controls = new QFrame;

    QGridLayout *controlsLayout = new QGridLayout;
    controlsLayout->setSpacing(0);
    controlsLayout->setContentsMargins(0, 0, 0, 0);
    controls->setLayout(controlsLayout);

    m_HVzoom = new Thumbwheel(Qt::Vertical);
    m_HVzoom->setFixedSize(QSize(40, 40));
    m_HVzoom->setToolTip(tr("Zoom"));

    // +/- 20 clicks seems to be the reasonable limit
    m_HVzoom->setMinimumValue(-20);
    m_HVzoom->setMaximumValue(20);
    m_HVzoom->setDefaultValue(0);
    m_HVzoom->setBright(true);
    m_HVzoom->setShowScale(true);
    m_lastHVzoomValue = m_HVzoom->getValue();
    controlsLayout->addWidget(m_HVzoom, 0, 0, Qt::AlignCenter);

    connect(m_HVzoom, &Thumbwheel::valueChanged, this,
            &MatrixWidget::slotPrimaryThumbwheelMoved);

    m_Hzoom = new Thumbwheel(Qt::Horizontal);
    m_Hzoom->setFixedSize(QSize(50, 16));
    m_Hzoom->setToolTip(tr("Horizontal Zoom"));

    m_Hzoom->setMinimumValue(-25);
    m_Hzoom->setMaximumValue(60);
    m_Hzoom->setDefaultValue(0); 
    m_Hzoom->setBright(false);
    controlsLayout->addWidget(m_Hzoom, 1, 0);
    connect(m_Hzoom, &Thumbwheel::valueChanged, this,
            &MatrixWidget::slotHorizontalThumbwheelMoved);

    m_Vzoom = new Thumbwheel(Qt::Vertical);
    m_Vzoom->setFixedSize(QSize(16, 50));
    m_Vzoom->setToolTip(tr("Vertical Zoom"));
    m_Vzoom->setMinimumValue(-25);
    m_Vzoom->setMaximumValue(60);
    m_Vzoom->setDefaultValue(0);
    m_Vzoom->setBright(false);
    controlsLayout->addWidget(m_Vzoom, 0, 1, Qt::AlignRight);

    connect(m_Vzoom, &Thumbwheel::valueChanged, this,
            &MatrixWidget::slotVerticalThumbwheelMoved);

    // a blank QPushButton forced square looks better than the tool button did
    m_reset = new QPushButton;
    m_reset->setFixedSize(QSize(10, 10));
    m_reset->setToolTip(tr("Reset Zoom"));
    controlsLayout->addWidget(m_reset, 1, 1, Qt::AlignCenter);

    connect(m_reset, &QAbstractButton::clicked, this, 
            &MatrixWidget::slotResetZoomClicked);

    pannerLayout->addWidget(controls);

    m_layout->addWidget(panner, PANNER_ROW, HEADER_COL, 1, 2);

    // Rulers being not defined still, they can't be added to m_layout.
    // This will be done in setSegments().

    // Move the scroll bar from m_view to MatrixWidget
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_layout->addWidget(m_view->horizontalScrollBar(),
                        HSCROLLBAR_ROW, MAIN_COL, 1, 1);

    // Hide or show the horizontal scroll bar when needed
    connect(m_view->horizontalScrollBar(), &QAbstractSlider::rangeChanged,
            this, &MatrixWidget::slotHScrollBarRangeChanged);

    connect(m_view, &Panned::viewportChanged,
            m_panner, &Panner::slotSetPannedRect);

    connect(m_view, &Panned::viewportChanged,
            m_pianoView, &Panned::slotSetViewport);

    connect(m_view, &Panned::viewportChanged,
            m_controlsWidget, &ControlRulerWidget::slotSetPannedRect);

    connect(m_view, &Panned::zoomIn,
            this, &MatrixWidget::slotZoomIn);
    connect(m_view, &Panned::zoomOut,
            this, &MatrixWidget::slotZoomOut);

    connect(m_panner, &Panner::pannedRectChanged,
            m_view, &Panned::slotSetViewport);

    connect(m_panner, &Panner::pannedRectChanged,
            m_pianoView, &Panned::slotSetViewport);

    connect(m_panner, &Panner::pannedRectChanged,
            m_controlsWidget, &ControlRulerWidget::slotSetPannedRect);

    connect(m_view, &Panned::pannedContentsScrolled,
            this, &MatrixWidget::slotScrollRulers);

    connect(m_panner, &Panner::zoomIn,
            this, &MatrixWidget::slotSyncPannerZoomIn);

    connect(m_panner, &Panner::zoomOut,
            this, &MatrixWidget::slotSyncPannerZoomOut);

    connect(m_pianoView, &Panned::wheelEventReceived,
            m_view, &Panned::slotEmulateWheelEvent);

    // Connect ControlRulerWidget for Auto-Scroll.
    connect(m_controlsWidget, &ControlRulerWidget::mousePress,
            this, &MatrixWidget::slotCRWMousePress);
    connect(m_controlsWidget, &ControlRulerWidget::mouseMove,
            this, &MatrixWidget::slotCRWMouseMove);
    connect(m_controlsWidget, &ControlRulerWidget::mouseRelease,
            this, &MatrixWidget::slotCRWMouseRelease);

    m_toolBox = new MatrixToolBox(this);

    // Relay context help from matrix tools
    connect(m_toolBox, &BaseToolBox::showContextHelp,
            this, &MatrixWidget::showContextHelp);

    // Relay context help from matrix rulers
    connect(m_controlsWidget, &ControlRulerWidget::showContextHelp,
            this, &MatrixWidget::showContextHelp);

    MatrixMover *matrixMoverTool = dynamic_cast <MatrixMover *> (m_toolBox->getTool(MatrixMover::ToolName()));
    if (matrixMoverTool) {
        connect(matrixMoverTool, SIGNAL(hoveredOverNoteChanged(int, bool, timeT)),
                m_controlsWidget, SLOT(slotHoveredOverNoteChanged(int, bool, timeT)));
    }

//    MatrixVelocity *matrixVelocityTool = dynamic_cast <MatrixVelocity *> (m_toolBox->getTool(MatrixVelocity::ToolName()));
//    if (matrixVelocityTool) {
//        connect(matrixVelocityTool, SIGNAL(hoveredOverNoteChanged()),
//                m_controlsWidget, SLOT(slotHoveredOverNoteChanged()));
//    }

    connect(this, &MatrixWidget::toolChanged,
            m_controlsWidget, &ControlRulerWidget::slotSetToolName);

    // Make sure MatrixScene always gets mouse move events even when the
    // button isn't pressed.  This way the keys on the piano keyboard
    // to the left are always highlighted to show which note we are on.
    m_view->setMouseTracking(true);

    connect(RosegardenMainWindow::self()->getDocument(),
                &RosegardenDocument::documentModified,
            this, &MatrixWidget::slotDocumentModified);

    connect(&m_autoScrollTimer, &QTimer::timeout,
            this, &MatrixWidget::slotOnAutoScrollTimer);
}

MatrixWidget::~MatrixWidget()
{
    // ??? QSharedPointer would be nice, but it opens up a can of worms
    //     since this is passed to reuse code that is used across the system.
    //     Panned and Panner in this case.  Probably worth doing.  Just a
    //     bit more work than usual.
    delete m_scene;
    // ??? See above.
    delete m_pianoScene;
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

    Composition &comp = document->getComposition();

    Track *track;
    Instrument *instr;

    // Look at segments to see if we need piano keyboard or key mapping ruler
    // (cf comment in MatrixScene::setSegments())
    m_onlyKeyMapping = true;
    std::vector<Segment *>::iterator si;
    for (si=segments.begin(); si!=segments.end(); ++si) {
        track = comp.getTrackById((*si)->getTrack());
        instr = document->getStudio().getInstrumentById(track->getInstrument());
        if (instr) {
            if (!instr->getKeyMapping()) {
                m_onlyKeyMapping = false;
            }
        }
    }
    // Note : m_onlyKeyMapping, whose value is defined above,
    // must be set before calling m_scene->setSegments()

    delete m_scene;
    m_scene = new MatrixScene();
    m_scene->setMatrixWidget(this);
    m_scene->setSegments(document, segments);

    m_referenceScale = m_scene->getReferenceScale();

    connect(m_scene, &MatrixScene::mousePressed,
            this, &MatrixWidget::slotDispatchMousePress);

    connect(m_scene, &MatrixScene::mouseMoved,
            this, &MatrixWidget::slotDispatchMouseMove);

    connect(m_scene, &MatrixScene::mouseReleased,
            this, &MatrixWidget::slotDispatchMouseRelease);

    connect(m_scene, &MatrixScene::mouseDoubleClicked,
            this, &MatrixWidget::slotDispatchMouseDoubleClick);

    connect(m_scene, &MatrixScene::segmentDeleted,
            this, &MatrixWidget::segmentDeleted);

    connect(m_scene, &MatrixScene::sceneDeleted,
            this, &MatrixWidget::sceneDeleted);

    m_view->setScene(m_scene);

    m_toolBox->setScene(m_scene);

    m_panner->setScene(m_scene);

    connect(m_view, &Panned::mouseLeaves,
            this, &MatrixWidget::slotMouseLeavesView);

    generatePitchRuler();

    m_controlsWidget->setSegments(document, segments);
    m_controlsWidget->setViewSegment((ViewSegment *)m_scene->getCurrentViewSegment());
    m_controlsWidget->setRulerScale(m_referenceScale);

    // For some reason this doesn't work in the constructor - not looked in detail
    // ( ^^^ it's because m_scene is only set after construction --cc)
    connect(m_scene, &MatrixScene::currentViewSegmentChanged,
            m_controlsWidget, &ControlRulerWidget::slotSetCurrentViewSegment);
    
    connect(m_scene, SIGNAL(selectionChanged(EventSelection *)),
            m_controlsWidget, SLOT(slotSelectionChanged(EventSelection *)));

    connect(m_controlsWidget, &ControlRulerWidget::childRulerSelectionChanged,
            m_scene, &MatrixScene::slotRulerSelectionChanged);

    connect(m_scene, SIGNAL(selectionChanged()),
            this, SIGNAL(selectionChanged()));

    m_topStandardRuler = new StandardRuler(document,
                                           m_referenceScale,
                                           false);

    m_bottomStandardRuler = new StandardRuler(document,
                                               m_referenceScale,
                                               true);

    m_tempoRuler = new TempoRuler(m_referenceScale,
                                  document,
                                  24,     // height
                                  true,   // small
                                  ThornStyle::isEnabled());

    m_chordNameRuler = new ChordNameRuler(m_referenceScale,
                                          document,
                                          segments,
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
            this, SLOT(slotStandardRulerDrag(timeT)));
    connect(m_bottomStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotStandardRulerDrag(timeT)));

    connect(m_topStandardRuler->getLoopRuler(), &LoopRuler::startMouseMove,
            this, &MatrixWidget::slotSRStartMouseMove);
    connect(m_topStandardRuler->getLoopRuler(), &LoopRuler::stopMouseMove,
            this, &MatrixWidget::slotSRStopMouseMove);
    connect(m_bottomStandardRuler->getLoopRuler(), &LoopRuler::startMouseMove,
            this, &MatrixWidget::slotSRStartMouseMove);
    connect(m_bottomStandardRuler->getLoopRuler(), &LoopRuler::stopMouseMove,
            this, &MatrixWidget::slotSRStopMouseMove);

    connect(m_document, SIGNAL(pointerPositionChanged(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));

    m_chordNameRuler->setReady();

    updateSegmentChangerBackground();

    // hide the changer widget if only one segment
    if (segments.size() == 1) m_changerWidget->hide();
}

void
MatrixWidget::generatePitchRuler()
{
    delete m_pianoScene;   // Delete the old m_pitchRuler if any
    m_localMapping.reset();
    bool isPercussion = false;

    Composition &comp = m_document->getComposition();
    const MidiKeyMapping *mapping = nullptr;
    Track *track = comp.getTrackById(m_scene->getCurrentSegment()->getTrack());
    m_instrument = m_document->getStudio().
                            getInstrumentById(track->getInstrument());
    if (m_instrument) {

        // Make instrument tell us if it gets destroyed.
        connect(m_instrument, &QObject::destroyed,
                this, &MatrixWidget::slotInstrumentGone);

        mapping = m_instrument->getKeyMapping();
        if (mapping) {
            //RG_DEBUG << "generatePitchRuler(): Instrument has key mapping: " << mapping->getName();
            m_localMapping.reset(new MidiKeyMapping(*mapping));
            m_localMapping->extend();
            isPercussion = true;
        } else {
            //RG_DEBUG << "generatePitchRuler(): Instrument has no key mapping";
            isPercussion = false;
        }
    }
    if (mapping && !m_localMapping->getMap().empty()) {
        m_pitchRuler = new PercussionPitchRuler(nullptr, m_localMapping,
                                                m_scene->getYResolution());
    } else {
        if (m_onlyKeyMapping) {
            // In such a case, a matrix resolution of 11 is used.
            // (See comments in MatrixScene::setSegments().)
            // As the piano keyboard works only with a resolution of 7, an
            // empty key mapping will be used in place of the keyboard.
            m_localMapping.reset(new MidiKeyMapping());
            m_localMapping->getMap()[0] = "";  // extent() doesn't work?
            m_localMapping->getMap()[127] = "";
            m_pitchRuler = new PercussionPitchRuler(nullptr, m_localMapping,
                                                    m_scene->getYResolution());
        } else {
            m_pitchRuler = new PianoKeyboard(nullptr);
        }
    }

    m_pitchRuler->setFixedSize(m_pitchRuler->sizeHint());
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width());

    m_pianoScene = new QGraphicsScene();
    QGraphicsProxyWidget *pianoKbd = m_pianoScene->addWidget(m_pitchRuler);
    m_pianoView->setScene(m_pianoScene);
    m_pianoView->centerOn(pianoKbd);

    QObject::connect(m_pitchRuler, &PitchRuler::hoveredOverKeyChanged,
                     this, &MatrixWidget::slotHoveredOverKeyChanged);

    QObject::connect(m_pitchRuler, &PitchRuler::keyPressed,
                     this, &MatrixWidget::slotKeyPressed);

    QObject::connect(m_pitchRuler, &PitchRuler::keySelected,
                     this, &MatrixWidget::slotKeySelected);

    // Don't send the "note off" midi message to a percussion instrument
    // when clicking on the pitch ruler
    if (!isPercussion || !m_drumMode) {
        connect(m_pitchRuler, &PitchRuler::keyReleased,
                this, &MatrixWidget::slotKeyReleased);
    }

    // If piano scene and matrix scene don't have the same height
    // one may shift from the other when scrolling vertically
    QRectF viewRect = m_scene->sceneRect();
    QRectF pianoRect = m_pianoScene->sceneRect();
    pianoRect.setHeight(viewRect.height() + 18);
    m_pianoScene->setSceneRect(pianoRect);
    //@@@ The 18 pixels have been added empirically in line above to
    //    avoid any offset between matrix and pitchruler at the end of
    //    vertical scroll. I have no idea from where they come from.


    // Apply current zoom to the new pitch ruler
    if (m_lastZoomWasHV) {
        // Set the view's matrix.
        // ??? Why?  Why only in this case?  Why not in the other case?
        //     Why isn't this handled elsewhere?  Is it?
        QMatrix m;
        m.scale(m_hZoomFactor, m_vZoomFactor);
        m_view->setMatrix(m);
        // Only vertical zoom factor is applied to pitch ruler
        QMatrix m2;
        m2.scale(1, m_vZoomFactor);
        m_pianoView->setMatrix(m2);
        m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width());
    } else {
        // Only vertical zoom factor is applied to pitch ruler
        QMatrix m;
        m.scale(1.0, m_vZoomFactor);
        m_pianoView->setMatrix(m);
        m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width());
    }

    // Move vertically the pianoView scene to fit the matrix scene.
    QRect mr = m_view->rect();
    QRect pr = m_pianoView->rect();
    QRectF smr = m_view->mapToScene(mr).boundingRect();
    QRectF spr = m_pianoView->mapToScene(pr).boundingRect();
    m_pianoView->centerOn(spr.center().x(), smr.center().y());

    m_pianoView->update();
}

bool
MatrixWidget::segmentsContainNotes() const
{
    if (!m_scene)
        return false;

    return m_scene->segmentsContainNotes();
}

void
MatrixWidget::setHorizontalZoomFactor(double factor)
{
    // NOTE: scaling the keyboard up and down works well for the primary zoom
    // because it maintains the same aspect ratio for each step.  I tried a few
    // different ways to deal with this before deciding that since
    // independent-axis zoom is a separate and mutually exclusive subsystem,
    // about the only sensible thing we can do is keep the keyboard scaled at
    // 1.0 horizontally, and only scale it vertically.  Git'r done.

    m_hZoomFactor = factor;
    if (m_referenceScale)
        m_referenceScale->setXZoomFactor(m_hZoomFactor);
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
    // Only vertical zoom factor is applied to pitch ruler
    QMatrix m;
    m.scale(1.0, m_vZoomFactor);
    m_pianoView->setMatrix(m);
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width());
    slotScrollRulers();
}

void
MatrixWidget::setVerticalZoomFactor(double factor)
{
    m_vZoomFactor = factor;
    if (m_referenceScale)
        m_referenceScale->setYZoomFactor(m_vZoomFactor);
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
    // Only vertical zoom factor is applied to pitch ruler
    QMatrix m;
    m.scale(1.0, m_vZoomFactor);
    m_pianoView->setMatrix(m);
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width());
}

void
MatrixWidget::zoomInFromPanner()
{
    m_hZoomFactor /= 1.1;
    m_vZoomFactor /= 1.1;
    if (m_referenceScale)
        m_referenceScale->setXZoomFactor(m_hZoomFactor);
    QMatrix m;
    m.scale(m_hZoomFactor, m_vZoomFactor);
    m_view->setMatrix(m);
    // Only vertical zoom factor is applied to pitch ruler
    QMatrix m2;
    m2.scale(1, m_vZoomFactor);
    m_pianoView->setMatrix(m2);
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width());
    slotScrollRulers();
}

void
MatrixWidget::zoomOutFromPanner()
{
    m_hZoomFactor *= 1.1;
    m_vZoomFactor *= 1.1;
    if (m_referenceScale)
        m_referenceScale->setXZoomFactor(m_hZoomFactor);
    QMatrix m;
    m.scale(m_hZoomFactor, m_vZoomFactor);
    m_view->setMatrix(m);
    // Only vertical zoom factor is applied to pitch ruler
    QMatrix m2;
    m2.scale(1, m_vZoomFactor);
    m_pianoView->setMatrix(m2);
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width());
    slotScrollRulers();
}

void
MatrixWidget::slotScrollRulers()
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
    if (!m_scene)
        return nullptr;

    return m_scene->getSelection();
}

void
MatrixWidget::setSelection(EventSelection *s, bool preview)
{
    if (!m_scene)
        return;

    m_scene->setSelection(s, preview);
}

const SnapGrid *
MatrixWidget::getSnapGrid() const
{
    if (!m_scene)
        return nullptr;

    return m_scene->getSnapGrid();
}

void
MatrixWidget::setSnap(timeT t)
{
    if (!m_scene)
        return;

    m_scene->setSnap(t);
}

void
MatrixWidget::selectAll()
{
    if (!m_scene)
        return;

    m_scene->selectAll();
}

void
MatrixWidget::clearSelection()
{
    // Actually we don't clear the selection immediately: if we're
    // using some tool other than the select tool, then the first
    // press switches us back to the select tool.

    // ??? Why?  Plus there's a bug.  The toolbar button does not
    //     become pressed for the select tool.  Instead it still
    //     shows the old tool.  Recommend changing this to do what
    //     the user has asked.  Just call setSelection().  I've
    //     tested it and it works fine.

    MatrixSelector *selector = dynamic_cast<MatrixSelector *>(m_currentTool);

    if (!selector)
        setSelectAndEditTool();
    else
        setSelection(nullptr, false);
}

Segment *
MatrixWidget::getCurrentSegment()
{
    if (!m_scene)
        return nullptr;

    return m_scene->getCurrentSegment();
}

void
MatrixWidget::previousSegment()
{
    if (!m_scene)
        return;

    Segment *s = m_scene->getPriorSegment();
    if (s)
        m_scene->setCurrentSegment(s);
    updatePointer(m_document->getComposition().getPosition());
    updateSegmentChangerBackground();
}

void
MatrixWidget::nextSegment()
{
    if (!m_scene)
        return;

    Segment *s = m_scene->getNextSegment();
    if (s)
        m_scene->setCurrentSegment(s);
    updatePointer(m_document->getComposition().getPosition());
    updateSegmentChangerBackground();
}

Device *
MatrixWidget::getCurrentDevice()
{
    Segment *segment = getCurrentSegment();
    if (!segment)
        return nullptr;

    Studio &studio = m_document->getStudio();
    Instrument *instrument =
        studio.getInstrumentById
        (segment->getComposition()->getTrackById(segment->getTrack())->
         getInstrument());
    if (!instrument)
        return nullptr;

    return instrument->getDevice();
}

void
MatrixWidget::slotDispatchMousePress(const MatrixMouseEvent *e)
{
    if (!m_currentTool)
        return;

    // Check for left and right *first*
    if ((e->buttons & Qt::LeftButton)  &&  (e->buttons & Qt::RightButton)) {
        m_currentTool->handleMidButtonPress(e);
    } else if (e->buttons & Qt::LeftButton) {
        m_currentTool->handleLeftButtonPress(e);
    } else if (e->buttons & Qt::MidButton) {
        m_currentTool->handleMidButtonPress(e);
    } else if (e->buttons & Qt::RightButton) {
        m_currentTool->handleRightButtonPress(e);
    }

    startAutoScroll();
}

void
MatrixWidget::slotDispatchMouseMove(const MatrixMouseEvent *e)
{
    if (m_hoverNoteIsVisible)
        m_pitchRuler->drawHoverNote(e->pitch);

    // Needed to remove black trailers left by hover note at high zoom levels.
    m_pianoView->update();

    if (!m_currentTool)
        return;

    m_followMode = m_currentTool->handleMouseMove(e);
}

void
MatrixWidget::slotEnsureTimeVisible(timeT t)
{
    QPointF pos = m_view->mapToScene(0,m_view->height()/2);

    pos.setX(m_scene->getRulerScale()->getXForTime(t));

    if (m_scene)
        m_scene->constrainToSegmentArea(pos);

    m_view->ensureVisible(QRectF(pos, pos));
}

void
MatrixWidget::slotDispatchMouseRelease(const MatrixMouseEvent *e)
{
    stopAutoScroll();

    if (!m_currentTool)
        return;

    m_currentTool->handleMouseRelease(e);
}

void
MatrixWidget::slotDispatchMouseDoubleClick(const MatrixMouseEvent *e)
{
    if (!m_currentTool)
        return;

    m_currentTool->handleMouseDoubleClick(e);
}

void
MatrixWidget::setHoverNoteVisible(bool visible)
{
    m_hoverNoteIsVisible = visible;

    if (!visible)
        m_pitchRuler->hideHoverNote();
}

void
MatrixWidget::setCanvasCursor(QCursor c)
{
    if (m_view)
        m_view->viewport()->setCursor(c);
}

void
MatrixWidget::setTool(QString name)
{
    MatrixTool *tool = dynamic_cast<MatrixTool *>(m_toolBox->getTool(name));
    if (!tool)
        return;

    if (m_currentTool)
        m_currentTool->stow();

    m_currentTool = tool;
    m_currentTool->ready();
    emit toolChanged(name);
}

void
MatrixWidget::setDrawTool()
{
    setTool(MatrixPainter::ToolName());
}

void
MatrixWidget::setEraseTool()
{
    setTool(MatrixEraser::ToolName());
}

void
MatrixWidget::setSelectAndEditTool()
{
    setTool(MatrixSelector::ToolName());

    MatrixSelector *selector = dynamic_cast<MatrixSelector *>(m_currentTool);
    if (selector) {
        //RG_DEBUG << "setSelectAndEditTool(): selector successfully set";

        // ??? This will pile up connections each time we select the arrow
        //     tool.  Need to use Qt::AutoConnection | Qt::UniqueConnection.
        //     This will cause connect() to fail.  Not sure how.  Might
        //     need to check for errors.
        connect(selector, &MatrixSelector::editTriggerSegment,
                this, &MatrixWidget::editTriggerSegment);
    }
}

void
MatrixWidget::setMoveTool()
{
    setTool(MatrixMover::ToolName());
}

void
MatrixWidget::setResizeTool()
{
    setTool(MatrixResizer::ToolName());
}

void
MatrixWidget::setVelocityTool()
{
    setTool(MatrixVelocity::ToolName());
}

void
MatrixWidget::setScrollToFollowPlayback(bool tracking)
{
    m_playTracking = tracking;

    if (m_playTracking)
        m_view->ensurePositionPointerInView(true);
}

void
MatrixWidget::showVelocityRuler()
{
    m_controlsWidget->slotTogglePropertyRuler(BaseProperties::VELOCITY);
}

void
MatrixWidget::showPitchBendRuler()
{
    m_controlsWidget->slotToggleControlRuler("PitchBend");
}

void
MatrixWidget::addControlRuler(QAction *action)
{
    QString name = action->text();

    //RG_DEBUG << "addControlRuler()";
    //RG_DEBUG << "  my name is " << name;

    // we just cheaply paste the code from MatrixView that created the menu to
    // figure out what its indices must point to (and thinking about this whole
    // thing, I bet it's all buggy as hell in a multi-track view where the
    // active segment can change, and the segment's track's device could be
    // completely different from whatever was first used to create the menu...
    // there will probably be refresh problems and crashes and general bugginess
    // 20% of the time, but a solution that works 80% of the time is worth
    // shipping, I just read on some blog, and damn the torpedoes)
    Controllable *c =
        dynamic_cast<MidiDevice *>(getCurrentDevice());
    if (!c) {
        c = dynamic_cast<SoftSynthDevice *>(getCurrentDevice());
        if (!c)
            return ;
    }

    const ControlList &list = c->getControlParameters();

    QString itemStr;
//  int i = 0;

    for (ControlList::const_iterator it = list.begin();
         it != list.end();
         ++it) {

        // Pitch Bend is treated separately now, and there's no point in adding
        // "unsupported" controllers to the menu, so skip everything else
        if (it->getType() != Controller::EventType)
            continue;

        QString hexValue;
        hexValue.sprintf("(0x%x)", it->getControllerValue());

        // strings extracted from data files must be QObject::tr()
        QString itemStr = QObject::tr("%1 Controller %2 %3")
                                     .arg(QObject::tr(it->getName().c_str()))
                                     .arg(it->getControllerValue())
                                     .arg(hexValue);
        
        if (name != itemStr)
            continue;

        //RG_DEBUG << "addControlRuler(): name: " << name << " should match  itemStr: " << itemStr;

        m_controlsWidget->slotAddControlRuler(*it);

//      if (i == menuIndex) m_controlsWidget->slotAddControlRuler(*p);
//      else i++;
    }   
}

void
MatrixWidget::slotHScrollBarRangeChanged(int min, int max)
{
    if (max > min)
        m_view->horizontalScrollBar()->show();
    else
        m_view->horizontalScrollBar()->hide();
}

void
MatrixWidget::updatePointer(timeT t)
{
    if (!m_scene)
        return;

    // Convert to scene X coords.
    double pointerX = m_scene->getRulerScale()->getXForTime(t);

    // Compute the limits of the scene.
    double sceneXMin = m_scene->sceneRect().left();
    double sceneXMax = m_scene->sceneRect().right();

    // If the pointer has gone outside the limits
    if (pointerX < sceneXMin  ||  sceneXMax < pointerX) {
        // Never move the pointer outside the scene (else the scene will grow)
        m_view->hidePositionPointer();
        m_panner->slotHidePositionPointer();
    } else {
        m_view->showPositionPointer(pointerX);
        m_panner->slotShowPositionPointer(pointerX);
    }
}

void
MatrixWidget::slotPointerPositionChanged(timeT t)
{
    // ??? We don't really need "t".  We could just use
    //     m_document->getComposition().getPosition().

    updatePointer(t);

    // Auto-scroll

    if (m_playTracking)
        m_view->ensurePositionPointerInView(true);  // page
}

void
MatrixWidget::slotStandardRulerDrag(timeT t)
{
    updatePointer(t);
}

void
MatrixWidget::slotSRStartMouseMove()
{
    m_followMode = FOLLOW_HORIZONTAL;

    startAutoScroll();
}

void
MatrixWidget::slotSRStopMouseMove()
{
    stopAutoScroll();
}

void
MatrixWidget::slotCRWMousePress()
{
    startAutoScroll();
}

void
MatrixWidget::slotCRWMouseMove(FollowMode followMode)
{
    m_followMode = followMode;
}

void
MatrixWidget::slotCRWMouseRelease()
{
    stopAutoScroll();
}

void
MatrixWidget::setTempoRulerVisible(bool visible)
{
    if (visible)
        m_tempoRuler->show();
    else
        m_tempoRuler->hide();
}

void
MatrixWidget::setChordNameRulerVisible(bool visible)
{
    if (visible)
        m_chordNameRuler->show();
    else
        m_chordNameRuler->hide();
}

void
MatrixWidget::showEvent(QShowEvent * event)
{
    QWidget::showEvent(event);
    slotScrollRulers();
}

void
MatrixWidget::slotHorizontalThumbwheelMoved(int v)
{
    // limits sanity check
    if (v < -25)
        v = -25;
    if (v > 60)
        v = 60;
    if (m_lastH < -25)
        m_lastH = -25;
    if (m_lastH > 60)
        m_lastH = 60;

    int steps = v - m_lastH;
    if (steps < 0)
        steps *= -1;

    bool zoomingIn = (v > m_lastH);
    double newZoom = m_hZoomFactor;

    for (int i = 0; i < steps; ++i) {
        if (zoomingIn)
            newZoom *= 1.1;
        else
            newZoom /= 1.1;
    }

    //RG_DEBUG << "slotHorizontalThumbwheelMoved(): v is: " << v << " h zoom factor was: " << m_lastH << " now: " << newZoom << " zooming " << (zoomingIn ? "IN" : "OUT");

    setHorizontalZoomFactor(newZoom);
    m_lastH = v;
    m_lastZoomWasHV = false;
}

void
MatrixWidget::slotVerticalThumbwheelMoved(int v)
{
    // limits sanity check
    if (v < -25)
        v = -25;
    if (v > 60)
        v = 60;
    if (m_lastV < -25)
        m_lastV = -25;
    if (m_lastV > 60)
        m_lastV = 60;

    int steps = v - m_lastV;
    if (steps < 0)
        steps *= -1;

    bool zoomingIn = (v > m_lastV);
    double newZoom = m_vZoomFactor;

    for (int i = 0; i < steps; ++i) {
        if (zoomingIn)
            newZoom *= 1.1;
        else
            newZoom /= 1.1;
    }

    //RG_DEBUG << "slotVerticalThumbwheelMoved(): v is: " << v << " z zoom factor was: " << m_lastV << " now: " << newZoom << " zooming " << (zoomingIn ? "IN" : "OUT");

    setVerticalZoomFactor(newZoom);
    m_lastV = v;
    m_lastZoomWasHV = false;
}

void
MatrixWidget::slotPrimaryThumbwheelMoved(int v)
{
    // little bit of kludge work to deal with value manipulations that are
    // outside of the constraints imposed by the primary zoom wheel itself
    if (v < -20)
        v = -20;
    if (v > 20)
        v = 20;
    if (m_lastHVzoomValue < -20)
        m_lastHVzoomValue = -20;
    if (m_lastHVzoomValue > 20)
        m_lastHVzoomValue = 20;

    // When dragging the wheel up and down instead of mouse wheeling it, it
    // steps according to its speed.  I don't see a sure way (and after all
    // there are no docs!) to make sure dragging results in a smooth 1:1
    // relationship when compared with mouse wheeling, and we are just hijacking
    // zoomInFromPanner() here, so we will look at the number of steps
    // between the old value and the last one, and call the slot that many times
    // in order to enforce the 1:1 relationship.
    int steps = v - m_lastHVzoomValue;
    if (steps < 0)
        steps *= -1;

    for (int i = 0; i < steps; ++i) {
        if (v < m_lastHVzoomValue)
            zoomInFromPanner();
        else if (v > m_lastHVzoomValue)
            zoomOutFromPanner();
    }

    m_lastHVzoomValue = v;
    m_lastZoomWasHV = true;
}

void
MatrixWidget::slotResetZoomClicked()
{
    //RG_DEBUG << "slotResetZoomClicked()";

    m_hZoomFactor = 1.0;
    m_vZoomFactor = 1.0;
    if (m_referenceScale) {
        m_referenceScale->setXZoomFactor(m_hZoomFactor);
        m_referenceScale->setYZoomFactor(m_vZoomFactor);
    }
    m_view->resetMatrix();
    QMatrix m;
    m.scale(m_hZoomFactor, m_vZoomFactor);
    m_view->setMatrix(m);
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
    // Only vertical zoom factor is applied to pitch ruler
    QMatrix m2;
    m2.scale(1, m_vZoomFactor);
    m_pianoView->setMatrix(m2);
    m_pianoView->setFixedWidth(m_pitchRuler->sizeHint().width());
    slotScrollRulers();

    // scale factor 1.0 = 100% zoom
    m_Hzoom->setValue(1);
    m_Vzoom->setValue(1);
    m_HVzoom->setValue(0);
    m_lastHVzoomValue = 0;
    m_lastH = 0;
    m_lastV = 0;
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

void
MatrixWidget::slotSegmentChangerMoved(int v)
{
    // see comments in slotPrimaryThumbWheelMoved() for an explanation of that
    // mechanism, which is repurposed and simplified here

    if (v < -120)
        v = -120;
    if (v > 120)
        v = 120;
    if (m_lastSegmentChangerValue < -120)
        m_lastSegmentChangerValue = -120;
    if (m_lastSegmentChangerValue > 120)
        m_lastSegmentChangerValue = 120;

    int steps = v - m_lastSegmentChangerValue;
    if (steps < 0) steps *= -1;

    for (int i = 0; i < steps; ++i) {
        if (v < m_lastSegmentChangerValue)
            nextSegment();
        else if (v > m_lastSegmentChangerValue)
            previousSegment();
    }

    m_lastSegmentChangerValue = v;
    updateSegmentChangerBackground();

    // If we are switching between a pitched instrument segment and a pecussion
    // segment or betwween two percussion segments with different percussion
    // sets, the pitch ruler may need to be regenerated.
    // TODO : test if regeneration is really needed before doing it
    generatePitchRuler();
}

void
MatrixWidget::updateSegmentChangerBackground()
{
    // set the changer widget background to the now current segment's
    // background, and reset the tooltip style to compensate
    Colour c = m_document->getComposition().getSegmentColourMap().
            getColourByIndex(m_scene->getCurrentSegment()->getColourIndex());

    QPalette palette = m_changerWidget->palette();
    palette.setColor(QPalette::Window, c.toQColor());
    m_changerWidget->setPalette(palette);

    // have to deal with all this ruckus to get a few pieces of info about the
    // track:
    Track *track = m_document->getComposition().getTrackById(
            m_scene->getCurrentSegment()->getTrack());
    int trackPosition = m_document->getComposition().getTrackPositionById(
            track->getId());
    QString trackLabel = QString::fromStdString(track->getLabel());

    // set up some tooltips...  I don't like this much, and it wants some kind
    // of dedicated float thing eventually, but let's not go nuts on a
    // last-minute feature
    m_segmentChanger->setToolTip(tr("<qt>Rotate wheel to change the active segment</qt>"));
    m_changerWidget->setToolTip(tr("<qt>Segment: \"%1\"<br>Track: %2 \"%3\"</qt>")
                                .arg(QString::fromStdString(m_scene->getCurrentSegment()->getLabel()))
                                .arg(trackPosition)
                                .arg(trackLabel));
}

void
MatrixWidget::slotHoveredOverKeyChanged(unsigned int y)
{
    //RG_DEBUG << "slotHoveredOverKeyChanged(" << y << ")";

    int evPitch = m_scene->calculatePitchFromY(y);
    m_pitchRuler->drawHoverNote(evPitch);
    m_pianoView->update();   // Needed to remove black trailers left by
                             // hover note at high zoom levels
}

void
MatrixWidget::slotMouseLeavesView()
{
    // The mouse has left the view, so the hover note in the pitch ruler
    // has to be unhighlighted.
    m_pitchRuler->hideHoverNote();

    // At high zoom levels, black trailers are left behind.  This makes
    // sure they are removed.  (Retest this.)
    m_pianoView->update();
}


void MatrixWidget::slotKeyPressed(unsigned int y, bool repeating)
{
    //RG_DEBUG << "slotKeyPressed(" << y << ")";

    slotHoveredOverKeyChanged(y);
    int evPitch = m_scene->calculatePitchFromY(y);

    // If we are part of a run up the keyboard, don't send redundant
    // note ons.  Otherwise we will send a note on for every tiny movement
    // of the mouse.
    if (m_lastNote == evPitch  &&  repeating)
        return;

    // Save value
    m_lastNote = evPitch;
    if (!repeating)
        m_firstNote = evPitch;

    Composition &comp = m_document->getComposition();
    Studio &studio = m_document->getStudio();

    MatrixViewSegment *current = m_scene->getCurrentViewSegment();
    Track *track = comp.getTrackById(current->getSegment().getTrack());
    if (!track)
        return;

    Instrument *ins = studio.getInstrumentById(track->getInstrument());

    StudioControl::playPreviewNote(
            ins,  // instrument
            evPitch + current->getSegment().getTranspose(),  // pitch
            MidiMaxValue,  // velocity
            RealTime(-1, 0),  // duration: Note-on, no note-off.
            false);  // oneshot
}

void MatrixWidget::slotKeySelected(unsigned int y, bool repeating)
{
    //RG_DEBUG << "slotKeySelected(" << y << ")";

    // This is called when shift is held down and a key is pressed on the
    // pitch ruler.  This will select all notes with this pitch in the
    // segment.

    slotHoveredOverKeyChanged(y);

//    getCanvasView()->scrollVertSmallSteps(y);
    
    int evPitch = m_scene->calculatePitchFromY(y);

    // If we are part of a run up the keyboard, don't send redundant
    // note ons.  Otherwise we will send a note on for every tiny movement
    // of the mouse.
    if (m_lastNote == evPitch  &&  repeating)
        return;

    // Save value
    m_lastNote = evPitch;
    if (!repeating)
        m_firstNote = evPitch;

    MatrixViewSegment *current = m_scene->getCurrentViewSegment();

    EventSelection *eventSelection = new EventSelection(current->getSegment());

    // For each Event in the current Segment...
    for (Segment::iterator i = current->getSegment().begin();
         current->getSegment().isBeforeEndMarker(i);
         ++i) {

        // If this is a Note event with a pitch...
        if ((*i)->isa(Note::EventType)  &&
            (*i)->has(BaseProperties::PITCH)) {

            MidiByte pitch = (*i)->get<Int>(BaseProperties::PITCH);

            // If this note is between the first note selected and
            // where we are now, select it.
            if (pitch >= std::min((int)m_firstNote, evPitch)  &&
                pitch <= std::max((int)m_firstNote, evPitch)) {
                eventSelection->addEvent(*i);
            }
        }
    }

    if (getSelection()) {
        // allow addFromSelection to deal with eliminating duplicates
        eventSelection->addFromSelection(getSelection());
    }

    setSelection(eventSelection, false);

    // now play the note as well
    Composition &comp = m_document->getComposition();
    Studio &studio = m_document->getStudio();

    Track *track = comp.getTrackById(current->getSegment().getTrack());
    if (!track)
        return;

    Instrument *ins = studio.getInstrumentById(track->getInstrument());

    StudioControl::playPreviewNote(
            ins,  // instrument
            evPitch + current->getSegment().getTranspose(),  // pitch
            MidiMaxValue,  // velocity
            RealTime(-1, 0),  // duration: Note-on, no note-off.
            false);  // oneshot
}

void MatrixWidget::slotKeyReleased(unsigned int y, bool repeating)
{
    //RG_DEBUG << "slotKeyReleased(" << y << ")";

    int evPitch = m_scene->calculatePitchFromY(y);

    // If we are part of a run up the keyboard, don't send a
    // note off for every tiny movement of the mouse.
    if (m_lastNote == evPitch  &&  repeating)
        return;

    // send note off (note on at zero velocity)

    Composition &comp = m_document->getComposition();
    Studio &studio = m_document->getStudio();

    MatrixViewSegment *current = m_scene->getCurrentViewSegment();
    Track *track = comp.getTrackById(current->getSegment().getTrack());
    if (!track)
        return;

    Instrument *ins = studio.getInstrumentById(track->getInstrument());

    StudioControl::playPreviewNote(
            ins,  // instrument
            evPitch + current->getSegment().getTranspose(),  // pitch
            0,  // velocity: 0 => note-off
            RealTime(0, 1),  // duration: Need non-zero to get through.
            false);  // oneshot
}

void
MatrixWidget::showInitialPointer()
{
    if (!m_scene)
        return;

    updatePointer(m_document->getComposition().getPosition());

    // QGraphicsView usually defaults to the center.  We want to
    // start at the left center.
    // Note: This worked fine at the end of setSegments() as well.
    //       This feels like a better place.  Especially since we
    //       might want to scroll to where the pointer is.
    // ??? Probably better to scroll left/right so that the beginning
    //     of the selected Segment is at the left edge.  That's a bit
    //     tricky, but if we don't do that, the user might end up someplace
    //     unexpected when editing multiple segments.
    m_view->centerOn(QPointF(0, m_view->sceneRect().center().y()));
}

void
MatrixWidget::slotInstrumentGone()
{
    m_instrument = nullptr;
}

void
MatrixWidget::slotPlayPreviewNote(Segment *segment, int pitch)
{
    m_scene->playNote(*segment, pitch, 100);
}

void
MatrixWidget::slotDocumentModified(bool)
{
    // This covers the test case when the user toggles the "Percussion"
    // checkbox on the MIPP.  Also covers the test case where the key
    // map changes due to a change in percussion program.

    // ??? Eventually, this should do a full refresh of the
    //     entire MatrixWidget and should cover all test cases where
    //     the UI must update.  Unfortunately, the design of MatrixWidget
    //     does not include an updateWidgets().  We'll probably have to
    //     redesign and rewrite MatrixWidget before this becomes possible.

    generatePitchRuler();
}

void
MatrixWidget::slotZoomIn()
{
    int v = m_lastH - 1;

    m_Hzoom->setValue(v);
    slotHorizontalThumbwheelMoved(v);
}

void
MatrixWidget::slotZoomOut()
{
    int v = m_lastH + 1;

    m_Hzoom->setValue(v);
    slotHorizontalThumbwheelMoved(v);
}

void
MatrixWidget::startAutoScroll()
{
    if (!m_autoScrollTimer.isActive())
        m_autoScrollTimer.start(30);  // msecs

    m_autoScrolling = true;
}

// We'll hit MaxScrollRate at this distance outside the viewport.
// ??? HiDPI: This needs to be bigger for the HiDPI case.
constexpr double maxDistance = 40;

double distanceToScrollRate(int distance)
{
    const double distanceNormalized = distance / maxDistance;
    // Apply a curve to reduce the touchiness.
    // Simple square curve.  Something more pronounced might be better.
    const double distanceWithCurve = distanceNormalized * distanceNormalized;

    const double minScrollRate = 1.2;
    const double maxScrollRate = 100;
    const double scrollRateRange = (maxScrollRate - minScrollRate);

    const double scrollRate = distanceWithCurve * scrollRateRange + minScrollRate;

    return std::min(scrollRate, maxScrollRate);
}

void
MatrixWidget::doAutoScroll()
{
    // Copied from RosegardenScrollView::doAutoScroll().
    // Will be copied to NotationWidget in the near future.

    // ??? Eventually, this will be used in three places, RosegardenScrollView,
    //     MatrixWidget, and NotationWidget.  Might be time to pull out into
    //     a separate class?  AutoScroller.  Clients will contain an instance
    //     and interact with it.

    // Make sure we're auto-scrolling.
    // ??? Necessary?  External users should only call start functions,
    //     not this.
    //startAutoScroll();

    const QPoint mousePos = m_view->mapFromGlobal(QCursor::pos());

    //RG_DEBUG << "doAutoScroll(): mousePos:" << mousePos;

    if (m_followMode & FOLLOW_HORIZONTAL) {

        // The following auto scroll behavior is patterned after Chromium,
        // Eclipse, and the GIMP.  Auto scroll will only happen if the
        // mouse is outside the viewport.  The auto scroll rate is
        // proportional to how far outside the viewport the mouse is.
        // If the right edge is too close to the edge of the screen
        // (e.g. when maximized), the auto scroll area is moved inside
        // of the viewport.

        int scrollX = 0;

        // If the mouse is to the left of the viewport
        if (mousePos.x() < 0) {

            // Set the scroll rate based on how far outside we are.
            scrollX = lround(-distanceToScrollRate(-mousePos.x()));

        } else {

            // Check if the mouse is to the right of the viewport

            // Assume we can place the auto scroll area outside the window.
            int xOffset = 0;

            const int rightSideOfScreen =
                    QApplication::desktop()->availableGeometry(this).right();

            const int rightSideOfViewport =
                    m_view->parentWidget()->mapToGlobal(
                            m_view->geometry().bottomRight()).x();

            const int spaceToTheRight = rightSideOfScreen - rightSideOfViewport;

            // If there's not enough space for the auto scroll area, move it
            // inside the viewport.
            if (spaceToTheRight < maxDistance)
                xOffset = static_cast<int>(-maxDistance + spaceToTheRight);

            // Limit where auto scroll begins.
            const int xMax = m_view->width() + xOffset;

            // If the mouse is to the right of the auto scroll limit
            if (mousePos.x() > xMax) {
                // Set the scroll rate based on how far outside we are.
                scrollX = lround(distanceToScrollRate(mousePos.x() - xMax));
            }

        }

        // Scroll if needed.
        if (scrollX) {
            QScrollBar *hScrollBar = m_view->horizontalScrollBar();
            hScrollBar->setValue(hScrollBar->value() + scrollX);
        }
    }

    if (m_followMode & FOLLOW_VERTICAL) {

        // This vertical auto scroll behavior is patterned after
        // Audacity.  Auto scroll will only happen if the mouse is
        // outside the viewport.  The auto scroll rate is fixed.

        int scrollY = 0;

        // If the mouse is above the viewport
        if (mousePos.y() < 0)
            scrollY = -10;

        // If the mouse is below the viewport
        if (mousePos.y() > m_view->height())
            scrollY = +10;

        // Scroll if needed.
        if (scrollY) {
            QScrollBar *vScrollBar = m_view->verticalScrollBar();
            vScrollBar->setValue(vScrollBar->value() + scrollY);
        }

    }
}

void
MatrixWidget::slotOnAutoScrollTimer()
{
    doAutoScroll();
}

void
MatrixWidget::stopAutoScroll()
{
    m_autoScrollTimer.stop();
    m_autoScrolling = false;
}


}

