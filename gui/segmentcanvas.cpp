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

#include <qpainter.h>
#include <qpopupmenu.h>
#include <qwhatsthis.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qbitmap.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kapp.h>
#include <kconfig.h>

#include "rosestrings.h"
#include "rosegardenguidoc.h"
#include "segmentcanvas.h"
#include "Segment.h"
#include "Composition.h"
#include "NotationTypes.h"
#include "rosegardenconfigurationpage.h"
#include "BaseProperties.h"

#include "rosedebug.h"
#include "colours.h"
#include "RulerScale.h"

#include <cassert>

using Rosegarden::Segment;
using Rosegarden::SegmentSelection;
using Rosegarden::Note;
using Rosegarden::RulerScale;
using Rosegarden::SnapGrid;
using Rosegarden::TrackId;
using Rosegarden::timeT;

class QCanvasRepeatRectangle : public QCanvasRectangle
{
public:
    QCanvasRepeatRectangle(QCanvas*);

    void setRepeatInterval(unsigned int i) { m_repeatInterval = i; }

    virtual void drawShape(QPainter&);

protected:
    unsigned int m_repeatInterval;
};

QCanvasRepeatRectangle::QCanvasRepeatRectangle(QCanvas* canvas)
    : QCanvasRectangle(canvas),
      m_repeatInterval(0)
{
}

void QCanvasRepeatRectangle::drawShape(QPainter& painter)
{
    QCanvasRectangle::drawShape(painter);

    int pos = (int)x() + m_repeatInterval - 1,
        width = rect().width(),
        height = rect().height();

    while (pos < (width + x())) {
        painter.drawLine(pos, int(y()), pos, int(y()) + height - 1);
        pos += m_repeatInterval;
    }
}


//////////////////////////////////////////////////////////////////////
//                SegmentItem
//////////////////////////////////////////////////////////////////////

QFont *SegmentItem::m_font = 0;
QFontMetrics *SegmentItem::m_fontMetrics = 0;
int SegmentItem::m_fontHeight = 0;

SegmentItem::SegmentItem(TrackId track, timeT startTime, timeT duration,
			 bool showPreview,
                         SnapGrid *snapGrid, QCanvas *canvas,
                         RosegardenGUIDoc *doc) :
    QCanvasRectangle(0, 0, 1, 1, canvas),
    m_segment(0),
    m_doc(doc),
    m_track(track),
    m_startTime(startTime),
    m_duration(duration),
    m_selected(false),
    m_snapGrid(snapGrid),
    m_repeatRectangle(0),
    m_showPreview(showPreview),
    m_previewIsCurrent(false)
{
    if (!m_font) makeFont();

    recalculateRectangle(true);
}

SegmentItem::SegmentItem(Segment *segment,
			 bool showPreview,
                         SnapGrid *snapGrid,
                         QCanvas *canvas,
                         RosegardenGUIDoc *doc) :
    QCanvasRectangle(0, 0, 1, 1, canvas),
    m_segment(segment),
    m_doc(doc),
    m_selected(false),
    m_snapGrid(snapGrid),
    m_repeatRectangle(0),
    m_showPreview(showPreview),
    m_previewIsCurrent(false)
{
    if (!m_font) makeFont();

    recalculateRectangle(true);
}

SegmentItem::~SegmentItem()
{
    if (m_repeatRectangle)
        CanvasItemGC::mark(m_repeatRectangle);
}

void SegmentItem::setShowPreview(bool preview)
{
    m_showPreview = preview;
}

void SegmentItem::makeFont()
{
    m_font = new QFont();
    m_font->setPixelSize(m_snapGrid->getYSnap() / 2);
    m_font->setWeight(QFont::Bold);
    m_font->setItalic(false);
    m_fontMetrics = new QFontMetrics(*m_font);
    m_fontHeight = m_fontMetrics->boundingRect("|^M,g").height();
}

void SegmentItem::drawShape(QPainter& painter)
{
    QCanvasRectangle::drawShape(painter);
    Rosegarden::RulerScale *rulerScale = m_snapGrid->getRulerScale();

    QRect previewRect =
	painter.hasClipping() ?
	painter.clipRegion().boundingRect() :
	painter.viewport();

    previewRect.moveBy(-painter.worldMatrix().dx(),
		       -painter.worldMatrix().dy());

    previewRect = previewRect.intersect(rect());

//    if (previewRect.width() <= 0 || previewRect.height() <= 0) return;

    timeT startTime = rulerScale->getTimeForX(previewRect.x());
    timeT   endTime = rulerScale->getTimeForX(previewRect.x() +
					      previewRect.width());

    if (m_showPreview && m_segment &&
	m_segment->getType() == Rosegarden::Segment::Audio)
    {
        // Draw waveform preview - fetch the audio waveform data as
        // a vector of floats (normalised between -1.0 and +1.0)
        // and then draw over the period we're viewing.
        //
/*
	if (!painter.hasClipping())
	    kdDebug(KDEBUG_AREA) << "SegmentCanvas::drawShape: clipping is off " << endl;
	kdDebug(KDEBUG_AREA) << "SegmentCanvas::drawShape: rect is "
			     << previewRect.width() << "x"
			     << previewRect.height() << " at "
			     << previewRect.x() << ","
			     << previewRect.y() << endl;
*/
        // Set up pen and get rectangle
        painter.setPen(RosegardenGUIColours::SegmentAudioPreview);
        
        // Fetch vector of floats adjusted to our resolution
        //
        Rosegarden::AudioFileManager &aFM = m_doc->getAudioFileManager();
        Rosegarden::Composition &comp = m_doc->getComposition();


        /*
	timeT audioStart = startTime - m_segment->getStartTime();
	timeT audioEnd = endTime - m_segment->getStartTime();

	if (audioEnd > m_segment->getAudioEndTime()) {
	    audioEnd = m_segment->getAudioEndTime();
	}
        */

        std::vector<float> values =
            aFM.getPreview(m_segment->getAudioFileID(),
	                   m_segment->getAudioStartTime(),
	                   m_segment->getAudioEndTime(),
                           previewRect.width());

        std::vector<float>::iterator it;

        // perhaps antialias this somehow at some point
        int i = 0;
        int height = rect().height()/2 - 3;
        for (it = values.begin(); it != values.end(); it++)
        {
            painter.drawLine(previewRect.x() + i,
                             previewRect.y() + rect().height()/2
                                 + (*it) * height,
                             previewRect.x() + i,
                             previewRect.y() + rect().height()/2
                                 - (*it) * height);
            i++;
        }

        // perhaps draw an XOR'd label at some point
        /*
	painter.setPen(RosegardenGUIColours::SegmentLabel);
        painter.setFont(*m_font);
        QRect labelRect = rect();
        labelRect.setX(labelRect.x() + 3);
        painter.drawText(labelRect, Qt::AlignLeft|Qt::AlignVCenter, m_label);
        */
    }
    else
    {
	if (m_showPreview && m_segment) {
            updatePreview();
            painter.drawPixmap(int(x()), int(y()), m_preview);
	}

        // draw label
	painter.setPen(RosegardenGUIColours::SegmentLabel);
        painter.setFont(*m_font);
        QRect labelRect = rect();
        labelRect.setX(labelRect.x() + 3);
        painter.drawText(labelRect, Qt::AlignLeft|Qt::AlignVCenter, m_label);
    }

    // recalculateRectangle(false);
}

