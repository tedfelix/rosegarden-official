// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <algorithm>

#include <qdragobject.h>
#include <qlayout.h>
#include <qcanvas.h>
#include <qlabel.h>
#include <qaccel.h>
#include <qtimer.h>
#include <qpixmap.h>

#include <kmessagebox.h>
#include <kapp.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kstddirs.h>

#include "RulerScale.h"
#include "Track.h"
#include "NotationTypes.h"

#include "constants.h"
#include "trackeditor.h"
#include "segmentcanvas.h"
#include "temporuler.h"
#include "chordnameruler.h"
#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "colours.h"
#include "multiviewcommandhistory.h"
#include "segmentcommands.h"
#include "barbuttons.h"
#include "trackbuttons.h"
#include "loopruler.h"
#include "qdeferscrollview.h"

#include "rosedebug.h"

using Rosegarden::Composition;
using Rosegarden::RulerScale;
using Rosegarden::timeT;
using Rosegarden::Segment;
using Rosegarden::TrackId;

TrackEditor::TrackEditor(RosegardenGUIDoc* doc,
                         QWidget* rosegardenguiview,
			 RulerScale *rulerScale,
                         bool showTrackLabels,
			 QWidget* parent, const char* name,
			 WFlags) :
    QWidget(parent, name),
    DCOPObject("TrackEditorIface"),
    m_doc(doc),
    m_rulerScale(rulerScale),
    m_topBarButtons(0),
    m_bottomBarButtons(0),
    m_trackButtons(0),
    m_segmentCanvas(0),
    m_trackButtonScroll(0),
    m_showTrackLabels(showTrackLabels),
    m_canvasWidth(0),
    m_compositionRefreshStatusId(doc->getComposition().getNewRefreshStatusId())
{
    // accept dnd
    setAcceptDrops(true);

    init(rosegardenguiview);
    slotReadjustCanvasSize();
}

TrackEditor::~TrackEditor()
{
    delete m_chordNameRuler;

    // flush gc (i.e. forget what's in there).
    // All remaining items will be deleted by the canvas anyway.
    //
    CanvasItemGC::flush();
}

