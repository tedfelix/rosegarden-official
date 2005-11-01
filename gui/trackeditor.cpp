// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#include "compositionview.h"
#include "compositionitemhelper.h"
#include "constants.h"
#include "trackeditor.h"
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
#include "segmenttool.h"
#include "qdeferscrollview.h"
#include "rosegardenguiview.h"
#include "sequencemanager.h"

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
                         double initialUnitsPerPixel,
			 QWidget* parent, const char* name,
			 WFlags) :
    DCOPObject("TrackEditorIface"),
    QWidget(parent, name),
    m_doc(doc),
    m_rulerScale(rulerScale),
    m_topBarButtons(0),
    m_bottomBarButtons(0),
    m_trackButtons(0),
    m_segmentCanvas(0),
    m_trackButtonScroll(0),
    m_showTrackLabels(showTrackLabels),
    m_canvasWidth(0),
    m_compositionRefreshStatusId(doc->getComposition().getNewRefreshStatusId()),
    m_playTracking(true),
    m_initialUnitsPerPixel(initialUnitsPerPixel)
{
    // accept dnd
    setAcceptDrops(true);

    init(rosegardenguiview);
    slotReadjustCanvasSize();
}

TrackEditor::~TrackEditor()
{
    delete m_chordNameRuler;
    delete m_compositionModel;
}

