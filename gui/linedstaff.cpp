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

#include "linedstaff.h"
#include "colours.h"
#include "rosestrings.h"
#include "rosedebug.h"
#include "SnapGrid.h"
#include "qcanvas.h"
#include "qcolor.h"

#include "LayoutEngine.h"
#include "Profiler.h"

// width of pointer
//
const int pointerWidth = 3;


LinedStaff::LinedStaff(QCanvas *canvas, Rosegarden::Segment *segment,
                          Rosegarden::SnapGrid *snapGrid, int id,
                          int resolution, int lineThickness) :
    Rosegarden::Staff(*segment),
    m_canvas(canvas),
    m_snapGrid(snapGrid),
    m_id(id),
    m_x(0.0),
    m_y(0),
    m_margin(0.0),
    m_resolution(resolution),
    m_lineThickness(lineThickness),
    m_pageMode(LinearMode),
    m_pageWidth(2000.0), // fairly arbitrary, but we need something non-zero
    m_pageHeight(0),
    m_rowSpacing(0),
    m_connectingLineLength(0),
    m_startLayoutX(0),
    m_endLayoutX(0),
    m_current(false),
    m_pointer(new QCanvasLine(canvas)),
    m_insertCursor(new QCanvasLine(canvas))
{
    initCursors();
}

LinedStaff::LinedStaff(QCanvas *canvas, Rosegarden::Segment *segment,
		       Rosegarden::SnapGrid *snapGrid,
		       int id, int resolution, int lineThickness,
		       double pageWidth, int pageHeight, int rowSpacing) :
    Rosegarden::Staff(*segment),
    m_canvas(canvas),
    m_snapGrid(snapGrid),
    m_id(id),
    m_x(0.0),
    m_y(0),
    m_margin(0.0),
    m_resolution(resolution),
    m_lineThickness(lineThickness),
    m_pageMode(pageHeight ? MultiPageMode : ContinuousPageMode),
    m_pageWidth(pageWidth),
    m_pageHeight(pageHeight),
    m_rowSpacing(rowSpacing),
    m_connectingLineLength(0),
    m_startLayoutX(0),
    m_endLayoutX(0),
    m_current(false),
    m_pointer(new QCanvasLine(canvas)),
    m_insertCursor(new QCanvasLine(canvas))
{
    initCursors();
}

LinedStaff::LinedStaff(QCanvas *canvas, Rosegarden::Segment *segment,
		       Rosegarden::SnapGrid *snapGrid,
		       int id, int resolution, int lineThickness,
		       PageMode pageMode, double pageWidth, int pageHeight,
		       int rowSpacing) :
    Rosegarden::Staff(*segment),
    m_canvas(canvas),
    m_snapGrid(snapGrid),
    m_id(id),
    m_x(0.0),
    m_y(0),
    m_margin(0.0),
    m_resolution(resolution),
    m_lineThickness(lineThickness),
    m_pageMode(pageMode),
    m_pageWidth(pageWidth),
    m_pageHeight(pageHeight),
    m_rowSpacing(rowSpacing),
    m_connectingLineLength(0),
    m_startLayoutX(0),
    m_endLayoutX(0),
    m_current(false),
    m_pointer(new QCanvasLine(canvas)),
    m_insertCursor(new QCanvasLine(canvas))
{
    initCursors();
}

LinedStaff::~LinedStaff()
{
    deleteBars();
    for (int i = 0; i < (int)m_staffLines.size(); ++i) clearStaffLineRow(i);
}

void
LinedStaff::initCursors()
{
    QPen pen(RosegardenGUIColours::Pointer);
    pen.setWidth(pointerWidth);

    m_pointer->setPen(pen);
    m_pointer->setBrush(RosegardenGUIColours::Pointer);

    pen.setColor(RosegardenGUIColours::InsertCursor);

    m_insertCursor->setPen(pen);
    m_insertCursor->setBrush(RosegardenGUIColours::InsertCursor);
}

