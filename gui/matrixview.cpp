// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <kapp.h>
#include <kconfig.h>
#include <kaction.h>
#include <kstddirs.h>
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

#include "matrixview.h"
#include "matrixstaff.h"
#include "matrixhlayout.h"
#include "matrixvlayout.h"
#include "matrixtool.h"
#include "matrixcommands.h"
#include "dialogs.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "ktmpstatusmsg.h"
#include "barbuttons.h"
#include "loopruler.h"
#include "temporuler.h"
#include "chordnameruler.h"
#include "pianokeyboard.h"
#include "editcommands.h"
#include "qdeferscrollview.h"
#include "matrixparameterbox.h"
#include "velocitycolour.h"
#include "widgets.h"
#include "zoomslider.h"
#include "notepixmapfactory.h"
#include "controlruler.h"
#include "studiocontrol.h"
#include "sequencemanager.h"

#include "rosedebug.h"

using Rosegarden::Segment;
using Rosegarden::EventSelection;
using Rosegarden::timeT;

static double xorigin = 0.0;

//----------------------------------------------------------------------

MatrixView::MatrixView(RosegardenGUIDoc *doc,
                       std::vector<Segment *> segments,
                       QWidget *parent)
    : EditView(doc, segments, 3, parent, "matrixview"),
      m_hlayout(&doc->getComposition()),
      m_vlayout(),
      m_snapGrid(new Rosegarden::SnapGrid(&m_hlayout)),
      m_lastEndMarkerTime(0),
      m_hoveredOverAbsoluteTime(0),
      m_hoveredOverNoteName(0),
      m_selectionCounter(0),
      m_previousEvPitch(0),
      m_canvasView(0),
      m_pianoView(0),
      m_lastNote(0),
      m_quantizations(
	  Rosegarden::StandardQuantization::getStandardQuantizations()),
      m_documentDestroyed(false)
{
    MATRIX_DEBUG << "MatrixView ctor\n";
    Rosegarden::Composition &comp = doc->getComposition();

    m_toolBox = new MatrixToolBox(this);

    initStatusBar();

    QCanvas *tCanvas = new QCanvas(this);

    MATRIX_DEBUG << "MatrixView : creating staff\n";

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new MatrixStaff(tCanvas, 
                                           segments[i],
                                           m_snapGrid,
                                           i,
                                           8, //!!! so random, so rare
                                           this));
	if (i == 0) m_staffs[i]->setCurrent(true);
    }

    MATRIX_DEBUG << "MatrixView : creating canvas view\n";

    m_pianoView = new QDeferScrollView(getCentralFrame());
    m_pianoKeyboard = new PianoKeyboard(m_pianoView->viewport());
    
    m_pianoView->setVScrollBarMode(QScrollView::AlwaysOff);
    m_pianoView->setHScrollBarMode(QScrollView::AlwaysOff);
    m_pianoView->addChild(m_pianoKeyboard);
    m_pianoView->setFixedWidth(m_pianoView->contentsWidth());

    m_grid->addWidget(m_pianoView, 2, 1);

    m_parameterBox = new MatrixParameterBox(getCentralFrame(), m_document);

    // Set the instrument we're using on this segment
    //
    Rosegarden::Track *track =
        comp.getTrackByIndex(m_staffs[0]->getSegment().getTrack());

    Rosegarden::Instrument *instr = m_document->getStudio().
        getInstrumentById(track->getInstrument());

    // Assign the instrument
    //
    m_parameterBox->useInstrument(instr);

    m_grid->addWidget(m_parameterBox, 2, 0);

    m_snapGrid->setSnapTime(Rosegarden::SnapGrid::SnapToBeat);

    m_canvasView = new MatrixCanvasView(*m_staffs[0],
                                        m_snapGrid,
					m_horizontalScrollBar,
                                        tCanvas,
                                        getCentralFrame());
    setCanvasView(m_canvasView);

    // do this after we have a canvas
    setupActions();

    // tool bars
    initActionsToolbar();
    initZoomToolbar();

    // Connect vertical scrollbars between matrix and piano
    //
    connect(m_canvasView->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotVerticalScrollPianoKeyboard(int)));

    connect(m_canvasView->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotVerticalScrollPianoKeyboard(int)));

    connect(m_pianoView, SIGNAL(gotWheelEvent(QWheelEvent*)),
            m_canvasView, SLOT(slotExternalWheelEvent(QWheelEvent*)));

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
        (getCanvasView(), SIGNAL(hoveredOverNoteChanged(const QString&)),
         this, SLOT(slotHoveredOverNoteChanged(const QString&)));

    QObject::connect
        (m_pianoKeyboard, SIGNAL(hoveredOverKeyChanged(unsigned int)),
         this,            SLOT  (slotHoveredOverKeyChanged(unsigned int)));

    QObject::connect
        (m_pianoKeyboard, SIGNAL(keyPressed(unsigned int, bool)),
         this,            SLOT  (slotKeyPressed(unsigned int, bool)));

    QObject::connect
        (m_pianoKeyboard, SIGNAL(keySelected(unsigned int, bool)),
         this,            SLOT  (slotKeySelected(unsigned int, bool)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverAbsoluteTimeChanged(unsigned int)),
         this,            SLOT  (slotHoveredOverAbsoluteTimeChanged(unsigned int)));

    QObject::connect
	(doc, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
	 this, SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    QObject::connect
	(doc, SIGNAL(destroyed()), this, SLOT(slotDocumentDestroyed()));

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

    BarButtons *topBarButtons = new BarButtons(&m_hlayout, int(xorigin), 25,
                                               false, getCentralFrame());
    setTopBarButtons(topBarButtons);

    QObject::connect
	(topBarButtons->getLoopRuler(),
	 SIGNAL(setPointerPosition(Rosegarden::timeT)),
	 this, SLOT(slotSetInsertCursorPosition(Rosegarden::timeT)));

    topBarButtons->getLoopRuler()->setBackgroundColor
	(RosegardenGUIColours::InsertCursorRuler);

    BarButtons *bottomBarButtons = new BarButtons(&m_hlayout, 0, 25,
                                                  true, getCentralFrame());
    bottomBarButtons->connectRulerToDocPointer(doc);
    setBottomBarButtons(bottomBarButtons);

    // Force height for the moment
    //
    m_pianoKeyboard->setFixedHeight(canvas()->height());


    // Set client label
    //
    if (segments.size() == 1) {

        setCaption(QString("%1 - Segment Track #%2")
                   .arg(doc->getTitle())
                   .arg(segments[0]->getTrack()));

    } else if (segments.size() == comp.getNbSegments()) {

        setCaption(QString("%1 - All Segments")
                   .arg(doc->getTitle()));

    } else {

        setCaption(QString("%1 - %2-Segment Partial View")
                   .arg(doc->getTitle())
                   .arg(segments.size()));
    }

    // Add a velocity ruler
    //
    addControlRuler(Rosegarden::BaseProperties::VELOCITY);

    m_chordNameRuler = new ChordNameRuler
	(&m_hlayout, &doc->getComposition(), 0, 20, getCentralFrame());
    addRuler(m_chordNameRuler);

    m_tempoRuler = new TempoRuler
	(&m_hlayout, &doc->getComposition(), 0, 20, false, getCentralFrame());
    addRuler(m_tempoRuler);

    // Scroll view to centre middle-C and warp to pointer position
    //
    m_canvasView->scrollBy(0, m_staffs[0]->getCanvasYForHeight(60) / 2);

    slotSetPointerPosition(comp.getPosition());

    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    slotTestClipboard();

    setCurrentSelection(0, false);

    // Change this when the matrix view will have its own page
    // in the config dialog.
    setConfigDialogPageIndex(2);

    // default zoom
    slotChangeHorizontalZoom(-1);

    // All toolbars should be created before this is called
    setAutoSaveSettings("MatrixView", true);

    readOptions();
}

