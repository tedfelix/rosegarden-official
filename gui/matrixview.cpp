// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
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
#include "dialogs.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "ktmpstatusmsg.h"
#include "barbuttons.h"
#include "loopruler.h"
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
      m_currentEventSelection(0),
      m_pushSegment(0),
      m_hlayout(&doc->getComposition()),
      m_vlayout(),
      m_snapGrid(new Rosegarden::SnapGrid(&m_hlayout)),
      m_hoveredOverAbsoluteTime(0),
      m_hoveredOverNoteName(0),
      m_selectionCounter(0),
      m_previousEvPitch(0),
      m_canvasView(0),
      m_pianoView(0),
      m_lastNote(0),
      m_quantizations(
              Rosegarden::StandardQuantization::getStandardQuantizations())
{
    MATRIX_DEBUG << "MatrixView ctor\n";
    Rosegarden::Composition &comp = doc->getComposition();

    m_toolBox = new MatrixToolBox(this);

    initStatusBar();

    QCanvas *tCanvas = new QCanvas(this);
    //tCanvas->resize(width() * 2, height() * 2);

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
        (getCanvasView(), SIGNAL(hoveredOverAbsoluteTimeChanged(unsigned int)),
         this,            SLOT  (slotHoveredOverAbsoluteTimeChanged(unsigned int)));

    QObject::connect
	(doc, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
	 this, SLOT(slotSetPointerPosition(Rosegarden::timeT)));

    MATRIX_DEBUG << "MatrixView : applying layout\n";

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, i18n("Couldn't apply piano roll layout"));
    else {
        MATRIX_DEBUG << "MatrixView : rendering elements\n";
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {

	    m_staffs[i]->positionAllElements();
            m_staffs[i]->getSegment().getRefreshStatus(m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);

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

    // Scroll view to centre middle-C and warp to pointer position
    //
    m_canvasView->scrollBy(0, m_staffs[0]->getCanvasYForHeight(60) / 2);

    slotSetPointerPosition(comp.getPosition());

#ifdef RGKDE3
    stateChanged("have_selection", KXMLGUIClient::StateReverse);
    slotTestClipboard();
#endif

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

    // Delete remaining canvas items.
    QCanvasItemList allItems = canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = allItems.begin(); it != allItems.end(); ++it) delete *it;

    delete m_snapGrid;
}

void MatrixView::slotSaveOptions()
{        
    m_config->setGroup("Matrix Options");
    // no options to save at the moment
}

