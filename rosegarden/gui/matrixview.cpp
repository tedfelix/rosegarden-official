// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/
#include <cmath>

#include <qiconset.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qslider.h>
#include <qtimer.h>
#include <qinputdialog.h>
#include <qpixmap.h>

#include <kapp.h>
#include <kaction.h>
#include <kconfig.h>
#include <kcombobox.h>
#include <kstddirs.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstdaction.h>
#include <kmessagebox.h>
#include <kstatusbar.h>
#include <ktoolbar.h>

#include "Instrument.h"
#include "Composition.h"
#include "Event.h"
#include "Quantizer.h"
#include "Property.h"
#include "BaseProperties.h"
#include "Profiler.h"
#include "Property.h"
#include "AudioLevel.h"
#include "ViewElement.h"
#include "Staff.h"
#include "SegmentNotationHelper.h"

#include "matrixview.h"
#include "matrixstaff.h"
#include "matrixhlayout.h"
#include "matrixvlayout.h"
#include "matrixtool.h"
#include "matrixcommands.h"
#include "midipitchlabel.h"
#include "constants.h"
#include "dialogs.h"
#include "rosestrings.h"
#include "notationstrings.h"
#include "rosegardenguidoc.h"
#include "ktmpstatusmsg.h"
#include "barbuttons.h"
#include "loopruler.h"
#include "temporuler.h"
#include "chordnameruler.h"
#include "pianokeyboard.h"
#include "percussionpitchruler.h"
#include "editcommands.h"
#include "notationcommands.h"
#include "qdeferscrollview.h"
#include "instrumentparameterbox.h"
#include "velocitycolour.h"
#include "widgets.h"
#include "zoomslider.h"
#include "rosegardengui.h"
#include "notepixmapfactory.h"
#include "controlruler.h"
#include "studiocontrol.h"
#include "Clipboard.h"
#include "eventfilter.h"
#include "MidiTypes.h"
#include "tempoview.h"
#include "notationelement.h"

#include "rosedebug.h"

#include <sys/time.h>

using Rosegarden::Segment;
using Rosegarden::EventSelection;
using Rosegarden::timeT;

static double xorigin = 0.0;

//----------------------------------------------------------------------

MatrixView::MatrixView(RosegardenGUIDoc *doc,
                       std::vector<Segment *> segments,
                       QWidget *parent,
		       bool drumMode)
    : EditView(doc, segments, 3, parent, "matrixview"),
      m_hlayout(&doc->getComposition()),
      m_vlayout(),
      m_snapGrid(new Rosegarden::SnapGrid(&m_hlayout)),
      m_lastEndMarkerTime(0),
      m_hoveredOverAbsoluteTime(0),
      m_hoveredOverNoteName(0),
      m_selectionCounter(0),
      m_insertModeLabel(0),
      m_previousEvPitch(0),
      m_dockLeft(0),
      m_canvasView(0),
      m_pianoView(0),
      m_localMapping(0),
      m_lastNote(0),
      m_quantizations(Rosegarden::BasicQuantizer::getStandardQuantizations()),
      m_chordNameRuler(0),
      m_tempoRuler(0),
      m_playTracking(true),
      m_dockVisible(true),
      m_drumMode(drumMode)
{
    RG_DEBUG << "MatrixView ctor: drumMode " << drumMode << "\n";

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/toolbar");
    QPixmap matrixPixmap(pixmapDir + "/matrix.xpm");

    m_dockLeft = createDockWidget("params dock", matrixPixmap, 0L,
                                  i18n("Instrument Parameters"));
    m_dockLeft->manualDock(m_mainDockWidget,            // dock target
                           KDockWidget::DockLeft, // dock site
                           20);                   // relation target/this (in percent)

    connect(m_dockLeft, SIGNAL(iMBeingClosed()),
            this, SLOT(slotParametersClosed()));
    connect(m_dockLeft, SIGNAL(hasUndocked()),
            this, SLOT(slotParametersClosed()));
    // Apparently, hasUndocked() is emitted when the dock widget's
    // 'close' button on the dock handle is clicked.
    connect(m_mainDockWidget, SIGNAL(docking(KDockWidget*, KDockWidget::DockPosition)),
            this, SLOT(slotParametersDockedBack(KDockWidget*, KDockWidget::DockPosition)));

    Rosegarden::Composition &comp = doc->getComposition();

    m_toolBox = new MatrixToolBox(this);

    initStatusBar();

    QCanvas *tCanvas = new QCanvas(this);

    m_config->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    if (m_config->readBoolEntry("backgroundtextures", true)) {
	QPixmap background;
	QString pixmapDir =
	    KGlobal::dirs()->findResource("appdata", "pixmaps/");
	if (background.load(QString("%1/misc/bg-paper-white.xpm").
			    arg(pixmapDir))) {
	    tCanvas->setBackgroundPixmap(background);
	}
    }

    m_config->setGroup(ConfigGroup);

    MATRIX_DEBUG << "MatrixView : creating staff\n";

    Rosegarden::Track *track =
        comp.getTrackById(segments[0]->getTrack());

    Rosegarden::Instrument *instr = getDocument()->getStudio().
        getInstrumentById(track->getInstrument());

    int resolution = 8;

    if (isDrumMode() && instr && instr->getKeyMapping()) {
	resolution = 11;
    }

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new MatrixStaff(tCanvas, 
                                           segments[i],
                                           m_snapGrid,
                                           i,
					   resolution,
                                           this));
	// staff has one too many rows to avoid a half-row at the top:
	m_staffs[i]->setY(-resolution / 2);
//!!!	if (isDrumMode()) m_staffs[i]->setX(resolution);
	if (i == 0) m_staffs[i]->setCurrent(true);
    }

    MATRIX_DEBUG << "MatrixView : creating canvas view\n";

    const Rosegarden::MidiKeyMapping *mapping = 0;

    if (instr) {
	mapping = instr->getKeyMapping();
	if (mapping) {
	    RG_DEBUG << "MatrixView: Instrument has key mapping: "
		     << mapping->getName() << endl;
            m_localMapping = new Rosegarden::MidiKeyMapping(*mapping);
            extendKeyMapping();
	} else {
	    RG_DEBUG << "MatrixView: Instrument has no key mapping\n";
	}
    }

    m_pianoView = new QDeferScrollView(getCentralWidget());

    QWidget* vport = m_pianoView->viewport();

    if (isDrumMode() && mapping &&
	!m_localMapping->getMap().empty()) {
	m_pitchRuler = new PercussionPitchRuler(vport,
						m_localMapping,
						resolution); // line spacing
    } else {
	m_pitchRuler = new PianoKeyboard(vport);
    }

    m_pianoView->setVScrollBarMode(QScrollView::AlwaysOff);
    m_pianoView->setHScrollBarMode(QScrollView::AlwaysOff);
    m_pianoView->addChild(m_pitchRuler);
    m_pianoView->setFixedWidth(m_pianoView->contentsWidth());

    m_grid->addWidget(m_pianoView, CANVASVIEW_ROW, 1);

    m_parameterBox = new InstrumentParameterBox(getDocument(), m_dockLeft);
    m_dockLeft->setWidget(m_parameterBox);

    RosegardenGUIApp *app = RosegardenGUIApp::self();
    connect(app,
	    SIGNAL(pluginSelected(Rosegarden::InstrumentId, int, int)),
	    m_parameterBox,
	    SLOT(slotPluginSelected(Rosegarden::InstrumentId, int, int)));
    connect(app,
	    SIGNAL(pluginBypassed(Rosegarden::InstrumentId, int, bool)),
	    m_parameterBox,
	    SLOT(slotPluginBypassed(Rosegarden::InstrumentId, int, bool)));
    connect(app,
	    SIGNAL(instrumentParametersChanged(Rosegarden::InstrumentId)),
	    m_parameterBox,
	    SLOT(slotInstrumentParametersChanged(Rosegarden::InstrumentId)));
    connect(m_parameterBox,
	    SIGNAL(instrumentParametersChanged(Rosegarden::InstrumentId)),
	    app,
	    SIGNAL(instrumentParametersChanged(Rosegarden::InstrumentId)));
    connect(m_parameterBox,
	    SIGNAL(selectPlugin(QWidget *, Rosegarden::InstrumentId, int)),
	    app,
	    SLOT(slotShowPluginDialog(QWidget *, Rosegarden::InstrumentId, int)));
    connect(m_parameterBox,
	    SIGNAL(showPluginGUI(Rosegarden::InstrumentId, int)),
	    app,
	    SLOT(slotShowPluginGUI(Rosegarden::InstrumentId, int)));
    connect(parent, // RosegardenGUIView
	    SIGNAL(checkTrackAssignments()),
	    this,
	    SLOT(slotCheckTrackAssignments()));

    // Assign the instrument
    //
    m_parameterBox->useInstrument(instr);

    if (m_drumMode) {
        connect(m_parameterBox,
	        SIGNAL(instrumentPercussionSetChanged(Rosegarden::Instrument *)),
	        this,
	        SLOT(slotPercussionSetChanged(Rosegarden::Instrument *)));
    }

    // Set the snap grid from the stored size in the segment
    //
    int snapGridSize = m_staffs[0]->getSegment().getSnapGridSize();

    MATRIX_DEBUG << "MatrixView : Snap Grid Size = " << snapGridSize << endl;

    if (snapGridSize != -1)
       m_snapGrid->setSnapTime(snapGridSize);
    else
       m_snapGrid->setSnapTime(Rosegarden::SnapGrid::SnapToBeat);

    m_canvasView = new MatrixCanvasView(*m_staffs[0],
                                        m_snapGrid,
					m_drumMode,
                                        tCanvas,
                                        getCentralWidget());
    setCanvasView(m_canvasView);

    // do this after we have a canvas
    setupActions();
    setupAddControlRulerMenu();

    stateChanged("parametersbox_closed", KXMLGUIClient::StateReverse);

    // tool bars
    initActionsToolbar();
    initZoomToolbar();

    // Connect vertical scrollbars between matrix and piano
    //
    connect(m_canvasView->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotVerticalScrollPianoKeyboard(int)));

    connect(m_canvasView->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotVerticalScrollPianoKeyboard(int)));

    connect(m_canvasView, SIGNAL(zoomIn()), this, SLOT(slotZoomIn()));
    connect(m_canvasView, SIGNAL(zoomOut()), this, SLOT(slotZoomOut()));

    connect(m_pianoView, SIGNAL(gotWheelEvent(QWheelEvent*)),
            m_canvasView, SLOT(slotExternalWheelEvent(QWheelEvent*)));

    // ensure the piano keyb keeps the right margins when the user toggles
    // the canvas view rulers
    //
    connect(m_canvasView, SIGNAL(bottomWidgetHeightChanged(int)),
            this, SLOT(slotCanvasBottomWidgetHeightChanged(int)));

    /*
    QObject::connect
        (getCanvasView(), SIGNAL(activeItemPressed(QMouseEvent*, QCanvasItem*)),
         this,            SLOT  (activeItemPressed(QMouseEvent*, QCanvasItem*)));
         */

    QObject::connect
        (getCanvasView(),
         SIGNAL(mousePressed(Rosegarden::timeT,
                             int, QMouseEvent*, MatrixElement*)),
         this, 
         SLOT(slotMousePressed(Rosegarden::timeT,
                              int, QMouseEvent*, MatrixElement*)));

    QObject::connect
        (getCanvasView(),
         SIGNAL(mouseMoved(Rosegarden::timeT, int, QMouseEvent*)),
         this,
         SLOT(slotMouseMoved(Rosegarden::timeT, int, QMouseEvent*)));

    QObject::connect
        (getCanvasView(),
         SIGNAL(mouseReleased(Rosegarden::timeT, int, QMouseEvent*)),
         this,
         SLOT(slotMouseReleased(Rosegarden::timeT, int, QMouseEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverNoteChanged(int)),
         this, SLOT(slotHoveredOverNoteChanged(int)));

    QObject::connect
        (m_pitchRuler, SIGNAL(hoveredOverKeyChanged(unsigned int)),
         this,         SLOT  (slotHoveredOverKeyChanged(unsigned int)));

    QObject::connect
        (m_pitchRuler, SIGNAL(keyPressed(unsigned int, bool)),
         this,         SLOT  (slotKeyPressed(unsigned int, bool)));

    QObject::connect
        (m_pitchRuler, SIGNAL(keySelected(unsigned int, bool)),
         this,         SLOT  (slotKeySelected(unsigned int, bool)));

    QObject::connect
        (m_pitchRuler, SIGNAL(keyReleased(unsigned int, bool)),
         this,         SLOT  (slotKeyReleased(unsigned int, bool)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverAbsoluteTimeChanged(unsigned int)),
         this,            SLOT  (slotHoveredOverAbsoluteTimeChanged(unsigned int)));

    QObject::connect
	(doc, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
	 this, SLOT(slotSetPointerPosition(Rosegarden::timeT)));
    QObject::connect
	(doc, SIGNAL(pointerDraggedToPosition(Rosegarden::timeT)),
	 this, SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    MATRIX_DEBUG << "MatrixView : applying layout\n";

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, i18n("Couldn't apply piano roll layout"));
    else {
        MATRIX_DEBUG << "MatrixView : rendering elements\n";
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {

	    m_staffs[i]->positionAllElements();
            m_staffs[i]->getSegment().getRefreshStatus
		(m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);
        }
    }

    BarButtons *topBarButtons = new BarButtons(getDocument(),
                                               &m_hlayout, int(xorigin), 25,
                                               false, getCentralWidget());
    setTopBarButtons(topBarButtons);

    QObject::connect
	(topBarButtons->getLoopRuler(),
	 SIGNAL(setPointerPosition(Rosegarden::timeT)),
	 this, SLOT(slotSetInsertCursorPosition(Rosegarden::timeT)));

    topBarButtons->getLoopRuler()->setBackgroundColor
	(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::InsertCursorRuler));

    connect(topBarButtons->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_canvasView, SLOT(startAutoScroll(int)));
    connect(topBarButtons->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_canvasView, SLOT(stopAutoScroll()));

    BarButtons *bottomBarButtons = new BarButtons(getDocument(),
                                                  &m_hlayout, 0, 25,
                                                  true, getBottomWidget());
    setBottomBarButtons(bottomBarButtons);

    connect(bottomBarButtons->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_canvasView, SLOT(startAutoScroll(int)));
    connect(bottomBarButtons->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_canvasView, SLOT(stopAutoScroll()));

    topBarButtons->connectRulerToDocPointer(doc);
    bottomBarButtons->connectRulerToDocPointer(doc);

    // Force height for the moment
    //
    m_pitchRuler->setFixedHeight(canvas()->height());


    updateViewCaption();

    // Add a velocity ruler
    //