void
LinedStaff::setResolution(int resolution)
{
    m_resolution = resolution;
}

void
LinedStaff::setLineThickness(int lineThickness)
{
    m_lineThickness = lineThickness;
}

void
LinedStaff::setPageMode(PageMode pageMode)
{
    m_pageMode = pageMode;
}

void
LinedStaff::setPageWidth(double pageWidth)
{
    m_pageWidth = pageWidth;
}

void
LinedStaff::setPageHeight(int pageHeight)
{
    m_pageHeight = pageHeight;
}

void
LinedStaff::setRowSpacing(int rowSpacing)
{
    m_rowSpacing = rowSpacing;
}

void
LinedStaff::setConnectingLineLength(int connectingLineLength)
{
    m_connectingLineLength = connectingLineLength;
}

int
LinedStaff::getId() const
{
    return m_id;
}

void
LinedStaff::setX(double x)
{
    m_x = x;
}

double
LinedStaff::getX() const
{
    return m_x;
}

void
LinedStaff::setY(int y)
{
    m_y = y;
}

int 
LinedStaff::getY() const
{
    return m_y;
}

void
LinedStaff::setMargin(double margin)
{
    m_margin = margin;
}

double
LinedStaff::getMargin() const
{
    if (m_pageMode != MultiPageMode) return 0;
    return m_margin;
}

double
LinedStaff::getTotalWidth() const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        return getCanvasXForRightOfRow(getRowForLayoutX(m_endLayoutX)) - m_x;

    case MultiPageMode:
        return getCanvasXForRightOfRow(getRowForLayoutX(m_endLayoutX)) + m_margin - m_x;

    case LinearMode: default:
        return getCanvasXForLayoutX(m_endLayoutX) - m_x;
    }
}

int
LinedStaff::getTotalHeight() const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
	return getCanvasYForTopOfStaff(getRowForLayoutX(m_endLayoutX)) +
	    getHeightOfRow() - m_y;

    case MultiPageMode:
	return m_pageHeight - m_y;

    case LinearMode: default:
	return getCanvasYForTopOfStaff(0) + getHeightOfRow() - m_y;
    }
}

int 
LinedStaff::getHeightOfRow() const
{
    return getTopLineOffset() + getLegerLineCount() * getLineSpacing()
	+ getBarLineHeight() + m_lineThickness;
}

bool
LinedStaff::containsCanvasCoords(double x, int y) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:

        for (int row  = getRowForLayoutX(m_startLayoutX);
             row <= getRowForLayoutX(m_endLayoutX); ++row) {
            if (y >= getCanvasYForTopOfStaff(row) &&
                y <  getCanvasYForTopOfStaff(row) + getHeightOfRow()) {
                return true;
            }
        }

        return false;

    case MultiPageMode:
    
        for (int row  = getRowForLayoutX(m_startLayoutX);
             row <= getRowForLayoutX(m_endLayoutX); ++row) {
            if (y >= getCanvasYForTopOfStaff(row) &&
                y <  getCanvasYForTopOfStaff(row) + getHeightOfRow() &&
		x >= getCanvasXForLeftOfRow(row) &&
		x <= getCanvasXForRightOfRow(row)) {
                return true;
            }
        }

        return false;

    case LinearMode: default:
        
        return (y >= getCanvasYForTopOfStaff() &&
                y <  getCanvasYForTopOfStaff() + getHeightOfRow());
    }
}

int
LinedStaff::getCanvasYForHeight(int h, int baseY) const
{
    int y;
    if (baseY >= 0) {
        y = getCanvasYForTopLine(getRowForCanvasCoords(getX() + getMargin(), baseY));
    } else {
        y = getCanvasYForTopLine();
    }

    y += getLayoutYForHeight(h);

    return y;
}