void SegmentItem::updatePreview()
{
    if (isPreviewCurrent()) return;

    kdDebug(KDEBUG_AREA) << "SegmentItem::updatePreview() "
                         << this << endl;

    QPainter painter(&m_preview);

    m_preview.fill(black);

    painter.setPen(RosegardenGUIColours::SegmentInternalPreview);
    painter.setBrush(RosegardenGUIColours::SegmentBlock);

    Segment::iterator start = m_segment->begin();
    Segment::iterator end = m_segment->end();
    // if (start == m_segment->end()) start = m_segment->begin();
    //else start = m_segment->findTime((*start)->getAbsoluteTime());

    Rosegarden::RulerScale *rulerScale = m_snapGrid->getRulerScale();

    for (Segment::iterator i = start; i != end; ++i) {

        long pitch = 0;
        if (!(*i)->isa(Rosegarden::Note::EventType) ||
            !(*i)->get<Rosegarden::Int>
            (Rosegarden::BaseProperties::PITCH, pitch)) {
            continue;
        }

        double x0 = rulerScale->getXForTime((*i)->getAbsoluteTime()) - rect().x();
        double x1 = rulerScale->getXForTime((*i)->getAbsoluteTime() +
                                            (*i)->getDuration()) - rect().x();

        int width = (int)(x1 - x0);
        if (width > 0) --width;
        if (width > 0) --width;

        double y0 = 0; // rect().y();
        double y1 = /*rect().y() + */ rect().height();
        double y = y1 + ((y0 - y1) * (pitch-16)) / 96;
        if (y < y0) y = y0;
        if (y > y1-1) y = y1-1;
		
        painter.drawLine((int)x0, (int)y, (int)x0 + width, (int)y);
        painter.drawLine((int)x0, (int)y+1, (int)x0 + width, (int)y+1);

    }

    m_preview.setMask(m_preview.createHeuristicMask());
    
    setPreviewCurrent(true);
}

void SegmentItem::recalculateRectangle(bool inheritFromSegment)
{
    canvas()->setChanged(rect());

    // Compute repeat rectangle if any
    //
    if (m_segment && inheritFromSegment) {

	m_track = m_segment->getTrack();
	m_startTime = m_segment->getStartTime();
#ifdef OLD_SEGMENT_API
	m_duration = m_segment->getDuration();
#else
	m_duration = m_segment->getEndTime() - m_startTime;
#endif
        m_label = strtoqstr(m_segment->getLabel());

	if (m_segment->isRepeating()) {

            if (!m_repeatRectangle)
                m_repeatRectangle = new QCanvasRepeatRectangle(canvas());

	    timeT repeatStart = m_startTime + m_duration;
	    timeT repeatEnd = m_segment->getRepeatEndTime();

	    m_repeatRectangle->setX
		(int(m_snapGrid->getRulerScale()->getXForTime(repeatStart)) + 1);
	    m_repeatRectangle->setY
		(m_snapGrid->getYBinCoordinate(m_track));
	    m_repeatRectangle->setSize
		((int)m_snapGrid->getRulerScale()->getWidthForDuration
		 (repeatStart, repeatEnd - repeatStart) + 1,
		 m_snapGrid->getYSnap());

            m_repeatRectangle->setRepeatInterval(rect().width() - 1);

            m_repeatRectangle->show();

	} else if (m_repeatRectangle) {
            m_repeatRectangle->hide();
        }
        
    }

    // Compute main rectangle
    //
    setX(m_snapGrid->getRulerScale()->getXForTime(m_startTime));
    setY(m_snapGrid->getYBinCoordinate(m_track));

    int h = m_snapGrid->getYSnap();
    double w = m_snapGrid->getRulerScale()->getWidthForDuration
	(m_startTime, m_duration); 

    setSize(int(w) + 1, h);

    // Compute label
    //
    bool dots = false;

    while (m_label.length() > 0 &&
	   (m_fontMetrics->boundingRect
	    (dots ? (m_label + "...") : m_label).width() >
	    width() - 5)) {
	if (!dots && m_label.length() > 6) {
	    m_label.truncate(m_label.length() - 4);
	    dots = true;
	} else if (dots && m_label.length() < 2) {
	    dots = false;
	} else {
	    m_label.truncate(m_label.length() - 1);
	}
    }

    if (dots) m_label += "...";
    canvas()->setChanged(rect());

    m_preview.resize(rect().size());
    setPreviewCurrent(false);
}