//!!!    addPropertyViewRuler(Rosegarden::BaseProperties::VELOCITY);

    m_chordNameRuler = new ChordNameRuler
	(&m_hlayout, doc, segments, 0, 20, getCentralWidget());
    m_chordNameRuler->setStudio(&getDocument()->getStudio());
    addRuler(m_chordNameRuler);

    m_tempoRuler = new TempoRuler
	(&m_hlayout, doc, factory(), 0, 24, false, getCentralWidget());
    static_cast<TempoRuler *>(m_tempoRuler)->connectSignals();
    addRuler(m_tempoRuler);

    // Scroll view to centre middle-C and warp to pointer position
    //
    m_canvasView->scrollBy(0, m_staffs[0]->getCanvasYForHeight(60) / 2);

    slotSetPointerPosition(comp.getPosition());

    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    slotTestClipboard();

    Rosegarden::timeT start = doc->getComposition().getLoopStart();
    Rosegarden::timeT end = doc->getComposition().getLoopEnd();
    m_topBarButtons->getLoopRuler()->slotSetLoopMarker(start, end);
    m_bottomBarButtons->getLoopRuler()->slotSetLoopMarker(start, end);

    setCurrentSelection(0, false);

    // Change this when the matrix view will have its own page
    // in the config dialog.
    setConfigDialogPageIndex(2);

    // default zoom
    slotChangeHorizontalZoom(-1);

    // All toolbars should be created before this is called
    setAutoSaveSettings("MatrixView", true);

    readOptions();
    setOutOfCtor();

    // Property and Control Rulers
    //
    if (getCurrentSegment()->getViewFeatures()) slotShowVelocityControlRuler();
    setupControllerTabs();

    setRewFFwdToAutoRepeat();
    slotCompositionStateUpdate();
}

MatrixView::~MatrixView()
{
    slotSaveOptions();

    delete m_chordNameRuler;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        delete m_staffs[i]; // this will erase all "notes" canvas items
    }

    // This looks silly but the reason is that on destruction of the
    // MatrixCanvasView, setCanvas() is called (this is in
    // ~QCanvasView so we can't do anything about it). This calls
    // QCanvasView::updateContentsSize(), which in turn updates the
    // view's scrollbars, hence calling QScrollBar::setValue(), and
    // sending the QSCrollbar::valueChanged() signal. But we have a
    // slot connected to that signal
    // (MatrixView::slotVerticalScrollPianoKeyboard), which scrolls
    // the pianoView. However at this stage the pianoView has already
    // been deleted, so a likely outcome is a crash.
    //
    // A solution is to zero out m_pianoView here, and to check if
    // it's non null in slotVerticalScrollPianoKeyboard.
    //
    m_pianoView = 0;

    delete m_snapGrid;

    if (m_localMapping) delete m_localMapping;
}

void MatrixView::slotSaveOptions()
{
    m_config->setGroup(ConfigGroup);

    m_config->writeEntry("Show Chord Name Ruler", getToggleAction("show_chords_ruler")->isChecked());
    m_config->writeEntry("Show Tempo Ruler",      getToggleAction("show_tempo_ruler")->isChecked());
    m_config->writeEntry("Show Parameters",       m_dockVisible);
    //getToggleAction("m_dockLeft->isVisible());

    m_config->sync();
}

void MatrixView::readOptions()
{
    EditView::readOptions();
    m_config->setGroup(ConfigGroup);

    bool opt = false;

    opt = m_config->readBoolEntry("Show Chord Name Ruler", false);
    getToggleAction("show_chords_ruler")->setChecked(opt);
    slotToggleChordsRuler();
    
    opt = m_config->readBoolEntry("Show Tempo Ruler", false);
    getToggleAction("show_tempo_ruler")->setChecked(opt);
    slotToggleTempoRuler();

    opt = m_config->readBoolEntry("Show Parameters", true);
    if (!opt)
    {
        m_dockLeft->undock();
        m_dockLeft->hide();
        stateChanged("parametersbox_closed", KXMLGUIClient::StateNoReverse);
        m_dockVisible = false;
    }

}

void MatrixView::setupActions()
{   
    EditViewBase::setupActions("matrix.rc");
    EditView::setupActions();

    //
    // Edition tools (eraser, selector...)
    //
    KRadioAction* toolAction = 0;

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIconSet icon(QPixmap(pixmapDir + "/toolbar/select.xpm"));

    toolAction = new KRadioAction(i18n("&Select"), icon, Key_F2,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("&Draw"), "pencil", Key_F3,
                                  this, SLOT(slotPaintSelected()),
                                  actionCollection(), "draw");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("&Erase"), "eraser", Key_F4,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("&Move"), "move", Key_F5,
                                  this, SLOT(slotMoveSelected()),
                                  actionCollection(), "move");
    toolAction->setExclusiveGroup("tools");

    QCanvasPixmap pixmap(pixmapDir + "/toolbar/resize.xpm");
    icon = QIconSet(pixmap);
    toolAction = new KRadioAction(i18n("Resi&ze"), icon, Key_F6,
                                  this, SLOT(slotResizeSelected()),
                                  actionCollection(), "resize");
    toolAction->setExclusiveGroup("tools");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("chord")));
    (new KToggleAction(i18n("C&hord Insert Mode"), icon, Key_H,
		       this, SLOT(slotUpdateInsertModeStatus()),
		       actionCollection(), "chord_mode"))->
	setChecked(false);

    pixmap.load(pixmapDir + "/toolbar/step_by_step.xpm");
    icon = QIconSet(pixmap);
    new KToggleAction(i18n("Ste&p Recording"), icon, 0, this,
                SLOT(slotToggleStepByStep()), actionCollection(),
                "toggle_step_by_step");

    pixmap.load(pixmapDir + "/toolbar/quantize.png");
    icon = QIconSet(pixmap);
    new KAction(EventQuantizeCommand::getGlobalName(), icon, Key_Equal, this,
                SLOT(slotTransformsQuantize()), actionCollection(),
                "quantize");

    new KAction(i18n("Repeat Last Quantize"), Key_Plus, this,
                SLOT(slotTransformsRepeatQuantize()), actionCollection(),
                "repeat_quantize");
    
    new KAction(AdjustMenuCollapseNotesCommand::getGlobalName(), Key_Equal + CTRL, this,
                SLOT(slotTransformsCollapseNotes()), actionCollection(),
               "collapse_notes");

    new KAction(i18n("&Legato"), Key_Minus, this,
                SLOT(slotTransformsLegato()), actionCollection(),
                "legatoize");

    new KAction(i18n("&Halve Speed"), Key_Less, this,
		SLOT(slotHalfSpeed()), actionCollection(),
		"half_speed");

    new KAction(i18n("&Double Speed"), Key_Greater, this,
		SLOT(slotDoubleSpeed()), actionCollection(),
		"double_speed");

    new KAction(RescaleCommand::getGlobalName(), 0, this,
		SLOT(slotRescale()), actionCollection(),
		"rescale");

    new KAction(ChangeVelocityCommand::getGlobalName(10), 0,
		Key_Up + SHIFT, this,
                SLOT(slotVelocityUp()), actionCollection(),
                "velocity_up");

    new KAction(ChangeVelocityCommand::getGlobalName(-10), 0,
		Key_Down + SHIFT, this,
                SLOT(slotVelocityDown()), actionCollection(),
                "velocity_down");

    new KAction(i18n("Set Event &Velocities..."), 0, this,
                SLOT(slotSetVelocities()), actionCollection(),
                "set_velocities");

    new KAction(i18n("Trigger Se&gment..."), 0, this,
                SLOT(slotTriggerSegment()), actionCollection(),
                "trigger_segment");

    new KAction(i18n("Remove Triggers..."), 0, this,
                SLOT(slotRemoveTriggers()), actionCollection(),
                "remove_trigger");

    new KAction(i18n("Select &All"), Key_A + CTRL, this,
                SLOT(slotSelectAll()), actionCollection(),
                "select_all");

    new KAction(i18n("&Delete"), Key_Delete, this,
                SLOT(slotEditDelete()), actionCollection(),
                "delete");

    new KAction(i18n("Cursor &Back"), 0, Key_Left, this,
		SLOT(slotStepBackward()), actionCollection(),
		"cursor_back");

    new KAction(i18n("Cursor &Forward"), 0, Key_Right, this,
		SLOT(slotStepForward()), actionCollection(),
		"cursor_forward");

    new KAction(i18n("Cursor Ba&ck Bar"), 0, Key_Left + CTRL, this,
		SLOT(slotJumpBackward()), actionCollection(),
		"cursor_back_bar");

    new KAction(i18n("Cursor For&ward Bar"), 0, Key_Right + CTRL, this,
		SLOT(slotJumpForward()), actionCollection(),
		"cursor_forward_bar");

    new KAction(i18n("Cursor Back and Se&lect"), SHIFT + Key_Left, this,
		SLOT(slotExtendSelectionBackward()), actionCollection(),
		"extend_selection_backward");

    new KAction(i18n("Cursor Forward and &Select"), SHIFT + Key_Right, this,
		SLOT(slotExtendSelectionForward()), actionCollection(),
		"extend_selection_forward");

    new KAction(i18n("Cursor Back Bar and Select"), SHIFT + CTRL + Key_Left, this,
		SLOT(slotExtendSelectionBackwardBar()), actionCollection(),
		"extend_selection_backward_bar");

    new KAction(i18n("Cursor Forward Bar and Select"), SHIFT + CTRL + Key_Right, this,
		SLOT(slotExtendSelectionForwardBar()), actionCollection(),
		"extend_selection_forward_bar");

    new KAction(i18n("Cursor to St&art"), 0,
		/* #1025717: conflicting meanings for ctrl+a - dupe with Select All
		  Key_A + CTRL, */ this,
		SLOT(slotJumpToStart()), actionCollection(),
		"cursor_start");

    new KAction(i18n("Cursor to &End"), 0, Key_E + CTRL, this,
		SLOT(slotJumpToEnd()), actionCollection(),
		"cursor_end");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-cursor-to-pointer")));
    new KAction(i18n("Cursor to &Playback Pointer"), icon, 0, this,
		SLOT(slotJumpCursorToPlayback()), actionCollection(),
		"cursor_to_playback_pointer");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-play")));
    new KAction(i18n("&Play"), icon, Key_Enter, this,
		SIGNAL(play()), actionCollection(), "play");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-stop")));
    new KAction(i18n("&Stop"), icon, Key_Insert, this,
		SIGNAL(stop()), actionCollection(), "stop");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-rewind")));
    new KAction(i18n("Re&wind"), icon, Key_End, this,
		SIGNAL(rewindPlayback()), actionCollection(),
		"playback_pointer_back_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-ffwd")));
    new KAction(i18n("&Fast Forward"), icon, Key_PageDown, this,
		SIGNAL(fastForwardPlayback()), actionCollection(),
		"playback_pointer_forward_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-rewind-end")));
    new KAction(i18n("Rewind to &Beginning"), icon, 0, this,
		SIGNAL(rewindPlaybackToBeginning()), actionCollection(),
		"playback_pointer_start");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-ffwd-end")));
    new KAction(i18n("Fast Forward to &End"), icon, 0, this,
		SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
		"playback_pointer_end");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-pointer-to-cursor")));
    new KAction(i18n("Playback Pointer to &Cursor"), icon, 0, this,
		SLOT(slotJumpPlaybackToCursor()), actionCollection(),
		"playback_pointer_to_cursor");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-solo")));
    new KToggleAction(i18n("&Solo"), icon, 0, this,
                SLOT(slotToggleSolo()), actionCollection(),
                "toggle_solo");

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-tracking")));
    (new KToggleAction(i18n("Scro&ll to Follow Playback"), icon, Key_Pause, this,
		       SLOT(slotToggleTracking()), actionCollection(),
		       "toggle_tracking"))->setChecked(m_playTracking);

    icon = QIconSet(NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap
                                                 ("transport-panic")));
    new KAction(i18n("Panic"), icon, Key_P + CTRL + ALT, this,
                SIGNAL(panic()), actionCollection(), "panic");

    new KAction(i18n("Set Loop to Selection"), Key_Semicolon + CTRL, this,
		SLOT(slotPreviewSelection()), actionCollection(),
		"preview_selection");

    new KAction(i18n("Clear L&oop"), Key_Colon + CTRL, this,
		SLOT(slotClearLoop()), actionCollection(),
		"clear_loop");

    new KAction(i18n("Clear Selection"), Key_Escape, this,
		SLOT(slotClearSelection()), actionCollection(),
		"clear_selection");

