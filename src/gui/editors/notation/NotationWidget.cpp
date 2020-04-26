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

#define RG_MODULE_STRING "[NotationWidget]"

#include "NotationWidget.h"
#define RG_NO_DEBUG_PRINT 1

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
#include "NotationStaff.h"

#include "base/RulerScale.h"
#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/PropertyName.h"
#include "base/BaseProperties.h"
#include "base/Controllable.h"
#include "base/Studio.h"
#include "base/Instrument.h"
#include "base/Device.h"
#include "base/MidiDevice.h"
#include "base/SoftSynthDevice.h"
#include "base/MidiTypes.h"
#include "base/ColourMap.h"
#include "base/Colour.h"
#include "base/Segment.h"

#include "document/RosegardenDocument.h"

#include "gui/application/RosegardenMainWindow.h"

#include "gui/widgets/Panner.h"
#include "gui/widgets/Panned.h"
#include "gui/widgets/Thumbwheel.h"

#include "gui/general/IconLoader.h"
#include "gui/general/ThornStyle.h"

#include "gui/rulers/ControlRulerWidget.h"
#include "gui/rulers/StandardRuler.h"
#include "gui/rulers/TempoRuler.h"
#include "gui/rulers/ChordNameRuler.h"
#include "gui/rulers/RawNoteRuler.h"
#include "gui/rulers/LoopRuler.h"

#include "gui/seqmanager/SequenceManager.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"

#include <QApplication>
#include <QGraphicsView>
#include <QGridLayout>
#include <QBoxLayout>
#include <QScrollBar>
#include <QTimer>
#include <QGraphicsProxyWidget>
#include <QToolTip>
#include <QToolButton>
#include <QSettings>
#include <QLabel>

namespace Rosegarden
{

NotationWidget::NotationWidget() :
    m_document(nullptr),
    m_view(nullptr),
    m_scene(nullptr),
    m_leftGutter(20),
    m_currentTool(nullptr),
    m_playTracking(true),
    m_hZoomFactor(1.0),
    m_vZoomFactor(1.0),
    m_referenceScale(nullptr),
    m_lastZoomWasHV(true),
    m_lastV(0),
    m_lastH(0),
    m_topStandardRuler(nullptr),
    m_bottomStandardRuler(nullptr),
    m_tempoRuler(nullptr),
    m_chordNameRuler(nullptr),
    m_rawNoteRuler(nullptr),
    m_controlRulerWidget(nullptr),
    m_headersGroup(nullptr),
    m_headersView(nullptr),
    m_headersScene(nullptr),
    m_headersButtons(nullptr),
    m_headersLastY(0),
    m_headersNeedRegeneration(false),
    m_headersTimer(nullptr),
    m_layout(nullptr),
    m_tempoRulerIsVisible(false),
    m_rawNoteRulerIsVisible(false),
    m_chordNameRulerIsVisible(false),
    m_headersAreVisible(false),
    m_chordMode(false),
    m_tupletMode(false),
    m_graceMode(false),
    m_tupledCount(2),
    m_untupledCount(3),
    m_updatesSuspended(false)
{
    m_layout = new QGridLayout;
    setLayout(m_layout);

    // Remove thick black lines beetween rulers and staves
    m_layout->setSpacing(0);

    // Remove black margins around the notation
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_view = new Panned;
    m_view->setBackgroundBrush(Qt::white);
    m_view->setRenderHints(QPainter::Antialiasing |
                           QPainter::TextAntialiasing |
                           QPainter::SmoothPixmapTransform);

    bool texture = false;
    QSettings settings;
    settings.beginGroup(NotationViewConfigGroup);
    texture = settings.value("backgroundtextures", true).toBool();
    settings.endGroup();

    QBrush bg = (texture ?
                QBrush(IconLoader().loadPixmap("bg-paper-grey"))
                :
                Qt::white);
    m_view->setBackgroundBrush(bg);
    m_layout->addWidget(m_view, PANNED_ROW, MAIN_COL, 1, 1);

    // Force the main notation scene row to expand when space available.
    m_layout->setRowStretch(PANNED_ROW, 1);

    m_panner = new QWidget;
    m_pannerLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    m_pannerLayout->setContentsMargins(0, 0, 0, 0);
    m_pannerLayout->setSpacing(0);
    m_panner->setLayout(m_pannerLayout);

    // the segment changer roller(s) (we have to have one horizontal one, and
    // one vertical one, to accommodate different layout modes)
    m_changerWidget = new QWidget;
    m_changerWidget->setAutoFillBackground(true);
    QVBoxLayout *changerWidgetLayout = new QVBoxLayout;
    m_changerWidget->setLayout(changerWidgetLayout);

    bool useRed = true;
    m_HsegmentChanger = new Thumbwheel(Qt::Vertical, useRed);
    m_HsegmentChanger->setFixedWidth(18);
    m_HsegmentChanger->setMinimumValue(-120);
    m_HsegmentChanger->setMaximumValue(120);
    m_HsegmentChanger->setDefaultValue(0);
    m_HsegmentChanger->setShowScale(true);
    m_HsegmentChanger->setValue(60);
    m_HsegmentChanger->setSpeed(0.05);

    m_lastSegmentChangerValue = m_HsegmentChanger->getValue();
    connect(m_HsegmentChanger, &Thumbwheel::valueChanged, this,
            &NotationWidget::slotSegmentChangerMoved);
    changerWidgetLayout->addWidget(m_HsegmentChanger);

    m_VsegmentChanger = new Thumbwheel(Qt::Horizontal, useRed);
    m_VsegmentChanger->setFixedHeight(18);
    m_VsegmentChanger->setMinimumValue(-120);
    m_VsegmentChanger->setMaximumValue(120);
    m_VsegmentChanger->setDefaultValue(0);
    m_VsegmentChanger->setShowScale(true);
    m_VsegmentChanger->setValue(60);
    m_VsegmentChanger->setSpeed(0.05);
    m_lastSegmentChangerValue = m_VsegmentChanger->getValue();
    connect(m_VsegmentChanger, &Thumbwheel::valueChanged, this,
            &NotationWidget::slotSegmentChangerMoved);
    changerWidgetLayout->addWidget(m_VsegmentChanger);
    m_VsegmentChanger->hide();

    m_pannerLayout->addWidget(m_changerWidget);

    // the panner
    m_hpanner = new Panner;
    m_hpanner->setMaximumHeight(80);
    m_hpanner->setBackgroundBrush(Qt::white);
    m_hpanner->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);
    m_hpanner->setRenderHints(nullptr);