MatrixView::~MatrixView()
{
    // Give the sequencer something to suck on while we close
    //
    if (!m_documentDestroyed) {
	m_document->getSequenceManager()->
	    setTemporarySequencerSliceSize(Rosegarden::RealTime(2, 0));
    }

    slotSaveOptions();

    delete m_currentEventSelection;
    m_currentEventSelection = 0;

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
}

void MatrixView::slotSaveOptions()
{        
    m_config->setGroup("Matrix Options");

    m_config->writeEntry("Show Chord Name Ruler", getToggleAction("show_chords_ruler")->isChecked());
    m_config->writeEntry("Show Tempo Ruler",      getToggleAction("show_tempo_ruler")->isChecked());

    m_config->sync();
}

void MatrixView::readOptions()
{
    EditView::readOptions();
    m_config->setGroup("Matrix Options");

    bool opt = false;

    opt = m_config->readBoolEntry("Show Chord Name Ruler", true);
    getToggleAction("show_chords_ruler")->setChecked(opt);
    slotToggleChordsRuler();
    
    opt = m_config->readBoolEntry("Show Tempo Ruler", false);
    getToggleAction("show_tempo_ruler")->setChecked(opt);
    slotToggleTempoRuler();
}

void MatrixView::setupActions()
{   
    EditView::setupActions("matrix.rc");

    // File menu
    KStdAction::close   (this, SLOT(slotCloseWindow()),    actionCollection());

    // Edit menu
    KStdAction::cut     (this, SLOT(slotEditCut()),        actionCollection());
    KStdAction::copy    (this, SLOT(slotEditCopy()),       actionCollection());
    KStdAction::paste   (this, SLOT(slotEditPaste()),      actionCollection());

    //
    // Edition tools (eraser, selector...)
    //
    KRadioAction* toolAction = 0;

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIconSet icon(QPixmap(pixmapDir + "/toolbar/select.xpm"));

    toolAction = new KRadioAction(i18n("&Select"), icon, 0,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("&Draw"), "pencil", 0,
                                  this, SLOT(slotPaintSelected()),
                                  actionCollection(), "draw");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("&Erase"), "eraser", 0,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("&Move"), "move", 0,
                                  this, SLOT(slotMoveSelected()),
                                  actionCollection(), "move");
    toolAction->setExclusiveGroup("tools");

    icon = QIconSet(QCanvasPixmap(pixmapDir + "/toolbar/resize.xpm"));
    toolAction = new KRadioAction(i18n("Resize"), icon, 0,
                                  this, SLOT(slotResizeSelected()),
                                  actionCollection(), "resize");
    toolAction->setExclusiveGroup("tools");

    new KAction(i18n(EventQuantizeCommand::getGlobalName()), 0, this,
                SLOT(slotTransformsQuantize()), actionCollection(),
                "quantize");

    new KAction(i18n(TransposeCommand::getGlobalName(1)), 0,
		Key_Up, this,
                SLOT(slotTransposeUp()), actionCollection(),
                "transpose_up");

    new KAction(i18n(TransposeCommand::getGlobalName(12)), 0,
		Key_Up + CTRL, this,
                SLOT(slotTransposeUpOctave()), actionCollection(),
                "transpose_up_octave");

    new KAction(i18n(TransposeCommand::getGlobalName(-1)), 0,
		Key_Down, this,
                SLOT(slotTransposeDown()), actionCollection(),
                "transpose_down");

    new KAction(i18n(TransposeCommand::getGlobalName(-12)), 0,
		Key_Down + CTRL, this,
                SLOT(slotTransposeDownOctave()), actionCollection(),
                "transpose_down_octave");

    new KAction(i18n(TransposeCommand::getGlobalName(0)), 0, this,
                SLOT(slotTranspose()), actionCollection(),
                "general_transpose");

    new KAction(i18n(ChangeVelocityCommand::getGlobalName(10)), 0,
		Key_Up + SHIFT, this,
                SLOT(slotVelocityUp()), actionCollection(),
                "velocity_up");

    new KAction(i18n(ChangeVelocityCommand::getGlobalName(-10)), 0,
		Key_Down + SHIFT, this,
                SLOT(slotVelocityDown()), actionCollection(),
                "velocity_down");

    new KAction(i18n("Set Event &Velocities..."), 0, this,
                SLOT(slotSetVelocities()), actionCollection(),
                "set_velocities");

    new KAction(i18n("Select &All"), 0, this,
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

    new KAction(i18n("Cursor to St&art"), 0, Key_A + CTRL, this,
		SLOT(slotJumpToStart()), actionCollection(),
		"cursor_start");

    new KAction(i18n("Cursor to &End"), 0, Key_E + CTRL, this,
		SLOT(slotJumpToEnd()), actionCollection(),
		"cursor_end");

    NotePixmapFactory npf;
    icon = QIconSet(NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap
                                                 ("transport-cursor-to-pointer")));
    new KAction(i18n("Cursor to &Playback Pointer"), icon, 0, this,
		SLOT(slotJumpCursorToPlayback()), actionCollection(),
		"cursor_to_playback_pointer");

    icon = QIconSet(NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap
                                                 ("transport-play")));
    new KAction(i18n("&Play"), icon, Key_Enter, this,
		SIGNAL(play()), actionCollection(), "play");

    icon = QIconSet(NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap
                                                 ("transport-stop")));
    new KAction(i18n("&Stop"), icon, Key_Insert, this,
		SIGNAL(stop()), actionCollection(), "stop");

    icon = QIconSet(NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap
                                                 ("transport-rewind")));
    new KAction(i18n("Re&wind"), icon, Key_End, this,
		SIGNAL(rewindPlayback()), actionCollection(),
		"playback_pointer_back_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap
                                                 ("transport-ffwd")));
    new KAction(i18n("&Fast Forward"), icon, Key_PageDown, this,
		SIGNAL(fastForwardPlayback()), actionCollection(),
		"playback_pointer_forward_bar");

    icon = QIconSet(NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap
                                                 ("transport-rewind-end")));
    new KAction(i18n("Rewind to &Beginning"), icon, 0, this,
		SIGNAL(rewindPlaybackToBeginning()), actionCollection(),
		"playback_pointer_start");

    icon = QIconSet(NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap
                                                 ("transport-ffwd-end")));
    new KAction(i18n("Fast Forward to &End"), icon, 0, this,
		SIGNAL(fastForwardPlaybackToEnd()), actionCollection(),
		"playback_pointer_end");

    icon = QIconSet(NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap
                                                 ("transport-pointer-to-cursor")));
    new KAction(i18n("Playback Pointer to &Cursor"), icon, 0, this,
		SLOT(slotJumpPlaybackToCursor()), actionCollection(),
		"playback_pointer_to_cursor");

        icon = QIconSet(NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap
                                                 ("transport-solo")));
    new KToggleAction(i18n("&Solo"), icon, 0, this,
                SLOT(slotPlaySolo()), actionCollection(),
                "toggle_solo");

    new KAction(i18n("Set Loop to Selection"), Key_Semicolon + CTRL, this,
		SLOT(slotPreviewSelection()), actionCollection(),
		"preview_selection");

    new KAction(i18n("Clear L&oop"), Key_Colon + CTRL, this,
		SLOT(slotClearLoop()), actionCollection(),
		"clear_loop");

    new KAction(i18n("Clear Selection"), Key_Escape, this,
		SLOT(slotClearSelection()), actionCollection(),
		"clear_selection");

    //!!! should be using NotePixmapFactory::makeNoteMenuLabel for these
    new KAction(i18n("Snap to 1/64"), Key_0, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_64");
    new KAction(i18n("Snap to 1/32"), Key_3, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_32");
    new KAction(i18n("Snap to 1/16"), Key_6, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_16");
    new KAction(i18n("Snap to 1/8"), Key_8, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_8");
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

    //!!! need slotInsertNoteFromAction() after notationviewslots.cpp
    createInsertPitchActionMenu();

    //
    // Settings menu
    //
    new KToggleAction(i18n("Show Ch&ord Name Ruler"), 0, this,
                      SLOT(slotToggleChordsRuler()),
                      actionCollection(), "show_chords_ruler");

    new KToggleAction(i18n("Show &Tempo Ruler"), 0, this,
                      SLOT(slotToggleTempoRuler()),
                      actionCollection(), "show_tempo_ruler");


    createGUI(getRCFileName());

    if (getSegmentsOnlyRests())
        actionCollection()->action("draw")->activate();
    else
        actionCollection()->action("select")->activate();
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
    if (m_documentDestroyed) return;

    Rosegarden::Profiler profiler("MatrixView::refreshSegment", true);

    MATRIX_DEBUG << "MatrixView::refreshSegment(" << startTime
                         << ", " << endTime << ")\n";

    m_document->getSequenceManager()->
	setTemporarySequencerSliceSize(Rosegarden::RealTime(3, 0));

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
    canvas()->resize(s.width(), s.height());
    getCanvasView()->resizeContents(s.width(), s.height());
}

void MatrixView::repaintRulers()
{
    for (unsigned int i = 0; i != m_controlRulers.size(); i++)
        m_controlRulers[i].first->repaint();
}


void MatrixView::updateView()
{
    canvas()->update();
}

MatrixCanvasView* MatrixView::getCanvasView()
{
    return dynamic_cast<MatrixCanvasView *>(m_canvasView);
}


void MatrixView::setCurrentSelection(EventSelection* s, bool preview)
{
    if (!m_currentEventSelection && !s)	{
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
		    playNote(s->getSegment(), pitch);
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

            // the regions overlap: use their union and just do one reposition
            Segment &segment(s ? s->getSegment() : oldSelection->getSegment());
            getStaff(segment)->positionElements(std::min(startA, startB),
                                                std::max(endA, endB));

        } else {
            // do two repositions, one for each -- here we know neither is null
            getStaff(oldSelection->getSegment())->positionElements(startA,
                                                                   endA);
            getStaff(s->getSegment())->positionElements(startB, endB);
        }
    }

    delete oldSelection;
    int eventsSelected = 0;
    if (s) eventsSelected = s->getSegmentEvents().size();
    if (s) {
        m_selectionCounter->setText
	    (i18n("  %1 event%2 selected ").
	     arg(eventsSelected).arg(eventsSelected == 1 ? "" : "s"));
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
    updateView();
}


void MatrixView::updateQuantizeCombo()
{
    Rosegarden::StandardQuantization *quantization = 0;

    if (m_currentEventSelection) {
	quantization =
	    Rosegarden::StandardQuantization::getStandardQuantization
	    (m_currentEventSelection);
    } else {
	quantization =
	    Rosegarden::StandardQuantization::getStandardQuantization
	    (&(m_staffs[0]->getSegment()));
    }

    timeT quantizeUnit = 0;
    if (quantization) quantizeUnit = quantization->unit;
    
    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {
	if (quantizeUnit == m_quantizations[i].unit) {
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

    QuantizeDialog *dialog = new QuantizeDialog(this,
                                                Quantizer::GlobalSource,
						Quantizer::RawEventData);

    if (dialog->exec() == QDialog::Accepted) {
	KTmpStatusMsg msg(i18n("Quantizing..."), this);
	addCommandToHistory(new EventQuantizeCommand
			    (*m_currentEventSelection,
			     dialog->getQuantizer()));
    }
}

void MatrixView::slotTranspose()
{
    if (!m_currentEventSelection) return;

    bool ok = false;
    int semitones = QInputDialog::getInteger
	(i18n("Transpose"),
	 i18n("Enter the number of semitones to transpose by:"),
	 0, -127, 127, 1, &ok, this);
    if (!ok || semitones == 0) return;

    KTmpStatusMsg msg(i18n("Transposing..."), this);
    addCommandToHistory(new TransposeCommand
                        (semitones, *m_currentEventSelection));
}

void MatrixView::slotTransposeUp()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing up one semitone..."), this);

    addCommandToHistory(new TransposeCommand(1, *m_currentEventSelection));
}

void MatrixView::slotTransposeUpOctave()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing up one octave..."), this);

    addCommandToHistory(new TransposeCommand(12, *m_currentEventSelection));
}

void MatrixView::slotTransposeDown()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing down one semitone..."), this);

    addCommandToHistory(new TransposeCommand(-1, *m_currentEventSelection));
}

void MatrixView::slotTransposeDownOctave()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Transposing down one octave..."), this);

    addCommandToHistory(new TransposeCommand(-12, *m_currentEventSelection));
}

void MatrixView::slotMousePressed(Rosegarden::timeT time, int pitch,
                                  QMouseEvent* e, MatrixElement* el)
{
    MATRIX_DEBUG << "MatrixView::mousePressed at pitch "
                         << pitch << ", time " << time << endl;

    m_tool->handleMousePress(time, pitch, 0, e, el);

    // play a preview
    //playPreview(pitch);
}

void MatrixView::slotMouseMoved(Rosegarden::timeT time, int pitch, QMouseEvent* e)
{
    if (activeItem()) {
        activeItem()->handleMouseMove(e);
	updateView();
    }
    else 
    {
        int follow = m_tool->handleMouseMove(time, pitch, e);
        
        if (follow & EditTool::FollowHorizontal)
	    getCanvasView()->slotScrollHorizSmallSteps(e->pos().x());

        if (follow & EditTool::FollowVertical)
	    getCanvasView()->slotScrollVertSmallSteps(e->pos().y());
	    
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
    if (activeItem()) {
        activeItem()->handleMouseRelease(e);
        setActiveItem(0);
        updateView();
    }

    // send the real event time now (not adjusted for beginning of bar)
    m_tool->handleMouseRelease(time, pitch, e);
    m_previousEvPitch = 0;
}

void
MatrixView::slotHoveredOverNoteChanged(const QString &noteName)
{
    m_hoveredOverNoteName->setText(noteName);
}

void
MatrixView::slotHoveredOverKeyChanged(unsigned int y)
{
    MatrixStaff& staff = *(m_staffs[0]);

    int evPitch = staff.getHeightAtCanvasY(y);

    if (evPitch != m_previousEvPitch) {
        m_hoveredOverNoteName->setText(staff.getNoteNameForPitch(evPitch));
        m_previousEvPitch = evPitch;
    }
}

void
MatrixView::slotHoveredOverAbsoluteTimeChanged(unsigned int time)
{
    timeT t = time;
    Rosegarden::RealTime rt =
	m_document->getComposition().getElapsedRealTime(t);
    long ms = rt.usec / 1000;

    QString message;
    message.sprintf(" Time: %ld (%ld.%03lds)", t, rt.sec, ms);

    m_hoveredOverAbsoluteTime->setText(message);
}

void
MatrixView::slotSetPointerPosition(timeT time, bool scroll)
{
    Rosegarden::Composition &comp = m_document->getComposition();
    int barNo = comp.getBarNumber(time);

    if (barNo < m_hlayout.getFirstVisibleBarOnStaff(*m_staffs[0]) ||
        barNo > m_hlayout. getLastVisibleBarOnStaff(*m_staffs[0])) {
        m_staffs[0]->hidePointer();
    } else {
        m_staffs[0]->setPointerPosition(m_hlayout, time);
    }

    if (scroll)
        getCanvasView()->slotScrollHoriz(static_cast<int>(getXbyWorldMatrix(m_hlayout.getXForTime(time))));

    updateView();
}

void
MatrixView::slotSetInsertCursorPosition(timeT time, bool scroll)
{
    //!!! For now.  Probably unlike slotSetPointerPosition this one
    // should snap to the nearest event or grid line.

    m_staffs[0]->setInsertCursorPosition(m_hlayout, time);

    if (scroll) {
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
				       m_document->getClipboard()));
}

void MatrixView::slotEditCopy()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), this);

    addCommandToHistory(new CopyCommand(*m_currentEventSelection,
					m_document->getClipboard()));

    emit usedSelection();
}