//    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/eventfilter.xpm"));
    new KAction(i18n("&Filter Selection"), "filter", Key_F + CTRL, this,
	    	SLOT(slotFilterSelection()), actionCollection(),
		"filter_selection");
    
    //!!! should be using NotationStrings::makeNoteMenuLabel for these
    new KAction(i18n("Snap to 1/64"), Key_0, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_64");
    new KAction(i18n("Snap to 1/48"), 0, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_48");
    new KAction(i18n("Snap to 1/32"), Key_3, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_32");
    new KAction(i18n("Snap to 1/24"), 0, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_24");
    new KAction(i18n("Snap to 1/16"), Key_6, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_16");
    new KAction(i18n("Snap to 1/12"), 0, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_12");
    new KAction(i18n("Snap to 1/8"), Key_8, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_8");
    new KAction(i18n("Snap to 1/6"), 0, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_6");
    new KAction(i18n("Snap to 1/4"), Key_4, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_4");
    new KAction(i18n("Snap to 1/2"), Key_2, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_2");
    new KAction(i18n("Snap to &Unit"), 0, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_unit");
    new KAction(i18n("Snap to Bea&t"), Key_1, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_beat");
    new KAction(i18n("Snap to &Bar"), Key_5, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_bar");
    new KAction(i18n("&No Snap"), 0, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_none");

    //
    // Settings menu
    //
    new KAction(i18n("Show Instrument Parameters"), 0, this,
                SLOT(slotDockParametersBack()),
                actionCollection(),
                "show_inst_parameters");

    new KToggleAction(i18n("Show Ch&ord Name Ruler"), 0, this,
                      SLOT(slotToggleChordsRuler()),
                      actionCollection(), "show_chords_ruler");

    new KToggleAction(i18n("Show &Tempo Ruler"), 0, this,
                      SLOT(slotToggleTempoRuler()),
                      actionCollection(), "show_tempo_ruler");

    createGUI(getRCFileName(), false);

    if (getSegmentsOnlyRestsAndClefs())
        actionCollection()->action("draw")->activate();
    else
        actionCollection()->action("select")->activate();
}

bool
MatrixView::isInChordMode()
{
    return ((KToggleAction *)actionCollection()->action("chord_mode"))->
	isChecked();
}

void MatrixView::slotDockParametersBack()
{
    m_dockLeft->dockBack();
}

void MatrixView::slotParametersClosed()
{
    stateChanged("parametersbox_closed");
    m_dockVisible = false;
}

void MatrixView::slotParametersDockedBack(KDockWidget* dw, KDockWidget::DockPosition)
{
    if (dw == m_dockLeft)
    {
        stateChanged("parametersbox_closed", KXMLGUIClient::StateReverse);
        m_dockVisible = true;
    }
}

void MatrixView::slotCheckTrackAssignments()
{
    Rosegarden::Track *track =
        m_staffs[0]->getSegment().getComposition()->
	getTrackById(m_staffs[0]->getSegment().getTrack());

    Rosegarden::Instrument *instr = getDocument()->getStudio().
        getInstrumentById(track->getInstrument());

    m_parameterBox->useInstrument(instr);
}

void MatrixView::initStatusBar()
{
    KStatusBar* sb = statusBar();
    
    m_hoveredOverNoteName      = new QLabel(sb);
    m_hoveredOverAbsoluteTime  = new QLabel(sb);

    m_hoveredOverNoteName->setMinimumWidth(32);
    m_hoveredOverAbsoluteTime->setMinimumWidth(160);

    sb->addWidget(m_hoveredOverAbsoluteTime);
    sb->addWidget(m_hoveredOverNoteName);

    m_insertModeLabel = new QLabel(sb);
    sb->addWidget(m_insertModeLabel);

    sb->insertItem(KTmpStatusMsg::getDefaultMsg(),
                   KTmpStatusMsg::getDefaultId(), 1);
    sb->setItemAlignment(KTmpStatusMsg::getDefaultId(), 
                         AlignLeft | AlignVCenter);

    m_selectionCounter = new QLabel(sb);
    sb->addWidget(m_selectionCounter);
}


bool MatrixView::applyLayout(int staffNo,
			     timeT startTime,
			     timeT endTime)
{
    Rosegarden::Profiler profiler("MatrixView::applyLayout", true);
    
    m_hlayout.reset();
    m_vlayout.reset();
        
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        if (staffNo >= 0 && (int)i != staffNo) continue;

        m_hlayout.scanStaff(*m_staffs[i], startTime, endTime);
        m_vlayout.scanStaff(*m_staffs[i], startTime, endTime);
    }

    m_hlayout.finishLayout();
    m_vlayout.finishLayout();

    if (m_staffs[0]->getSegment().getEndMarkerTime() != m_lastEndMarkerTime ||
	m_lastEndMarkerTime == 0 ||
	isCompositionModified()) {
	readjustCanvasSize();
	m_lastEndMarkerTime = m_staffs[0]->getSegment().getEndMarkerTime();
    }
    
    return true;
}

void MatrixView::refreshSegment(Segment *segment,
				timeT startTime, timeT endTime)
{
    Rosegarden::Profiler profiler("MatrixView::refreshSegment", true);

    MATRIX_DEBUG << "MatrixView::refreshSegment(" << startTime
                         << ", " << endTime << ")\n";

    applyLayout(-1, startTime, endTime);

    if (!segment) segment = m_segments[0];

    if (endTime == 0) endTime = segment->getEndTime();
    else if (startTime == endTime) {
        startTime = segment->getStartTime();
        endTime   = segment->getEndTime();
    }

    m_staffs[0]->positionElements(startTime, endTime);
    repaintRulers();
}

QSize MatrixView::getViewSize()
{
    return canvas()->size();
}

void MatrixView::setViewSize(QSize s)
{
    MATRIX_DEBUG  << "MatrixView::setViewSize() w = " << s.width()
                  << endl;

    canvas()->resize(s.width(), s.height());
    getCanvasView()->resizeContents(s.width(), s.height());

    MATRIX_DEBUG  << "MatrixView::setViewSize() contentsWidth = " << getCanvasView()->contentsWidth()
                  << endl;
}

void MatrixView::repaintRulers()
{
    for (unsigned int i = 0; i != m_propertyViewRulers.size(); i++)
        m_propertyViewRulers[i].first->repaint();
}


void MatrixView::updateView()
{
    canvas()->update();
}

MatrixCanvasView* MatrixView::getCanvasView()
{
    return dynamic_cast<MatrixCanvasView *>(m_canvasView);
}


void MatrixView::setCurrentSelection(EventSelection* s, bool preview,
				     bool redrawNow)
{
    //!!! rather too much here shared with notationview -- could much of
    // this be in editview?

    if (m_currentEventSelection == s)	{
	updateQuantizeCombo();
	return;
    }

    if (m_currentEventSelection) {
        getStaff(0)->positionElements(m_currentEventSelection->getStartTime(),
                                      m_currentEventSelection->getEndTime());
    }

    EventSelection *oldSelection = m_currentEventSelection;
    m_currentEventSelection = s;

    timeT startA, endA, startB, endB;

    if (oldSelection) {
        startA = oldSelection->getStartTime();
        endA   = oldSelection->getEndTime();
        startB = s ? s->getStartTime() : startA;
        endB   = s ? s->getEndTime()   : endA;
    } else {
        // we know they can't both be null -- first thing we tested above
        startA = startB = s->getStartTime();
        endA   = endB   = s->getEndTime();
    }

    // refreshSegment takes start==end to mean refresh everything
    if (startA == endA) ++endA;
    if (startB == endB) ++endB;

    bool updateRequired = true;

    if (s) {

        bool foundNewEvent = false;

        for (EventSelection::eventcontainer::iterator i =
                 s->getSegmentEvents().begin();
             i != s->getSegmentEvents().end(); ++i) {

            if (oldSelection && oldSelection->getSegment() == s->getSegment()
                && oldSelection->contains(*i)) continue;

            foundNewEvent = true;

	    if (preview) {
		long pitch;
		if ((*i)->get<Rosegarden::Int>
		    (Rosegarden::BaseProperties::PITCH, pitch)) {
		    long velocity = -1;
		    (void)((*i)->get<Rosegarden::Int>
			   (Rosegarden::BaseProperties::VELOCITY, velocity));
		    if (!((*i)->has(Rosegarden::BaseProperties::TIED_BACKWARD)&&
		          (*i)->get<Rosegarden::Bool>
			           (Rosegarden::BaseProperties::TIED_BACKWARD)))
		        playNote(s->getSegment(), pitch, velocity);
		}
	    }
	}
	
        if (!foundNewEvent) {
            if (oldSelection &&
                oldSelection->getSegment() == s->getSegment() &&
                oldSelection->getSegmentEvents().size() ==
                s->getSegmentEvents().size()) updateRequired = false;
        }
    }

    if (updateRequired) {

        if ((endA >= startB && endB >= startA) &&
            (!s || !oldSelection ||
             oldSelection->getSegment() == s->getSegment())) {

	    Segment &segment(s ? s->getSegment() :
			     oldSelection->getSegment());

	    if (redrawNow) {
		// recolour the events now
		getStaff(segment)->positionElements(std::min(startA, startB),
						    std::max(endA, endB));
	    } else {
		// mark refresh status and then request a repaint
		segment.getRefreshStatus
		    (m_segmentsRefreshStatusIds
		     [getStaff(segment)->getId()]).
		    push(std::min(startA, startB), std::max(endA, endB));
	    }
	    
	} else {
	    // do two refreshes, one for each -- here we know neither is null

	    if (redrawNow) {
		// recolour the events now
		getStaff(oldSelection->getSegment())->positionElements(startA,
								       endA);
		
		getStaff(s->getSegment())->positionElements(startB, endB);
	    } else {
		// mark refresh status and then request a repaint

		oldSelection->getSegment().getRefreshStatus
		    (m_segmentsRefreshStatusIds
		     [getStaff(oldSelection->getSegment())->getId()]).
		    push(startA, endA);
		
		s->getSegment().getRefreshStatus
		    (m_segmentsRefreshStatusIds
		     [getStaff(s->getSegment())->getId()]).
		    push(startB, endB);
	    }
        }
    }

    delete oldSelection;
    if (s) {
	int eventsSelected = s->getSegmentEvents().size();
        m_selectionCounter->setText
	    (i18n("  1 event selected ",
		  "  %n events selected ", eventsSelected));
    } else {
        m_selectionCounter->setText(i18n("  No selection "));
    }
    m_selectionCounter->update();


    // Clear states first, then enter only those ones that apply
    // (so as to avoid ever clearing one after entering another, in
    // case the two overlap at all)
    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_notes_in_selection", KXMLGUIClient::StateReverse);
    stateChanged("have_rests_in_selection", KXMLGUIClient::StateReverse);

    if (s) {
        stateChanged("have_selection", KXMLGUIClient::StateNoReverse);
        if (s->contains(Rosegarden::Note::EventType)) {
            stateChanged("have_notes_in_selection",
                         KXMLGUIClient::StateNoReverse);
        }
        if (s->contains(Rosegarden::Note::EventRestType)) {
            stateChanged("have_rests_in_selection",
                         KXMLGUIClient::StateNoReverse);
        }
    }

    updateQuantizeCombo();

    if (redrawNow) updateView();
    else update();
}


void MatrixView::updateQuantizeCombo()
{
    Rosegarden::timeT unit = 0;

    if (m_currentEventSelection) {
	unit =
	    Rosegarden::BasicQuantizer::getStandardQuantization
	    (m_currentEventSelection);
    } else {
	unit =
	    Rosegarden::BasicQuantizer::getStandardQuantization
	    (&(m_staffs[0]->getSegment()));
    }

    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {
	if (unit == m_quantizations[i]) {
	    m_quantizeCombo->setCurrentItem(i);
	    return;
	}
    }

    m_quantizeCombo->setCurrentItem(m_quantizeCombo->count() - 1); // "Off"
}

void MatrixView::slotPaintSelected()
{
    EditTool* painter = m_toolBox->getTool(MatrixPainter::ToolName);

    setTool(painter);
}

void MatrixView::slotEraseSelected()
{
    EditTool* eraser = m_toolBox->getTool(MatrixEraser::ToolName);

    setTool(eraser);
}

void MatrixView::slotSelectSelected()
{
    EditTool* selector = m_toolBox->getTool(MatrixSelector::ToolName);

    connect(selector, SIGNAL(gotSelection()),
            this, SLOT(slotNewSelection()));

    connect(selector, SIGNAL(editTriggerSegment(int)),
            this, SIGNAL(editTriggerSegment(int)));

    setTool(selector);
}

void MatrixView::slotMoveSelected()
{
    EditTool* mover = m_toolBox->getTool(MatrixMover::ToolName);

    setTool(mover);
}

void MatrixView::slotResizeSelected()
{
    EditTool* resizer = m_toolBox->getTool(MatrixResizer::ToolName);

    setTool(resizer);
}

void MatrixView::slotTransformsQuantize()
{
    using Rosegarden::Quantizer;

    if (!m_currentEventSelection) return;

    QuantizeDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
	KTmpStatusMsg msg(i18n("Quantizing..."), this);
	addCommandToHistory(new EventQuantizeCommand
			    (*m_currentEventSelection,
			     dialog.getQuantizer()));
    }
}

void MatrixView::slotTransformsRepeatQuantize()
{
    using Rosegarden::Quantizer;

    if (!m_currentEventSelection) return;

    KTmpStatusMsg msg(i18n("Quantizing..."), this);
    addCommandToHistory(new EventQuantizeCommand
			(*m_currentEventSelection,
			 "Quantize Dialog Grid", false)); // no i18n (config group name)
}

void MatrixView::slotTransformsCollapseNotes()
{
    if (!m_currentEventSelection) return;
	KTmpStatusMsg msg(i18n("Collapsing notes..."), this);

    addCommandToHistory(new AdjustMenuCollapseNotesCommand
		        (*m_currentEventSelection));
}


void MatrixView::slotTransformsLegato()
{
    using Rosegarden::Quantizer;

    if (!m_currentEventSelection) return;

    KTmpStatusMsg msg(i18n("Making legato..."), this);
    addCommandToHistory(new EventQuantizeCommand
			(*m_currentEventSelection,
			 new Rosegarden::LegatoQuantizer(0))); // no quantization
}

void
MatrixView::slotHalfSpeed()
{
    if (!m_currentEventSelection) return;

    KTmpStatusMsg msg(i18n("Halving speed..."), this);

    addCommandToHistory(
            new RescaleCommand(*m_currentEventSelection,
                m_currentEventSelection->getTotalDuration() * 2,
                false));
}

void
MatrixView::slotDoubleSpeed()
{
    if (!m_currentEventSelection) return;

    KTmpStatusMsg msg(i18n("Doubling speed..."), this);

    addCommandToHistory(
            new RescaleCommand(*m_currentEventSelection,
                m_currentEventSelection->getTotalDuration() / 2,
                 false));
}

void
MatrixView::slotRescale()
{
    if (!m_currentEventSelection) return;

    RescaleDialog dialog
	(this,
	 &getDocument()->getComposition(),
	 m_currentEventSelection->getStartTime(),
	 m_currentEventSelection->getEndTime() -
	 m_currentEventSelection->getStartTime(),
	 true);

    if (dialog.exec() == QDialog::Accepted) {
	KTmpStatusMsg msg(i18n("Rescaling..."), this);
	addCommandToHistory(new RescaleCommand
			    (*m_currentEventSelection,
			     dialog.getNewDuration(),
			     dialog.shouldCloseGap()));
    }
}


void MatrixView::slotMousePressed(Rosegarden::timeT time, int pitch,
                                  QMouseEvent* e, MatrixElement* el)
{
    MATRIX_DEBUG << "MatrixView::mousePressed at pitch "
                         << pitch << ", time " << time << endl;

    // Don't allow moving/insertion before the beginning of the
    // segment
    timeT curSegmentStartTime = getCurrentSegment()->getStartTime();
    if (curSegmentStartTime > time) time=curSegmentStartTime;

    m_tool->handleMousePress(time, pitch, 0, e, el);

    if (e->button() != RightButton) {
	getCanvasView()->startAutoScroll();
    }

    // play a preview
    //playPreview(pitch);
}

void MatrixView::slotMouseMoved(Rosegarden::timeT time, int pitch, QMouseEvent* e)
{
    // Don't allow moving/insertion before the beginning of the
    // segment
    timeT curSegmentStartTime = getCurrentSegment()->getStartTime();
    if (curSegmentStartTime > time) time=curSegmentStartTime;

    if (activeItem()) {
        activeItem()->handleMouseMove(e);
	updateView();
    }
    else 
    {
        int follow = m_tool->handleMouseMove(time, pitch, e);
        getCanvasView()->setScrollDirectionConstraint(follow);
        
//        if (follow != RosegardenCanvasView::NoFollow) {
//            getCanvasView()->doAutoScroll();
//        }
        
        // play a preview
        if (pitch != m_previousEvPitch)
        {
            //playPreview(pitch);
            m_previousEvPitch = pitch;
        }
    }

}

void MatrixView::slotMouseReleased(Rosegarden::timeT time, int pitch, QMouseEvent* e)
{
    // Don't allow moving/insertion before the beginning of the
    // segment
    timeT curSegmentStartTime = getCurrentSegment()->getStartTime();
    if (curSegmentStartTime > time) time=curSegmentStartTime;

    if (activeItem()) {
        activeItem()->handleMouseRelease(e);
        setActiveItem(0);
        updateView();
    }

    // send the real event time now (not adjusted for beginning of bar)
    m_tool->handleMouseRelease(time, pitch, e);
    m_previousEvPitch = 0;
    getCanvasView()->stopAutoScroll();
}

void
MatrixView::slotHoveredOverNoteChanged(int evPitch)
{
    Rosegarden::MidiPitchLabel label(evPitch);
    m_hoveredOverNoteName->setText(QString("%1 (%2)").
            arg(label.getQString()).arg(evPitch));
    m_pitchRuler->drawHoverNote(evPitch);
}

void
MatrixView::slotHoveredOverKeyChanged(unsigned int y)
{
    MatrixStaff& staff = *(m_staffs[0]);

    int evPitch = staff.getHeightAtCanvasCoords(-1, y);

    if (evPitch != m_previousEvPitch) {
	Rosegarden::MidiPitchLabel label(evPitch);
        m_hoveredOverNoteName->setText(QString("%1 (%2)"). 
                    arg(label.getQString()).arg(evPitch));
        m_previousEvPitch = evPitch;
    }
}

void
MatrixView::slotHoveredOverAbsoluteTimeChanged(unsigned int time)
{
    timeT t = time;
    Rosegarden::RealTime rt =
	getDocument()->getComposition().getElapsedRealTime(t);
    long ms = rt.msec();

    // At the advice of doc.trolltech.com/3.0/qstring.html#sprintf
    // we replaced this    QString format("%ld (%ld.%03lds)");
    // to support Unicode
    QString msString=QString("%1").arg(ms);
    QString message = i18n("Time: %1 (%2.%3s)").arg(t).arg(rt.sec).arg(msString.rightJustify(3,'0'));

    m_hoveredOverAbsoluteTime->setText(message);
}

void
MatrixView::slotSetPointerPosition(timeT time)
{
    slotSetPointerPosition(time, m_playTracking);
}

void
MatrixView::slotSetPointerPosition(timeT time, bool scroll)
{
    Rosegarden::Composition &comp = getDocument()->getComposition();
    int barNo = comp.getBarNumber(time);

    if (barNo >= m_hlayout.getLastVisibleBarOnStaff(*m_staffs[0])) {

	Segment &seg = m_staffs[0]->getSegment();

	if (seg.isRepeating() && time < seg.getRepeatEndTime()) {
	    time =
		seg.getStartTime() +
		((time - seg.getStartTime()) %
		 (seg.getEndMarkerTime() - seg.getStartTime()));
	    m_staffs[0]->setPointerPosition(m_hlayout, time);
	} else {
	    m_staffs[0]->hidePointer();
	    scroll = false;
	}
    } else if (barNo < m_hlayout.getFirstVisibleBarOnStaff(*m_staffs[0])) {
	m_staffs[0]->hidePointer();
	scroll = false;
    } else {
        m_staffs[0]->setPointerPosition(m_hlayout, time);
    }

    if (scroll && !getCanvasView()->isAutoScrolling())
        getCanvasView()->slotScrollHoriz(static_cast<int>(getXbyWorldMatrix(m_hlayout.getXForTime(time))));

    updateView();
}

void
MatrixView::slotSetInsertCursorPosition(timeT time, bool scroll)
{
    //!!! For now.  Probably unlike slotSetPointerPosition this one
    // should snap to the nearest event or grid line.

    m_staffs[0]->setInsertCursorPosition(m_hlayout, time);

    if (scroll && !getCanvasView()->isAutoScrolling()) {
        getCanvasView()->slotScrollHoriz(static_cast<int>(getXbyWorldMatrix(m_hlayout.getXForTime(time))));
    }

    updateView();
}


//////////////////////////////////////////////////////////////////////
//                    Slots
//////////////////////////////////////////////////////////////////////

//
// Cut, Copy, Paste
//
void MatrixView::slotEditCut()
{
    MATRIX_DEBUG << "MatrixView::slotEditCut()\n";

    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), this);

    addCommandToHistory(new CutCommand(*m_currentEventSelection,
				       getDocument()->getClipboard()));
}

void MatrixView::slotEditCopy()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), this);

    addCommandToHistory(new CopyCommand(*m_currentEventSelection,
					getDocument()->getClipboard()));

    emit usedSelection();
}

void MatrixView::slotEditPaste()
{
    if (getDocument()->getClipboard()->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return;
    }

    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), this);
    
    PasteEventsCommand *command = new PasteEventsCommand
	(m_staffs[0]->getSegment(), getDocument()->getClipboard(),
	 getInsertionTime(), PasteEventsCommand::MatrixOverlay);

    if (!command->isPossible()) {
	slotStatusHelpMsg(i18n("Couldn't paste at this point"));
    } else {
	addCommandToHistory(command);
    }
}

void MatrixView::slotEditDelete()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Deleting selection..."), this);

    addCommandToHistory(new EraseCommand(*m_currentEventSelection));

    // clear and clear 
    setCurrentSelection(0, false);
}

// Propagate a key press upwards
//
void MatrixView::slotKeyPressed(unsigned int y, bool repeating)
{
    slotHoveredOverKeyChanged(y);

    getCanvasView()->slotScrollVertSmallSteps(y);

    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();

    MatrixStaff& staff = *(m_staffs[0]);
    Rosegarden::MidiByte evPitch = staff.getHeightAtCanvasCoords(-1, y);

    // Don't do anything if we're part of a run up the keyboard
    // and the pitch hasn't changed
    //
    if (m_lastNote == evPitch && repeating)
        return;

    // Save value
    m_lastNote = evPitch;
    if (!repeating) m_firstNote = evPitch;

    Rosegarden::Track *track = comp.getTrackById(
            staff.getSegment().getTrack());

    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return;

    Rosegarden::MappedEvent mE(ins->getId(),
                               Rosegarden::MappedEvent::MidiNote,
                               evPitch + staff.getSegment().getTranspose(),
                               Rosegarden::MidiMaxValue,
                               Rosegarden::RealTime::zeroTime,
                               Rosegarden::RealTime::zeroTime,
                               Rosegarden::RealTime::zeroTime);
    Rosegarden::StudioControl::sendMappedEvent(mE);

}


void MatrixView::slotKeySelected(unsigned int y, bool repeating)
{
    slotHoveredOverKeyChanged(y);

    getCanvasView()->slotScrollVertSmallSteps(y);

    MatrixStaff& staff = *(m_staffs[0]);
    Rosegarden::Segment &segment(staff.getSegment());
    Rosegarden::MidiByte evPitch = staff.getHeightAtCanvasCoords(-1, y);

    // Don't do anything if we're part of a run up the keyboard
    // and the pitch hasn't changed
    //
    if (m_lastNote == evPitch && repeating)
        return;

    // Save value
    m_lastNote = evPitch;
    if (!repeating) m_firstNote = evPitch;

    EventSelection *s = new EventSelection(segment);

    for (Rosegarden::Segment::iterator i = segment.begin();
	 segment.isBeforeEndMarker(i); ++i) {

	if ((*i)->isa(Rosegarden::Note::EventType) &&
	    (*i)->has(Rosegarden::BaseProperties::PITCH)) {

	    Rosegarden::MidiByte p = (*i)->get<Rosegarden::Int>
		(Rosegarden::BaseProperties::PITCH);
	    if (p >= std::min(m_firstNote, evPitch) &&
		p <= std::max(m_firstNote, evPitch)) {
		s->addEvent(*i);
	    }
	}
    }

    if (m_currentEventSelection) {
        // allow addFromSelection to deal with eliminating duplicates
	s->addFromSelection(m_currentEventSelection);
    }

    setCurrentSelection(s, false);

    // now play the note as well

    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Track *track = comp.getTrackById(segment.getTrack());
    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return;

    Rosegarden::MappedEvent mE(ins->getId(),
                               Rosegarden::MappedEvent::MidiNoteOneShot,
                               evPitch + segment.getTranspose(),
                               Rosegarden::MidiMaxValue,
                               Rosegarden::RealTime::zeroTime,
                               Rosegarden::RealTime(0, 250000000),
                               Rosegarden::RealTime::zeroTime);
    Rosegarden::StudioControl::sendMappedEvent(mE);
}

void MatrixView::slotKeyReleased(unsigned int y, bool repeating)
{
    MatrixStaff& staff = *(m_staffs[0]);
    Rosegarden::MidiByte evPitch = staff.getHeightAtCanvasCoords(-1, y);

    if (m_lastNote == evPitch && repeating)
        return;

    Rosegarden::Segment &segment(staff.getSegment());

    // send note off (note on at zero velocity)

    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();
    Rosegarden::Track *track = comp.getTrackById(segment.getTrack());
    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return;

    Rosegarden::MappedEvent mE(ins->getId(),
                               Rosegarden::MappedEvent::MidiNote,
                               evPitch + segment.getTranspose(),
                               0,
                               Rosegarden::RealTime::zeroTime,
                               Rosegarden::RealTime::zeroTime,
                               Rosegarden::RealTime::zeroTime);
    Rosegarden::StudioControl::sendMappedEvent(mE);
}

void MatrixView::slotVerticalScrollPianoKeyboard(int y)
{
    if (m_pianoView) // check that the piano view still exists (see dtor)
        m_pianoView->setContentsPos(0, y);
}

void MatrixView::slotInsertNoteFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    Segment &segment = *getCurrentSegment();
    int pitch = 0;

    Rosegarden::Accidental accidental = 
	Rosegarden::Accidentals::NoAccidental;

    Rosegarden::timeT time(getInsertionTime());
    Rosegarden::Key key = segment.getKeyAtTime(time);
    Rosegarden::Clef clef = segment.getClefAtTime(time);

    try {

	pitch = getPitchFromNoteInsertAction(name, accidental, clef, key);

    } catch (...) {
	
	KMessageBox::sorry
	    (this, i18n("Unknown note insert action %1").arg(name));
	return;
    }

    KTmpStatusMsg msg(i18n("Inserting note"), this);
	
    MATRIX_DEBUG << "Inserting note at pitch " << pitch << endl;

    Rosegarden::Event modelEvent(Rosegarden::Note::EventType, 0, 1);
    modelEvent.set<Rosegarden::Int>(Rosegarden::BaseProperties::PITCH, pitch);
    modelEvent.set<Rosegarden::String>(Rosegarden::BaseProperties::ACCIDENTAL, accidental);
    Rosegarden::timeT endTime(time + m_snapGrid->getSnapTime(time));

    MatrixInsertionCommand* command = 
	new MatrixInsertionCommand(segment, time, endTime, &modelEvent);

    addCommandToHistory(command);
    
    if (!isInChordMode()) {
	slotSetInsertCursorPosition(endTime);
    }
}

void MatrixView::closeWindow()
{
    delete this;
}

bool MatrixView::canPreviewAnotherNote()
{
    static time_t lastCutOff = 0;
    static int sinceLastCutOff = 0;

    time_t now = time(0);
    ++sinceLastCutOff;

    if ((now - lastCutOff) > 0) {
	sinceLastCutOff = 0;
	lastCutOff = now;
    } else {
	if (sinceLastCutOff >= 20) {
	    // don't permit more than 20 notes per second, to avoid
	    // gungeing up the sound drivers
	    MATRIX_DEBUG << "Rejecting preview (too busy)" << endl;
	    return false;
	}
    }

    return true;
}

void MatrixView::playNote(Rosegarden::Event *event)
{
    // Only play note events
    //
    if (!event->isa(Rosegarden::Note::EventType))
        return;

    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();

    // Get the Instrument
    //
    Rosegarden::Track *track = comp.getTrackById(
            m_staffs[0]->getSegment().getTrack());

    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    if (ins == 0)
        return;

    if (!canPreviewAnotherNote()) return;

    // Get a velocity
    //
    Rosegarden::MidiByte velocity = Rosegarden::MidiMaxValue / 4; // be easy on the user's ears
    long eventVelocity = 0;
    if (event->get<Rosegarden::Int>(Rosegarden::BaseProperties::VELOCITY, eventVelocity))
        velocity = eventVelocity;

    Rosegarden::RealTime duration =
            comp.getElapsedRealTime(event->getDuration());

    // create
    Rosegarden::MappedEvent mE(ins->getId(),
                               Rosegarden::MappedEvent::MidiNoteOneShot,
                               (Rosegarden::MidiByte)
                               event->get<Rosegarden::Int>
                               (Rosegarden::BaseProperties::PITCH) +
			       m_staffs[0]->getSegment().getTranspose(),
                               velocity,
                               Rosegarden::RealTime::zeroTime,
                               duration,
                               Rosegarden::RealTime::zeroTime);

    Rosegarden::StudioControl::sendMappedEvent(mE);
}


void MatrixView::playNote(const Rosegarden::Segment &segment, int pitch,
			  int velocity)
{
    Rosegarden::Composition &comp = getDocument()->getComposition();
    Rosegarden::Studio &studio = getDocument()->getStudio();

    Rosegarden::Track *track = comp.getTrackById(segment.getTrack());

    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return;

    if (velocity < 0) velocity = Rosegarden::MidiMaxValue;

    Rosegarden::MappedEvent mE(ins->getId(),
                               Rosegarden::MappedEvent::MidiNoteOneShot,
                               pitch + segment.getTranspose(),
                               velocity,
                               Rosegarden::RealTime::zeroTime,
                               Rosegarden::RealTime(0, 250000000),
                               Rosegarden::RealTime::zeroTime);

    Rosegarden::StudioControl::sendMappedEvent(mE);
}

MatrixStaff* 
MatrixView::getStaff(const Rosegarden::Segment &segment)
{
    for (unsigned int i = 0; i < m_staffs.size(); ++i)
    {
        if (&(m_staffs[i]->getSegment()) == &segment)
            return m_staffs[i];
    }

    return 0;
}


void
MatrixView::setSingleSelectedEvent(int staffNo, Rosegarden::Event *event,
				   bool preview, bool redrawNow)
{
    setSingleSelectedEvent(getStaff(staffNo)->getSegment(), event,
			   preview, redrawNow);
}

void
MatrixView::setSingleSelectedEvent(Rosegarden::Segment &segment,
                                   Rosegarden::Event *event,
				   bool preview, bool redrawNow)
{
    setCurrentSelection(0, false);

    EventSelection *selection = new EventSelection(segment);
    selection->addEvent(event);

    //!!!
    // this used to say
    //   setCurrentSelection(selection, true)
    // since the default arg for preview is false, this changes the
    // default semantics -- test what circumstance this matters in
    // and choose an acceptable solution for both matrix & notation
    setCurrentSelection(selection, preview, redrawNow);
}

// A new selection has been acquired by a tool - set the appropriate
// information in matrix parameter pane.
//
void
MatrixView::slotNewSelection()
{
    MATRIX_DEBUG << "MatrixView::slotNewSelection\n";

//    m_parameterBox->setSelection(m_currentEventSelection);
}


void
MatrixView::slotSetSnapFromIndex(int s)
{
    slotSetSnap(m_snapValues[s]);
}


void
MatrixView::slotSetSnapFromAction()
{
    const QObject *s = sender();
    QString name = s->name();

    if (name.left(5) == "snap_") {
	int snap = name.right(name.length() - 5).toInt();
	if (snap > 0) {
	    slotSetSnap
		(Rosegarden::Note(Rosegarden::Note::Semibreve).getDuration() /
		 snap);
	} else if (name == "snap_none") {
	    slotSetSnap(Rosegarden::SnapGrid::NoSnap);
	} else if (name == "snap_beat") {
	    slotSetSnap(Rosegarden::SnapGrid::SnapToBeat);
	} else if (name == "snap_bar") {
	    slotSetSnap(Rosegarden::SnapGrid::SnapToBar);
	} else if (name == "snap_unit") {
	    slotSetSnap(Rosegarden::SnapGrid::SnapToUnit);
	} else {
	    MATRIX_DEBUG << "Warning: MatrixView::slotSetSnapFromAction: unrecognised action " << name << endl;
	}
    }
}


void
MatrixView::slotSetSnap(timeT t)
{
    MATRIX_DEBUG << "MatrixView::slotSetSnap: time is " << t << endl;
    m_snapGrid->setSnapTime(t);

    for (unsigned int i = 0; i < m_snapValues.size(); ++i) {
	if (m_snapValues[i] == t) {
	    m_snapGridCombo->setCurrentItem(i);
	    break;
	}
    }

    for (unsigned int i = 0; i < m_staffs.size(); ++i)
        m_staffs[i]->sizeStaff(m_hlayout);

    m_segments[0]->setSnapGridSize(t);

    updateView();
}

void
MatrixView::slotQuantizeSelection(int q)
{
    MATRIX_DEBUG << "MatrixView::slotQuantizeSelection\n";

    using Rosegarden::Quantizer;
    Rosegarden::timeT unit =
	((unsigned int)q < m_quantizations.size() ? m_quantizations[q] : 0);

    Rosegarden::Quantizer *quant =
	new Rosegarden::BasicQuantizer
	(unit ? unit :
	 Rosegarden::Note(Rosegarden::Note::Shortest).getDuration(), false);

    if (unit)
    {
        KTmpStatusMsg msg(i18n("Quantizing..."), this);
	if (m_currentEventSelection &&
	    m_currentEventSelection->getAddedEvents()) {
	    addCommandToHistory(new EventQuantizeCommand
				(*m_currentEventSelection, quant));
	} else {
	    Segment &s = m_staffs[0]->getSegment();
	    addCommandToHistory(new EventQuantizeCommand
				(s, s.getStartTime(), s.getEndMarkerTime(),
				 quant));
	}
    }
    else
    {
        KTmpStatusMsg msg(i18n("Unquantizing..."), this);
	if (m_currentEventSelection &&
	    m_currentEventSelection->getAddedEvents()) {
	    addCommandToHistory(new EventUnquantizeCommand
				(*m_currentEventSelection, quant));
	} else {
	    Segment &s = m_staffs[0]->getSegment();
	    addCommandToHistory(new EventUnquantizeCommand
				(s, s.getStartTime(), s.getEndMarkerTime(),
				 quant));
	}
    }
}

void
MatrixView::initActionsToolbar()
{
    MATRIX_DEBUG << "MatrixView::initActionsToolbar" << endl;

    KToolBar *actionsToolbar = toolBar("Actions Toolbar");

    if (!actionsToolbar)
    {
        MATRIX_DEBUG << "MatrixView::initActionsToolbar - "
                     << "tool bar not found" << endl;
        return;
    }

    // The SnapGrid combo
    //
    QLabel *sLabel = new QLabel(i18n(" Grid: "), actionsToolbar, "kde toolbar widget");
    sLabel->setIndent(10);

    using Rosegarden::Note;
    QPixmap noMap = NotePixmapFactory::toQPixmap(NotePixmapFactory::makeToolbarPixmap("menu-no-note"));

    m_snapGridCombo = new KComboBox(actionsToolbar);

    Rosegarden::timeT crotchetDuration = Note(Note::Crotchet).getDuration();
    m_snapValues.push_back(Rosegarden::SnapGrid::NoSnap);
    m_snapValues.push_back(Rosegarden::SnapGrid::SnapToUnit);
    m_snapValues.push_back(crotchetDuration / 16);
    m_snapValues.push_back(crotchetDuration / 12);
    m_snapValues.push_back(crotchetDuration / 8);
    m_snapValues.push_back(crotchetDuration / 6);
    m_snapValues.push_back(crotchetDuration / 4);
    m_snapValues.push_back(crotchetDuration / 3);
    m_snapValues.push_back(crotchetDuration / 2);
    m_snapValues.push_back(crotchetDuration * 3 / 2);
    m_snapValues.push_back(crotchetDuration);
    m_snapValues.push_back(crotchetDuration * 2);
    m_snapValues.push_back(Rosegarden::SnapGrid::SnapToBeat);
    m_snapValues.push_back(Rosegarden::SnapGrid::SnapToBar);

    for (unsigned int i = 0; i < m_snapValues.size(); i++)
    {
	if (m_snapValues[i] == Rosegarden::SnapGrid::NoSnap) {
	    m_snapGridCombo->insertItem(i18n("None"));
	} else if (m_snapValues[i] == Rosegarden::SnapGrid::SnapToUnit) {
	    m_snapGridCombo->insertItem(i18n("Unit"));
	} else if (m_snapValues[i] == Rosegarden::SnapGrid::SnapToBeat) {
	    m_snapGridCombo->insertItem(i18n("Beat"));
	} else if (m_snapValues[i] == Rosegarden::SnapGrid::SnapToBar) {
	    m_snapGridCombo->insertItem(i18n("Bar"));
	} else {

	    timeT err = 0;
	    QString label = NotationStrings::makeNoteMenuLabel(m_snapValues[i], true, err);
	    QPixmap pixmap = NotePixmapFactory::toQPixmap(NotePixmapFactory::makeNoteMenuPixmap(m_snapValues[i], err));
	    m_snapGridCombo->insertItem((err ? noMap : pixmap), label);
	}

	if (m_snapValues[i] == m_snapGrid->getSnapSetting()) {
	    m_snapGridCombo->setCurrentItem(m_snapGridCombo->count() - 1);
	}
    }

    connect(m_snapGridCombo, SIGNAL(activated(int)),
            this, SLOT(slotSetSnapFromIndex(int)));

    // Quantize combo
    //
    QLabel *qLabel = new QLabel(i18n(" Quantize: "), actionsToolbar, "kde toolbar widget");
    qLabel->setIndent(10);

    m_quantizeCombo = new KComboBox(actionsToolbar);

    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {

	Rosegarden::timeT time = m_quantizations[i];
	Rosegarden::timeT error = 0;
	QString label = NotationStrings::makeNoteMenuLabel(time, true, error);
	QPixmap pmap = NotePixmapFactory::toQPixmap(NotePixmapFactory::makeNoteMenuPixmap(time, error));
	m_quantizeCombo->insertItem(error ? noMap : pmap, label);
    }

    m_quantizeCombo->insertItem(noMap, i18n("Off"));

    connect(m_quantizeCombo, SIGNAL(activated(int)),
            this, SLOT(slotQuantizeSelection(int)));
}

void
MatrixView::initZoomToolbar()
{
    MATRIX_DEBUG << "MatrixView::initZoomToolbar" << endl;

    KToolBar *zoomToolbar = toolBar("Zoom Toolbar");

    if (!zoomToolbar)
    {
        MATRIX_DEBUG << "MatrixView::initZoomToolbar - "
                     << "tool bar not found" << endl;
        return;
    }

    std::vector<double> zoomSizes; // in units-per-pixel

    //double defaultBarWidth44 = 100.0;
    //double duration44 = Rosegarden::TimeSignature(4,4).getBarDuration();

    static double factors[] = { 0.025, 0.05, 0.1, 0.2, 0.5,
                                1.0, 1.5, 2.5, 5.0, 10.0, 20.0 };
    // Zoom labels
    //
    for (unsigned int i = 0; i < sizeof(factors)/sizeof(factors[0]); ++i)
    {
//         zoomSizes.push_back(duration44 / (defaultBarWidth44 * factors[i]));

//         zoomSizes.push_back(factors[i] / 2); // GROSS HACK - see in matrixstaff.h - BREAKS MATRIX VIEW, see bug 1000595
        zoomSizes.push_back(factors[i]);
    }

    m_hZoomSlider = new ZoomSlider<double>
        (zoomSizes, -1, QSlider::Horizontal, zoomToolbar, "kde toolbar widget");
    m_hZoomSlider->setTracking(true);
    m_hZoomSlider->setFocusPolicy(QWidget::NoFocus);

    m_zoomLabel = new QLabel(zoomToolbar, "kde toolbar widget");
    m_zoomLabel->setIndent(10);
    m_zoomLabel->setFixedWidth(80);

    connect(m_hZoomSlider,
            SIGNAL(valueChanged(int)),
            SLOT(slotChangeHorizontalZoom(int)));

}


void
MatrixView::slotChangeHorizontalZoom(int)
{
    double zoomValue = m_hZoomSlider->getCurrentSize();
    
//     m_zoomLabel->setText(i18n("%1%").arg(zoomValue*100.0 * 2)); // GROSS HACK - see in matrixstaff.h - BREAKS MATRIX VIEW, see bug 1000595
    m_zoomLabel->setText(i18n("%1%").arg(zoomValue*100.0));
    
    MATRIX_DEBUG << "MatrixView::slotChangeHorizontalZoom() : zoom factor = "
                 << zoomValue << endl;
    
    // Set zoom matrix
    //
    QWMatrix zoomMatrix;
    zoomMatrix.scale(zoomValue, 1.0);
    m_canvasView->setWorldMatrix(zoomMatrix);
    
    // make control rulers zoom too
    //
    setControlRulersZoom(zoomMatrix);
    
    if (m_topBarButtons) m_topBarButtons->setHScaleFactor(zoomValue);
    if (m_bottomBarButtons) m_bottomBarButtons->setHScaleFactor(zoomValue);
    
    for (unsigned int i = 0; i < m_propertyViewRulers.size(); ++i)
    {
	m_propertyViewRulers[i].first->setHScaleFactor(zoomValue);
	m_propertyViewRulers[i].first->repaint();
    }

    if (m_topBarButtons) m_topBarButtons->update();
    if (m_bottomBarButtons) m_bottomBarButtons->update();

    // If you do adjust the viewsize then please remember to 
    // either re-center() or remember old scrollbar position
    // and restore.
    //

    int newWidth = computePostLayoutWidth();

//     int newWidth = int(getXbyWorldMatrix(getCanvasView()->canvas()->width()));

    // We DO NOT resize the canvas(), only the area it's displaying on
    //
    getCanvasView()->resizeContents(newWidth, getViewSize().height());

    // This forces a refresh of the h. scrollbar, even if the canvas width
    // hasn't changed
    //
    getCanvasView()->polish();
}

void
MatrixView::slotZoomIn()
{
    m_hZoomSlider->increment();
}

void
MatrixView::slotZoomOut()
{
    m_hZoomSlider->decrement();
}

/// Scrolls the view such that the given time is centered
void
MatrixView::scrollToTime(timeT t) {
    double layoutCoord = m_hlayout.getXForTime(t);
    getCanvasView()->slotScrollHoriz(int(layoutCoord));
}

unsigned int
MatrixView::addPropertyViewRuler(const Rosegarden::PropertyName &property)
{
    // Try and find this controller if it exists
    //
    for (unsigned int i = 0; i != m_propertyViewRulers.size(); i++)
    {
        if (m_propertyViewRulers[i].first->getPropertyName() == property)
            return i;
    }

    int height = 20;

    PropertyViewRuler *newRuler = new PropertyViewRuler(&m_hlayout,
                                                        m_segments[0],
                                                        property,
                                                        xorigin,
                                                        height,
                                                        getCentralWidget());

    addRuler(newRuler);

    PropertyBox *newControl = new PropertyBox(strtoqstr(property), 
                                              m_parameterBox->width() + m_pitchRuler->width(),
                                              height,
                                              getCentralWidget());

    addPropertyBox(newControl);

    m_propertyViewRulers.push_back(
            std::pair<PropertyViewRuler*, PropertyBox*>(newRuler, newControl));
                             
    return m_propertyViewRulers.size() - 1;
}


bool
MatrixView::removePropertyViewRuler(unsigned int number)
{
    if (number > m_propertyViewRulers.size() - 1)
        return false;

    std::vector<std::pair<PropertyViewRuler*, PropertyBox*> >::iterator it 
        = m_propertyViewRulers.begin();
    while(number--) it++;

    delete it->first;
    delete it->second;
    m_propertyViewRulers.erase(it);

    return true;
}

Rosegarden::RulerScale*
MatrixView::getHLayout()
{
    return &m_hlayout;
}

Rosegarden::Staff*
MatrixView::getCurrentStaff()
{
    return getStaff(0);
}

Rosegarden::Segment *
MatrixView::getCurrentSegment()
{
    MatrixStaff *staff = getStaff(0);
    return (staff ? &staff->getSegment() : 0);
}

timeT
MatrixView::getInsertionTime()
{
    MatrixStaff *staff = m_staffs[0];
    double ix = staff->getLayoutXOfInsertCursor();
    return m_hlayout.getTimeForX(ix);
}

void
MatrixView::slotStepBackward()
{
    Rosegarden::timeT time(getInsertionTime());
    slotSetInsertCursorPosition(Rosegarden::SnapGrid(&m_hlayout).snapTime
				(time-1,
				 Rosegarden::SnapGrid::SnapLeft));
}

void
MatrixView::slotStepForward()
{
    Rosegarden::timeT time(getInsertionTime());
    slotSetInsertCursorPosition(Rosegarden::SnapGrid(&m_hlayout).snapTime
				(time+1,
				 Rosegarden::SnapGrid::SnapRight));
}

void
MatrixView::slotJumpCursorToPlayback()
{
    slotSetInsertCursorPosition(getDocument()->getComposition().getPosition());
}

void
MatrixView::slotJumpPlaybackToCursor()
{
    emit jumpPlaybackTo(getInsertionTime());
}

void
MatrixView::slotToggleTracking()
{
    m_playTracking = !m_playTracking;
}

void
MatrixView::slotSelectAll()
{
    Rosegarden::Segment *segment = m_segments[0];
    Rosegarden::Segment::iterator it = segment->begin();
    EventSelection *selection = new EventSelection(*segment);

    for (; segment->isBeforeEndMarker(it); it++)
        if ((*it)->isa(Rosegarden::Note::EventType))
            selection->addEvent(*it);

    setCurrentSelection(selection, false);
}


void MatrixView::slotPreviewSelection()
{
    if (!m_currentEventSelection) return;

    getDocument()->slotSetLoop(m_currentEventSelection->getStartTime(),
			    m_currentEventSelection->getEndTime());
}


void MatrixView::slotClearLoop()
{
    getDocument()->slotSetLoop(0, 0);
}


void MatrixView::slotClearSelection()
{
    // Actually we don't clear the selection immediately: if we're
    // using some tool other than the select tool, then the first
    // press switches us back to the select tool.

    MatrixSelector *selector = dynamic_cast<MatrixSelector *>(m_tool);
    
    if (!selector) {
	slotSelectSelected();
    } else {
	setCurrentSelection(0);
    }
}

void MatrixView::slotFilterSelection()
{
    RG_DEBUG << "MatrixView::slotFilterSelection" << endl;

    Segment *segment = getCurrentSegment();
    EventSelection *existingSelection = m_currentEventSelection;
    if (!segment || !existingSelection) return;

    EventFilterDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        RG_DEBUG << "slotFilterSelection- accepted" << endl;

	bool haveEvent = false;

        EventSelection *newSelection = new EventSelection(*segment);
        EventSelection::eventcontainer &ec =
            existingSelection->getSegmentEvents();
        for (EventSelection::eventcontainer::iterator i =
             ec.begin(); i != ec.end(); ++i) {
            if (dialog.keepEvent(*i)) {
		haveEvent = true;
		newSelection->addEvent(*i);
	    }
        }

	if (haveEvent) setCurrentSelection(newSelection);
	else setCurrentSelection(0);
    }
}


void
MatrixView::readjustCanvasSize()
{
    int maxHeight = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        MatrixStaff &staff = *m_staffs[i];

        staff.sizeStaff(m_hlayout);

//         if (staff.getTotalWidth() + staff.getX() > maxWidth) {
//             maxWidth = staff.getTotalWidth() + staff.getX() + 1;
//         }

        if (staff.getTotalHeight() + staff.getY() > maxHeight) {
	    if (isDrumMode()) {
		maxHeight = staff.getTotalHeight() + staff.getY() + 5;
	    } else {
		maxHeight = staff.getTotalHeight() + staff.getY() + 1;
	    }
        }

    }

    int newWidth = computePostLayoutWidth();

    // now get the EditView to do the biz
    readjustViewSize(QSize(newWidth, maxHeight), true);

    repaintRulers();
}