int
LinedStaff::getLayoutYForHeight(int h) const
{
    int y = ((getTopLineHeight() - h) * getLineSpacing()) / getHeightPerLine();
    if (h < getTopLineHeight() && (h % getHeightPerLine() != 0)) ++y;

    return y;
}

int
LinedStaff::getHeightAtCanvasY(int y) const
{
    //!!! the lazy route: approximate, then get the right value
    // by calling getCanvasYForHeight a few times... ugh

//    RG_DEBUG << "\nNotationStaff::heightOfYCoord: y = " << y
//                         << ", getTopLineOffset() = " << getTopLineOffset()
//                         << ", getLineSpacing() = " << m_npf->getLineSpacing()
//                         << endl;

    int row = getRowForCanvasCoords(getX() + getMargin(), y);
    int ph = (y - getCanvasYForTopLine(row)) * getHeightPerLine() /
        getLineSpacing();
    ph = getTopLineHeight() - ph;

    int i;
    int mi = -2;
    int md = getLineSpacing() * 2;

    int testi = -2;
    int testMd = 1000;

    for (i = -1; i <= 1; ++i) {
        int d = y - getCanvasYForHeight(ph + i, y);
        if (d < 0) d = -d;
        if (d < md) { md = d; mi = i; }
        if (d < testMd) { testMd = d; testi = i; }
    }
    
    if (mi > -2) {
//         RG_DEBUG << "LinedStaff::getHeightAtCanvasY: " << y
//                              << " -> " << (ph + mi) << " (mi is " << mi << ", distance "
//                              << md << ")" << endl;
//         if (mi == 0) {
//             RG_DEBUG << "GOOD APPROXIMATION" << endl;
//         } else {
//             RG_DEBUG << "BAD APPROXIMATION" << endl;
//         }
        return ph + mi;
    } else {
        RG_DEBUG << "LinedStaff::getHeightAtCanvasY: heuristic got " << ph << ", nothing within range (closest was " << (ph + testi) << " which is " << testMd << " away)" << endl;
        return 0;
    }
}

QRect
LinedStaff::getBarExtents(double x, int y) const
{
    int row = getRowForCanvasCoords(x, y);

    for (int i = 1; i < m_barLines.size(); ++i) {

	double layoutX = m_barLines[i].first;
        int barRow = getRowForLayoutX(layoutX);

        QCanvasLine *line = m_barLines[i].second;

        if (m_pageMode != LinearMode && (barRow < row)) continue;
        if (line->x() <= x) continue;

        return QRect(int(m_barLines[i-1].second->x()),
                     getCanvasYForTopOfStaff(barRow),
                     int(line->x() - m_barLines[i-1].second->x()),
                     getHeightOfRow());
    }

    // failure
    return QRect(int(getX() + getMargin()), getCanvasYForTopOfStaff(), 4, getHeightOfRow());
}

double
LinedStaff::getCanvasXForLayoutX(double x) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
	return m_x + x - (m_pageWidth * getRowForLayoutX(x));

    case MultiPageMode:
    {
	int pageNo = getRowForLayoutX(x) / getRowsPerPage();
	double cx = m_x + x - (m_pageWidth * getRowForLayoutX(x));
	cx += m_margin + (m_margin * 2 + m_pageWidth) * pageNo;
	return cx;
    }

    case LinearMode: default:
	return m_x + x;
    }
}  

LinedStaff::LinedStaffCoords
LinedStaff::getLayoutCoordsForCanvasCoords(double x, int y) const
{
    int row = getRowForCanvasCoords(x, y);
    return LinedStaffCoords
	((row * m_pageWidth) + x - getCanvasXForLeftOfRow(row),
	 y - getCanvasYForTopOfStaff(row));
}

LinedStaff::LinedStaffCoords
LinedStaff::getCanvasCoordsForLayoutCoords(double x, int y) const
{
    int row = getRowForLayoutX(x);
    return LinedStaffCoords
	(getCanvasXForLayoutX(x), getCanvasYForTopLine(row) + y);
}