void
TrackEditor::init(QWidget* rosegardenguiview)
{
    QGridLayout *grid = new QGridLayout(this, 4, 2);

    QCanvas *canvas = new QCanvas(this);
    canvas->resize(100, 100); // call slotReadjustCanvasSize later
    canvas->setBackgroundColor(RosegardenGUIColours::SegmentCanvas);

    kapp->config()->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    if (kapp->config()->readBoolEntry("backgroundtextures", false)) {
	QPixmap background;
	QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
	if (background.load(QString("%1/misc/bg-paper-grey.xpm").
			    arg(pixmapDir))) {
	    canvas->setBackgroundPixmap(background);
	}
    }

    int trackLabelWidth = 230;
    int barButtonsHeight = 25;

    m_chordNameRuler = new ChordNameRuler(m_rulerScale,
					  m_doc,
					  0.0,
					  20,
					  this);
    grid->addWidget(m_chordNameRuler, 0, 1);

    m_tempoRuler = new TempoRuler(m_rulerScale,
				  &m_doc->getComposition(),
				  0.0,
				  18,
				  true,
				  this);

    grid->addWidget(m_tempoRuler, 1, 1);

    //
    // Top Bar Buttons
    //
    m_topBarButtons = new BarButtons(m_doc,
                                     m_rulerScale,
				     0,
                                     barButtonsHeight,
                                     false,
                                     this, "topbarbuttons");
    m_topBarButtons->connectRulerToDocPointer(m_doc);

    grid->addWidget(m_topBarButtons, 2, 1);

    //
    // Segment Canvas
    //
    m_segmentCanvas = new SegmentCanvas(m_doc,
                                        m_rulerScale,
                                        getTrackCellHeight(),
                                        canvas, this);

    //
    // Bottom Bar Buttons
    //
    m_bottomBarButtons = new BarButtons(m_doc,
                                        m_rulerScale,
					0, 
                                        barButtonsHeight,
                                        true,
                                        m_segmentCanvas, "bottombarbuttons");
    m_bottomBarButtons->connectRulerToDocPointer(m_doc);

    m_segmentCanvas->setBottomFixedWidget(m_bottomBarButtons);

    grid->addWidget(m_segmentCanvas, 3, 1);

  
    // Track Buttons
    //
    // (must be put in a QScrollView)
    //
    m_trackButtonScroll = new QDeferScrollView(this);
    grid->addWidget(m_trackButtonScroll, 3, 0);

    int canvasHeight = getTrackCellHeight() *
	std::max(40u, m_doc->getComposition().getNbTracks());

    m_trackButtons = new TrackButtons(m_doc,
                                      getTrackCellHeight(),
                                      trackLabelWidth,
                                      m_showTrackLabels,
                                      canvasHeight,
                                      m_trackButtonScroll->viewport());
    m_trackButtonScroll->addChild(m_trackButtons);
    m_trackButtonScroll->setHScrollBarMode(QScrollView::AlwaysOff);
    m_trackButtonScroll->setVScrollBarMode(QScrollView::AlwaysOff);
    m_trackButtonScroll->setMinimumWidth(m_trackButtonScroll->contentsWidth());

    connect(m_trackButtons, SIGNAL(widthChanged()),
            this, SLOT(slotTrackButtonsWidthChanged()));

    connect(m_trackButtons, SIGNAL(trackSelected(int)),
            rosegardenguiview, SLOT(slotSelectTrackSegments(int)));

    connect(m_trackButtons, SIGNAL(instrumentSelected(int)),
            rosegardenguiview, SLOT(slotUpdateInstrumentParameterBox(int)));

    connect(this, SIGNAL(stateChange(const QString&, bool)),
            rosegardenguiview, SIGNAL(stateChange(const QString&, bool)));

    connect(m_trackButtons, SIGNAL(modified()),
            m_doc, SLOT(slotDocumentModified()));

    connect(m_trackButtons, SIGNAL(muteButton(Rosegarden::TrackId, bool)),
            rosegardenguiview, SLOT(slotSetMuteButton(Rosegarden::TrackId, bool)));

    connect(m_trackButtons, SIGNAL(newRecordButton()),
            m_doc, SLOT(slotNewRecordButton()));

    // Synchronize bar buttons' scrollview with segment canvas' scrollbar
    //
    connect(m_segmentCanvas->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(slotVerticalScrollTrackButtons(int)));

    connect(m_segmentCanvas->verticalScrollBar(), SIGNAL(sliderMoved(int)),
            this, SLOT(slotVerticalScrollTrackButtons(int)));

    // scrolling with mouse wheel
    connect(m_trackButtonScroll, SIGNAL(gotWheelEvent(QWheelEvent*)),
            m_segmentCanvas, SLOT(slotExternalWheelEvent(QWheelEvent*)));

    // Connect horizontal scrollbar
    //
    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_topBarButtons, SLOT(slotScrollHoriz(int)));
    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_topBarButtons, SLOT(slotScrollHoriz(int)));

    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_bottomBarButtons, SLOT(slotScrollHoriz(int)));
    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_bottomBarButtons, SLOT(slotScrollHoriz(int)));

    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_tempoRuler, SLOT(slotScrollHoriz(int)));
    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_tempoRuler, SLOT(slotScrollHoriz(int)));

    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            m_chordNameRuler, SLOT(slotScrollHoriz(int)));
    connect(m_segmentCanvas->horizontalScrollBar(), SIGNAL(sliderMoved(int)),
            m_chordNameRuler, SLOT(slotScrollHoriz(int)));

    connect(this, SIGNAL(needUpdate()), m_segmentCanvas, SLOT(slotUpdate()));

    connect(m_segmentCanvas, 
            SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)),
            rosegardenguiview,
            SLOT(slotSelectedSegments(const Rosegarden::SegmentSelection &)));

    connect(getCommandHistory(), SIGNAL(commandExecuted()),
	    this, SLOT(update()));

    connect(m_doc, SIGNAL(pointerPositionChanged(Rosegarden::timeT)),
	    this, SLOT(slotSetPointerPosition(Rosegarden::timeT)));
 
    connect(m_doc, SIGNAL(loopChanged(Rosegarden::timeT,
					   Rosegarden::timeT)),
	    this, SLOT(slotSetLoop(Rosegarden::timeT, Rosegarden::timeT)));

    // create the position pointer
    m_pointer = new QCanvasRectangle(canvas);
    m_pointer->setPen(RosegardenGUIColours::Pointer);
    m_pointer->setBrush(RosegardenGUIColours::Pointer);
    m_pointer->setSize(3, canvas->height());
    m_pointer->setX(-2);
    m_pointer->setY(0);
    m_pointer->setZ(10);
    m_pointer->show();
}