Segment* SegmentItem::getSegment() const
{
    return m_segment;
}

void SegmentItem::setSegment(Segment *segment)
{
    m_segment = segment;
    recalculateRectangle(true);
}

void SegmentItem::setStartTime(timeT t)
{
    m_startTime = t;
    recalculateRectangle(false);
}

void SegmentItem::setDuration(timeT d)
{
    m_duration = d;
    recalculateRectangle(false);
}

void SegmentItem::setTrack(TrackId track)
{
    m_track = track;
    recalculateRectangle(false);
}

void SegmentItem::normalize()
{
    if (m_duration < 0) {
	m_duration = -m_duration;
	m_startTime -= m_duration;
	recalculateRectangle(false);
    }
}

// Set this SegmentItem as selected/highlighted - we send
// in the QBrush we need at the same time
//
void SegmentItem::setSelected(bool select, const QBrush &brush)
{
    setBrush(brush);
    m_selected = select;
    setZ(select ? 2 : 1); // selected items come to the front
}

void SegmentItem::showRepeatRect(bool s)
{
    if (!m_repeatRectangle) return;
    
    if (s)
        m_repeatRectangle->show();
    else
        m_repeatRectangle->hide();
}

//////////////////////////////////////////////////////////////////////
////             SegmentSplitLine
//////////////////////////////////////////////////////////////////////
SegmentSplitLine::SegmentSplitLine(int x, int y, int height,
                                   Rosegarden::RulerScale *rulerScale,
                                   QCanvas* canvas):
                                   QCanvasLine(canvas),
                                   m_rulerScale(rulerScale),
                                   m_height(height)
{
    setPen(RosegardenGUIColours::SegmentSplitLine);
    setBrush(RosegardenGUIColours::SegmentSplitLine);
    setZ(3);
    moveLine(x, y);
}

void SegmentSplitLine::moveLine(int x, int y)
{
    setPoints(x, y, x, y + m_height);
    show();
}

void SegmentSplitLine::hideLine()
{
    hide();
}



//////////////////////////////////////////////////////////////////////
//                SegmentCanvas
//////////////////////////////////////////////////////////////////////


SegmentCanvas::SegmentCanvas(RosegardenGUIDoc *doc,
                             RulerScale *rulerScale, QScrollBar* hsb,
                             int vStep,
			     QCanvas* c, QWidget* parent,
			     const char* name, WFlags f) :
    RosegardenCanvasView(hsb, c, parent, name, f),
    m_tool(0),
    m_grid(rulerScale, vStep),
    m_currentItem(0),
    m_recordingSegment(0),
    m_splitLine(0),
    m_brush(RosegardenGUIColours::SegmentBlock),
    m_highlightBrush(RosegardenGUIColours::SegmentHighlightBlock),
    m_pen(RosegardenGUIColours::SegmentBorder),
    m_editMenu(new QPopupMenu(this)),
    m_fineGrain(false),
    m_showPreviews(true),
    m_doc(doc),
    m_config(kapp->config())
{
    QWhatsThis::add(this, i18n("Segments Canvas - Create and manipulate your segments here"));

}

SegmentCanvas::~SegmentCanvas()
{
    // nothing here - canvas items are deleted by the Track Editor
}

void SegmentCanvas::slotSetTool(ToolType t)
{
    kdDebug(KDEBUG_AREA) << "SegmentCanvas::slotSetTool(" << t << ")"
                         << this << "\n";

    if (m_tool)
      delete m_tool;

    m_tool = 0;

    switch(t) {
    case Pencil:
        m_tool = new SegmentPencil(this, m_doc);
        break;
    case Eraser:
        m_tool = new SegmentEraser(this, m_doc);
        break;
    case Mover:
        m_tool = new SegmentMover(this, m_doc);
        break;
    case Resizer:
        m_tool = new SegmentResizer(this, m_doc);
        break;
    case Selector:
        m_tool = new SegmentSelector(this, m_doc);
        break;
    case Splitter:
        m_tool = new SegmentSplitter(this, m_doc);
        break;
    case Joiner:
        m_tool = new SegmentJoiner(this, m_doc);
        break;

    default:
        KMessageBox::error(0, QString("SegmentCanvas::slotSetTool() : unknown tool id %1").arg(t));
    }
}

void SegmentCanvas::updateAllSegmentItems()
{
    // delete repeat rects which need to be deleted
    //
    CanvasItemGC::gc();

    // store the segments we currently show here to speed up
    // determining if new segments were added to the composition
    // 
    std::vector<Segment*> currentSegments;
    bool foundOneSegmentDeleted = false;

    QCanvasItemList l = canvas()->allItems();
    
    for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
        SegmentItem *item = dynamic_cast<SegmentItem*>(*it);
        if (item) {
            Segment* segment = item->getSegment();

            if (!segment->getComposition()) { // this segment has been deleted
		removeFromSelection(segment);
                delete item;
                foundOneSegmentDeleted = true;
            }
            else {
                item->recalculateRectangle(true);
                currentSegments.push_back(segment);
            }
        }
    }

    unsigned int nbSegmentsInComposition = m_doc->getComposition().getNbSegments();
    unsigned int nbCurrentSegments = currentSegments.size();
    
    if ((nbSegmentsInComposition != nbCurrentSegments) ||
        (foundOneSegmentDeleted))
        // if the composition and the segment canvas have the same
        // number of segments, but one of them got deleted they can't
        // be all the same segments, so we also have to look
    {
	using Rosegarden::Composition;

	// check all composition's segments if there's a SegmentItem for it
	//
	const Composition::segmentcontainer& compositionSegments =
	    m_doc->getComposition().getSegments();

	for (Composition::segmentcontainer::const_iterator i =
		 compositionSegments.begin();
	     i != compositionSegments.end(); ++i) {
	    
	    Segment* seg = (*i);
	    
	    if (std::find(currentSegments.begin(),
			  currentSegments.end(), seg) ==
		currentSegments.end()) {
		// found one - update
		addSegmentItem(seg);
	    }
	}
    }

    slotUpdate();
}