void MatrixView::slotVelocityUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Raising velocities..."), this);

    addCommandToHistory
	(new ChangeVelocityCommand(10, *m_currentEventSelection));
}

void MatrixView::slotVelocityDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Lowering velocities..."), this);

    addCommandToHistory
	(new ChangeVelocityCommand(-10, *m_currentEventSelection));
}


// Set the velocities of the current selection (if we have one)
//
void
MatrixView::slotSetVelocities()
{
    if (!m_currentEventSelection) return;

    int avVely = 0;
    int count = 0;

    for (EventSelection::eventcontainer::iterator i =
         m_currentEventSelection->getSegmentEvents().begin();
         i != m_currentEventSelection->getSegmentEvents().end(); ++i)
    {

        if ((*i)->has(Rosegarden::BaseProperties::VELOCITY))
        {
            avVely += (*i)->
                get<Rosegarden::Int>(Rosegarden::BaseProperties::VELOCITY);
            count++;
        }
    }

    if (count > 0) avVely = int(double(avVely)/double(count));
    else avVely = 100;
	
    EventParameterDialog dialog(this,
				i18n("Set Event Velocities"),
				Rosegarden::BaseProperties::VELOCITY,
				avVely);

    if (dialog.exec() == QDialog::Accepted) {
	KTmpStatusMsg msg(i18n("Setting Velocities..."), this);
	addCommandToHistory(new SelectionPropertyCommand
			    (m_currentEventSelection,
                             Rosegarden::BaseProperties::VELOCITY,
                             dialog.getPattern(),
                             dialog.getValue1(),
                             dialog.getValue2()));
    }
}