    m_pannerLayout->addWidget(m_hpanner);

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
            &NotationWidget::slotPrimaryThumbwheelMoved);

    m_Hzoom = new Thumbwheel(Qt::Horizontal);
    m_Hzoom->setFixedSize(QSize(50, 16));
    m_Hzoom->setToolTip(tr("Horizontal Zoom"));

    m_Hzoom->setMinimumValue(-25);
    m_Hzoom->setMaximumValue(60);
    m_Hzoom->setDefaultValue(0);
    m_Hzoom->setBright(false);
    controlsLayout->addWidget(m_Hzoom, 1, 0);
    connect(m_Hzoom, &Thumbwheel::valueChanged, this,
            &NotationWidget::slotHorizontalThumbwheelMoved);

    m_Vzoom = new Thumbwheel(Qt::Vertical);
    m_Vzoom->setFixedSize(QSize(16, 50));
    m_Vzoom->setToolTip(tr("Vertical Zoom"));
    m_Vzoom->setMinimumValue(-25);
    m_Vzoom->setMaximumValue(60);
    m_Vzoom->setDefaultValue(0);
    m_Vzoom->setBright(false);
    controlsLayout->addWidget(m_Vzoom, 0, 1, Qt::AlignRight);

    connect(m_Vzoom, &Thumbwheel::valueChanged, this,
            &NotationWidget::slotVerticalThumbwheelMoved);

    // a blank QPushButton forced square looks better than the tool button did
    m_reset = new QPushButton;
    m_reset->setFixedSize(QSize(10, 10));
    m_reset->setToolTip(tr("Reset Zoom"));
    controlsLayout->addWidget(m_reset, 1, 1, Qt::AlignCenter);

    connect(m_reset, &QAbstractButton::clicked, this,
            &NotationWidget::slotResetZoomClicked);

    m_pannerLayout->addWidget(controls);

    m_layout->addWidget(m_panner, PANNER_ROW, HEADER_COL, 1, 2);

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
    headersCloseButton->setToolTip(tr("Close track headers"));
    connect(headersCloseButton, &QAbstractButton::clicked,
            this, &NotationWidget::slotCloseHeaders);

    // Insert the button in a layout to push it on the right
    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(20);
    buttonsLayout->addWidget(headersCloseButton);

    // Put the layout inside a widget and the widget above the headers
    m_headersButtons = new QWidget;
    m_headersButtons->setLayout(buttonsLayout);
    m_layout->addWidget(m_headersButtons, TOPRULER_ROW, HEADER_COL, 1, 1);


    // Hide or show the horizontal scroll bar when needed
    connect(m_view->horizontalScrollBar(), &QAbstractSlider::rangeChanged,
            this, &NotationWidget::slotHScrollBarRangeChanged);

    connect(m_view, &Panned::viewportChanged,
            m_hpanner, &Panner::slotSetPannedRect);

    connect(m_hpanner, &Panner::pannedRectChanged,
            m_view, &Panned::slotSetViewport);

    connect(m_hpanner, &Panner::pannerChanged,
             this, &NotationWidget::slotAdjustHeadersVerticalPos);

    connect(m_view, &Panned::pannedContentsScrolled,
            this, &NotationWidget::slotHScroll);

    connect(m_view, &Panned::zoomIn,
            this, &NotationWidget::slotZoomIn);
    connect(m_view, &Panned::zoomOut,
            this, &NotationWidget::slotZoomOut);

    connect(m_hpanner, &Panner::zoomIn,
            this, &NotationWidget::slotSyncPannerZoomIn);

    connect(m_hpanner, &Panner::zoomOut,
            this, &NotationWidget::slotSyncPannerZoomOut);

    connect(m_headersView, &Panned::wheelEventReceived,
            m_view, &Panned::slotEmulateWheelEvent);

    connect(this, &NotationWidget::adjustNeeded,
            this, &NotationWidget::slotAdjustHeadersHorizontalPos,
            Qt::QueuedConnection);

    m_toolBox = new NotationToolBox(this);

    slotSetTool(NoteRestInserter::ToolName());

    // When a clef or a key is modified, the same signal "staffModified()" is
    // emitted three times from the concerned header.
    // Following timer is here to try limiting CPU usage by executing code
    // only once when the same signal is emitted several times in less than
    // 100 ms. See comment and code in slotGenerateHeaders().
    m_headersTimer = new QTimer(this);
    connect(m_headersTimer, &QTimer::timeout,
            this, &NotationWidget::slotGenerateHeaders);
    m_headersTimer->setSingleShot(true);
    m_headersTimer->setInterval(100);  // 0.1 s

    m_autoScroller.connectScrollArea(m_view);
}

NotationWidget::~NotationWidget()
{ clearAll(); }