void MatrixView::slotEditPaste()
{
    if (m_document->getClipboard()->isEmpty()) {
        slotStatusHelpMsg(i18n("Clipboard is empty"));
        return;
    }

    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), this);

    double ix = m_staffs[0]->getLayoutXOfInsertCursor();
    timeT time = m_hlayout.getTimeForX(ix);
    
    PasteEventsCommand *command = new PasteEventsCommand
	(m_staffs[0]->getSegment(), m_document->getClipboard(), time,
	 PasteEventsCommand::MatrixOverlay);

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
    getCanvasView()->slotScrollVertSmallSteps(y);

    Rosegarden::Composition &comp = m_document->getComposition();
    Rosegarden::Studio &studio = m_document->getStudio();

    MatrixStaff& staff = *(m_staffs[0]);
    Rosegarden::MidiByte evPitch = staff.getHeightAtCanvasY(y);

    // Don't do anything if we're part of a run up the keyboard
    // and the pitch hasn't changed
    //
    if (m_lastNote == evPitch && repeating)
        return;

    // Save value
    m_lastNote = evPitch;
    if (!repeating) m_firstNote = evPitch;

    Rosegarden::Track *track = comp.getTrackByIndex(
            m_staffs[0]->getSegment().getTrack());

    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return;

    // Send out note of half second duration
    //
    Rosegarden::MappedEvent *mE = 
      new Rosegarden::MappedEvent(ins->getId(),
                                  Rosegarden::MappedEvent::MidiNoteOneShot,
                                  evPitch,
                                  Rosegarden::MidiMaxValue,
                                  Rosegarden::RealTime(0, 0),
                                  Rosegarden::RealTime(0, 500000),
                                  Rosegarden::RealTime(0, 0));
    Rosegarden::StudioControl::sendMappedEvent(mE);

}