void
MatrixView::slotTriggerSegment()
{
    if (!m_currentEventSelection) return;
    
    TriggerSegmentDialog dialog(this, &getDocument()->getComposition());
    if (dialog.exec() != QDialog::Accepted) return;

    addCommandToHistory(new SetTriggerCommand(*m_currentEventSelection,
					      dialog.getId(),
					      true,
					      dialog.getRetune(),
					      dialog.getTimeAdjust(),
					      Rosegarden::Marks::NoMark,
					      i18n("Trigger Segment")));
}

void
MatrixView::slotRemoveTriggers()
{
    if (!m_currentEventSelection) return;

    addCommandToHistory(new ClearTriggersCommand(*m_currentEventSelection,
						 i18n("Remove Triggers")));
}


void
MatrixView::slotToggleChordsRuler()
{
    toggleWidget(m_chordNameRuler, "show_chords_ruler");
}

void
MatrixView::slotToggleTempoRuler()
{
    toggleWidget(m_tempoRuler, "show_tempo_ruler");
}

void
MatrixView::paintEvent(QPaintEvent* e)
{
    //!!! There's a lot of code shared between matrix and notation for
    // dealing with step recording (the insertable note event stuff).
    // It should probably be factored out into a base class, but I'm
    // not sure I wouldn't rather wait until the functionality is all
    // sorted in both matrix and notation so we can be sure how much
    // of it is actually common.

    EditView::paintEvent(e);
    
    // now deal with any backlog of insertable notes that appeared
    // during paint (because it's not safe to modify a segment from
    // within a sub-event-loop in a processEvents call from a paint)
    if (!m_pendingInsertableNotes.empty()) {
	std::vector<std::pair<int, int> > notes = m_pendingInsertableNotes;
	m_pendingInsertableNotes.clear();
	for (unsigned int i = 0; i < notes.size(); ++i) {
	    slotInsertableNoteEventReceived(notes[i].first, notes[i].second, true);
	}
    }
}