// Safe to call this more than once.
void
NotationWidget::clearAll()
{
    delete m_scene;
    m_scene = nullptr;
    delete m_headersScene;
    m_headersScene = nullptr;
    delete m_referenceScale;
    m_referenceScale = nullptr;
}

void
NotationWidget::setSegments(RosegardenDocument *document,
                            std::vector<Segment *> segments)
{
    RG_DEBUG << "*** NotationWidget::setSegments() - total segments: " << segments.size();

    // The "hide redundant clefs and keys" mechanism can't work if
    // segments don't begin with default clef and key events.
    // As such a lack is only visible in notation, the following code
    // adds these events, if they are missing, just before passing the
    // segments to notation scene.
    for (std::vector<Segment *>::iterator it = segments.begin();
         it != segments.end(); ++it) {
        (*it)->enforceBeginWithClefAndKey();
    }

    if (m_document) {
        disconnect(m_document, &RosegardenDocument::pointerPositionChanged,
                   this, &NotationWidget::slotPointerPositionChanged);
    }

    m_document = document;

    delete m_referenceScale;

    delete m_scene;
    m_scene = new NotationScene();
    m_scene->setNotationWidget(this);

    if (m_updatesSuspended) m_scene->suspendLayoutUpdates();

    m_scene->setLeftGutter(m_leftGutter);
    m_scene->setStaffs(document, segments);

    m_referenceScale = new ZoomableRulerScale(m_scene->getRulerScale());

    connect(m_scene, &NotationScene::mousePressed,
            this, &NotationWidget::slotDispatchMousePress);

    connect(m_scene, &NotationScene::mouseMoved,
            this, &NotationWidget::slotDispatchMouseMove);

    connect(m_scene, &NotationScene::mouseReleased,
            this, &NotationWidget::slotDispatchMouseRelease);

    connect(m_scene, &NotationScene::mouseDoubleClicked,
            this, &NotationWidget::slotDispatchMouseDoubleClick);

    connect(m_scene, &NotationScene::wheelTurned,
            this, &NotationWidget::slotDispatchWheelTurned);

    // Bug #2960243: the Qt::QueuedConnection flag is mandatory to avoid
    // a crash after deleting the notation scene from inside its own code.
    connect(m_scene, &NotationScene::sceneNeedsRebuilding,
            this, &NotationWidget::sceneNeedsRebuilding, Qt::QueuedConnection);

    // To fix this, create a new slot called slotCurrentStaffChanged() and
    // have it call slotUpdatePointerPosition(true).
    //connect(m_scene, SIGNAL(currentStaffChanged()),
    //        this, SLOT(slotUpdatePointerPosition(true)));

    // There is a Panner::updateScene(), but it's not a slot.
    //connect(m_scene, SIGNAL(selectionChanged()),
    //        m_view, SLOT(updateScene()));

    m_view->setScene(m_scene);

    m_toolBox->setScene(m_scene);

    m_hpanner->setScene(m_scene);
    m_hpanner->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);

    // clean these up if they're left over from a previous run of setSegments
    if (m_topStandardRuler) delete m_topStandardRuler;
    if (m_bottomStandardRuler) delete m_bottomStandardRuler;
    if (m_tempoRuler) delete m_tempoRuler;
    if (m_chordNameRuler) delete m_chordNameRuler;
    if (m_rawNoteRuler) delete m_rawNoteRuler;
    if (m_controlRulerWidget) delete m_controlRulerWidget;

    m_controlRulerWidget = new ControlRulerWidget;
    m_layout->addWidget(m_controlRulerWidget, CONTROLS_ROW, MAIN_COL, 1, 1);
    m_controlRulerWidget->setSegments(document, segments);
    m_controlRulerWidget->setViewSegment(m_scene->getCurrentStaff());
    m_controlRulerWidget->setRulerScale(m_referenceScale, m_leftGutter);

    connect(m_view, &Panned::viewportChanged,
            m_controlRulerWidget, &ControlRulerWidget::slotSetPannedRect);

    // Relay context help from notation rulers
    connect(m_controlRulerWidget, &ControlRulerWidget::showContextHelp,
            this, &NotationWidget::showContextHelp);

    connect(m_scene, &NotationScene::layoutUpdated,
            m_controlRulerWidget, &ControlRulerWidget::slotUpdateRulers);

    connect(m_scene, &NotationScene::selectionChanged,
            m_controlRulerWidget, &ControlRulerWidget::slotSelectionChanged);

    connect(m_scene, &NotationScene::currentViewSegmentChanged,
            m_controlRulerWidget, &ControlRulerWidget::slotSetCurrentViewSegment);

    connect(this, &NotationWidget::toolChanged,
            m_controlRulerWidget, &ControlRulerWidget::slotSetToolName);

    // Connect ControlRulerWidget for Auto-Scroll.
    connect(m_controlRulerWidget, &ControlRulerWidget::mousePress,
            this, &NotationWidget::slotCRWMousePress);
    connect(m_controlRulerWidget, &ControlRulerWidget::mouseMove,
            this, &NotationWidget::slotCRWMouseMove);
    connect(m_controlRulerWidget, &ControlRulerWidget::mouseRelease,
            this, &NotationWidget::slotCRWMouseRelease);

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

    m_rawNoteRuler = new RawNoteRuler(m_referenceScale,
                                      segments[0],
                                      20);  // why not 24 as other rulers ?

    m_layout->addWidget(m_topStandardRuler, TOPRULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_bottomStandardRuler, BOTTOMRULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_tempoRuler, TEMPORULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_chordNameRuler, CHORDNAMERULER_ROW, MAIN_COL, 1, 1);
    m_layout->addWidget(m_rawNoteRuler, RAWNOTERULER_ROW, MAIN_COL, 1, 1);

    connect(m_topStandardRuler, &StandardRuler::dragPointerToPosition,
            this, &NotationWidget::slotStandardRulerDrag);
    connect(m_bottomStandardRuler, &StandardRuler::dragPointerToPosition,
            this, &NotationWidget::slotStandardRulerDrag);

    connect(m_topStandardRuler->getLoopRuler(), &LoopRuler::startMouseMove,
            this, &NotationWidget::slotSRStartMouseMove);
    connect(m_topStandardRuler->getLoopRuler(), &LoopRuler::stopMouseMove,
            this, &NotationWidget::slotSRStopMouseMove);
    connect(m_bottomStandardRuler->getLoopRuler(), &LoopRuler::startMouseMove,
            this, &NotationWidget::slotSRStartMouseMove);
    connect(m_bottomStandardRuler->getLoopRuler(), &LoopRuler::stopMouseMove,
            this, &NotationWidget::slotSRStopMouseMove);

    connect(m_tempoRuler, &TempoRuler::mousePress,
            this, &NotationWidget::slotTRMousePress);
    connect(m_tempoRuler, &TempoRuler::mouseRelease,
            this, &NotationWidget::slotTRMouseRelease);

    connect(m_document, &RosegardenDocument::pointerPositionChanged,
            this, &NotationWidget::slotPointerPositionChanged);

    m_topStandardRuler->connectRulerToDocPointer(document);
    m_bottomStandardRuler->connectRulerToDocPointer(document);

    m_chordNameRuler->setReady();

    slotUpdateSegmentChangerBackground();

    // hide the segment changer if only one segment
    if (segments.size() == 1) m_changerWidget->hide();
    else m_changerWidget->show();

    slotGenerateHeaders();

    // Regenerate headers when font size changed
    connect(m_scene, &NotationScene::staffsPositionned,
            this, &NotationWidget::slotGenerateHeaders);

    // Switch raw note ruler to another segment when needed
    connect(m_scene, &NotationScene::currentViewSegmentChanged,
            this, &NotationWidget::slotUpdateRawNoteRuler);

    // Show current segment color on the background of the segment changer
    connect(m_scene, &NotationScene::currentViewSegmentChanged,
            this, &NotationWidget::slotUpdateSegmentChangerBackground);

    hideOrShowRulers();
    
    // If setSegments() is called on an already existing NotationWidget,
    // NotationScene and Rulers need the same zoom factor and horizontal
    // position.
    if (m_referenceScale) m_referenceScale->setXZoomFactor(m_hZoomFactor);
    slotHScroll();

    // This call lets scene do setup and layout that didn't happen
    // while updates were suspended.  We need this now so that scene
    // calculates position correctly for slotPointerPositionChanged.
    resumeLayoutUpdates();

    // Draw the pointer
    updatePointer(m_document->getComposition().getPosition());
    // And jump to where it is.
    // Note that calls to scrollToTopLeft() might override this.
    m_view->ensurePositionPointerInView(false);

    connect(m_scene, &NotationScene::currentStaffChanged,
            this, &NotationWidget::slotStaffChanged);
}

