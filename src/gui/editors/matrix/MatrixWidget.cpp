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
#include <QSettings>
#include <QLabel>

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
#include "gui/rulers/LoopRuler.h"

#include "gui/general/IconLoader.h"
#include "gui/general/ThornStyle.h"

#include "gui/studio/StudioControl.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"

#include "base/Composition.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/RulerScale.h"
#include "base/PropertyName.h"
#include "base/BaseProperties.h"
#include "base/Controllable.h"
#include "base/Studio.h"
#include "base/Instrument.h"
#include "base/InstrumentStaticSignals.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/SoftSynthDevice.h"
#include "base/MidiTypes.h"
#include "base/ColourMap.h"
#include "base/Colour.h"
#include "base/MidiTypes.h"

namespace Rosegarden
{


MatrixWidget::MatrixWidget(bool drumMode) :
    m_document(nullptr),
    m_view(nullptr),
    m_scene(nullptr),
    m_toolBox(nullptr),
    m_currentTool(nullptr),
    m_instrument(nullptr),
    m_drumMode(drumMode),
    m_onlyKeyMapping(false),
    m_playTracking(true),
    m_hZoomFactor(1.0),
    m_vZoomFactor(1.0),
    m_currentVelocity(100),
    m_referenceScale(nullptr),
    m_inMove(false),
    m_lastZoomWasHV(true),
    m_lastV(0),
    m_lastH(0),
    m_pitchRuler(nullptr),
    m_pianoView(nullptr),
    m_pianoScene(nullptr),
    m_localMapping(nullptr),
    m_topStandardRuler(nullptr),
    m_bottomStandardRuler(nullptr),
    m_tempoRuler(nullptr),
    m_chordNameRuler(nullptr),
    m_layout(nullptr),
    m_hSliderHacked(false),
    m_lastNote(0),
    m_hoverNoteIsVisible(true)
{
    //MATRIX_DEBUG << "MatrixWidget ctor";

    m_layout = new QGridLayout;
    setLayout(m_layout);

    // Remove thick black lines beetween rulers and matrix
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
    m_hpanner = new Panner;
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
                        HSLIDER_ROW, MAIN_COL, 1, 1);

    // Hide or show the horizontal scroll bar when needed
    connect(m_view->horizontalScrollBar(), &QAbstractSlider::rangeChanged,
            this, &MatrixWidget::slotHScrollBarRangeChanged);

    connect(m_view, &Panned::viewportChanged,
            m_hpanner, &Panner::slotSetPannedRect);

    connect(m_view, &Panned::viewportChanged,
            m_pianoView, &Panned::slotSetViewport);

    connect(m_view, &Panned::viewportChanged,
            m_controlsWidget, &ControlRulerWidget::slotSetPannedRect);

    connect(m_view, &Panned::zoomIn,
            this, &MatrixWidget::slotZoomIn);
    connect(m_view, &Panned::zoomOut,
            this, &MatrixWidget::slotZoomOut);

    connect(m_hpanner, &Panner::pannedRectChanged,
            m_view, &Panned::slotSetViewport);

    connect(m_hpanner, &Panner::pannedRectChanged,
            m_pianoView, &Panned::slotSetViewport);

    connect(m_hpanner, &Panner::pannedRectChanged,
            m_controlsWidget, &ControlRulerWidget::slotSetPannedRect);

    connect(m_view, &Panned::pannedContentsScrolled,
            this, &MatrixWidget::slotScrollRulers);

    connect(m_hpanner, &Panner::zoomIn,
            this, &MatrixWidget::slotSyncPannerZoomIn);

    connect(m_hpanner, &Panner::zoomOut,
            this, &MatrixWidget::slotSyncPannerZoomOut);

    connect(m_pianoView, &Panned::wheelEventReceived,
            m_view, &Panned::slotEmulateWheelEvent);

    connect(m_controlsWidget, &ControlRulerWidget::dragScroll,
            this, &MatrixWidget::slotEnsureTimeVisible);
    
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