void
MatrixView::updateViewCaption()
{
    // Set client label
    //
    QString view = i18n("Matrix");
    if (isDrumMode()) view = i18n("Percussion");

    if (m_segments.size() == 1) {

	Rosegarden::TrackId trackId = m_segments[0]->getTrack();
	Rosegarden::Track *track =
	    m_segments[0]->getComposition()->getTrackById(trackId);

	int trackPosition = -1;
	if (track) trackPosition = track->getPosition();

        setCaption(i18n("%1 - Segment Track #%2 - %3")
                   .arg(getDocument()->getTitle())
                   .arg(trackPosition + 1)
		   .arg(view));

    } else if (m_segments.size() == getDocument()->getComposition().getNbSegments()) {

        setCaption(i18n("%1 - All Segments - %2")
                   .arg(getDocument()->getTitle())
		   .arg(view));

    } else {

        setCaption(i18n("%1 - 1 Segment - %2",
			"%1 - %n Segments - %2",
			m_segments.size())
                   .arg(getDocument()->getTitle())
		   .arg(view));
    }
}

int MatrixView::computePostLayoutWidth()
{
    Segment *segment = m_segments[0];
    Rosegarden::Composition *composition = segment->getComposition();
    int   endX = int(m_hlayout.getXForTime
		     (composition->getBarEndForTime
		      (segment->getEndMarkerTime())));
    int startX = int(m_hlayout.getXForTime
		     (composition->getBarStartForTime
		      (segment->getStartTime())));

    int newWidth = int(getXbyWorldMatrix(endX - startX));

    MATRIX_DEBUG << "MatrixView::readjustCanvasSize() : startX = " 
                 << startX
                 << " endX = " << endX
                 << " newWidth = " << newWidth
                 << " endmarkertime : " << segment->getEndMarkerTime()
                 << " barEnd for time : " << composition->getBarEndForTime(segment->getEndMarkerTime())
                 << endl;

    newWidth += 12;
    if (isDrumMode()) newWidth += 12;

    return newWidth;
}