void
NotationWidget::scrollToTopLeft()
{
    m_view->centerOn(QPointF(0,0));
}

void
NotationWidget::suspendLayoutUpdates()
{
    if (m_scene) m_scene->suspendLayoutUpdates();
    m_updatesSuspended = true;
}

void
NotationWidget::resumeLayoutUpdates()
{
    if (m_scene) m_scene->resumeLayoutUpdates();
    m_updatesSuspended = false;
}

void
NotationWidget::slotGenerateHeaders()
{
    if (!linearMode()) return;  // Staff headers don't exist out of linear mode
    
    m_headersNeedRegeneration = false;

    if (m_headersGroup) disconnect(m_headersGroup, &HeadersGroup::headersResized,
                                   this, &NotationWidget::slotHeadersResized);
    m_headersGroup = new HeadersGroup(m_document);

    m_headersGroup->setTracks(this, m_scene);

    m_headersGroup->setFixedSize(m_headersGroup->sizeHint());
    m_headersView->setFixedWidth(m_headersGroup->sizeHint().width());
    m_headersButtons->setFixedWidth(m_headersGroup->sizeHint().width());

    delete m_headersScene;  // delete the old m_headersGroup if any
    m_headersScene = new QGraphicsScene();
    QGraphicsProxyWidget
        *headersProxy = m_headersScene->addWidget(m_headersGroup);
    m_headersView->setScene(m_headersScene);
    m_headersView->centerOn(headersProxy);
    m_headersView->setMinimumHeight(0);

    // If headers scene and notation scene don't have the same height
    // one may shift from the other when scrolling vertically
    QRectF viewRect = m_scene->sceneRect();
    QRectF headersRect = m_headersScene->sceneRect();
    headersRect.setHeight(viewRect.height());
    m_headersScene->setSceneRect(headersRect);
    connect(m_headersGroup, &HeadersGroup::headersResized,
            this, &NotationWidget::slotHeadersResized);
}

void
NotationWidget::setCanvasCursor(QCursor c)
{
    if (m_view) m_view->viewport()->setCursor(c);
}