void MatrixView::readOptions()
{
    EditView::readOptions();
    m_config->setGroup("Matrix Options");
    // no options to read at the moment
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

    toolAction = new KRadioAction(i18n("Select"), icon, 0,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("Paint"), "pencil", 0,
                                  this, SLOT(slotPaintSelected()),
                                  actionCollection(), "draw");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("Erase"), "eraser", 0,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("Move"), "move", 0,
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

    new KAction(i18n("Select All"), 0, this,
                SLOT(slotSelectAll()), actionCollection(),
                "select_all");

    new KAction(i18n("&Delete"), Key_Delete, this,
                SLOT(slotEditDelete()), actionCollection(),
                "delete");

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

    readjustCanvasSize();
    
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
    canvas()->resize(s.width(), s.height());

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
    if (!s) {
	m_quantizeCombo->setCurrentItem(m_quantizeCombo->count() - 1); // "Off"
    }
    if (!m_currentEventSelection && !s)	return;

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

    timeT quantizeUnit = -1;

    if (s) {

        bool foundNewEvent = false;

        for (EventSelection::eventcontainer::iterator i =
                 s->getSegmentEvents().begin();
             i != s->getSegmentEvents().end(); ++i) {

	    //!!! Consider moving this out into a static method
	    // of StandardQuantization (from here to

	    if ((*i)->isa(Rosegarden::Note::EventType)) {

		timeT absTime = (*i)->getAbsoluteTime();
		timeT myQuantizeUnit = 0;

		// m_quantizations is in descending order of duration;
		// stop when we reach one that divides into the note's time

		for (unsigned int i = 0; i < m_quantizations.size(); ++i) {
		    if (absTime % m_quantizations[i].unit == 0) {
			myQuantizeUnit = m_quantizations[i].unit;
			break;
		    }
		}

		if (quantizeUnit < 0 || myQuantizeUnit < quantizeUnit) {
		    quantizeUnit = myQuantizeUnit;
		}
	    }

	    // here, in a loop, returning a quantize unit that
	    // corresponds to one of the standard quantizations)

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

    if (quantizeUnit >= 0) {
	for (unsigned int i = 0; i < m_quantizations.size(); ++i) {
	    if (quantizeUnit == m_quantizations[i].unit) {
		m_quantizeCombo->setCurrentItem(i);
		break;
	    }
	    if (i == m_quantizations.size() - 1) {
		m_quantizeCombo->setCurrentItem(i+1); // "Off"
	    }
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


#ifdef RGKDE3
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
#endif

    updateView();
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
        if (m_tool->handleMouseMove(time, pitch, e)) {
	    slotScrollHorizSmallSteps(e->pos().x());
	}
	    
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
        slotScrollHoriz(getXbyWorldMatrix(m_hlayout.getXForTime(time)));

    updateView();
}

void
MatrixView::slotSetInsertCursorPosition(timeT time)
{
    //!!! For now.  Probably unlike slotSetPointerPosition this one
    // should snap to the nearest event.

    m_staffs[0]->setInsertCursorPosition(m_hlayout, time);
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
    Rosegarden::Composition &comp = m_document->getComposition();
    Rosegarden::Studio &studio = m_document->getStudio();

    MatrixStaff& staff = *(m_staffs[0]);
    Rosegarden::MidiByte evPitch = staff.getHeightAtCanvasY(y);

    // Don't do anything if we're part of a run up the keyboard
    //
    if (m_lastNote == evPitch && repeating)
        return;

    // Save value
    m_lastNote = evPitch;

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
    try
    {
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
    catch(...) {;}

}


void MatrixView::slotVerticalScrollPianoKeyboard(int y)
{
    if (m_pianoView) // check that the piano view still exists (see dtor)
        m_pianoView->setContentsPos(0, y);
}

void MatrixView::closeWindow()
{
    delete this;
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

    try
    {
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
    catch(...) {;}
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

    try
    {
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
    catch(...) {;}
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
    if (!m_currentEventSelection) return;
    if (m_currentEventSelection->getAddedEvents() == 0) return;

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
        addCommandToHistory(
                new EventQuantizeCommand(*m_currentEventSelection, quant));
    }
    else
    {
        KTmpStatusMsg msg(i18n("Unquantizing..."), this);
        addCommandToHistory(
                new EventUnquantizeCommand(*m_currentEventSelection, quant));
    }

    //setCurrentSelection(oldSelection, false);
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
    QPixmap noMap = npf.makeToolbarPixmap("menu-no-note");

    m_snapGridCombo = new RosegardenComboBox(false, false, actionsToolbar);

    Rosegarden::timeT crotchetDuration = Note(Note::Crotchet).getDuration();
    m_snapValues.push_back(Rosegarden::SnapGrid::NoSnap);
    m_snapValues.push_back(Rosegarden::SnapGrid::SnapToUnit);
    m_snapValues.push_back(crotchetDuration / 16);
    m_snapValues.push_back(crotchetDuration / 8);
    m_snapValues.push_back(crotchetDuration / 4);
    m_snapValues.push_back(crotchetDuration / 2);
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
	    QPixmap pixmap = npf.makeNoteMenuPixmap(m_snapValues[i], err);
	    m_snapGridCombo->insertItem((err ? noMap : pixmap), label);
	}
    }

    connect(m_snapGridCombo, SIGNAL(activated(int)),
            this, SLOT(slotSetSnapFromIndex(int)));

    connect(m_snapGridCombo, SIGNAL(propagate(int)),
            this, SLOT(slotSetSnapFromIndex(int)));

    new KAction(i18n("Snap to 1/64"), Key_0, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_64");
    new KAction(i18n("Snap to 1/32"), Key_3, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_32");
    new KAction(i18n("Snap to 1/16"), Key_6, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_16");
    new KAction(i18n("Snap to 1/8"), Key_8, this,
                SLOT(slotSetSnapFromAction()), actionCollection(), "snap_8");

    // Quantize combo
    //
    QLabel *qLabel = new QLabel(i18n("Quantize"), actionsToolbar);
    qLabel->setIndent(10);

    m_quantizeCombo = new RosegardenComboBox(false, false, actionsToolbar);

    for (unsigned int i = 0; i < m_quantizations.size(); ++i) {

	Rosegarden::timeT time = m_quantizations[i].unit;
	Rosegarden::timeT error = 0;
	QString label = npf.makeNoteMenuLabel(time, true, error);
	QPixmap pmap = npf.makeNoteMenuPixmap(time, error);
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
//     double duration44 = Rosegarden::TimeSignature(4,4).getBarDuration();
    double value = m_hZoomSlider->getCurrentSize();
//     m_zoomLabel->setText(i18n("%1%").arg(duration44/value));
    m_zoomLabel->setText(i18n("%1%").arg(value*100.0));

    MATRIX_DEBUG << "MatrixView::slotChangeHorizontalZoom() : zoom factor = "
                 << value << endl;

    // Set zoom matrix
    //
    QWMatrix zoomMatrix;
    zoomMatrix.scale(value, 1.0);
    m_canvasView->setWorldMatrix(zoomMatrix);

    BarButtons* barButtons = dynamic_cast<BarButtons*>(m_topBarButtons);
    if (barButtons) barButtons->setHScaleFactor(value);

    barButtons = dynamic_cast<BarButtons*>(m_bottomBarButtons);
    if (barButtons) barButtons->setHScaleFactor(value);

    for (unsigned int i = 0; i < m_controlRulers.size(); ++i)
        m_controlRulers[i].first->setHScaleFactor(value);

//     for (unsigned int i = 0; i < m_staffs.size(); ++i)
//     {
//         m_staffs[i]->setTimeScaleFactor(1.0/m_hZoomSlider->getCurrentSize());
//         m_staffs[i]->sizeStaff(m_hlayout);
//     }

    if (m_topBarButtons) m_topBarButtons->update();
    if (m_bottomBarButtons) m_bottomBarButtons->update();

    // If you do adjust the viewsize then please remember to 
    // either re-center() or remember old scrollbar position
    // and restore.
    //
    Rosegarden::timeT length = m_segments[0]->getEndTime() -
                               m_segments[0]->getStartTime();

    int newWidth = getXbyInverseWorldMatrix(m_hlayout.getXForTime(length));
    setViewSize(QSize(newWidth, getViewSize().height()));
    applyLayout();

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


void
MatrixView::readjustCanvasSize()
{
    double maxWidth = 0.0;
    int maxHeight = 0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {

        MatrixStaff &staff = *m_staffs[i];

        staff.sizeStaff(m_hlayout);

        if (staff.getTotalWidth() + staff.getX() > maxWidth) {
            maxWidth = staff.getTotalWidth() + staff.getX() + 1;
        }

        if (staff.getTotalHeight() + staff.getY() > maxHeight) {
            maxHeight = staff.getTotalHeight() + staff.getY() + 1;
        }

    }

    // now get the EditView to do the biz
    readjustViewSize(QSize(int(maxWidth), maxHeight));
    repaintRulers();
}