bool MatrixView::getMinMaxPitches(int& minPitch, int& maxPitch)
{
    minPitch = MatrixVLayout::maxMIDIPitch + 1;
    maxPitch = MatrixVLayout::minMIDIPitch - 1;

    std::vector<MatrixStaff*>::iterator sit;
    for (sit=m_staffs.begin(); sit!=m_staffs.end(); ++sit) {

        MatrixElementList *mel = (*sit)->getViewElementList();
        MatrixElementList::iterator eit;
        for (eit=mel->begin(); eit!=mel->end(); ++eit){

            NotationElement *el = static_cast<NotationElement*>(*eit);
            if (el->isNote()){
                Rosegarden::Event* ev = el->event();
                int pitch = ev->get<Rosegarden::Int>
                                        (Rosegarden::BaseProperties::PITCH);
                if (minPitch > pitch) minPitch = pitch;
                if (maxPitch < pitch) maxPitch = pitch;
            }
        }
    }

    return maxPitch >= minPitch;
}

void MatrixView::extendKeyMapping()
{
    int minStaffPitch, maxStaffPitch;
    if (getMinMaxPitches(minStaffPitch, maxStaffPitch)) {
        int minKMPitch = m_localMapping->getPitchForOffset(0);
        int maxKMPitch = m_localMapping->getPitchForOffset(0)
                                     + m_localMapping->getPitchExtent() - 1;
        if (minStaffPitch < minKMPitch)
                m_localMapping->getMap()[minStaffPitch] = std::string("");
        if (maxStaffPitch > maxKMPitch)
                m_localMapping->getMap()[maxStaffPitch] = std::string("");
    }
}