Segment *
NotationWidget::getCurrentSegment()
{
    if (!m_scene) return nullptr;
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
    m_layout->removeWidget(m_panner);
    if (tall) {
        m_panner->setMaximumHeight(QWIDGETSIZE_MAX);
        m_hpanner->setMaximumHeight(QWIDGETSIZE_MAX);
        m_panner->setMaximumWidth(80);
        m_hpanner->setMaximumWidth(80);
        m_pannerLayout->setDirection(QBoxLayout::TopToBottom);
        m_HsegmentChanger->hide();
        m_VsegmentChanger->show();
        m_lastSegmentChangerValue = m_VsegmentChanger->getValue();
        m_HsegmentChanger->setValue(m_lastSegmentChangerValue);
        m_layout->addWidget(m_panner, PANNED_ROW, VPANNER_COL);
    } else {
        m_panner->setMaximumHeight(80);
        m_hpanner->setMaximumHeight(80);
        m_panner->setMaximumWidth(QWIDGETSIZE_MAX);
        m_hpanner->setMaximumWidth(QWIDGETSIZE_MAX);
        m_pannerLayout->setDirection(QBoxLayout::LeftToRight);
        m_VsegmentChanger->hide();
        m_HsegmentChanger->show();
        m_lastSegmentChangerValue = m_HsegmentChanger->getValue();
        m_VsegmentChanger->setValue(m_lastSegmentChangerValue);
        m_layout->addWidget(m_panner, PANNER_ROW, HEADER_COL, 1, 2);
    }
}

bool
NotationWidget::linearMode() const
{
    if (!m_scene) return true;    // Default to true when no scene
    return m_scene->getPageMode() == StaffLayout::LinearMode;
}

void
NotationWidget::slotSetLinearMode()
{
    if (!m_scene) return;
    if (m_scene->getPageMode() == StaffLayout::ContinuousPageMode) {
        locatePanner(false);
    }
    m_scene->setPageMode(StaffLayout::LinearMode);
    hideOrShowRulers();
}

void
NotationWidget::slotSetContinuousPageMode()
{
    if (!m_scene) return;
    if (m_scene->getPageMode() == StaffLayout::ContinuousPageMode) return;
    locatePanner(true);
    m_scene->setPageMode(StaffLayout::ContinuousPageMode);
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
    hideOrShowRulers();
}

void
NotationWidget::slotSetFontName(QString name)
{
    if (m_scene) m_scene->setFontName(name);

    // Note: See slotSetFontSize, if standard rulers and position do not refresh
}

void
NotationWidget::slotSetFontSize(int size)
{
    if (m_scene)
        m_scene->setFontSize(size);

    // Force standard rulers and pointer pointer to refresh -- otherwise
    m_bottomStandardRuler->updateStandardRuler();
    m_topStandardRuler->updateStandardRuler();

    updatePointer(m_document->getComposition().getPosition());
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
    emit toolChanged(name);
}

void
NotationWidget::slotSetEraseTool()
{
    slotSetTool(NotationEraser::ToolName());
}

void
NotationWidget::slotSetSelectTool()
{
    slotSetTool(NotationSelector::ToolName());
}

void
NotationWidget::slotSetSelectNoTiesTool()
{
    slotSetTool(NotationSelectorNoTies::ToolName());
}

void
NotationWidget::slotSetNoteRestInserter()
{
    slotSetTool(NoteRestInserter::ToolName());
}

void
NotationWidget::slotSetNoteInserter()
{
    NoteRestInserter *noteRestInserter = dynamic_cast<NoteRestInserter *>
        (m_toolBox->getTool(NoteRestInserter::ToolName()));
    noteRestInserter->setToRestInserter(false); // set to insert notes.

    slotSetTool(NoteRestInserter::ToolName());
}

void
NotationWidget::slotSetRestInserter()
{
    NoteRestInserter *noteRestInserter = dynamic_cast<NoteRestInserter *>
        (m_toolBox->getTool(NoteRestInserter::ToolName()));
    noteRestInserter->setToRestInserter(true); // set to insert notes.

    slotSetTool(NoteRestInserter::ToolName());
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
        (m_toolBox->getTool(NoteRestInserter::ToolName()));
    if (ni) {
        ni->slotSetAccidental(accidental, follow);
        return;
    }
}

void
NotationWidget::slotSetClefInserter()
{
    slotSetTool(ClefInserter::ToolName());
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
    slotSetTool(TextInserter::ToolName());
}

void
NotationWidget::slotSetGuitarChordInserter()
{
    slotSetTool(GuitarChordInserter::ToolName());
}

void
NotationWidget::slotSetPlayTracking(bool tracking)
{
    m_playTracking = tracking;
    if (m_playTracking) {
        m_view->ensurePositionPointerInView(true);
    }
}

void
NotationWidget::slotTogglePlayTracking()
{
    slotSetPlayTracking(!m_playTracking);
}

void
NotationWidget::updatePointerPosition(bool moveView)
{
    updatePointer(m_document->getComposition().getPosition());

    if (moveView)
        m_view->ensurePositionPointerInView(false);  // page
}

void
NotationWidget::updatePointer(timeT t)
{
    if (!m_scene)
        return;

    SequenceManager *seqMgr = m_document->getSequenceManager();

    bool rolling =
            (seqMgr  &&
             (seqMgr->getTransportStatus() == PLAYING  ||
              seqMgr->getTransportStatus() == RECORDING));

    //RG_DEBUG << "updatePointer(" << t << "): rolling = " << rolling;

    NotationScene::CursorCoordinates cursorPos =
            m_scene->getCursorCoordinates(t);

    // While rolling, display a playback position pointer that stretches
    // across all staves.
    const QLineF p = rolling ? cursorPos.allStaffs : cursorPos.currentStaff;
    if (p == QLineF())
        return;

    // p will also contain sensible Y (although not 100% sensible yet)
    double pointerX = p.x1();
    double pointerY = std::min(p.y1(), p.y2());
    double pointerHeight = fabs(p.dy());

    double sceneXMin = m_scene->sceneRect().left();
    double sceneXMax = m_scene->sceneRect().right();

    // If the pointer has gone outside the limits
    if (pointerX < sceneXMin  ||  sceneXMax < pointerX) {
        // Never move the pointer outside the scene (else the scene will grow)
        m_view->hidePositionPointer();
        m_hpanner->slotHidePositionPointer();
    } else {
        m_view->showPositionPointer(QPointF(pointerX, pointerY),
                                    pointerHeight);
        m_hpanner->slotShowPositionPointer(QPointF(pointerX, pointerY),
                                           pointerHeight);
    }
}