void TrackEditor::slotReadjustCanvasSize()
{
    Composition &comp = m_doc->getComposition();
    int lastBar = comp.getBarNumber(comp.getEndMarker());
    
    m_canvasWidth = (int)(m_rulerScale->getBarPosition(lastBar) +
                          m_rulerScale->getBarWidth(lastBar));

    // Not very satisfactory
    //
//     int canvasHeight = std::max(getTrackCellHeight() * comp.getNbTracks(),
//                                 m_segmentCanvas->viewport()->height()
//                                 //QApplication::desktop()->height()
//                                 );

    RG_DEBUG << "TrackEditor::slotReadjustCanvasSize() : nbTracks = "
             << comp.getNbTracks() << endl;

    int canvasHeight = getTrackCellHeight() * std::max(40u, comp.getNbTracks());

    m_segmentCanvas->canvas()->resize(m_canvasWidth, canvasHeight);

    m_pointer->setSize(3, canvasHeight);
}

void TrackEditor::slotTrackButtonsWidthChanged()
{
    // We need to make sure the trackButtons geometry is fully updated
    //
    kapp->processEvents();

    m_trackButtonScroll->setMinimumWidth(m_trackButtons->width());
    m_doc->slotDocumentModified();
}


int TrackEditor::getTrackCellHeight() const
{
    int size;
    static QFont defaultFont;

    // do some scrabbling around for a reasonable size
    //
    size = defaultFont.pixelSize();

    if (size < 8)
    {
        if (QApplication::font(this).pixelSize() < 8)
            size = 12;
        else
            size = QApplication::font(this).pixelSize();
    }

    return size + 12;
}

void
TrackEditor::setupSegments()
{
    RG_DEBUG << "TrackEditor::setupSegments() begin" << endl;

    if (!m_doc) return; // sanity check
    
    Composition &comp = m_doc->getComposition();

    for (Composition::iterator i = comp.begin(); i != comp.end(); ++i) {

        if (!(*i)) continue;

	RG_DEBUG << "TrackEditor::setupSegments() add segment"
			     << " - start idx : " << (*i)->getStartTime()
			     << " - nb time steps : " << ((*i)->getEndTime() - (*i)->getStartTime())
			     << " - track : " << (*i)->getTrack()
			     << endl;

	m_segmentCanvas->addSegmentItem((*i));
    }
}

bool TrackEditor::isCompositionModified()
{
    return m_doc->getComposition().getRefreshStatus
	(m_compositionRefreshStatusId).needsRefresh();
}

void TrackEditor::setCompositionModified(bool c)
{
    m_doc->getComposition().getRefreshStatus
	(m_compositionRefreshStatusId).setNeedsRefresh(c);
}


void TrackEditor::paintEvent(QPaintEvent* e)
{
    if (isCompositionModified()) {

        RG_DEBUG << "TrackEditor::paintEvent: composition is modified, update everything\n";

	slotReadjustCanvasSize();
        m_segmentCanvas->updateAllSegmentItems();
        m_trackButtons->slotUpdateTracks();

	Composition &composition = m_doc->getComposition();

        if (composition.getNbSegments() == 0) {
            emit stateChange("have_segments", false); // no segments : reverse state
            emit stateChange("have_selection", false); // no segments : reverse state
        }
        else {
            emit stateChange("have_segments", true);
            if (m_segmentCanvas->haveSelection())
                emit stateChange("have_selection", true);
            else
                emit stateChange("have_selection", false); // no selection : reverse state
        }

        setCompositionModified(false);

    } else if (m_segmentCanvas->isShowingPreviews()) { 

	for (Composition::iterator i = m_doc->getComposition().begin();
	     i != m_doc->getComposition().end(); ++i) {

	    SegmentRefreshStatusIdMap::iterator ri =
		m_segmentsRefreshStatusIds.find(*i);

	    bool refresh = false;

	    if (ri == m_segmentsRefreshStatusIds.end()) {
		
		//RG_DEBUG << "TrackEditor::paintEvent: adding segment " << *i << " to map" << endl;
		m_segmentsRefreshStatusIds[*i] = (*i)->getNewRefreshStatusId();

	    } else {
	    
		unsigned int refreshStatusId = m_segmentsRefreshStatusIds[*i];
		Rosegarden::SegmentRefreshStatus &refreshStatus =
		    (*i)->getRefreshStatus(refreshStatusId);

		refresh = refreshStatus.needsRefresh();
		refreshStatus.setNeedsRefresh(false);
	    }

	    if (refresh) {
		m_segmentCanvas->updateSegmentItem(*i);
	    }
	}
    }

    QWidget::paintEvent(e);
}