int
LinedStaff::getRowForCanvasCoords(double x, int y) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
	return ((y - m_y) / m_rowSpacing);

    case MultiPageMode:
	return (getRowsPerPage() *
		(int(x - m_x - m_margin) / int(m_margin*2 + m_pageWidth))) +
	    ((y - m_y) / m_rowSpacing);

    case LinearMode: default:
	return (int)((x - m_x) / m_pageWidth);
    }
}

int
LinedStaff::getCanvasYForTopOfStaff(int row) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
	if (row <= 0) return m_y;
	else return m_y + (row * m_rowSpacing);
    
    case MultiPageMode:
	if (row <= 0) return m_y;
	else return m_y + ((row % getRowsPerPage()) * m_rowSpacing);
	
    case LinearMode: default:
	return m_y;
    }
}

double
LinedStaff::getCanvasXForLeftOfRow(int row) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
	return m_x;

    case MultiPageMode:
	return m_x + m_margin +
	    (m_margin*2 + m_pageWidth) * (row / getRowsPerPage());

    case LinearMode: default:
	return m_x + (row * m_pageWidth);
    }
}

void
LinedStaff::sizeStaff(Rosegarden::HorizontalLayoutEngine &layout)
{
    Rosegarden::Profiler profiler("LinedStaff::sizeStaff", true);

    deleteBars();
    deleteTimeSignatures();

//    RG_DEBUG << "LinedStaff::sizeStaff" << endl;

    int lastBar = layout.getLastVisibleBarOnStaff(*this);

    double xleft = 0, xright = 0;
    bool haveXLeft = false;

    xright = layout.getBarPosition(lastBar);

    Rosegarden::TimeSignature currentTimeSignature;

    for (int barNo = layout.getFirstVisibleBarOnStaff(*this);
	 barNo <= lastBar; ++barNo) {

	double x = layout.getBarPosition(barNo);

	if (!haveXLeft) {
	    xleft = x;
	    haveXLeft = true;
	}

	double timeSigX = 0;
	Rosegarden::Event *timeSig =
	    layout.getTimeSignaturePosition(*this, barNo, timeSigX);
	
	if (timeSig && barNo < lastBar) {
	    currentTimeSignature = Rosegarden::TimeSignature(*timeSig);
	    insertTimeSignature(timeSigX, currentTimeSignature);
	}

	RG_DEBUG << "LinedStaff::sizeStaff: inserting bar at " << x << " on staff " << this << endl;
	
	insertBar(x,
		  ((barNo == lastBar) ? 0 :
		   (layout.getBarPosition(barNo + 1) - x)),
		  layout.isBarCorrectOnStaff(*this, barNo - 1),
		  currentTimeSignature);
    }

    m_startLayoutX = xleft;
    m_endLayoutX = xright;

    drawStaffName();
    resizeStaffLines();
}