void
NotationWidget::slotPointerPositionChanged(timeT t)
{
    updatePointer(t);

    if (m_playTracking)
        m_view->ensurePositionPointerInView(true);  // page
}

void
NotationWidget::slotStandardRulerDrag(timeT t)
{
    updatePointer(t);
}

void
NotationWidget::slotSRStartMouseMove()
{
    m_autoScroller.setFollowMode(FOLLOW_HORIZONTAL);
    m_autoScroller.start();
}

void
NotationWidget::slotSRStopMouseMove()
{
    m_autoScroller.stop();
}

void
NotationWidget::slotCRWMousePress()
{
    m_autoScroller.start();
}

void
NotationWidget::slotCRWMouseMove(FollowMode followMode)
{
    m_autoScroller.setFollowMode(followMode);
}

void
NotationWidget::slotCRWMouseRelease()
{
    m_autoScroller.stop();
}

void
NotationWidget::slotTRMousePress()
{
    m_autoScroller.setFollowMode(FOLLOW_HORIZONTAL);
    m_autoScroller.start();
}

void
NotationWidget::slotTRMouseRelease()
{
    m_autoScroller.stop();
}

void
NotationWidget::slotDispatchMousePress(const NotationMouseEvent *e)
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

    m_autoScroller.start();
}

void
NotationWidget::slotDispatchMouseMove(const NotationMouseEvent *e)
{
    if (!m_currentTool)
        return;

    FollowMode followMode = m_currentTool->handleMouseMove(e);

    m_autoScroller.setFollowMode(followMode);

    if (e->staff) {
        QString s = e->staff->getNoteNameAtSceneCoords(e->sceneX, e->sceneY);
        emit hoveredOverNoteChanged(s);
    }
}

void
NotationWidget::slotDispatchMouseRelease(const NotationMouseEvent *e)
{
    m_autoScroller.stop();

    if (!m_currentTool)
        return;

    m_currentTool->handleMouseRelease(e);
}

void
NotationWidget::slotDispatchMouseDoubleClick(const NotationMouseEvent *e)
{
    if (!m_currentTool) return;
    m_currentTool->handleMouseDoubleClick(e);
}

void
NotationWidget::slotDispatchWheelTurned(int delta, const NotationMouseEvent *e)
{
    if (!m_currentTool) return;
    m_currentTool->handleWheelTurned(delta, e);
}

EventSelection *
NotationWidget::getSelection() const
{
    if (m_scene) return m_scene->getSelection();
    else return nullptr;
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

//RG_DEBUG << "\nXproxy0=" << m_headersProxy->scenePos().x() << "\n";

    double xinit;

    double x = xinit = m_headersView->mapToScene(0, 0).x();
//RG_DEBUG << " x0=" << x << "\n";

    // First trial
    if ((x > 1) || (x < -1)) {
        QRectF view = m_headersView->sceneRect();
        view.moveLeft(0.0);
        m_headersView->setSceneRect(view);
        x = m_headersView->mapToScene(0, 0).x();
    }
//RG_DEBUG << "x1=" << x << "\n";

    // Second trial. Why isn't the first iteration always sufficient ?
    // Number of iterations is limited to 3.
    int n = 1;
    while ((x > 1) || (x < -1)) {
//RG_DEBUG << "n=" << n << " xt2=" << x << "\n";
        QRectF view = m_headersView->sceneRect();
        view.translate(-x, 0);
        m_headersView->setSceneRect(view);
        x = m_headersView->mapToScene(0, 0).x();
        if (n++ > 3) break;
    }

//RG_DEBUG << "x2=" << x << "\n";

    // Third trial.
    // If precedent trial doesn't succeed, try again with a coefficient...
    // Number of iterations is limited to 6.    int m = 1;
    int m = 1;
    while ((x > 1) || (x < -1)) {
//RG_DEBUG << "m=" << m << " xt3=" << x << "\n";
        QRectF view = m_headersView->sceneRect();
        view.translate(-x * 0.477, 0);
        m_headersView->setSceneRect(view);
        x = m_headersView->mapToScene(0, 0).x();
        if (m++ > 6) break;
    }

//RG_DEBUG << "x3=" << x << "\n";

    // Probably totally useless here.
    m_headersView->update();

    // Now, sometimes, although x is null or almost null, the headers are
    // not fully visible !!??

//RG_DEBUG << "Xproxy1=" << m_headersProxy->scenePos().x() << "\n";

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

    // For some reason, in current step, the headers are very often placed a
    // bit too much on the right. Following call should fix their horizontal
    // position. Argument "last" is set to true because experience shows that
    // a single call is sufficient.
    slotAdjustHeadersHorizontalPos(true);
}

double
NotationWidget::getViewLeftX()
{
    return m_view->mapToScene(0, 0).x();
}

double
NotationWidget::getViewRightX()
{
    return m_view->mapToScene(m_view->width(), 0).x();
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

    // Apply zoom correction
    int x = (xs - m_leftGutter) * m_hZoomFactor;

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
    if (visible && linearMode()) m_tempoRuler->show();
    else m_tempoRuler->hide();
    m_tempoRulerIsVisible = visible;
}