void
TrackEditor::init(QWidget* rosegardenguiview)
{
    QGridLayout *grid = new QGridLayout(this, 4, 2);

    int trackLabelWidth = 230;
    int barButtonsHeight = 25;

    m_chordNameRuler = new ChordNameRuler(m_rulerScale,
					  m_doc,
					  0.0,
					  20,
					  this);
    grid->addWidget(m_chordNameRuler, 0, 1);

    m_tempoRuler = new TempoRuler(m_rulerScale,
				  m_doc,
				  0.0,
				  24,
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
    m_compositionModel = new CompositionModelImpl(m_doc->getComposition(),
                                                  m_doc->getStudio(),
                                                  m_rulerScale, getTrackCellHeight());
    
    m_segmentCanvas = new CompositionView(m_doc, m_compositionModel, this);

    kapp->config()->setGroup(Rosegarden::GeneralOptionsConfigGroup);
    if (kapp->config()->readBoolEntry("backgroundtextures", true)) {
	QPixmap background;
	QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
	if (background.load(QString("%1/misc/bg-segmentcanvas.xpm").
			    arg(pixmapDir))) {
	    m_segmentCanvas->viewport()->setBackgroundPixmap(background);
	}
    }

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
    m_trackButtonScroll->setBottomMargin(m_bottomBarButtons->height() +
                                         m_segmentCanvas->horizontalScrollBar()->height());

    connect(m_trackButtons, SIGNAL(widthChanged()),
            this, SLOT(slotTrackButtonsWidthChanged()));

    connect(m_trackButtons, SIGNAL(trackSelected(int)),
            rosegardenguiview, SLOT(slotSelectTrackSegments(int)));

    connect(m_trackButtons, SIGNAL(instrumentSelected(int)),
            rosegardenguiview, SLOT(slotUpdateInstrumentParameterBox(int)));

    connect(this, SIGNAL(stateChange(QString, bool)),
            rosegardenguiview, SIGNAL(stateChange(QString, bool)));

    connect(m_trackButtons, SIGNAL(modified()),
            m_doc, SLOT(slotDocumentModified()));

    connect(m_trackButtons, SIGNAL(muteButton(Rosegarden::TrackId, bool)),
            rosegardenguiview, SLOT(slotSetMuteButton(Rosegarden::TrackId, bool)));

    connect(m_trackButtons, SIGNAL(newRecordButton()),
            m_doc, SLOT(slotNewRecordButton()));

    // connect loop rulers' follow-scroll signals
    connect(m_topBarButtons->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_segmentCanvas, SLOT(startAutoScroll(int)));
    connect(m_topBarButtons->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_segmentCanvas, SLOT(stopAutoScroll()));
    connect(m_bottomBarButtons->getLoopRuler(), SIGNAL(startMouseMove(int)),
            m_segmentCanvas, SLOT(startAutoScroll(int)));
    connect(m_bottomBarButtons->getLoopRuler(), SIGNAL(stopMouseMove()),
            m_segmentCanvas, SLOT(stopAutoScroll()));

    connect(m_segmentCanvas, SIGNAL(contentsMoving(int, int)),
            this, SLOT(slotCanvasScrolled(int, int)));

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
}

void TrackEditor::slotReadjustCanvasSize()
{
    m_segmentCanvas->updateSize();
}

void TrackEditor::slotTrackButtonsWidthChanged()
{
    // We need to make sure the trackButtons geometry is fully updated
    //
    RosegardenProgressDialog::processEvents();

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
        m_trackButtons->slotUpdateTracks();
        m_segmentCanvas->updateContents();

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

void
TrackEditor::slotCanvasScrolled(int x, int y)
{
    if ((m_topBarButtons && m_topBarButtons->getLoopRuler() &&
	 m_topBarButtons->getLoopRuler()->hasActiveMousePress()) ||
	(m_bottomBarButtons && m_bottomBarButtons->getLoopRuler() &&
	 m_bottomBarButtons->getLoopRuler()->hasActiveMousePress())) {

	int mx = m_segmentCanvas->viewport()->mapFromGlobal(QCursor::pos()).x();
        m_segmentCanvas->setPointerPos(x + mx);

        // bad idea, creates a feedback loop
// 	timeT t = m_segmentCanvas->grid().getRulerScale()->getTimeForX(x + mx);
// 	slotSetPointerPosition(t);
    }
}


// Move the position pointer
void
TrackEditor::slotSetPointerPosition(Rosegarden::timeT position)
{
    Rosegarden::SimpleRulerScale *ruler = 
        dynamic_cast<Rosegarden::SimpleRulerScale*>(m_rulerScale);

    if (!ruler) return;

    double pos = m_segmentCanvas->grid().getRulerScale()->getXForTime(position);

    int currentPointerPos = m_segmentCanvas->getPointerPos();

    double distance = pos - currentPointerPos;
    if (distance < 0.0) distance = -distance;

    if (distance >= 1.0) {

        if (m_doc && m_doc->getSequenceManager() &&
            (m_doc->getSequenceManager()->getTransportStatus() != STOPPED)) {

            if (m_playTracking) {
                getSegmentCanvas()->slotScrollHoriz(int(double(position) / ruler->getUnitsPerPixel()));
            }
        } else /*if (!getSegmentCanvas()->isAutoScrolling())*/ {
            int newpos = int(double(position) / ruler->getUnitsPerPixel());
//             RG_DEBUG << "TrackEditor::slotSetPointerPosition("
//                      << position
//                      << ") : calling canvas->slotScrollHoriz() "
//                      << newpos << endl;
            getSegmentCanvas()->doAutoScroll();
            getSegmentCanvas()->slotScrollHorizSmallSteps(newpos);
        }

        m_segmentCanvas->setPointerPos(pos);
    }
    
}

void
TrackEditor::slotToggleTracking()
{
    m_playTracking = !m_playTracking;
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
    m_segmentCanvas->getModel()->clearSelected();

    // Create the compound command
    //
    for (it = segments.begin(); it != segments.end(); it++)
    {
        macro->addCommand(new SegmentEraseCommand(*it,
						  &m_doc->getAudioFileManager()));
    }

    addCommandToHistory(macro);

}

void
TrackEditor::slotTurnRepeatingSegmentToRealCopies()
{
    RG_DEBUG << "TrackEditor::slotTurnRepeatingSegmentToRealCopies" << endl;

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

    int heightAdjust = 0;
    //int widthAdjust = 0;

    // Adjust any drop event height position by visible rulers
    //
    if (m_topBarButtons && m_topBarButtons->isVisible()) 
        heightAdjust += m_topBarButtons->height();

    if (m_tempoRuler && m_tempoRuler->isVisible()) 
        heightAdjust += m_tempoRuler->height();

    if (m_chordNameRuler && m_chordNameRuler->isVisible()) 
        heightAdjust += m_chordNameRuler->height();

    QPoint posInSegmentCanvas = 
	m_segmentCanvas->viewportToContents
	(m_segmentCanvas->
	 viewport()->mapFrom(this, event->pos()));

    int trackPos = m_segmentCanvas->grid().getYBin(posInSegmentCanvas.y());

    Rosegarden::timeT time =
        m_segmentCanvas->grid().getRulerScale()->
            getTimeForX(posInSegmentCanvas.x());
    

    if (QUriDrag::decode(event, uri)) {
        RG_DEBUG << "TrackEditor::dropEvent() : got URI :"
                             << uri.first() << endl;
        QString uriPath = uri.first();
        
        if (uriPath.endsWith(".rg")) {
            emit droppedDocument(uriPath);
        } else {

            QStringList files;
            QUriDrag::decodeLocalFiles(event, files);
            QString filePath = files.first();

            RG_DEBUG << "TrackEditor::dropEvent() : got filename: "
                     << filePath << endl;

            RG_DEBUG << "TrackEditor::dropEvent() : dropping at track pos = " 
                     << trackPos
                     << ", time = "
                     << time 
                     << ", x = "
                     << event->pos().x()
                     << ", mapped x = "
                     << posInSegmentCanvas.x()
                     << endl;

            Rosegarden::Track* track = m_doc->getComposition().getTrackByPosition(trackPos);
            if (track)
            {
                QString audioText;
                QTextOStream t(&audioText);

                t << filePath << "\n";
                t << track->getId() << "\n";
                t << time << "\n";

                emit droppedNewAudio(audioText);
            }

        }
            
    } else if (QTextDrag::decode(event, text)) {
        RG_DEBUG << "TrackEditor::dropEvent() : got text info " << endl;
        //<< text << endl;

        if (text.endsWith(".rg")) {
            emit droppedDocument(text);
            //
            // WARNING
            //
            // DO NOT PERFORM ANY OPERATIONS AFTER THAT
            // EMITTING THIS SIGNAL TRIGGERS THE LOADING OF A NEW DOCUMENT
            // AND AS A CONSEQUENCE THE DELETION OF THIS TrackEditor OBJECT
            //
        } else {

            QTextIStream s(&text);

            QString id;
            Rosegarden::AudioFileId audioFileId;
            Rosegarden::RealTime startTime, endTime;

            // read the audio info checking for end of stream
            s >> id;
            s >> audioFileId;
            s >> startTime.sec;
            s >> startTime.nsec;
            s >> endTime.sec;
            s >> endTime.nsec;

            if (id == "AudioFileManager") { // only create something if this is data from the right client
                

                // Drop this audio segment if we have a valid track number
                // (could also check for time limits too)
                //
                Rosegarden::Track* track = m_doc->getComposition().getTrackByPosition(trackPos);
                if (track) {

                    RG_DEBUG << "TrackEditor::dropEvent() : dropping at track pos = " 
                             << trackPos
                             << ", time = "
                             << time 
                             << ", x = "
                             << event->pos().x()
                             << ", map = "
                             << posInSegmentCanvas.x()
                             << endl;

                    QString audioText;
                    QTextOStream t(&audioText);
                    t << audioFileId << "\n";
                    t << track->getId() << "\n";
                    t << time << "\n"; // time on canvas
                    t << startTime.sec << "\n";
                    t << startTime.nsec << "\n";
                    t << endTime.sec << "\n";
                    t << endTime.nsec << "\n";

                    emit droppedAudio(audioText);
                }

            } else {

                KMessageBox::sorry(this, i18n("You can't drop files into Rosegarden from this client.  Try using Konqueror instead."));

            }

        }

        // SEE WARNING ABOVE - DON'T DO ANYTHING, THIS OBJECT MAY NOT
        // EXIST AT THIS POINT.

    }
}

#include "trackeditor.moc"