void
LinedStaff::deleteBars()
{
    for (BarLineList::iterator i = m_barLines.begin();
         i != m_barLines.end(); ++i) {
        delete i->second;
    }

    for (BarLineList::iterator i = m_beatLines.begin();
	 i != m_beatLines.end(); ++i) {
	delete i->second;
    }

    for (BarLineList::iterator i = m_barConnectingLines.begin();
         i != m_barConnectingLines.end(); ++i) {
        delete i->second;
    }

    m_barLines.clear();
    m_beatLines.clear();
    m_barConnectingLines.clear();
}
/*!!!
void
LinedStaff::insertBar(double layoutX, double width, bool isCorrect,
			 const Rosegarden::TimeSignature &timeSig)
{
//    RG_DEBUG << "insertBar: " << layoutX << ", " << width
//			 << ", " << isCorrect << endl;

    // rather arbitrary (dup in resizeStaffLineRow)
    int barThickness = m_resolution / 12 + 1;

    int testRow = getRowForLayoutX(layoutX);
    double testX = getCanvasXForLayoutX(layoutX);
    int starti = 0;

    if (testX < getX() + getMargin() + 2 && testRow > 1) {
	// first bar on new row
	starti = -barThickness;
    }

    for (int i = starti; i < barThickness; ++i) {

	int row = getRowForLayoutX(layoutX + i);
	double x = getCanvasXForLayoutX(layoutX + i);
	int y = getCanvasYForTopLine(row);

        QCanvasLine *line = new QCanvasLine(m_canvas);

        line->setPoints(0, 0, 0, getBarLineHeight());
        line->moveBy(x + i, y);

	if (elementsInSpaces()) {
	    line->moveBy(0, -(getLineSpacing()/2 + 1));
	}

        if (isCorrect) line->setPen(QPen(RosegardenGUIColours::BarLine, 1));
        else line->setPen(QPen(RosegardenGUIColours::BarLineIncorrect, 1));
        line->setZ(-1);
        line->show();

	// The bar lines have to be in order of layout-x (there's no
	// such interesting stipulation for beat or connecting lines)
        BarLine barLine(layoutX, line);
        BarLineList::iterator insertPoint = lower_bound
            (m_barLines.begin(), m_barLines.end(), barLine, compareBars);
        m_barLines.insert(insertPoint, barLine);

	if (showBeatLines() && i == 0) {

	    double beats; // number of grid lines per bar may be fractional

            // If the snap time is zero we default to beat markers
            //
            if (m_snapGrid && m_snapGrid->getSnapTime(x))
                beats = double(timeSig.getBarDuration()) /
		        double(m_snapGrid->getSnapTime(x));
            else
                beats = timeSig.getBeatsPerBar();

	    double dx = width / beats;

            // ensure minimum useful scaling
	    // no! we can't do this, the canvas may scale differently
	    // from how we see it so we don't actually know how far
	    // apart the lines are (and the scale could change at any
	    // time).  besides, if we did do this we'd have to do a
	    // bit more work to reduce the number of beats as well
//            while (dx > 0.0 && dx < 2.0) dx += dx;

	    for (int beat = 1; beat < beats; ++beat) {

		line = new QCanvasLine(m_canvas);

		line->setPoints(0, 0, 0, getBarLineHeight());
		line->moveBy
		    (getCanvasXForLayoutX(layoutX + beat * dx),
		     getCanvasYForTopLine(row));
		
		if (elementsInSpaces()) {
		    line->moveBy(0, -(getLineSpacing()/2 + 1));
		}
		
		line->setPen(QPen(RosegardenGUIColours::BeatLine, 1));
		line->setZ(-1);
		line->show();

		BarLine beatLine(layoutX + beat * dx, line);
		m_beatLines.push_back(beatLine);
	    }
	}

        if (m_connectingLineLength > 0) {

            line = new QCanvasLine(m_canvas);
            
            line->setPoints(0, 0, 0, m_connectingLineLength);
            line->moveBy
                (getCanvasXForLayoutX(layoutX + i),
                 getCanvasYForTopLine(row));

	    if (elementsInSpaces()) {
		line->moveBy(0, -(getLineSpacing()/2 + 1));
	    }

            line->setPen
                (QPen(RosegardenGUIColours::StaffConnectingLine, 1));
            line->setZ(-3);
            line->show();

            BarLine connectingLine(layoutX, line);
	    m_barConnectingLines.push_back(connectingLine);
        }
    }
}
*/