void SegmentCanvas::updateSegmentItem(Segment *segment)
{
    SegmentItem *item = findSegmentItem(segment);
    if (!item) {
	addSegmentItem(segment);
    } else {
	item->recalculateRectangle(true);
    }
}

void SegmentCanvas::removeSegmentItem(Segment *segment)
{
    SegmentItem *item = findSegmentItem(segment);
    removeFromSelection(segment);
    delete item;
}

void
SegmentCanvas::removeFromSelection(Segment *segment)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);
    if (!selTool) return;
    selTool->removeFromSelection(segment);
}

void
SegmentCanvas::addToSelection(Segment *segment)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);
    if (!selTool) return;
    selTool->addToSelection(segment);
}


SegmentItem*
SegmentCanvas::findSegmentItem(Rosegarden::Segment *segment)
{
    //!!! slow.
    QCanvasItemList l = canvas()->allItems();
    
    if (l.count()) {
        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
            SegmentItem *item = dynamic_cast<SegmentItem*>(*it);
	    if (item && (item->getSegment() == segment)) {
                return item;
	    }
        }
    }

    return 0;
}

SegmentItem*
SegmentCanvas::findSegmentClickedOn(QPoint pos)
{
    QCanvasItemList l=canvas()->collisions(pos);

    if (l.count()) {
        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it) {
            if (SegmentItem *item = dynamic_cast<SegmentItem*>(*it))
                return item;
        }
    }

    return 0;
}

void SegmentCanvas::contentsMousePressEvent(QMouseEvent* e)
{
    if (e->button() == LeftButton) { // delegate event handling to tool

        // ensure that we have a valid tool
        //
        if (m_tool)
            m_tool->handleMouseButtonPress(e);
        else
            kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMousePressEvent() :"
                                 << this << " no tool\n";

    } else if (e->button() == RightButton) { // popup menu if over a part

        SegmentItem *item = findSegmentClickedOn(e->pos());

        if (item) {
            m_currentItem = item;
            //             kdDebug(KDEBUG_AREA) << "SegmentCanvas::contentsMousePressEvent() : edit m_currentItem = "
            //                                  << m_currentItem << endl;

            if (m_currentItem->getSegment()->getType() == 
                                             Rosegarden::Segment::Audio)
            {
                m_editMenu->clear();
                m_editMenu->insertItem(i18n("Edit Audio"),
                                       this, SLOT(slotOnEditAudio()));
            }
            else
            {
                m_editMenu->clear();
                m_editMenu->insertItem(i18n("Edit as Notation"),
                                       this, SLOT(slotOnEditNotation()));

                m_editMenu->insertItem(i18n("Edit as Matrix"),
                                       this, SLOT(slotOnEditMatrix()));
            }

            m_editMenu->exec(QCursor::pos());
        }
    }
}

void SegmentCanvas::contentsMouseDoubleClickEvent(QMouseEvent* e)
{
    SegmentItem *item = findSegmentClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
        // TODO : edit style should be user configurable

        if (m_currentItem->getSegment()->getType() == Rosegarden::Segment::Audio)
            emit editSegmentAudio(m_currentItem->getSegment());
        else
        {
            if (m_config->readUnsignedNumEntry("doubleclickclient",
                                               Rosegarden::GeneralConfigurationPage::NotationView) == 
                    Rosegarden::GeneralConfigurationPage::NotationView)
                emit editSegmentNotation(m_currentItem->getSegment());
            else
                emit editSegmentMatrix(m_currentItem->getSegment());
        }
    }
}

void SegmentCanvas::contentsMouseReleaseEvent(QMouseEvent *e)
{
    if (!m_tool) return;

    if (e->button() == LeftButton) m_tool->handleMouseButtonRelease(e);
}

void SegmentCanvas::contentsMouseMoveEvent(QMouseEvent* e)
{
    if (!m_tool) return;

    if (m_tool->handleMouseMove(e)) {
        emit scrollTo(e->pos().x());
    }
}

// Show the split line. This is where we perform Segment splits.
//
void SegmentCanvas::slotShowSplitLine(int x, int y)
{
    if (m_splitLine == 0)
        m_splitLine = new SegmentSplitLine(x, y,
					   m_grid.getYSnap() - 1,
                                           m_grid.getRulerScale(),
                                           canvas());
    else
        m_splitLine->moveLine(x, y);

    slotUpdate();
}

// Hide the split line
//
void SegmentCanvas::slotHideSplitLine()
{
    if (m_splitLine) m_splitLine->hideLine();
    slotUpdate();
}

void SegmentCanvas::slotExternalWheelEvent(QWheelEvent* e)
{
    e->accept();
    wheelEvent(e);
}


SegmentItem *
SegmentCanvas::addSegmentItem(TrackId track, timeT startTime, timeT duration)
{
    SegmentItem *newItem = new SegmentItem
	(track, startTime, duration, m_showPreviews, &m_grid, canvas(), m_doc);

    newItem->setPen(m_pen);
    newItem->setBrush(m_brush);
    newItem->setVisible(true);     
    newItem->setZ(1);           // Segment at Z=1, Pointer at Z=10 [rwb]

    return newItem;
}