    // scrollbar hack from notation, but this one only affects horizontal
    connect(m_view->horizontalScrollBar(), &QAbstractSlider::valueChanged,
            this, &MatrixWidget::slotInitialHSliderHack);

    // Make sure MatrixScene always gets mouse move events even when the
    // button isn't pressed.  This way the keys on the piano keyboard
    // to the left are always highlighted to show which note we are on.
    m_view->setMouseTracking(true);

    connect(RosegardenMainWindow::self()->getDocument(),
                &RosegardenDocument::documentModified,
            this, &MatrixWidget::slotDocumentModified);
}

MatrixWidget::~MatrixWidget()
{
    //MATRIX_DEBUG << "MatrixWidget::~MatrixWidget() - start";
    delete m_scene;
    delete m_pianoScene;
    delete m_localMapping;
    //MATRIX_DEBUG << "MatrixWidget::~MatrixWidget() - end";
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

    m_hpanner->setScene(m_scene);

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
            this, SLOT(slotPointerPositionChanged(timeT)));
    connect(m_bottomStandardRuler, SIGNAL(dragPointerToPosition(timeT)),
            this, SLOT(slotPointerPositionChanged(timeT)));

    connect(m_topStandardRuler->getLoopRuler(), &LoopRuler::dragLoopToPosition,
            this, &MatrixWidget::slotEnsureTimeVisible);
    connect(m_bottomStandardRuler->getLoopRuler(), &LoopRuler::dragLoopToPosition,
            this, &MatrixWidget::slotEnsureTimeVisible);

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
    delete m_localMapping;
    m_localMapping = nullptr;    // To avoid a double delete
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
            //MATRIX_DEBUG << "MatrixView: Instrument has key mapping: " << mapping->getName();
            m_localMapping = new MidiKeyMapping(*mapping);
            m_localMapping->extend();
            isPercussion = true;
        } else {
            //MATRIX_DEBUG << "MatrixView: Instrument has no key mapping";
            isPercussion = false;
        }
    }
    if (mapping && !m_localMapping->getMap().empty()) {
        m_pitchRuler = new PercussionPitchRuler(nullptr, m_localMapping,
                                                m_scene->getYResolution());
    } else {
        if (m_onlyKeyMapping) {
            //!!! In such a case, a matrix resolution of 11 is used.
            // (See comments in MatrixScene::setSegments())
            // As the piano keyboard works only with a resolution of 7, an
            // empty key mapping will be used in place of the keyboard.
            m_localMapping = new MidiKeyMapping();
            m_localMapping->getMap()[0] = "";  //!!! extent() doesn't work ???
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

    QObject::connect
    (m_pitchRuler, &PitchRuler::hoveredOverKeyChanged,
     this, &MatrixWidget::slotHoveredOverKeyChanged);

    QObject::connect
    (m_pitchRuler, &PitchRuler::keyPressed,
     this, &MatrixWidget::slotKeyPressed);

    QObject::connect
    (m_pitchRuler, &PitchRuler::keySelected,
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
    if (!m_scene) return false;
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
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
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
    if (m_referenceScale) m_referenceScale->setYZoomFactor(m_vZoomFactor);
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
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
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
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
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
    if (!m_scene) return nullptr;
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
    if (!m_scene) return nullptr;
    return m_scene->getSnapGrid();
}

void
MatrixWidget::setSnap(timeT t)
{
    if (!m_scene) return;
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
    if (!m_scene) return nullptr;
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
    slotPointerPositionChanged(m_document->getComposition().getPosition(), false);
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
    slotPointerPositionChanged(m_document->getComposition().getPosition(), false);
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
    if (m_hoverNoteIsVisible) {
        m_pitchRuler->drawHoverNote(e->pitch);
    }
    m_pianoView->update();   // Needed to remove black trailers left by
                             // hover note at high zoom levels

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
        QTimer::singleShot(100, this, &MatrixWidget::slotEnsureLastMouseMoveVisible);
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
MatrixWidget::setHoverNoteVisible(bool visible)
{
    m_hoverNoteIsVisible = visible;
    if (! visible) m_pitchRuler->hideHoverNote();
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
MatrixWidget::setDrawTool()
{
    slotSetTool(MatrixPainter::ToolName());
}

void
MatrixWidget::setEraseTool()
{
    slotSetTool(MatrixEraser::ToolName());
}

void
MatrixWidget::setSelectAndEditTool()
{
    slotSetTool(MatrixSelector::ToolName());

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
    slotSetTool(MatrixMover::ToolName());
}

void
MatrixWidget::setResizeTool()
{
    slotSetTool(MatrixResizer::ToolName());
}

void
MatrixWidget::setVelocityTool()
{
    slotSetTool(MatrixVelocity::ToolName());
}

void
MatrixWidget::setScrollToFollowPlayback(bool tracking)
{
    m_playTracking = tracking;
    if (m_playTracking) {
        m_view->ensurePositionPointerInView(true);
    }
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
            it != list.end(); ++it) {

        // Pitch Bend is treated separately now, and there's no point in adding
        // "unsupported" controllers to the menu, so skip everything else
        if (it->getType() != Controller::EventType) continue;

        QString hexValue;
        hexValue.sprintf("(0x%x)", it->getControllerValue());

        // strings extracted from data files must be QObject::tr()
        QString itemStr = QObject::tr("%1 Controller %2 %3")
                                     .arg(QObject::tr(it->getName().c_str()))
                                     .arg(it->getControllerValue())
                                     .arg(hexValue);
        
        if (name != itemStr) continue;

        //MATRIX_DEBUG << "  name: " << name << " should match  itemStr: " << itemStr;

        m_controlsWidget->slotAddControlRuler(*it);

//      if (i == menuIndex) m_controlsWidget->slotAddControlRuler(*p);
//      else i++;
    }   
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
MatrixWidget::slotPointerPositionChanged(timeT t, bool moveView)
{
    QObject *s = sender();
    bool fromDocument = (s == m_document);

    if (!m_scene) return;

    double sceneX = m_scene->getRulerScale()->getXForTime(t);

    // Find the limits of the current segment
    Segment *currentSeg = getCurrentSegment();
    if (currentSeg && !moveView) {
        double segSceneTime = m_scene->getRulerScale()->getXForTime(currentSeg->getStartTime());
        if (segSceneTime > sceneX) {
            // Move pointer to start of current segment
            sceneX = segSceneTime;
        } else {
            segSceneTime = m_scene->getRulerScale()->getXForTime(
                    currentSeg->getEndMarkerTime());
            if (segSceneTime < sceneX) {
                   sceneX = segSceneTime;
            }
        }
    }
    
    // Never move the pointer outside the scene (else the scene will grow)
    double x1 = m_scene->sceneRect().x();
    double x2 = x1 + m_scene->sceneRect().width();

    if ((sceneX < x1) || (sceneX > x2)) {
        m_view->hidePositionPointer();
        m_hpanner->slotHidePositionPointer();
    } else {
        m_view->showPositionPointer(sceneX);
        m_hpanner->slotShowPositionPointer(sceneX);
    }

    if (m_playTracking || !fromDocument) {
        if (moveView) m_view->ensurePositionPointerInView(fromDocument);
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
    slotScrollRulers();
}

void
MatrixWidget::slotHorizontalThumbwheelMoved(int v)
{
    // limits sanity check
    if (v < -25) v = -25;
    if (v > 60) v = 60;
    if (m_lastH < -25) m_lastH = -25;
    if (m_lastH > 60) m_lastH = 60;

    int steps = v - m_lastH;
    if (steps < 0) steps *= -1;

    bool zoomingIn = (v > m_lastH);
    double newZoom = m_hZoomFactor;

    for (int i = 0; i < steps; ++i) {
        if (zoomingIn) newZoom *= 1.1;
        else newZoom /= 1.1;
    }

    //MATRIX_DEBUG << "v is: " << v << " h zoom factor was: " << m_lastH << " now: " << newZoom << " zooming " << (zoomingIn ? "IN" : "OUT");

    setHorizontalZoomFactor(newZoom);
    m_lastH = v;
    m_lastZoomWasHV = false;
}

void
MatrixWidget::slotVerticalThumbwheelMoved(int v)
{
    // limits sanity check
    if (v < -25) v = -25;
    if (v > 60) v = 60;
    if (m_lastV < -25) m_lastV = -25;
    if (m_lastV > 60) m_lastV = 60;

    int steps = v - m_lastV;
    if (steps < 0) steps *= -1;

    bool zoomingIn = (v > m_lastV);
    double newZoom = m_vZoomFactor;

    for (int i = 0; i < steps; ++i) {
        if (zoomingIn) newZoom *= 1.1;
        else newZoom /= 1.1;
    }

    //MATRIX_DEBUG << "v is: " << v << " z zoom factor was: " << m_lastV << " now: " << newZoom << " zooming " << (zoomingIn ? "IN" : "OUT");

    setVerticalZoomFactor(newZoom);
    m_lastV = v;
    m_lastZoomWasHV = false;
}

void
MatrixWidget::slotPrimaryThumbwheelMoved(int v)
{
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
    // zoomInFromPanner() here, so we will look at the number of steps
    // between the old value and the last one, and call the slot that many times
    // in order to enforce the 1:1 relationship.
    int steps = v - m_lastHVzoomValue;
    if (steps < 0) steps *= -1;

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
    //MATRIX_DEBUG << "MatrixWidget::slotResetZoomClicked()";

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
MatrixWidget::slotInitialHSliderHack(int)
{
    if (m_hSliderHacked) return;

    m_hSliderHacked = true;

    //MATRIX_DEBUG << "MatrixWidget::slotInitialHSliderHack()";
    //MATRIX_DEBUG << "  h slider position was: " << m_view->horizontalScrollBar()->sliderPosition();
    m_view->horizontalScrollBar()->setSliderPosition(0);
    //MATRIX_DEBUG << "  h slider position now: " << m_view->horizontalScrollBar()->sliderPosition();
}

void
MatrixWidget::slotSegmentChangerMoved(int v)
{
    // see comments in slotPrimaryThumbWheelMoved() for an explanation of that
    // mechanism, which is repurposed and simplified here

    if (v < -120) v = -120;
    if (v > 120) v = 120;
    if (m_lastSegmentChangerValue < -120) m_lastSegmentChangerValue = -120;
    if (m_lastSegmentChangerValue > 120) m_lastSegmentChangerValue = 120;

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
    //!!! TODO : test if regeneration is really needed before doing it
    generatePitchRuler();
}

void
MatrixWidget::updateSegmentChangerBackground()
{
    // set the changer widget background to the now current segment's
    // background, and reset the tooltip style to compensate
    Colour c = m_document->getComposition().getSegmentColourMap().getColourByIndex(m_scene->getCurrentSegment()->getColourIndex());

    QPalette palette = m_changerWidget->palette();
    palette.setColor(QPalette::Window, c.toQColor());
    m_changerWidget->setPalette(palette);

    // have to deal with all this ruckus to get a few pieces of info about the
    // track:
    Track *track = m_document->getComposition().getTrackById(m_scene->getCurrentSegment()->getTrack());
    int trackPosition = m_document->getComposition().getTrackPositionById(track->getId());
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
    //MATRIX_DEBUG << "MatrixWidget::slotHoveredOverKeyChanged(" << y << ")";

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

    // Don't do anything if we're part of a run up the keyboard
    // and the pitch hasn't changed
    if (m_lastNote == evPitch && repeating)  return ;

    // Save value
    m_lastNote = evPitch;
    if (!repeating) m_firstNote = evPitch;

    Composition &comp = m_document->getComposition();
    Studio &studio = m_document->getStudio();

    MatrixViewSegment *current = m_scene->getCurrentViewSegment();
    Track *track = comp.getTrackById(current->getSegment().getTrack());
    if (!track) return;

    Instrument *ins = studio.getInstrumentById(track->getInstrument());

    StudioControl::
        playPreviewNote(ins,
                        evPitch + current->getSegment().getTranspose(),
                        MidiMaxValue,
                        RealTime(-1, 0),  // Note-on, no note-off.
                        false);  // "MidiNote"
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

    // Don't do anything if we're part of a run up the keyboard
    // and the pitch hasn't changed
    //
    if (m_lastNote == evPitch && repeating) return ;

    // Save value
    m_lastNote = evPitch;
    if (!repeating) m_firstNote = evPitch;

    MatrixViewSegment *current = m_scene->getCurrentViewSegment();

    EventSelection *s = new EventSelection(current->getSegment());

    for (Segment::iterator i = current->getSegment().begin();
            current->getSegment().isBeforeEndMarker(i); ++i) {

        if ((*i)->isa(Note::EventType) &&
                (*i)->has(BaseProperties::PITCH)) {

            MidiByte p = (*i)->get
                         <Int>
                         (BaseProperties::PITCH);
            if (p >= std::min((int)m_firstNote, evPitch) &&
                    p <= std::max((int)m_firstNote, evPitch)) {
                s->addEvent(*i);
            }
        }
    }

    if (getSelection()) {
        // allow addFromSelection to deal with eliminating duplicates
        s->addFromSelection(getSelection());
    }

    setSelection(s, false);

    // now play the note as well
    Composition &comp = m_document->getComposition();
    Studio &studio = m_document->getStudio();

    Track *track = comp.getTrackById(current->getSegment().getTrack());
    if (!track) return;

    Instrument *ins = studio.getInstrumentById(track->getInstrument());

    StudioControl::
        playPreviewNote(ins,
                        evPitch + current->getSegment().getTranspose(),
                        MidiMaxValue,
                        RealTime(-1, 0),  // Note-on, no note-off.
                        false);  // "MidiNote"
}

void MatrixWidget::slotKeyReleased(unsigned int y, bool repeating)
{
    //RG_DEBUG << "slotKeyReleased(" << y << ")";

    int evPitch = m_scene->calculatePitchFromY(y);

    if (m_lastNote == evPitch && repeating) return;

    // send note off (note on at zero velocity)

    Composition &comp = m_document->getComposition();
    Studio &studio = m_document->getStudio();

    MatrixViewSegment *current = m_scene->getCurrentViewSegment();
    Track *track = comp.getTrackById(current->getSegment().getTrack());
    if (!track) return;

    Instrument *ins = studio.getInstrumentById(track->getInstrument());

    StudioControl::
        playPreviewNote(ins,
                        evPitch + current->getSegment().getTranspose(),
                        0,  // Velocity 0 => note-off
                        RealTime(0, 1),  // Need a non-zero duration to get through.
                        false);  // "MidiNote"
}

void
MatrixWidget::showInitialPointer()
{
    if (!m_scene) return;

    timeT t = getCurrentSegment()->getStartTime();

    double sceneX = m_scene->getRulerScale()->getXForTime(t);

    // Never move the pointer outside the scene (else the scene will grow)
    double x1 = m_scene->sceneRect().x();
    double x2 = x1 + m_scene->sceneRect().width();

    if ((sceneX < x1) || (sceneX > x2)) {
        // Place insertion marker at begining of scene.
        m_view->showPositionPointer(x1);
        m_hpanner->slotShowPositionPointer(x1);
    } else {
        m_view->showPositionPointer(sceneX);
        m_hpanner->slotShowPositionPointer(sceneX);
    }
}

/// Instrument is destroyed
void
MatrixWidget::
slotInstrumentGone()
{ m_instrument = nullptr; }

void
MatrixWidget::slotPlayPreviewNote(Segment * segment, int pitch)
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


}