void TrackEditor::slotAddTracks(unsigned int nbNewTracks,
                                Rosegarden::InstrumentId id)
{
    Composition &comp = m_doc->getComposition();

    AddTracksCommand* command = new AddTracksCommand(&comp, nbNewTracks, id); 
    addCommandToHistory(command);
    slotReadjustCanvasSize();
}

void TrackEditor::slotDeleteTracks(std::vector<Rosegarden::TrackId> tracks)
{
    Composition &comp = m_doc->getComposition();

    DeleteTracksCommand* command = new DeleteTracksCommand(&comp, tracks);
    addCommandToHistory(command);
}


void TrackEditor::addSegment(int track, int time, unsigned int duration)
{
    if (!m_doc) return; // sanity check

    SegmentInsertCommand *command =
	new SegmentInsertCommand(m_doc, track, time, duration);

    addCommandToHistory(command);
}


void TrackEditor::slotSegmentOrderChanged(int section, int fromIdx, int toIdx)
{
    RG_DEBUG << QString("TrackEditor::segmentOrderChanged(section : %1, from %2, to %3)")
        .arg(section).arg(fromIdx).arg(toIdx) << endl;

    //!!! how do we get here? need to involve a command
    emit needUpdate();
}

// Move the position pointer
void
TrackEditor::slotSetPointerPosition(Rosegarden::timeT position)
{

//    RG_DEBUG << "TrackEditor::setPointerPosition: time is " << position << endl;
    if (!m_pointer) return;

    m_pointer->setSize(3, m_segmentCanvas->canvas()->height());

    double canvasPosition = m_rulerScale->getXForTime(position);
    double distance = (double)canvasPosition - m_pointer->x();

    if (distance < 0.0) distance = -distance;
    if (distance >= 1.0) {

	m_pointer->setX(canvasPosition - 1);
        getSegmentCanvas()->slotScrollHoriz((int)canvasPosition);
	emit needUpdate();
    }
}

void
TrackEditor::slotSetLoop(Rosegarden::timeT start, Rosegarden::timeT end)
{
    getTopBarButtons()->getLoopRuler()->slotSetLoopMarker(start, end);
    getBottomBarButtons()->getLoopRuler()->slotSetLoopMarker(start, end);
}

void
TrackEditor::slotSetSelectAdd(bool value)
{
    m_segmentCanvas->slotSetSelectAdd(value);
}

void
TrackEditor::slotSetSelectCopy(bool value)
{
     m_segmentCanvas->slotSetSelectCopy(value);
}

void
TrackEditor::slotSetFineGrain(bool value)
{
     m_segmentCanvas->slotSetFineGrain(value);
}

// Show a Segment as its being recorded
//
void
TrackEditor::slotUpdateRecordingSegmentItem(Rosegarden::Segment *segment)
{
    Composition &comp = m_doc->getComposition();
    //int y = segment->getTrack() * getTrackCellHeight();

    timeT startTime = segment->getStartTime();

    // Show recording SegmentItem from recording start point to
    // current point position
    //
    timeT endTime = comp.getPosition();

    m_segmentCanvas->showRecordingSegmentItem(segment->getTrack(),
                                              startTime, endTime);

    emit needUpdate();
}

void
TrackEditor::slotDeleteRecordingSegmentItem()
{
    m_segmentCanvas->deleteRecordingSegmentItem();
    emit needUpdate();
}


MultiViewCommandHistory*
TrackEditor::getCommandHistory()
{
    return m_doc->getCommandHistory();
}


void
TrackEditor::addCommandToHistory(KCommand *command)
{
    getCommandHistory()->addCommand(command);
}

void
TrackEditor::slotScrollToTrack(int track)
{
    // Find the vertical track pos
    int newY=track * getTrackCellHeight();

    RG_DEBUG << "TrackEditor::scrollToTrack(" << track <<
        ") scrolling to Y " << newY << endl;

    // Scroll the segment view; it will scroll tracks by connected signals
    //    slotVerticalScrollTrackButtons(newY);
    m_segmentCanvas->slotScrollVertSmallSteps(newY);
}