void MatrixView::slotKeySelected(unsigned int y, bool repeating)
{
    getCanvasView()->slotScrollVertSmallSteps(y);

    MatrixStaff& staff = *(m_staffs[0]);
    Rosegarden::Segment &segment(staff.getSegment());
    Rosegarden::MidiByte evPitch = staff.getHeightAtCanvasY(y);

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

    Rosegarden::Composition &comp = m_document->getComposition();
    Rosegarden::Studio &studio = m_document->getStudio();
    Rosegarden::Track *track = comp.getTrackByIndex(segment.getTrack());
    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return;

    // Send out note of half second duration
    //
    Rosegarden::MappedEvent *mE = 
      new Rosegarden::MappedEvent(ins->getId(),
                                  Rosegarden::MappedEvent::MidiNoteOneShot,
                                  evPitch,
                                  Rosegarden::MidiMaxValue,
                                  Rosegarden::RealTime(0, 0),
                                  Rosegarden::RealTime(0, 500000),
                                  Rosegarden::RealTime(0, 0));
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

    try {

	pitch = getPitchFromNoteInsertAction(name, accidental);

    } catch (...) {
	
	KMessageBox::sorry
	    (this, QString(i18n("Unknown note insert action %1").arg(name)));
	return;
    }

    KTmpStatusMsg msg(i18n("Inserting note"), this);
	
    MATRIX_DEBUG << "Inserting note at pitch " << pitch << endl;

    Rosegarden::Event modelEvent(Rosegarden::Note::EventType, 0, 1);
    modelEvent.set<Rosegarden::Int>(Rosegarden::BaseProperties::PITCH, pitch);
    modelEvent.set<Rosegarden::String>(Rosegarden::BaseProperties::ACCIDENTAL, accidental);
    Rosegarden::timeT time(getInsertionTime());
    Rosegarden::timeT endTime(time + m_snapGrid->getSnapTime(time));

    MatrixInsertionCommand* command = 
	new MatrixInsertionCommand(segment, time, endTime, &modelEvent);

    addCommandToHistory(command);
    
    slotSetInsertCursorPosition(endTime); //!!! + chord mode?
}