SegmentItem *
SegmentCanvas::addSegmentItem(Segment *segment)
{
    SegmentItem *newItem = new SegmentItem
	(segment, m_showPreviews, &m_grid, canvas(), m_doc);

    newItem->setPen(m_pen);
    newItem->setBrush(m_brush);
    newItem->setVisible(true);     
    newItem->setZ(1);           // Segment at Z=1, Pointer at Z=10 [rwb]

    return newItem;
}

void SegmentCanvas::showRecordingSegmentItem(TrackId track,
                                             timeT startTime, timeT duration)
{
    if (m_recordingSegment) {

	m_recordingSegment->setStartTime(startTime);
	m_recordingSegment->setDuration(duration);
	m_recordingSegment->setTrack(track);

    } else {
	
	m_recordingSegment = addSegmentItem(track, startTime, duration);
	m_recordingSegment->
            setPen(RosegardenGUIColours::RecordingSegmentBorder);
        m_recordingSegment->
            setBrush(RosegardenGUIColours::RecordingSegmentBlock);
	m_recordingSegment->setZ(2);
    }
}


void SegmentCanvas::deleteRecordingSegmentItem()
{
    if (m_recordingSegment) {
	m_recordingSegment->setVisible(false);
	delete m_recordingSegment;
	m_recordingSegment = 0;
        canvas()->update();
    }
}



void SegmentCanvas::slotOnEditNotation()
{
    emit editSegmentNotation(m_currentItem->getSegment());
}

void SegmentCanvas::slotOnEditMatrix()
{
    emit editSegmentMatrix(m_currentItem->getSegment());
}

void SegmentCanvas::slotOnEditAudio()
{
    emit editSegmentAudio(m_currentItem->getSegment());
}



// Select a SegmentItem on the canvas according to a
// passed Segment pointer
//
//
void SegmentCanvas::slotSelectSegments(const SegmentSelection &segments)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return;

    QCanvasItemList itemList = canvas()->allItems();
    QCanvasItemList::Iterator it;

    // clear any SegmentItems currently selected
    //
    selTool->clearSelected();

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        SegmentItem* segItem = dynamic_cast<SegmentItem*>(*it);
        
        if (segItem) { 

            for (SegmentSelection::const_iterator segIt = segments.begin();
		 segIt != segments.end(); ++segIt) {

                if (segItem->getSegment() == (*segIt)) {

                    selTool->slotSelectSegmentItem(segItem);
                }
            }
        }
    }
}

// Get a selected Segments if we're using a Selector tool
//
SegmentSelection
SegmentCanvas::getSelectedSegments()
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (selTool)
        return selTool->getSelectedSegments();

    return SegmentSelection();
}

bool
SegmentCanvas::haveSelection()
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return false;
    return (selTool->getSelectedSegments().size() > 0);
}

void
SegmentCanvas::clearSelected()
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (selTool)
        return selTool->clearSelected();
}



// enter/exit selection add mode - this means that the SHIFT key
// (or similar) has been depressed and if we're in Select mode we
// can add Selections to the one we currently have
//
//
void SegmentCanvas::slotSetSelectAdd(const bool &value)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return;

    selTool->setSegmentAdd(value);
}


// enter/exit selection copy mode - this means that the CTRL key
// (or similar) has been depressed and if we're in Select mode we
// can copy the current selection with a click and drag (overrides
// the default movement behaviour for selection).
//
//
void SegmentCanvas::slotSetSelectCopy(const bool &value)
{
    SegmentSelector* selTool = dynamic_cast<SegmentSelector*>(m_tool);

    if (!selTool) return;

    selTool->setSegmentCopy(value);
}


void SegmentCanvas::setSnapGrain(bool fine)
{
    if (m_fineGrain) {
	grid().setSnapTime(SnapGrid::NoSnap);
    } else {
	grid().setSnapTime(fine ? SnapGrid::SnapToBeat : SnapGrid::SnapToBar);
    }
}


void SegmentCanvas::setShowPreviews(bool previews)
{
    QCanvasItemList itemList = canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it) {
        SegmentItem* segItem = dynamic_cast<SegmentItem*>(*it);
	if (segItem) {
	    segItem->setShowPreview(previews);
	    canvas()->setChanged(segItem->rect());
	}
    }

    m_showPreviews = previews;
    slotUpdate();
}


void SegmentCanvas::slotSetFineGrain(bool value)
{
    m_fineGrain = value;
}


// Find a SegmentItem for a given Segment
//
SegmentItem*
SegmentCanvas::getSegmentItem(Rosegarden::Segment *segment)
{
    QCanvasItemList itemList = canvas()->allItems();
    QCanvasItemList::Iterator it;

    for (it = itemList.begin(); it != itemList.end(); ++it)
    {
        SegmentItem* segItem = dynamic_cast<SegmentItem*>(*it);
        
         if (segItem)
         { 
             if (segItem->getSegment() == segment)
                 return segItem;
         }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////
//                 Segment Tools
//////////////////////////////////////////////////////////////////////

SegmentTool::SegmentTool(SegmentCanvas* canvas, RosegardenGUIDoc *doc)
    : m_canvas(canvas),
      m_currentItem(0),
      m_doc(doc)
{
    m_canvas->setCursor(Qt::arrowCursor);
}

SegmentTool::~SegmentTool()
{
}

void
SegmentTool::addCommandToHistory(KCommand *command)
{
    m_doc->getCommandHistory()->addCommand(command);
}


//////////////////////////////
// SegmentPencil
//////////////////////////////

SegmentPencil::SegmentPencil(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d),
      m_newRect(false),
      m_track(0),
      m_startTime(0),
      m_duration(0)
{
    m_canvas->setCursor(Qt::ibeamCursor);
    kdDebug(KDEBUG_AREA) << "SegmentPencil()\n";
}

void SegmentPencil::handleMouseButtonPress(QMouseEvent *e)
{
    m_newRect = false;
    m_currentItem = 0;

    // Check if we're clicking on a rect
    //
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item) return;

    m_canvas->setSnapGrain(false);

    TrackId track = m_canvas->grid().getYBin(e->pos().y());

    // Don't do anything if the user clicked beyond the track buttons
    //
    if (track >= m_doc->getComposition().getNbTracks()) return;

    timeT time = m_canvas->grid().snapX(e->pos().x(), SnapGrid::SnapLeft);
    timeT duration = m_canvas->grid().getSnapTime(e->pos().x());
    if (duration == 0) duration = Note(Note::Shortest).getDuration();
    
    m_currentItem = m_canvas->addSegmentItem(track, time, duration);
    m_newRect = true;
    
    m_canvas->slotUpdate();
}

void SegmentPencil::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;
    m_currentItem->normalize();

    if (m_newRect) {
        SegmentInsertCommand *command =
            new SegmentInsertCommand(m_doc,
                                     m_currentItem->getTrack(),
                                     m_currentItem->getStartTime(),
                                     m_currentItem->getDuration());

        addCommandToHistory(command);
    }

    delete m_currentItem;
    m_currentItem = 0;
    m_newRect = false;
}