void
LinedStaff::insertBar(double layoutX, double width, bool isCorrect,
		      const Rosegarden::TimeSignature &timeSig)
{
//    RG_DEBUG << "insertBar: " << layoutX << ", " << width
//			 << ", " << isCorrect << endl;

    // rather arbitrary
    int barThickness = m_resolution / 12 + 1;

    int testRow = getRowForLayoutX(layoutX);
    double testX = getCanvasXForLayoutX(layoutX);
    int starti = 0;
/*!!!
    if (testX < getX() + getMargin() + 2 && testRow > 1) {
	// first bar on new row
	starti = -barThickness;
    }
*/

    int row = getRowForLayoutX(layoutX);
    double x = getCanvasXForLayoutX(layoutX);
    int y = getCanvasYForTopLine(row);

    QCanvasRectangle *line = new QCanvasRectangle
	(0, 0, barThickness, getBarLineHeight(), m_canvas);

    if (elementsInSpaces()) {
	line->moveBy(x, y - (getLineSpacing()/2 + 1));
    } else {
	line->moveBy(x, y);
    }

    if (isCorrect) {
	line->setPen(RosegardenGUIColours::BarLine);
	line->setBrush(RosegardenGUIColours::BarLine);
    } else {
	line->setPen(RosegardenGUIColours::BarLineIncorrect);
	line->setBrush(RosegardenGUIColours::BarLineIncorrect);
    }

    line->setZ(-1);
    line->show();

    // The bar lines have to be in order of layout-x (there's no
    // such interesting stipulation for beat or connecting lines)
    BarLine barLine(layoutX, line);
    BarLineList::iterator insertPoint = lower_bound
	(m_barLines.begin(), m_barLines.end(), barLine, compareBars);
    m_barLines.insert(insertPoint, barLine);

    if (showBeatLines()) {

	double beats; // number of grid lines per bar may be fractional

	// If the snap time is zero we default to beat markers
	//
	if (m_snapGrid && m_snapGrid->getSnapTime(x))
	    beats = double(timeSig.getBarDuration()) /
		double(m_snapGrid->getSnapTime(x));
	else
	    beats = timeSig.getBeatsPerBar();

	double dx = width / beats;

	for (int beat = 1; beat < beats; ++beat) {

	    line = new QCanvasRectangle
		(0, 0, barThickness, getBarLineHeight(), m_canvas);
	    
	    if (elementsInSpaces()) {
		line->moveBy(x + beat * dx, y - (getLineSpacing()/2 + 1));
	    } else {
		line->moveBy(x + beat * dx, y);
	    }
		
	    line->setPen(RosegardenGUIColours::BeatLine);
	    line->setBrush(RosegardenGUIColours::BeatLine);
	    line->setZ(-1);
	    line->show();

	    BarLine beatLine(layoutX + beat * dx, line);
	    m_beatLines.push_back(beatLine);
	}
    }
    
    if (m_connectingLineLength > 0) {
	
	line = new QCanvasRectangle
	    (0, 0, barThickness, m_connectingLineLength, m_canvas);
	
	if (elementsInSpaces()) {
	    line->moveBy(x, y - (getLineSpacing()/2 + 1));
	} else {
	    line->moveBy(x, y);
	}
	
	line->setPen(RosegardenGUIColours::StaffConnectingLine);
	line->setBrush(RosegardenGUIColours::StaffConnectingLine);
	line->setZ(-3);
	line->show();
	
	BarLine connectingLine(layoutX, line);
	m_barConnectingLines.push_back(connectingLine);
    }
}

bool
LinedStaff::compareBars(const BarLine &barLine1, const BarLine &barLine2)
{
    return (barLine1.first < barLine2.first);
}

bool
LinedStaff::compareBarToLayoutX(const BarLine &barLine1, int x)
{
    return (barLine1.first < x);
}

void
LinedStaff::deleteTimeSignatures()
{
    // default implementation is empty
}

void
LinedStaff::insertTimeSignature(double, const Rosegarden::TimeSignature &)
{
    // default implementation is empty
}

void
LinedStaff::drawStaffName()
{
    // default implementation is empty
}