void MatrixView::closeWindow()
{
    delete this;
}

bool MatrixView::canPreviewAnotherNote()
{
    static clock_t lastCutOff = 0;
    static int sinceLastCutOff = 0;

    clock_t now = clock();
    ++sinceLastCutOff;

    if (((now - lastCutOff) / CLOCKS_PER_SEC) >= 1) {
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

    Rosegarden::Composition &comp = m_document->getComposition();
    Rosegarden::Studio &studio = m_document->getStudio();

    // Get the Instrument
    //
    Rosegarden::Track *track = comp.getTrackByIndex(
            m_staffs[0]->getSegment().getTrack());

    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    if (ins == 0)
        return;

    if (!canPreviewAnotherNote()) return;

    // check for null instrument
    //

    // Get a velocity
    //
    Rosegarden::MidiByte velocity = Rosegarden::MidiMaxValue;
    if (event->has(Rosegarden::BaseProperties::VELOCITY))
    {
        velocity = event->get<Rosegarden::Int>
                    (Rosegarden::BaseProperties::VELOCITY);
    }

    Rosegarden::RealTime duration =
            comp.getElapsedRealTime(event->getDuration());

    // create
    Rosegarden::MappedEvent *mE = 
      new Rosegarden::MappedEvent(ins->getId(),
                                  Rosegarden::MappedEvent::MidiNoteOneShot,
                                  (Rosegarden::MidiByte)
                                      event->get<Rosegarden::Int>
                                        (Rosegarden::BaseProperties::PITCH),
                                  velocity,
                                  Rosegarden::RealTime(0, 0),
                                  duration,
                                  Rosegarden::RealTime(0, 0));

    Rosegarden::StudioControl::sendMappedEvent(mE);
}


void MatrixView::playNote(const Rosegarden::Segment &segment, int pitch)
{
    Rosegarden::Composition &comp = m_document->getComposition();
    Rosegarden::Studio &studio = m_document->getStudio();

    Rosegarden::Track *track = comp.getTrackByIndex(segment.getTrack());

    Rosegarden::Instrument *ins =
        studio.getInstrumentById(track->getInstrument());

    // check for null instrument
    //
    if (ins == 0)
        return;

    // Send out note of half second duration
    //
    Rosegarden::MappedEvent *mE = 
      new Rosegarden::MappedEvent(ins->getId(),
                                  Rosegarden::MappedEvent::MidiNoteOneShot,
                                  pitch,
                                  Rosegarden::MidiMaxValue,
                                  Rosegarden::RealTime(0, 0),
                                  Rosegarden::RealTime(0, 500000),
                                  Rosegarden::RealTime(0, 0));

    Rosegarden::StudioControl::sendMappedEvent(mE);
}

void
MatrixView::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Key_Shift:
            m_shiftDown = true;
            break;

        case Key_Control:
            m_controlDown = true;
            break;

        default:
            event->ignore();
            break;
    }
}