// Ignore velocity for the moment -- we need the option to use or ignore it
void
MatrixView::slotInsertableNoteEventReceived(int pitch, int velocity, bool noteOn)
{
    // hjj:
    // The default insertion mode is implemented equivalently in
    // notationviewslots.cpp:
    //  - proceed if notes do not overlap
    //  - make the chord if notes do overlap, and do not proceed

    static int numberOfNotesOn = 0;
    static time_t lastInsertionTime = 0;
    if (!noteOn) {
	numberOfNotesOn--;
	return;
    }

    KToggleAction *action = dynamic_cast<KToggleAction *>
	(actionCollection()->action("toggle_step_by_step"));
    if (!action) {
	MATRIX_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
	return;
    }
    if (!action->isChecked()) return;

    if (m_inPaintEvent) {
	m_pendingInsertableNotes.push_back(std::pair<int, int>(pitch, velocity));
	return;
    }

    Segment &segment = *getCurrentSegment();

    // If the segment is transposed, we want to take that into
    // account.  But the note has already been played back to the user
    // at its untransposed pitch, because that's done by the MIDI THRU
    // code in the sequencer which has no way to know whether a note
    // was intended for step recording.  So rather than adjust the
    // pitch for playback according to the transpose setting, we have
    // to adjust the stored pitch in the opposite direction.

    pitch -= segment.getTranspose();

    KTmpStatusMsg msg(i18n("Inserting note"), this);

    MATRIX_DEBUG << "Inserting note at pitch " << pitch << endl;

    Rosegarden::Event modelEvent(Rosegarden::Note::EventType, 0, 1);
    modelEvent.set<Rosegarden::Int>(Rosegarden::BaseProperties::PITCH, pitch);
    static Rosegarden::timeT insertionTime(getInsertionTime());
    if (insertionTime >= segment.getEndMarkerTime()) {
	MATRIX_DEBUG << "WARNING: off end of segment" << endl;
	return;
    }
    time_t now;
    time (&now);
    double elapsed = difftime(now,lastInsertionTime);
    time (&lastInsertionTime);

    if (numberOfNotesOn <= 0 || elapsed > 10.0 ) {
      numberOfNotesOn = 0;
      insertionTime = getInsertionTime();
    }
    numberOfNotesOn++;
    Rosegarden::timeT endTime(insertionTime + m_snapGrid->getSnapTime(insertionTime));

    if (endTime <= insertionTime) {
	static bool showingError = false;
	if (showingError) return;
	showingError = true;
	KMessageBox::sorry(this, i18n("Can't insert note: No grid duration selected"));
	showingError = false;
        return;
    }

    MatrixInsertionCommand* command = 
	new MatrixInsertionCommand(segment, insertionTime, endTime, &modelEvent);

    addCommandToHistory(command);
    
    if (!isInChordMode()) {
	slotSetInsertCursorPosition(endTime);
    }
}

void
MatrixView::slotInsertableNoteOnReceived(int pitch, int velocity)
{
    MATRIX_DEBUG << "MatrixView::slotInsertableNoteOnReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, true);
}

void
MatrixView::slotInsertableNoteOffReceived(int pitch, int velocity)
{
    MATRIX_DEBUG << "MatrixView::slotInsertableNoteOffReceived: " << pitch << endl;
    slotInsertableNoteEventReceived(pitch, velocity, false);
}

void
MatrixView::slotToggleStepByStep()
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
	(actionCollection()->action("toggle_step_by_step"));
    if (!action) {
	MATRIX_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
	return;
    }
    if (action->isChecked()) { // after toggling, that is
	emit stepByStepTargetRequested(this);
    } else {
	emit stepByStepTargetRequested(0);
    }
}

void
MatrixView::slotUpdateInsertModeStatus()
{
    QString message;
    if (isInChordMode()) {
	message = i18n(" Chord");
    } else {
	message = "";
    }
    m_insertModeLabel->setText(message);
}


void
MatrixView::slotStepByStepTargetRequested(QObject *obj)
{
    KToggleAction *action = dynamic_cast<KToggleAction *>
	(actionCollection()->action("toggle_step_by_step"));
    if (!action) {
	MATRIX_DEBUG << "WARNING: No toggle_step_by_step action" << endl;
	return;
    }
    action->setChecked(obj == this);
}

void
MatrixView::slotInstrumentLevelsChanged(Rosegarden::InstrumentId id,
					const Rosegarden::LevelInfo &info)
{
    if (!m_parameterBox) return;

    Rosegarden::Composition &comp = getDocument()->getComposition();

    Rosegarden::Track *track =
        comp.getTrackById(m_staffs[0]->getSegment().getTrack());
    if (!track || track->getInstrument() != id) return;

    Rosegarden::Instrument *instr = getDocument()->getStudio().
        getInstrumentById(track->getInstrument());
    if (!instr || instr->getType() != Rosegarden::Instrument::SoftSynth) return;

    float dBleft = Rosegarden::AudioLevel::fader_to_dB
	(info.level, 127, Rosegarden::AudioLevel::LongFader);
    float dBright = Rosegarden::AudioLevel::fader_to_dB
	(info.levelRight, 127, Rosegarden::AudioLevel::LongFader);

    m_parameterBox->setAudioMeter(dBleft, dBright,
				  Rosegarden::AudioLevel::DB_FLOOR,
				  Rosegarden::AudioLevel::DB_FLOOR);
}

void
MatrixView::slotPercussionSetChanged(Rosegarden::Instrument * newInstr)
{
    // Must be called only when in drum mode
    assert(m_drumMode);

    int resolution = 8;
    if (newInstr && newInstr->getKeyMapping()) {
	resolution = 11;
    }

    const Rosegarden::MidiKeyMapping *mapping = 0;
    if (newInstr) {
	mapping = newInstr->getKeyMapping();
    }

    // Construct a local new keymapping :
    if (m_localMapping) delete m_localMapping;
    if (mapping) {
        m_localMapping = new Rosegarden::MidiKeyMapping(*mapping);
        extendKeyMapping();
    } else {
        m_localMapping = 0;
    }

    m_staffs[0]->setResolution(resolution);

    delete m_pitchRuler;

    QWidget *vport = m_pianoView->viewport();
    
    // Create a new pitchruler widget
    PitchRuler *pitchRuler;
    if (newInstr && newInstr->getKeyMapping() &&
	!newInstr->getKeyMapping()->getMap().empty()) {
	pitchRuler = new PercussionPitchRuler(vport,
						m_localMapping,
						resolution); // line spacing
    } else {
	pitchRuler = new PianoKeyboard(vport);
    }


    QObject::connect
        (pitchRuler, SIGNAL(hoveredOverKeyChanged(unsigned int)),
         this,         SLOT  (slotHoveredOverKeyChanged(unsigned int)));

    QObject::connect
        (pitchRuler, SIGNAL(keyPressed(unsigned int, bool)),
         this,         SLOT  (slotKeyPressed(unsigned int, bool)));

    QObject::connect
        (pitchRuler, SIGNAL(keySelected(unsigned int, bool)),
         this,         SLOT  (slotKeySelected(unsigned int, bool)));

    QObject::connect
        (pitchRuler, SIGNAL(keyReleased(unsigned int, bool)),
         this,         SLOT  (slotKeyReleased(unsigned int, bool)));

    // Replace the old pitchruler widget
    m_pitchRuler = pitchRuler;
    m_pianoView->addChild(m_pitchRuler);
    m_pitchRuler->show();
    m_pianoView->setFixedWidth(pitchRuler->sizeHint().width());

    // Update matrix canvas
    readjustCanvasSize();
    bool layoutApplied = applyLayout();
    if (!layoutApplied)
            KMessageBox::sorry(0, i18n("Couldn't apply piano roll layout"));
    else {
        MATRIX_DEBUG << "MatrixView : rendering elements\n";
        m_staffs[0]->positionAllElements();
        m_staffs[0]->getSegment().getRefreshStatus
            (m_segmentsRefreshStatusIds[0]).setNeedsRefresh(false);
        update();
    }
}

void
MatrixView::slotCanvasBottomWidgetHeightChanged(int newHeight)
{
    m_pianoView->setBottomMargin(newHeight +
                                 m_canvasView->horizontalScrollBar()->height());
}


const char* const MatrixView::ConfigGroup = "Matrix Options";

#include "matrixview.moc"