void
LinedStaff::resizeStaffLines()
{
    int firstRow = getRowForLayoutX(m_startLayoutX);
    int  lastRow = getRowForLayoutX(m_endLayoutX);

    RG_DEBUG << "LinedStaff::resizeStaffLines: firstRow "
                         << firstRow << ", lastRow " << lastRow
                         << " (startLayoutX " << m_startLayoutX
                         << ", endLayoutX " << m_endLayoutX << ")" <<  endl;

    assert(lastRow >= firstRow);
    
    int i;
    while ((int)m_staffLines.size() <= lastRow) {
        m_staffLines.push_back(LineList());
        m_staffConnectingLines.push_back(0);
    }

    // Remove all the staff lines that precede the start of the staff

    for (i = 0; i < firstRow; ++i) clearStaffLineRow(i);

    // now i == firstRow

    while (i <= lastRow) {

        double x0;
        double x1;

        if (i == firstRow) {
            x0 = getCanvasXForLayoutX(m_startLayoutX);
        } else {
            x0 = getCanvasXForLeftOfRow(i);
        }

        if (i == lastRow) {
            x1 = getCanvasXForLayoutX(m_endLayoutX);
        } else {
            x1 = getCanvasXForRightOfRow(i);
        }

        resizeStaffLineRow(i, x0, x1 - x0);

        ++i;
    }

    // now i == lastRow + 1

    while (i < (int)m_staffLines.size()) clearStaffLineRow(i++);
}


// m_staffLines[row] must already exist (although it may be empty)

void
LinedStaff::clearStaffLineRow(int row)
{
    for (int h = 0; h < (int)m_staffLines[row].size(); ++h) {
        delete m_staffLines[row][h];
    }
    m_staffLines[row].clear();

    delete m_staffConnectingLines[row];
    m_staffConnectingLines[row] = 0;
}


// m_staffLines[row] must already exist (although it may be empty)

void
LinedStaff::resizeStaffLineRow(int row, double x, double length)
{
//    RG_DEBUG << "LinedStaff::resizeStaffLineRow: row "
//                         << row << ", offset " << offset << ", length " 
//                         << length << ", pagewidth " << getPageWidth() << endl;


    // If the resolution is 8 or less, we want to reduce the blackness
    // of the staff lines somewhat to make them less intrusive

    int level = 0;
    int z = 1;
    if (m_resolution < 6) {
        z = -1;
        level = (9 - m_resolution) * 32;
        if (level > 200) level = 200;
    }

    QColor lineColour(level, level, level);

    int h, j;

/*!!! No longer really good enough. But we could potentially use the
  bar positions to sort this out

    if (m_pageMode && row > 0 && offset == 0.0) {
        offset = (double)m_npf->getBarMargin() / 2;
        length -= offset;
    }
*/

    int y;

    delete m_staffConnectingLines[row];

    if (m_pageMode != LinearMode && m_connectingLineLength > 0.1) { //!!! handle multi page
	// rather arbitrary (dup in insertBar)
	int barThickness = m_resolution / 12 + 1;
        y = getCanvasYForTopLine(row);
	QCanvasRectangle *line = new QCanvasRectangle
	    (x + length, y, barThickness, m_connectingLineLength, m_canvas);
	line->setPen(RosegardenGUIColours::StaffConnectingTerminatingLine);
	line->setBrush(RosegardenGUIColours::StaffConnectingTerminatingLine);
        line->setZ(-2);
        line->show();
    } else {
	m_staffConnectingLines[row] = 0;
    }

    while ((int)m_staffLines[row].size() <= getLineCount() * m_lineThickness) {
        m_staffLines[row].push_back(0);
    }

    int lineIndex = 0;

    for (h = 0; h < getLineCount(); ++h) {

        for (j = 0; j < m_lineThickness; ++j) {

	    //!!! use QCanvasRectangle if lineThickness > 1
	    QCanvasLine *line;

            if (m_staffLines[row][lineIndex] != 0) {
                line = (QCanvasLine *)m_staffLines[row][lineIndex];
            } else {
                line = new QCanvasLine(m_canvas);
            }

            y = getCanvasYForHeight
                (getBottomLineHeight() + getHeightPerLine() * h,
                 getCanvasYForTopLine(row)) + j;

	    if (elementsInSpaces()) {
		y -= getLineSpacing()/2 + 1;
	    }

//            RG_DEBUG << "LinedStaff: drawing line from ("
//                                 << x << "," << y << ") to (" << (x+length-1)
//                                 << "," << y << ")" << endl;

            line->setPoints(int(x), y, int(x + length), y);

//            if (j > 0) line->setSignificant(false);

            line->setPen(QPen(lineColour, 1));
            line->setZ(z);

            if (m_staffLines[row][lineIndex] == 0) {
                m_staffLines[row][lineIndex] = line;
            }

            line->show();

            ++lineIndex;
        }
    }

    while (lineIndex < (int)m_staffLines[row].size()) {
        delete m_staffLines[row][lineIndex];
        m_staffLines[row][lineIndex] = 0;
        ++lineIndex;
    }
}    