void
MatrixView::keyReleaseEvent(QKeyEvent *event)
{
    switch(event->key())
    {
        case Key_Shift:
            m_shiftDown = false;
            break;

        case Key_Control:
            m_controlDown = false;
            break;

        default:
            event->ignore();
            break;
    }
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
MatrixView::setSingleSelectedEvent(int staffNo, Rosegarden::Event *event)
{
    setSingleSelectedEvent(getStaff(staffNo)->getSegment(), event);
}

void
MatrixView::setSingleSelectedEvent(Rosegarden::Segment &segment,
                                   Rosegarden::Event *event)
{
    setCurrentSelection(0, false);

    EventSelection *selection = new EventSelection(segment);
    selection->addEvent(event);
    setCurrentSelection(selection, true);
}

// A new selection has been acquired by a tool - set the appropriate
// information in matrix parameter pane.
//
void
MatrixView::slotNewSelection()
{
    MATRIX_DEBUG << "MatrixView::slotNewSelection\n";

    m_parameterBox->setSelection(m_currentEventSelection);
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

    updateView();
}

void
MatrixView::slotQuantizeSelection(int q)
{
    MATRIX_DEBUG << "MatrixView::slotQuantizeSelection\n";

    using Rosegarden::Quantizer;
    Rosegarden::timeT unit = m_quantizations[q].unit;

    Rosegarden::Quantizer quant(Quantizer::GlobalSource,
                                Quantizer::RawEventData,
                                Quantizer::PositionQuantize,
                                unit,
                                m_quantizations[q].maxDots);

    if (quant.getUnit() != 0)
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

    KToolBar *actionsToolbar = toolBar("actionsToolBar");

    if (!actionsToolbar)
    {
        MATRIX_DEBUG << "MatrixView::initActionsToolbar - "
                     << "tool bar not found" << endl;
        return;
    }

    // The SnapGrid combo
    //
    QLabel *sLabel = new QLabel(i18n("Grid"), actionsToolbar);
    sLabel->setIndent(10);

    using Rosegarden::Note;
    NotePixmapFactory npf;
    QPixmap noMap = NotePixmapFactory::toQPixmap(npf.makeToolbarPixmap("menu-no-note"));

    m_snapGridCombo = new RosegardenComboBox(false, false, actionsToolbar);

    Rosegarden::timeT crotchetDuration = Note(Note::Crotchet).getDuration();
    m_snapValues.push_back(Rosegarden::SnapGrid::NoSnap);
    m_snapValues.push_back(Rosegarden::SnapGrid::SnapToUnit);
    m_snapValues.push_back(crotchetDuration / 16);
    m_snapValues.push_back(crotchetDuration / 8);
    m_snapValues.push_back(crotchetDuration / 4);
    m_snapValues.push_back(crotchetDuration / 2);
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
	    QString label = npf.makeNoteMenuLabel(m_snapValues[i], true, err);
	    QPixmap pixmap = NotePixmapFactory::toQPixmap(npf.makeNoteMenuPixmap(m_snapValues[i], err));
	    m_snapGridCombo->insertItem((err ? noMap : pixmap), label);
	}
    }

    connect(m_snapGridCombo, SIGNAL(activated(int)),
            this, SLOT(slotSetSnapFromIndex(int)));

    connect(m_snapGridCombo, SIGNAL(propagate(int)),
            this, SLOT(slotSetSnapFromIndex(int)));

    // Quantize combo
    //
    QLabel *qLabel = new QLabel(i18n("Quantize"), actionsToolbar);
    qLabel->setIndent(10);

    m_quantizeCombo = new RosegardenComboBox(false, false, actionsToolbar);

    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {

	Rosegarden::timeT time = m_quantizations[i].unit;
	Rosegarden::timeT error = 0;
	QString label = npf.makeNoteMenuLabel(time, true, error);
	QPixmap pmap = NotePixmapFactory::toQPixmap(npf.makeNoteMenuPixmap(time, error));
	m_quantizeCombo->insertItem(error ? noMap : pmap, label);
    }

    m_quantizeCombo->insertItem(noMap, i18n("Off"));

    connect(m_quantizeCombo, SIGNAL(activated(int)),
            this, SLOT(slotQuantizeSelection(int)));

    // mouse wheel
    connect(m_quantizeCombo, SIGNAL(propagate(int)),
            this, SLOT(slotQuantizeSelection(int)));

}