bool SegmentPencil::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return false;

    m_canvas->setSnapGrain(false);

    SnapGrid::SnapDirection direction = SnapGrid::SnapRight;
    if (e->pos().x() < m_currentItem->x()) direction = SnapGrid::SnapLeft;

    timeT time = m_canvas->grid().snapX(e->pos().x(), direction);
    timeT duration = time - m_currentItem->getStartTime();

    timeT snap = m_canvas->grid().getSnapTime(e->pos().x());
    if (snap == 0) snap = Note(Note::Shortest).getDuration();

    if ((duration >= 0 && duration <  snap) ||
	(duration <  0 && duration > -snap)) {
	duration = (duration < 0 ? -snap : snap);
    }

    if (direction == SnapGrid::SnapLeft) {
	duration += std::max(m_currentItem->getDuration(), timeT(0));
    }

    m_currentItem->setDuration(duration);
    m_canvas->slotUpdate();
    return true;
}

//////////////////////////////
// SegmentEraser
//////////////////////////////

SegmentEraser::SegmentEraser(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d)
{
    m_canvas->setCursor(Qt::pointingHandCursor);

    kdDebug(KDEBUG_AREA) << "SegmentEraser()\n";
}

void SegmentEraser::handleMouseButtonPress(QMouseEvent *e)
{
    m_currentItem = m_canvas->findSegmentClickedOn(e->pos());
}

void SegmentEraser::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem)
    {
        addCommandToHistory(
                new SegmentEraseCommand(m_currentItem->getSegment()));
    }

    m_canvas->canvas()->update();
    
    m_currentItem = 0;
}

bool SegmentEraser::handleMouseMove(QMouseEvent*)
{
    return false;
}

//////////////////////////////
// SegmentMover
//////////////////////////////

SegmentMover::SegmentMover(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d)
{
    m_canvas->setCursor(Qt::sizeAllCursor);

    kdDebug(KDEBUG_AREA) << "SegmentMover()\n";
}

void SegmentMover::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item) {
        m_currentItem = item;
	m_currentItemStartX = item->x();
	m_clickPoint = e->pos();
        m_currentItem->showRepeatRect(false);
    }
}

void SegmentMover::handleMouseButtonRelease(QMouseEvent*)
{
    if (m_currentItem)
    {
        SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand("Move Segment");

        command->addSegment(m_currentItem->getSegment(),
                            m_currentItem->getStartTime(),
                            m_currentItem->getDuration(),
                            m_currentItem->getTrack());
        addCommandToHistory(command);
        m_currentItem->showRepeatRect(true);
    }

    m_currentItem = 0;
}

bool SegmentMover::handleMouseMove(QMouseEvent *e)
{
    if (m_currentItem) {

	m_canvas->setSnapGrain(true);

	int x = e->pos().x() - m_clickPoint.x();
	m_currentItem->setStartTime(m_canvas->grid().snapX
				    (m_currentItemStartX + x));

	TrackId track = m_canvas->grid().getYBin(e->pos().y());
        m_currentItem->setTrack(track);
        m_canvas->canvas()->update();

	return true;
    }

    return false;
}

//////////////////////////////
// SegmentResizer
//////////////////////////////

SegmentResizer::SegmentResizer(SegmentCanvas *c, RosegardenGUIDoc *d,
			       int edgeThreshold)
    : SegmentTool(c, d),
      m_edgeThreshold(edgeThreshold)
{
    m_canvas->setCursor(Qt::sizeHorCursor);

    kdDebug(KDEBUG_AREA) << "SegmentResizer()\n";
}

void SegmentResizer::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem* item = m_canvas->findSegmentClickedOn(e->pos());

    if (item && cursorIsCloseEnoughToEdge(item, e, m_edgeThreshold)) {
        m_currentItem = item;
    }
}

void SegmentResizer::handleMouseButtonRelease(QMouseEvent*)
{
    if (!m_currentItem) return;
    m_currentItem->normalize();

    // normalisation may mean start time has changed as well as duration
    SegmentReconfigureCommand *command =
                new SegmentReconfigureCommand("Resize Segment");

    command->addSegment(m_currentItem->getSegment(),
                        m_currentItem->getStartTime(),
                        m_currentItem->getDuration(),
                        m_currentItem->getTrack());
    addCommandToHistory(command);

    m_currentItem = 0;
}