void
NotationWidget::setChordNameRulerVisible(bool visible)
{
    if (visible && linearMode()) m_chordNameRuler->show();
    else m_chordNameRuler->hide();
    m_chordNameRulerIsVisible = visible;
}

void
NotationWidget::setRawNoteRulerVisible(bool visible)
{
    if (visible && linearMode()) m_rawNoteRuler->show();
    else m_rawNoteRuler->hide();
    m_rawNoteRulerIsVisible = visible;
}

void
NotationWidget::setHeadersVisible(bool visible)
{
    // Headers are shown in linear mode only
    if (visible && linearMode()) {
        if (m_headersNeedRegeneration) slotGenerateHeaders();
        m_headersView->show();
        m_headersButtons->show();
    } else {
        m_headersView->hide();
        m_headersButtons->hide();
    }
    m_headersAreVisible = visible;
    emit headersVisibilityChanged(visible);
}

void
NotationWidget::setHeadersVisibleIfNeeded()
{
    int viewHeight = m_view->height();
    int headersHeight = m_headersGroup->getUsedHeight();

    // Headers only have to be visible when all the staves don't vertically
    // fit inside the view
    bool visible = headersHeight > viewHeight;
    setHeadersVisible(visible);
}

void
NotationWidget::toggleHeadersView()
{
    m_headersAreVisible = !m_headersAreVisible;
    emit headersVisibilityChanged(m_headersAreVisible);
    if (m_headersAreVisible && linearMode()) {
        if (m_headersNeedRegeneration) slotGenerateHeaders();
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
    if (linearMode()) {
        if (m_tempoRulerIsVisible) m_tempoRuler->show();
        if (m_rawNoteRulerIsVisible) m_rawNoteRuler->show();
        if (m_chordNameRulerIsVisible) m_chordNameRuler->show();
        if (m_headersAreVisible) {
            if (m_headersNeedRegeneration) slotGenerateHeaders();
            m_headersView->show();
            m_headersButtons->show();
        }
        if (m_controlRulerWidget->isAnyRulerVisible()) m_controlRulerWidget->show();
        m_bottomStandardRuler->show();
        m_topStandardRuler->show();
    } else {
        if (m_tempoRulerIsVisible) m_tempoRuler->hide();
        if (m_rawNoteRulerIsVisible) m_rawNoteRuler->hide();
        if (m_chordNameRulerIsVisible) m_chordNameRuler->hide();
        if (m_headersAreVisible) {
            m_headersView->hide();
            m_headersButtons->hide();
        }
        if (m_controlRulerWidget->isAnyRulerVisible()) m_controlRulerWidget->hide();
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
NotationWidget::slotHeadersResized(int)
{
    // Set headers view width to accomodate headers width.
    m_headersView->setFixedWidth(
        m_headersGroup->sizeHint().width() * m_hZoomFactor);
}

void
NotationWidget::slotSetSymbolInserter()
{
    slotSetTool(SymbolInserter::ToolName());
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
    // Fixes problem with sustaining notes while adding notes with
    // the pencil tool.  Also avoids moving playback position in
    // playback mode, allowing editing of a loop in real-time.
    disconnect(m_document, &RosegardenDocument::pointerPositionChanged,
               RosegardenMainWindow::self(),
               &RosegardenMainWindow::slotSetPointerPosition);

    m_document->slotSetPointerPosition(t);

    connect(m_document, &RosegardenDocument::pointerPositionChanged,
            RosegardenMainWindow::self(),
            &RosegardenMainWindow::slotSetPointerPosition);
}

void
NotationWidget::slotHorizontalThumbwheelMoved(int v)
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

    //RG_DEBUG << "v is: " << v << " h zoom factor was: " << m_lastH << " now: " << newZoom << " zooming " << (zoomingIn ? "IN" : "OUT");

    setHorizontalZoomFactor(newZoom);
    m_lastH = v;
    m_lastZoomWasHV = false;
}

void
NotationWidget::slotVerticalThumbwheelMoved(int v)
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

    //RG_DEBUG << "v is: " << v << " z zoom factor was: " << m_lastV << " now: " << newZoom << " zooming " << (zoomingIn ? "IN" : "OUT");

    setVerticalZoomFactor(newZoom);
    m_lastV = v;
    m_lastZoomWasHV = false;
}

void
NotationWidget::slotPrimaryThumbwheelMoved(int v)
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
    m_lastZoomWasHV = true;
}

void
NotationWidget::slotResetZoomClicked()
{
    RG_DEBUG << "NotationWidget::slotResetZoomClicked()";

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
    m_headersView->setMatrix(m);
    m_headersView->setFixedWidth(m_headersGroup->sizeHint().width());
    slotHScroll();

    // scale factor 1.0 = 100% zoom
    m_Hzoom->setValue(1);
    m_Vzoom->setValue(1);
    m_HVzoom->setValue(0);
    m_lastHVzoomValue = 0;
    m_lastH = 0;
    m_lastV = 0;
}

void
NotationWidget::slotSyncPannerZoomIn()
{
    int v = m_lastHVzoomValue - 1;

    m_HVzoom->setValue(v);
    slotPrimaryThumbwheelMoved(v);
}

void
NotationWidget::slotSyncPannerZoomOut()
{
    int v = m_lastHVzoomValue + 1;

    m_HVzoom->setValue(v);
    slotPrimaryThumbwheelMoved(v);
}


void
NotationWidget::setHorizontalZoomFactor(double factor)
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
    QMatrix m;
    m.scale(1.0, m_vZoomFactor);
    m_headersView->setMatrix(m);
    m_headersView->setFixedWidth(m_headersGroup->sizeHint().width());
    slotHScroll();
}