void
TrackEditor::slotDeleteSelectedSegments()
{
    KMacroCommand *macro = new KMacroCommand("Delete Segments");

    Rosegarden::SegmentSelection segments =
            m_segmentCanvas->getSelectedSegments();

    if (segments.size() == 0)
        return;

    Rosegarden::SegmentSelection::iterator it;

    // Clear the selection before erasing the Segments
    // the selection points to
    //
    m_segmentCanvas->clearSelected();

    // Create the compound command
    //
    for (it = segments.begin(); it != segments.end(); it++)
    {
        macro->addCommand(new SegmentEraseCommand(*it));
    }

    addCommandToHistory(macro);

}

void
TrackEditor::slotTurnRepeatingSegmentToRealCopies()
{
    std::cout << "TrackEditor::slotTurnRepeatingSegmentToRealCopies"
              << std::endl;

    Rosegarden::SegmentSelection segments =
            m_segmentCanvas->getSelectedSegments();

    if (segments.size() == 0)
        return;

    QString text;

    if (segments.size() == 1)
        text = i18n("Turn Repeating Segment into Real Copies");
    else
        text = i18n("Turn Repeating Segments into Real Copies");

    KMacroCommand *macro = new KMacroCommand(text);

    Rosegarden::SegmentSelection::iterator it = segments.begin();
    for (; it != segments.end(); it++)
    {
        if ((*it)->isRepeating())
        {
            macro->addCommand(new SegmentRepeatToCopyCommand(*it));
        }
    }

    addCommandToHistory(macro);

}


void
TrackEditor::slotVerticalScrollTrackButtons(int y)
{
    m_trackButtonScroll->setContentsPos(0, y);
}


void TrackEditor::dragEnterEvent(QDragEnterEvent *event)
{
    event->accept(QUriDrag::canDecode(event) ||
                  QTextDrag::canDecode(event));
}

void TrackEditor::dropEvent(QDropEvent* event)
{
    QStrList uri;
    QString text;

    if (QUriDrag::decode(event, uri)) {
        RG_DEBUG << "TrackEditor::dropEvent() : got URI :"
                             << uri.first() << endl;
        QString uriPath = uri.first();
        
        if (uriPath.endsWith(".rg")) {
            emit droppedDocument(uriPath);
        } else {

            int trackPos = m_segmentCanvas->grid().
                getYBin(event->pos().y() + 
                        m_segmentCanvas->verticalScrollBar()->value());

            Rosegarden::timeT time = 
                m_segmentCanvas->grid().getRulerScale()->
                getTimeForX(event->pos().y() + 
                            m_segmentCanvas->horizontalScrollBar()->value());

            if (m_doc->getComposition().getTrackById(trackPos))
            {
                QString audioText;
                QTextOStream t(&audioText);

                t << uriPath;
                t << trackPos;
                t << time;

                emit droppedNewAudio(audioText);
            }

        }
            
    } else if (QTextDrag::decode(event, text)) {
        RG_DEBUG << "TrackEditor::dropEvent() : got text info " << endl;
        //<< text << endl;

        if (text.endsWith(".rg")) {
            emit droppedDocument(text);
        } else {

            QTextIStream s(&text);

            Rosegarden::AudioFileId audioFileId;
            Rosegarden::InstrumentId instrumentId;
            Rosegarden::RealTime startTime, endTime;

            // read the audio info
            s >> audioFileId;
            s >> instrumentId;
            s >> startTime.sec;
            s >> startTime.usec;
            s >> endTime.sec;
            s >> endTime.usec;

            // create a track and

            int trackPos = m_segmentCanvas->grid().
                getYBin(event->pos().y() + 
                        m_segmentCanvas->verticalScrollBar()->value());

            Rosegarden::timeT time = 
                m_segmentCanvas->grid().getRulerScale()->
                getTimeForX(event->pos().y() + 
                            m_segmentCanvas->horizontalScrollBar()->value());

            RG_DEBUG << "TrackEditor::dropEvent() : dropping at track pos = " 
                     << trackPos
                     << ", time = "
                     << time << endl;

            // Drop this audio segment if we have a valid track number
            // (could also check for time limits too)
            //
            if (m_doc->getComposition().getTrackById(trackPos))
            {
                QString audioText;
                QTextOStream t(&audioText);
                t << audioFileId << "\n";
                t << trackPos << "\n"; // track id
                t << time << "\n"; // time on canvas
                t << startTime.sec << "\n";
                t << startTime.usec << "\n";
                t << endTime.sec << "\n";
                t << endTime.usec << "\n";

                emit droppedAudio(audioText);
            }
        }
    }
    
}