bool SegmentResizer::handleMouseMove(QMouseEvent *e)
{
    if (!m_currentItem) return false;

    m_canvas->setSnapGrain(true);

    timeT time = m_canvas->grid().snapX(e->pos().x());
    timeT duration = time - m_currentItem->getStartTime();

    timeT snap = m_canvas->grid().getSnapTime(e->pos().x());
    if (snap == 0) snap = Note(Note::Shortest).getDuration();

    if ((duration > 0 && duration <  snap) ||
	(duration < 0 && duration > -snap)) {
	m_currentItem->setDuration(duration < 0 ? -snap : snap);
    } else {
	m_currentItem->setDuration(duration);
    }

    m_canvas->canvas()->update();
    return true;
}

bool SegmentResizer::cursorIsCloseEnoughToEdge(SegmentItem* p, QMouseEvent* e,
					       int edgeThreshold)
{
    return ( abs(p->rect().x() + p->rect().width() - e->x()) < edgeThreshold);
}

//////////////////////////////
// SegmentSelector (bo!)
//////////////////////////////

SegmentSelector::SegmentSelector(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d),
      m_segmentAddMode(false),
      m_segmentCopyMode(false),
      m_segmentQuickCopyDone(false),
      m_dispatchTool(0)
{
    kdDebug(KDEBUG_AREA) << "SegmentSelector()\n";

    connect(this, SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)),
            c,     SIGNAL(selectedSegments(const Rosegarden::SegmentSelection &)));
}

SegmentSelector::~SegmentSelector()
{
    clearSelected();
    delete m_dispatchTool;
}

void
SegmentSelector::removeFromSelection(Rosegarden::Segment *segment)
{
    for (SegmentItemList::iterator i = m_selectedItems.begin();
	 i != m_selectedItems.end(); ++i) {
	if (i->second->getSegment() == segment) {
	    m_selectedItems.erase(i);
	    return;
	}
    }
}

void
SegmentSelector::addToSelection(Rosegarden::Segment *segment)
{
    SegmentItem *item = m_canvas->getSegmentItem(segment);
    if (!item) return;
    for (SegmentItemList::iterator i = m_selectedItems.begin();
	 i != m_selectedItems.end(); ++i) {
	if (i->second == item) return;
    }
    m_selectedItems.push_back
	(SegmentItemPair(QPoint(int(item->x()), int(item->y())), item));
}

void
SegmentSelector::clearSelected()
{
    // For the moment only clear all selected
    //
    SegmentItemList::iterator it;
    for (it = m_selectedItems.begin();
         it != m_selectedItems.end();
         it++)
    {
        it->second->setSelected(false, m_canvas->getSegmentBrush());
    }

    // now clear the selection
    //
    m_selectedItems.clear();

    // clear the current item
    //
    m_currentItem = 0;

    // send update
    //
    m_canvas->canvas()->update();
}

void
SegmentSelector::handleMouseButtonPress(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    // If we're in segmentAddMode then we don't clear the
    // selection vector
    //
    if (!m_segmentAddMode) {
	clearSelected();
    }

    if (item) {
	
	if (!m_segmentAddMode &&
	    SegmentResizer::cursorIsCloseEnoughToEdge(item, e, 10)) {
	    m_dispatchTool = new SegmentResizer(m_canvas, m_doc, 10);
	    m_dispatchTool->handleMouseButtonPress(e);
	    return;
	}

        m_currentItem = item;
        m_clickPoint = e->pos();
        slotSelectSegmentItem(m_currentItem);

    } else {

	if (!m_segmentAddMode) {
	    m_dispatchTool = new SegmentPencil(m_canvas, m_doc);
	    m_dispatchTool->handleMouseButtonPress(e);
	    return;
	}
    }
 
    // Tell the RosegardenGUIView that we've selected some new Segments -
    // when the list is empty we're just unselecting.
    //
    emit selectedSegments(getSelectedSegments());

}

SegmentSelection
SegmentSelector::getSelectedSegments()
{
    SegmentSelection segments;
    SegmentItemList::iterator it;

    for (it = m_selectedItems.begin();
         it != m_selectedItems.end();
         ++it)
    {
        segments.insert(it->second->getSegment());
    }

    return segments;
}


void
SegmentSelector::slotSelectSegmentItem(SegmentItem *selectedItem)
{
    // If we're selecting a Segment through this method
    // then don't set the m_currentItem
    //
    selectedItem->setSelected(true, m_canvas->getHighlightBrush());
    m_selectedItems.push_back(SegmentItemPair
                 (QPoint((int)selectedItem->x(), (int)selectedItem->y()),
                  selectedItem));
    m_canvas->canvas()->update();
}

void
SegmentSelector::handleMouseButtonRelease(QMouseEvent *e)
{
    if (m_dispatchTool) {
	m_dispatchTool->handleMouseButtonRelease(e);
	delete m_dispatchTool;
	m_dispatchTool = 0;
	m_canvas->setCursor(Qt::arrowCursor);
	return;
    }

    if (!m_currentItem) return;
    m_canvas->setCursor(Qt::arrowCursor);

    if (m_currentItem->isSelected())
    {
	SegmentItemList::iterator it;

	bool haveChange = false;

	SegmentReconfigureCommand *command =
	    new SegmentReconfigureCommand
	    (m_selectedItems.size() == 1 ? "Move Segment" :
	                                   "Move Segments");

	for (it = m_selectedItems.begin();
	     it != m_selectedItems.end();
	     it++)
	{
	    SegmentItem *item = it->second;

	    if (item->getStartTime() != item->getSegment()->getStartTime() ||
#ifdef OLD_SEGMENT_API
		item->getDuration()  != item->getSegment()->getDuration() ||
#else
		item->getDuration()  != (item->getSegment()->getEndTime() -
					 item->getSegment()->getStartTime()) ||
#endif
		item->getTrack()     != item->getSegment()->getTrack()) {

		command->addSegment(item->getSegment(),
				    item->getStartTime(),
				    item->getDuration(),
				    item->getTrack());

		haveChange = true;
	    }
	}

	if (haveChange) addCommandToHistory(command);
	m_canvas->canvas()->update();
    }
    
    // if we've just finished a quick copy then drop the Z level back
    if (m_segmentQuickCopyDone)
    {
        m_segmentQuickCopyDone = false;
//        m_currentItem->setZ(2); // see SegmentItem::setSelected  --??
    }

    m_currentItem = 0;
}