void
NotationWidget::setVerticalZoomFactor(double factor)
{
    m_vZoomFactor = factor;
    if (m_referenceScale) m_referenceScale->setYZoomFactor(m_vZoomFactor);
    m_view->resetMatrix();
    m_view->scale(m_hZoomFactor, m_vZoomFactor);
    QMatrix m;
    m.scale(1.0, m_vZoomFactor);
    m_headersView->setMatrix(m);
    m_headersView->setFixedWidth(m_headersGroup->sizeHint().width());
}

double
NotationWidget::getHorizontalZoomFactor() const
{
    return m_hZoomFactor;
}

double
NotationWidget::getVerticalZoomFactor() const
{
    return m_vZoomFactor;
}

void
NotationWidget::slotToggleVelocityRuler()
{
    m_controlRulerWidget->slotTogglePropertyRuler(BaseProperties::VELOCITY);
}

void
NotationWidget::slotTogglePitchbendRuler()
{
    m_controlRulerWidget->togglePitchBendRuler();
}

void
NotationWidget::slotAddControlRuler(QAction *action)
{
    QString name = action->text();

    // FIX #1543: If name happens to come to us with an & included somewhere,
    // strip the & so the string will match the one we are comparing later on.
    //
    name.replace(QRegExp("&"), "");

//    RG_DEBUG << "my name is " << name.toStdString();

    // we just cheaply paste the code from NotationView that created the menu to
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
        hexValue.sprintf("(0x%x)", it->getControllerNumber());

        // strings extracted from data files must be QObject::tr()
        QString itemStr = QObject::tr("%1 Controller %2 %3")
                                     .arg(QObject::tr(it->getName().c_str()))
                                     .arg(it->getControllerNumber())
                                     .arg(hexValue);

        if (name != itemStr) continue;

        RG_DEBUG << "name: " << name.toStdString() << " should match  itemStr: " << itemStr.toStdString();

        m_controlRulerWidget->slotAddControlRuler(*it);

//      if (i == menuIndex) m_controlsWidget->slotAddControlRuler(*p);
//      else i++;
    }
}

Device *
NotationWidget::getCurrentDevice()
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

// used in pitchtracker
void
NotationWidget::addWidgetToBottom(QWidget *bottomWidget)
{
    m_layout->addWidget(bottomWidget, BOTTOM_ROW, MAIN_COL, 1, 1);
}

void
NotationWidget::slotSegmentChangerMoved(int v)
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
        if (v < m_lastSegmentChangerValue) emit currentSegmentNext();
        else if (v > m_lastSegmentChangerValue) emit currentSegmentPrior();
    }

    m_lastSegmentChangerValue = v;
}

void
NotationWidget::slotStaffChanged()
{
    // Draw the pointer
    updatePointer(m_document->getComposition().getPosition());
    // Make sure it's in view.
    m_view->ensurePositionPointerInView(false);
}

void
NotationWidget::slotUpdateSegmentChangerBackground()
{
    // set the changer widget background to the now current segment's
    // background, and reset the tooltip style to compensate
    Colour c = m_document->getComposition().getSegmentColourMap().getColourByIndex(m_scene->getCurrentSegment()->getColourIndex());

    QPalette palette = m_changerWidget->palette();
    palette.setColor(QPalette::Window, c);
    m_changerWidget->setPalette(palette);

    // have to deal with all this ruckus to get a few pieces of info about the
    // track:
    Track *track = m_document->getComposition().getTrackById(m_scene->getCurrentSegment()->getTrack());
    int trackPosition = m_document->getComposition().getTrackPositionById(track->getId());
    QString trackLabel = QString::fromStdString(track->getLabel());

    // set up some tooltips...  I don't like this much, and it wants some kind
    // of dedicated float thing eventually, but let's not go nuts on a
    // last-minute feature
    m_HsegmentChanger->setToolTip(tr("<qt>Rotate wheel to change the active segment</qt>"));
    m_VsegmentChanger->setToolTip(tr("<qt>Rotate wheel to change the active segment</qt>"));
    m_changerWidget->setToolTip(tr("<qt>Segment: \"%1\"<br>Track: %2 \"%3\"</qt>")
                                .arg(QString::fromStdString(m_scene->getCurrentSegment()->getLabel()))
                                .arg(trackPosition)
                                .arg(trackLabel));
}

void
NotationWidget::slotUpdateRawNoteRuler(ViewSegment *vs)
{
    Segment *seg = &(vs->getSegment());
    m_rawNoteRuler->setCurrentSegment(seg);
    m_rawNoteRuler->update();
}

void
NotationWidget::slotRegenerateHeaders() {
    // Don't use CPU time to regenerate headers if they are not visible
    if (linearMode() && m_headersAreVisible) {
        // When a clef or a key is modified, the signal "staffModified()" is
        // emitted three times. A 100 ms delay, which should not be noticeable
        // too much, is introduced here to avoid unnecessarily destroying and
        // recreating the headers.
        m_headersTimer->start();
            // slotGenerateHeaders() should be called in 100 ms unless
            // slotRegenerateHeaders() is called again before.
    } else {
        m_headersNeedRegeneration = true;
    }
}

void
NotationWidget::slotZoomIn()
{
    int v = m_lastH - 1;

    m_Hzoom->setValue(v);
    slotHorizontalThumbwheelMoved(v);
}

void
NotationWidget::slotZoomOut()
{
    int v = m_lastH + 1;

    m_Hzoom->setValue(v);
    slotHorizontalThumbwheelMoved(v);
}


}