void
LinedStaff::setCurrent(bool current)
{
    m_current = current;
    if (m_current) {
	m_insertCursor->show();
    } else {
	m_insertCursor->hide();
    }
}

double
LinedStaff::getLayoutXOfPointer() const
{
    double x = m_pointer->x();
    int row = getRowForCanvasCoords(x, int(m_pointer->y()));
    return getLayoutCoordsForCanvasCoords(x, getCanvasYForTopLine(row)).first;
}

double
LinedStaff::getLayoutXOfInsertCursor() const
{
    if (!m_current) return -1;
    double x = m_insertCursor->x();
    int row = getRowForCanvasCoords(x, int(m_insertCursor->y()));
    return getLayoutCoordsForCanvasCoords(x, getCanvasYForTopLine(row)).first;
}

void
LinedStaff::setPointerPosition(double canvasX, int canvasY)
{
    int row = getRowForCanvasCoords(canvasX, canvasY);
    canvasY = getCanvasYForTopOfStaff(row);
    m_pointer->setX(int(canvasX));
    m_pointer->setY(int(canvasY));
    m_pointer->setZ(-30); // behind everything else
    m_pointer->setPoints(0, 0, 0, getHeightOfRow() - 1);
    m_pointer->show();
}

void
LinedStaff::setPointerPosition(Rosegarden::HorizontalLayoutEngine &layout,
				  Rosegarden::timeT time)
{
    double x = layout.getXForTime(time);
    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords(x, 0);
    setPointerPosition(coords.first, coords.second);
}

void
LinedStaff::hidePointer()
{
    m_pointer->hide();
}

void
LinedStaff::setInsertCursorPosition(double canvasX, int canvasY)
{
    if (!m_current) return;
    
    int row = getRowForCanvasCoords(canvasX, canvasY);
    canvasY = getCanvasYForTopOfStaff(row);
    m_insertCursor->setX(canvasX);
    m_insertCursor->setY(canvasY);
    m_insertCursor->setZ(-28); // behind everything else except playback pointer
    m_insertCursor->setPoints(0, 0, 0, getHeightOfRow() - 1);
    m_insertCursor->show();
}

void
LinedStaff::setInsertCursorPosition(Rosegarden::HorizontalLayoutEngine &layout,
				       Rosegarden::timeT time)
{
    double x = layout.getXForTime(time);
    LinedStaffCoords coords = getCanvasCoordsForLayoutCoords(x, 0);
    setInsertCursorPosition(coords.first, coords.second);
}

void
LinedStaff::hideInsertCursor()
{
    m_insertCursor->hide();
}

void
LinedStaff::renderElements(Rosegarden::ViewElementList::iterator,
                           Rosegarden::ViewElementList::iterator)
{
    // nothing -- we assume rendering will be done by the implementation
    // of positionElements
}

void
LinedStaff::renderAllElements()
{
    renderElements(getViewElementList()->begin(),
                   getViewElementList()->end());
}

void
LinedStaff::positionAllElements()
{
    positionElements(getSegment().getStartTime(),
		     getSegment().getEndTime());
}