// In Select mode we implement movement on the Segment
// as movement _of_ the Segment - as with SegmentMover
//
bool
SegmentSelector::handleMouseMove(QMouseEvent *e)
{
    if (m_dispatchTool) {
	return m_dispatchTool->handleMouseMove(e);
    }

    if (!m_currentItem) return false;
    m_canvas->setCursor(Qt::sizeAllCursor);

    if (m_segmentCopyMode && !m_segmentQuickCopyDone)
    {
	SegmentQuickCopyCommand *command =
            new SegmentQuickCopyCommand(m_currentItem->getSegment());
        // addCommand to generate the new Segment
        //
        addCommandToHistory(command);

        Rosegarden::Segment *newSegment = command->getCopy();

        // generate SegmentItem
        //
	m_canvas->updateAllSegmentItems();
        m_segmentQuickCopyDone = true;

        // Don't understand why swapping selected item is causing
        // problem hereafter - so leaving this commented out.
        //
        //SegmentItem *newItem = m_canvas->getSegmentItem(newSegment);
        //clearSelected();
        //m_currentItem = newItem;
        //m_currentItem->setZ(3); // bring it to the top
        //slotSelectSegmentItem(newItem);
    }

    m_canvas->setSnapGrain(true);

    if (m_currentItem->isSelected())
    {
	SegmentItemList::iterator it;
	
	for (it = m_selectedItems.begin();
	     it != m_selectedItems.end();
	     it++)
	{
	    int x = e->pos().x() - m_clickPoint.x(),
		y = e->pos().y() - m_clickPoint.y();
	    it->second->setStartTime(m_canvas->grid().snapX(it->first.x() + x));

	    TrackId track = m_canvas->grid().getYBin(it->first.y() + y);

            // Make sure we don't set a non-existing track
            // TODO: make this suck less. Either the tool should
            // not allow it in the first place, or we automatically
            // create new tracks - might make undo very tricky though
            //
            if (track >= m_doc->getComposition().getNbTracks()) 
                track = m_doc->getComposition().getNbTracks() - 1;

	    it->second->setTrack(track);
	}

	m_canvas->canvas()->update();
    }

    return true;
}


//////////////////////////////
//
// SegmentSplitter
//
//////////////////////////////


SegmentSplitter::SegmentSplitter(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d)
{
    kdDebug(KDEBUG_AREA) << "SegmentSplitter()\n";
    m_canvas->setCursor(Qt::splitHCursor);
}

SegmentSplitter::~SegmentSplitter()
{
}

void
SegmentSplitter::handleMouseButtonPress(QMouseEvent *e)
{
    // Remove cursor and replace with line on a SegmentItem
    // at where the cut will be made
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item)
    {
        m_canvas->setCursor(Qt::blankCursor);
        drawSplitLine(e);
    }

}

// Actually perform a split if we're on a Segment.
// Return the Segment pointer and the desired split
// time to the Document level.
//
void
SegmentSplitter::handleMouseButtonRelease(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item)
    {
	m_canvas->setSnapGrain(true);

        SegmentSplitCommand *command =
            new SegmentSplitCommand(item->getSegment(),
                                    m_canvas->grid().snapX(e->pos().x()));

        addCommandToHistory(command);
    }
 
    // Reinstate the cursor
    m_canvas->setCursor(Qt::splitHCursor);
    m_canvas->slotHideSplitLine();
}


bool
SegmentSplitter::handleMouseMove(QMouseEvent *e)
{
    SegmentItem *item = m_canvas->findSegmentClickedOn(e->pos());

    if (item)
    {
        m_canvas->setCursor(Qt::blankCursor);
        drawSplitLine(e);
	return true;
    }
    else
    {
        m_canvas->setCursor(Qt::splitHCursor);
        m_canvas->slotHideSplitLine();
	return false;
    }
}

// Draw the splitting line
//
void
SegmentSplitter::drawSplitLine(QMouseEvent *e)
{ 
    m_canvas->setSnapGrain(true);

    // Turn the real X into a snapped X
    //
    timeT xT = m_canvas->grid().snapX(e->pos().x());
    int x = (int)(m_canvas->grid().getRulerScale()->getXForTime(xT));

    // Need to watch y doesn't leak over the edges of the
    // current Segment.
    //
    int y = m_canvas->grid().snapY(e->pos().y());

    m_canvas->slotShowSplitLine(x, y);
}


void
SegmentSplitter::contentsMouseDoubleClickEvent(QMouseEvent*)
{
}


//////////////////////////////
//
// SegmentJoiner
//
//////////////////////////////
SegmentJoiner::SegmentJoiner(SegmentCanvas *c, RosegardenGUIDoc *d)
    : SegmentTool(c, d)
{
    kdDebug(KDEBUG_AREA) << "SegmentJoiner()\n";
}

SegmentJoiner::~SegmentJoiner()
{
}

void
SegmentJoiner::handleMouseButtonPress(QMouseEvent*)
{
}

void
SegmentJoiner::handleMouseButtonRelease(QMouseEvent*)
{
}


bool
SegmentJoiner::handleMouseMove(QMouseEvent*)
{
    return false;
}

void
SegmentJoiner::contentsMouseDoubleClickEvent(QMouseEvent*)
{
}