void
MatrixView::initZoomToolbar()
{
    MATRIX_DEBUG << "MatrixView::initZoomToolbar" << endl;

    KToolBar *zoomToolbar = toolBar("zoomToolBar");

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
        zoomSizes.push_back(factors[i]);
    }

    m_hZoomSlider = new ZoomSlider<double>
        (zoomSizes, -1, QSlider::Horizontal, zoomToolbar);
    m_hZoomSlider->setTracking(true);
    m_hZoomSlider->setFocusPolicy(QWidget::NoFocus);

    m_zoomLabel = new QLabel(zoomToolbar);
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

    m_zoomLabel->setText(i18n("%1%").arg(zoomValue*100.0));

    MATRIX_DEBUG << "MatrixView::slotChangeHorizontalZoom() : zoom factor = "
                 << zoomValue << endl;

    // Set zoom matrix
    //
    QWMatrix zoomMatrix;
    zoomMatrix.scale(zoomValue, 1.0);
    m_canvasView->setWorldMatrix(zoomMatrix);

    if (m_topBarButtons) m_topBarButtons->setHScaleFactor(zoomValue);
    if (m_bottomBarButtons) m_bottomBarButtons->setHScaleFactor(zoomValue);

    for (unsigned int i = 0; i < m_controlRulers.size(); ++i)
    {
        m_controlRulers[i].first->setHScaleFactor(zoomValue);
        m_controlRulers[i].first->repaint();
    }

    if (m_topBarButtons) m_topBarButtons->update();
    if (m_bottomBarButtons) m_bottomBarButtons->update();

    // If you do adjust the viewsize then please remember to 
    // either re-center() or remember old scrollbar position
    // and restore.
    //

    int endX = int(m_hlayout.getXForTime(m_segments[0]->getEndMarkerTime()));
    int startX = int(m_hlayout.getXForTime(m_segments[0]->getStartTime()));

    int newWidth = int(getXbyWorldMatrix(endX - startX));

    // We DO NOT resize the canvas(), only the area it's displaying on
    //
    getCanvasView()->resizeContents(newWidth, getViewSize().height());

    // This forces a refresh of the h. scrollbar, even if the canvas width
    // hasn't changed
    //
    getCanvasView()->polish();
}

