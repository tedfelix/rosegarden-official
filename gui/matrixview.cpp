// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include <kapp.h>
#include <kconfig.h>
#include <kaction.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kstdaction.h>
#include <kmessagebox.h>
#include <kstatusbar.h>

#include "Instrument.h"
#include "Composition.h"
#include "Event.h"
#include "Quantizer.h"

#include "BaseProperties.h"
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

#include "rosedebug.h"

using Rosegarden::Segment;
using Rosegarden::EventSelection;
using Rosegarden::timeT;

//----------------------------------------------------------------------

MatrixView::MatrixView(RosegardenGUIDoc *doc,
                       std::vector<Segment *> segments,
                       QWidget *parent)
    : EditView(doc, segments, true, parent, "matrixview"),
      m_currentEventSelection(0),
      m_hlayout(&doc->getComposition()),
      m_vlayout(),
      m_hoveredOverAbsoluteTime(0),
      m_hoveredOverNoteName(0),
      m_previousEvPitch(0),
      m_canvasView(0),
      m_pianoView(0),
      m_lastNote(0),
      m_selectedProperty(getViewLocalPropertyPrefix() + "Selected")
{
    kdDebug(KDEBUG_AREA) << "MatrixView ctor\n";

    m_toolBox = new MatrixToolBox(this);

    setupActions();

    initStatusBar();

    readOptions();

    QCanvas *tCanvas = new QCanvas(this);
    tCanvas->resize(width() * 2, height() * 2);

    kdDebug(KDEBUG_AREA) << "MatrixView : creating staff\n";

    for (unsigned int i = 0; i < segments.size(); ++i) {
        m_staffs.push_back(new MatrixStaff(tCanvas, segments[i], i,
                                           8)); //!!! so random, so rare
	if (i == 0) m_staffs[i]->setCurrent(true);
    }

    kdDebug(KDEBUG_AREA) << "MatrixView : creating canvas view\n";

    m_pianoView = new QDeferScrollView(getCentralFrame());
    m_pianoKeyboard = new PianoKeyboard(m_pianoView->viewport());
    
    m_pianoView->setVScrollBarMode(QScrollView::AlwaysOff);
    m_pianoView->setHScrollBarMode(QScrollView::AlwaysOff);
    m_pianoView->addChild(m_pianoKeyboard);
    m_pianoView->setFixedWidth(m_pianoView->contentsWidth());

    m_grid->addWidget(m_pianoView, 2, 0);

    m_canvasView = new MatrixCanvasView(*m_staffs[0], m_horizontalScrollBar,
                                        tCanvas, getCentralFrame());
    setCanvasView(m_canvasView);

    // Connect vertical scrollbars between matrix and piano
    //
    connect(m_canvasView->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotVerticalScrollPianoKeyboard(int)));

    connect(m_canvasView->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotVerticalScrollPianoKeyboard(int)));

    connect(m_pianoView, SIGNAL(gotWheelEvent(QWheelEvent*)),
            m_canvasView, SLOT(slotExternalWheelEvent(QWheelEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(activeItemPressed(QMouseEvent*, QCanvasItem*)),
         this,            SLOT  (activeItemPressed(QMouseEvent*, QCanvasItem*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mousePressed(Rosegarden::timeT, int, QMouseEvent*, MatrixElement*)),
         this,            SLOT  (slotMousePressed(Rosegarden::timeT, int, QMouseEvent*, MatrixElement*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mouseMoved(Rosegarden::timeT, int, QMouseEvent*)),
         this,            SLOT  (slotMouseMoved(Rosegarden::timeT, int, QMouseEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(mouseReleased(Rosegarden::timeT, int, QMouseEvent*)),
         this,            SLOT  (slotMouseReleased(Rosegarden::timeT, int, QMouseEvent*)));

    QObject::connect
        (getCanvasView(), SIGNAL(hoveredOverNoteChanged(const QString&)),
         this,            SLOT  (slotHoveredOverNoteChanged(const QString&)));

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

    kdDebug(KDEBUG_AREA) << "MatrixView : applying layout\n";

    bool layoutApplied = applyLayout();
    if (!layoutApplied) KMessageBox::sorry(0, i18n("Couldn't apply piano roll layout"));
    else {
        kdDebug(KDEBUG_AREA) << "MatrixView : rendering elements\n";
        for (unsigned int i = 0; i < m_staffs.size(); ++i) {

	    m_staffs[i]->positionAllElements();
            m_staffs[i]->getSegment().getRefreshStatus(m_segmentsRefreshStatusIds[i]).setNeedsRefresh(false);

        }
    }

    BarButtons *topBarButtons = new BarButtons(&m_hlayout, 0, 25,
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

    } else if (segments.size() == doc->getComposition().getNbSegments()) {

        setCaption(QString("%1 - All Segments")
                   .arg(doc->getTitle()));

    } else {

        setCaption(QString("%1 - %2-Segment Partial View")
                   .arg(doc->getTitle())
                   .arg(segments.size()));
    }

    // Scroll view to half way so we get a better chance of seeing something 
    //
    /* - can't use for the moment until the scroll/closing crash is sorted
    m_canvasView->setContentsPos(0, m_canvasView->contentsHeight()/2 -
                                     m_canvasView->visibleHeight()/2);
    */


}

MatrixView::~MatrixView()
{
    saveOptions();

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
}

void MatrixView::saveOptions()
{        
    m_config->setGroup("Matrix Options");
    EditView::saveOptions();

    m_config->writeEntry("Show Tools Toolbar", true); // in case we
                                                      // may want to
                                                      // let the user
                                                      // hide this
                                                      // toolbar in the future
    toolBar("toolsToolBar")->saveSettings(m_config,
                                          "Matrix Options toolsToolbar");
}

void MatrixView::readOptions()
{
    m_config->setGroup("Matrix Options");
    EditView::readOptions();

    bool opt;

    opt = m_config->readBoolEntry("Show Tools Toolbar", true);
    // at the moment we always show this toolbar - the view isn't of
    // much use without it

    toolBar("toolsToolBar")->applySettings(m_config,
                                           "Matrix Options toolsToolbar");
}

void MatrixView::setupActions()
{   
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

    toolAction = new KRadioAction(i18n("Paint"), "pencil", 0,
                                  this, SLOT(slotPaintSelected()),
                                  actionCollection(), "paint");
    toolAction->setExclusiveGroup("tools");

    toolAction = new KRadioAction(i18n("Erase"), "eraser", 0,
                                  this, SLOT(slotEraseSelected()),
                                  actionCollection(), "erase");
    toolAction->setExclusiveGroup("tools");

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QIconSet icon(QPixmap(pixmapDir + "/toolbar/select.xpm"));

    toolAction = new KRadioAction(i18n("Select"), icon, 0,
                                  this, SLOT(slotSelectSelected()),
                                  actionCollection(), "select");
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

    KStdAction::showToolbar(this, SLOT(slotToggleToolBar()), actionCollection());
    KStdAction::showStatusbar(this, SLOT(slotToggleStatusBar()), actionCollection());

    KStdAction::saveOptions(this, SLOT(save_options()), actionCollection());
    KStdAction::preferences(this, SLOT(customize()), actionCollection());

    KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());

    createGUI("matrix.rc");

    actionCollection()->action("paint")->activate();
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

}


bool MatrixView::applyLayout(int /*staffNo*/)
{
    m_hlayout.reset();
    m_vlayout.reset();
        
    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_hlayout.scanStaff(*m_staffs[i]);
        m_vlayout.scanStaff(*m_staffs[i]);
    }

    m_hlayout.finishLayout();
    m_vlayout.finishLayout();

    double maxWidth = 0.0, maxHeight = 0.0;

    for (unsigned int i = 0; i < m_staffs.size(); ++i) {
        m_staffs[i]->sizeStaff(m_hlayout);

        if (m_staffs[i]->getX() + m_staffs[i]->getTotalWidth() > maxWidth) {
            maxWidth = m_staffs[i]->getX() + m_staffs[i]->getTotalWidth();
        }

        if (m_staffs[i]->getY() + m_staffs[i]->getTotalHeight() > maxHeight) {
            maxHeight = m_staffs[i]->getY() + m_staffs[i]->getTotalHeight();
        }
    }

    readjustViewSize(QSize(int(maxWidth), int(maxHeight)), true);
    
    return true;
}

void MatrixView::refreshSegment(Segment *segment,
				timeT startTime, timeT endTime)
{
    kdDebug(KDEBUG_AREA) << "MatrixView::refreshSegment(" << startTime
                         << ", " << endTime << ")\n";

    applyLayout();

    if (!segment) segment = m_segments[0];

    if (endTime == 0) endTime = segment->getEndTime();
    else if (startTime == endTime) {
        startTime = segment->getStartTime();
        endTime   = segment->getEndTime();
    }

    m_staffs[0]->positionElements(startTime, endTime);
}

QSize MatrixView::getViewSize()
{
    return canvas()->size();
}

void MatrixView::setViewSize(QSize s)
{
    canvas()->resize(s.width(), s.height());

}

void MatrixView::updateView()
{
    canvas()->update();
}

MatrixCanvasView* MatrixView::getCanvasView()
{
    return dynamic_cast<MatrixCanvasView *>(m_canvasView);
}


void MatrixView::setCurrentSelection(EventSelection* s)
{
    if (m_currentEventSelection) {
        m_currentEventSelection->removeSelectionFromSegment
	    (m_selectedProperty);
        getStaff(0)->positionElements(m_currentEventSelection->getStartTime(),
                                      m_currentEventSelection->getEndTime());
    }

    delete m_currentEventSelection;
    m_currentEventSelection = s;

    if (s) {
        s->recordSelectionOnSegment(m_selectedProperty);
        getStaff(0)->positionElements(s->getStartTime(),
                                      s->getEndTime());
    }

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
						Quantizer::RawEventData,
						Quantizer::RawEventData);

    if (dialog->exec() == QDialog::Accepted) {
	KTmpStatusMsg msg(i18n("Quantizing..."), statusBar());
	addCommandToHistory(new EventQuantizeCommand
			    (*m_currentEventSelection,
			     dialog->getQuantizer()));
    }
}

void MatrixView::slotMousePressed(Rosegarden::timeT time, int pitch,
                                  QMouseEvent* e, MatrixElement* el)
{
    kdDebug(KDEBUG_AREA) << "MatrixView::mousePressed at pitch "
                         << pitch << ", time " << time << endl;

    Rosegarden::Segment &segment = m_staffs[0]->getSegment();
    Rosegarden::Composition *composition = segment.getComposition();
    Rosegarden::timeT firstBar =
        composition->getBarStart(composition->
                getBarNumber(segment.getStartTime()));

    m_tool->handleMousePress(time, pitch, 0, e, el);

    // play a preview
    playPreview(pitch);

}

void MatrixView::slotMouseMoved(Rosegarden::timeT time, int pitch, QMouseEvent* e)
{
    if (activeItem()) {
        activeItem()->handleMouseMove(e);
	updateView();
    }
    else 
    {
        Rosegarden::Segment &segment = m_staffs[0]->getSegment();
        Rosegarden::Composition *composition = segment.getComposition();
        Rosegarden::timeT firstBar =
            composition->getBarStart(composition->
                    getBarNumber(segment.getStartTime()));

        if (m_tool->handleMouseMove(time - firstBar, pitch, e)) {
	    slotScrollHorizSmallSteps(e->pos().x());
	}
	    
        // play a preview
        if (pitch != m_previousEvPitch)
        {
            playPreview(pitch);
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
MatrixView::slotSetPointerPosition(timeT time)
{
    Rosegarden::Composition &comp = m_document->getComposition();
    int barNo = comp.getBarNumber(time);

    if (barNo < m_hlayout.getFirstVisibleBarOnStaff(*m_staffs[0]) ||
        barNo > m_hlayout. getLastVisibleBarOnStaff(*m_staffs[0])) {
        m_staffs[0]->hidePointer();
    } else {
        m_staffs[0]->setPointerPosition(m_hlayout, time);
    }

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
    kdDebug(KDEBUG_AREA) << "MatrixView::slotEditCut()\n";

    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Cutting selection to clipboard..."), statusBar());

    addCommandToHistory(new CutCommand(*m_currentEventSelection,
				       m_document->getClipboard()));
}

void MatrixView::slotEditCopy()
{
    if (!m_currentEventSelection) return;
    KTmpStatusMsg msg(i18n("Copying selection to clipboard..."), statusBar());

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

    KTmpStatusMsg msg(i18n("Inserting clipboard contents..."), statusBar());

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
    Rosegarden::MappedEvent *mE = 
        new Rosegarden::MappedEvent(ins->getID(),
                                    Rosegarden::MappedEvent::MidiNoteOneShot,
                                    evPitch,
                                    Rosegarden::MidiMaxValue,
                                    Rosegarden::RealTime(0, 0),
                                    Rosegarden::RealTime(0, 500000),
                                    Rosegarden::RealTime(0, 0));

    emit keyPressed(mE);
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
    Rosegarden::MappedEvent *mE = 
        new Rosegarden::MappedEvent(ins->getID(),
                                    Rosegarden::MappedEvent::MidiNoteOneShot,
                                    (Rosegarden::MidiByte)
                                        event->get<Rosegarden::Int>
                                            (Rosegarden::BaseProperties::PITCH),
                                    velocity,
                                    Rosegarden::RealTime(0, 0),
                                    duration,
                                    Rosegarden::RealTime(0, 0));

    emit keyPressed(mE);
}


void MatrixView::playPreview(int pitch)
{
    Rosegarden::Composition &comp = m_document->getComposition();
    Rosegarden::Studio &studio = m_document->getStudio();

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
        new Rosegarden::MappedEvent(ins->getID(),
                                    Rosegarden::MappedEvent::MidiNoteOneShot,
                                    pitch,
                                    Rosegarden::MidiMaxValue,
                                    Rosegarden::RealTime(0, 0),
                                    Rosegarden::RealTime(0, 500000),
                                    Rosegarden::RealTime(0, 0));

    emit keyPressed(mE);
}