/// Scrolls the view such that the given time is centered
void
MatrixView::scrollToTime(timeT t) {
    double layoutCoord = m_hlayout.getXForTime(t);
    getCanvasView()->slotScrollHoriz(int(layoutCoord));
}

unsigned int
MatrixView::addControlRuler(const Rosegarden::PropertyName &property)
{
    // Try and find this controller if it exists
    //
    for (unsigned int i = 0; i != m_controlRulers.size(); i++)
    {
        if (m_controlRulers[i].first->getPropertyName() == property)
            return i;
    }

    int height = 20;

    ControlRuler *newRuler = new ControlRuler(&m_hlayout,
	                                      m_segments[0],
                                              property,
                                              m_staffs[0]->getVelocityColour(),
                                              xorigin,
                                              height,
                                              getCentralFrame());

    addRuler(newRuler);

    ControlBox *newControl =
        new ControlBox(strtoqstr(property), 
                       m_parameterBox->width() + m_pianoKeyboard->width(),
                       height,
                       getCentralFrame());

    addControl(newControl);

    m_controlRulers.push_back(
            std::pair<ControlRuler*, ControlBox*>(newRuler, newControl));
                             
    return m_controlRulers.size() - 1;
}


bool
MatrixView::removeControlRuler(unsigned int number)
{
    if (number > m_controlRulers.size() - 1)
        return false;

    std::vector<std::pair<ControlRuler*, ControlBox*> >::iterator it 
        = m_controlRulers.begin();
    while(number--) it++;

    delete it->first;
    delete it->second;
    m_controlRulers.erase(it);

    return true;
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
MatrixView::slotJumpCursorToPlayback()
{
    slotSetInsertCursorPosition(m_document->getComposition().getPosition());
}

void
MatrixView::slotJumpPlaybackToCursor()
{
    emit jumpPlaybackTo(getInsertionTime());
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

    m_document->slotSetLoop(m_currentEventSelection->getStartTime(),
			    m_currentEventSelection->getEndTime());
}


void MatrixView::slotClearLoop()
{
    m_document->slotSetLoop(0, 0);
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
            maxHeight = staff.getTotalHeight() + staff.getY() + 1;
        }

    }

    int endX = int(m_hlayout.getXForTime(m_segments[0]->getEndMarkerTime()));
    int startX = int(m_hlayout.getXForTime(m_segments[0]->getStartTime()));

    int newWidth = int(getXbyWorldMatrix(endX - startX));

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
	
    EventParameterDialog *dialog
        = new EventParameterDialog(this,
                                   i18n("Set Event Velocities"),
                                   Rosegarden::BaseProperties::VELOCITY,
                                   avVely);

    if (dialog->exec() == QDialog::Accepted) {
	KTmpStatusMsg msg(i18n("Setting Velocities..."), this);
	addCommandToHistory(new SelectionPropertyCommand
			    (m_currentEventSelection,
                             Rosegarden::BaseProperties::VELOCITY,
                             dialog->getPattern(),
                             dialog->getValue1(),
                             dialog->getValue2()));
    }
}

void
MatrixView::slotDocumentDestroyed()
{
    MATRIX_DEBUG << "MatrixView::slotDocumentDestroyed()\n";
    m_documentDestroyed = true;
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
