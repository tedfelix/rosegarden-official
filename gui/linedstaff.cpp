
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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


template <class T>
LinedStaff<T>::LinedStaff(QCanvas *canvas, Rosegarden::Segment *segment,
			  int id, int resolution, int lineThickness) :
    Rosegarden::Staff<T>(segment),
    m_canvas(canvas),
    m_id(id),
    m_x(0.0),
    m_y(0),
    m_resolution(resolution),
    m_lineThickness(lineThickness),
    m_pageMode(false),
    m_pageWidth(0.0),
    m_rowSpacing(0.0)
{
    // nothing
}

template <class T>
LinedStaff<T>::LinedStaff(QCanvas *canvas, Rosegarden::Segment *segment,
			  int id, int resolution, int lineThickness,
			  double pageWidth, int rowSpacing) :
    Rosegarden::Staff<T>(segment),
    m_canvas(canvas),
    m_id(id),
    m_x(0.0),
    m_y(0),
    m_resolution(resolution),
    m_lineThickness(lineThickness),
    m_pageMode(true),
    m_pageWidth(pageWidth),
    m_rowSpacing(rowSpacing)
{
    // nothing
}

template <class T>
LinedStaff<T>::~LinedStaff()
{
    // nothing yet
}

template <class T>
void
LinedStaff<T>::setResolution(int resolution)
{
    m_resolution = resolution;
}

template <class T>
void
LinedStaff<T>::setLineThickness(int lineThickness)
{
    m_lineThickness = lineThickness;
}

template <class T>
void
LinedStaff<T>::setPageMode(bool pageMode)
{
    m_pageMode = pageMode;
}

template <class T>
void
LinedStaff<T>::setPageWidth(double pageWidth)
{
    m_pageWidth = pageWidth;
}

template <class T>
void
LinedStaff<T>::setRowSpacing(int rowSpacing)
{
    m_rowSpacing = rowSpacing;
}

template <class T>
int
LinedStaff<T>::getId() const
{
    return m_id;
}

template <class T>
void
LinedStaff<T>::setX(double x)
{
    m_x = x;
}

template <class T>
void
LinedStaff<T>::setY(int y)
{
    m_y = y;
}

template <class T>
double
LinedStaff<T>::getTotalWidth() const
{
    if (m_pageMode) {
	return getCanvasXForRightOfRow(getRowForLayoutX(m_endLayoutX)) - m_x;
    } else {
	return getCanvasXForLayoutX(m_endLayoutX) - m_x;
    }
}

template <class T>
int
LinedStaff<T>::getTotalHeight() const
{
    return getCanvasYForTopOfStaff(getRowForLayoutX(m_endLayoutX)) +
	getHeightOfRow() - m_y;
}

template <class T>
int 
LinedStaff<T>::getHeightOfRow() const
{
    return getTopLineOffset() * 2 + getBarLineHeight() + m_lineThickness;
}

template <class T>
bool
LinedStaff<T>::containsY(int y) const
{
    if (m_pageMode) {

	int row;
    
	for (row  = getRowForLayoutX(m_startLayoutX);
	     row <= getRowForLayoutX(m_endLayoutX); ++row) {
	    if (y >= getCanvasYForTopOfStaff(row) &&
		y <  getCanvasYForTopOfStaff(row) + getHeightOfRow()) {
		return true;
	    }
	}

	return false;

    } else {
	
	return (y >= getCanvasYForTopOfStaff() &&
		y <  getCanvasYForTopOfStaff() + getHeightOfRow());
    }
}

template <class T>
int
LinedStaff<T>::getCanvasYForHeight(int height, int baseY) const
{
    int y;
    if (baseY >= 0) {
	y = getCanvasYForTopLine(getRowForCanvasY(baseY));
    } else {
	y = getCanvasYForTopLine();
    }

    y += ((getTopLineHeight() - h) * getLineSpacing()) / getHeightPerLine();
    if (h < getTopLineHeight() && (h % getHeightPerLine() != 0)) ++y;

    return y;
}

template <class T>
int
LinedStaff<T>::getHeightAtCanvasY(int y) const
{
    //!!! the lazy route: approximate, then get the right value
    // by calling getCanvasYForHeight a few times... ugh

//    kdDebug(KDEBUG_AREA) << "\nNotationStaff::heightOfYCoord: y = " << y
//			 << ", getTopLineOffset() = " << getTopLineOffset()
//			 << ", getLineSpacing() = " << m_npf->getLineSpacing()
//			 << endl;

    int row = getRowForCanvasY(y);
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
	kdDebug(KDEBUG_AREA) << "LinedStaff::getHeightAtCanvasY: " << y
			     << " -> " << (ph + mi) << " (mi is " << mi << ", distance "
			     << md << ")" << endl;
	if (mi == 0) {
	    kdDebug(KDEBUG_AREA) << "GOOD APPROXIMATION" << endl;
	} else {
	    kdDebug(KDEBUG_AREA) << "BAD APPROXIMATION" << endl;
	}
	return ph + mi;
    } else {
	kdDebug(KDEBUG_AREA) << "LinedStaff::getHeightAtCanvasY: heuristic got " << ph << ", nothing within range (closest was " << (ph + testi) << " which is " << testMd << " away)" << endl;
	return 0;
    }
}

template <class T>
QRect
LinedStaff<T>::getBarExtents(double x, int y) const
{
    int row = getRowForCanvasY(y);

    for (unsigned int i = 1; i < m_barLines.size(); ++i) {

	int layoutX = m_barLines[i].first;
	int barRow = getRowForLayoutX(layoutX);

	QCanvasLine *line = m_barLines[i].second;

	if (m_pageMode && (barRow < row)) continue;
	if (line->x() <= x) continue;

	return QRect(m_barLines[i-1].second->x(),
		     getCanvasYForTopOfStaff(barRow),
		     line->x() - m_barLines[i-1].second->x(),
		     getHeightOfRow());
    }

    // failure
    return QRect(m_x, getCanvasYForTopOfStaff(), 4, getHeightOfRow());
}

template <class T>
void
LinedStaff<T>::sizeStaff(Rosegarden::HorizontalLayoutEngine<T> &layout)
{
    deleteBars();
    deleteTimeSignatures();

    unsigned int barCount = layout->getBarLineCount(*this);

    int xleft = 0, xright = 0;
    bool haveXLeft = false;

    if (barCount > 0) xright = layout->getBarLineX(*this, barCount - 1);
    else xright = layout->getTotalWidth();
    
    for (unsigned int i = 0; i < barCount; ++i) {

	if (layout->isBarLineVisible(*this, i)) {

	    int x = layout->getBarLineX(*this, i);

	    if (!haveXLeft && layout->isBarLineVisible(*this, i)) {
		xleft = x;
		haveXLeft = true;
	    }

	    insertBar(x, layout->isBarLineCorrect(*this, i));

	    Event *timeSig = layout->getTimeSignatureInBar(*this, i, x);
	    if (timeSig && i < barCount - 1) {
		insertTimeSignature(x, TimeSignature(*timeSig));
	    }
	}
    }

    m_startLayoutX = xleft;
    m_endLayoutX = xright;
    resizeStaffLines();

    //!!! updateRuler();
}

template <class T>
void
LinedStaff<T>::deleteBars()
{
    for (BarLineList::iterator i = m_barLines.begin();
	 i != m_barLines.end(); ++i) {
	delete i->second;
    }

    for (BarLineList::iterator i = m_barConnectingLines.begin();
	 i != m_barConnectingLines.end(); ++i) {
	delete i->second;
    }

    m_barLines.clear();
    m_barConnectingLines.clear();
}

template <class T>
void
LinedStaff<T>::insertBar(int layoutX, bool isCorrect)
{
    //!!! Where to get this from?
    int barThickness = 1;

    for (int i = 0; i < barThickness; ++i) {

        QCanvasLine *line = new QCanvasLine(m_canvas);
	int row = getRowForLayoutX(layoutX);

        line->setPoints(0, 0, 0, getBarLineHeight());
        line->moveBy
	    (getCanvasXForLayoutX(layoutX) + i, getCanvasYForTopLine(row));

        if (correct) line->setPen(QPen(RosegardenGUIColours::BarLine, 1));
        else line->setPen(QPen(RosegardenGUIColours::BarLineIncorrect, 1));
        line->show();
	line->setZ(-1);

	BarLine barLine(layoutX, line);
        BarLineList::iterator insertPoint = lower_bound
	    (m_barLines.begin(), m_barLines.end(), barLine, compareBars);
        m_barLines.insert(insertPoint, barLine);

//!!!
#ifdef NOT_DEFINED_YET
	if (m_connectingLineHeight > 0) {

	    QCanvasLine *connectingLine = new QCanvasLine(canvas());
	    
	    connectingLine->setPoints(0, 0, 0, m_connectingLineHeight);
	    connectingLine->moveBy
		(getCanvasXForLayoutX(layoutX) + i,
		 getCanvasYForTopLine(row) + getBarLineHeight());

	    connectingLine->setPen
		(QPen(RosegardenGUIColours::StaffConnectingLine, 1));
	    connectingLine->setZ(-3);
	    connectingLine->show();

	    BarLine connectingLine(barPos, connectingLine);
	    insertPoint = lower_bound
		(m_barConnectingLines.begin(), m_barConnectingLines.end(),
		 connectingLine, compareBars);
	    m_barConnectingLines.insert(insertPoint, connectingLine);
	}
#endif
    }
}

template <class T>
bool
LinedStaff<T>::compareBars(const BarLine &barLine1, const BarLine &barLine2)
{
    return (barLine1.first < barLine2.first);
}

template <class T>
bool
LinedStaff<T>::compareBarToLayoutX(const BarLine &barLine1, int x)
{
    return (barLine1.first < x);
}

template <class T>
void
LinedStaff<T>::deleteTimeSignatures()
{
    // default implementation is empty
}

template <class T>
void
LinedStaff<T>::insertTimeSignature(int, const Rosegarden::TimeSignature &)
{
    // default implementation is empty
}

template <class T>
void
LinedStaff<T>::resizeStaffLines()
{
    int firstRow = getRowForLayoutX(m_startLayoutX);
    int  lastRow = getRowForLayoutX(m_endLayoutX);
    
    int i;

    while ((int)m_staffLines.size() <= lastRow) {
	m_staffLines.push_back(LineList());
	m_staffConnectingLines.push_back(0);
    }

    // Remove all the staff lines that precede the start of the staff

    for (i = 0; i < firstRow; ++i) clearStaffLineRow(i);

    // now i == firstRow

    while (i <= lastRow) {

	double x0 = 0;
	double x1 = m_pageWidth;

	if (i == firstRow) {
	    x0 = getXForLayoutX(m_startLayoutX);
	}

	if (i == lastRow) {
	    x1 = getXForLayoutX(m_endLayoutX);
	}

	resizeStaffLineRow(i, x0, x1 - x0);

	++i;
    }

    // now i == lastRow + 1

    while (i < (int)m_staffLines.size()) clearStaffLineRow(i++);
}


// m_staffLines[row] must already exist (although it may be empty)

template <class T>
void
LinedStaff<T>::clearStaffLineRow(int row)
{
    for (int h = 0; h < (int)m_staffLines[row].size(); ++h) {
	delete m_staffLines[row][h];
    }
    m_staffLines[row].clear();

    delete m_staffConnectingLines[row];
    m_staffConnectingLines[row] = 0;
}


// m_staffLines[row] must already exist (although it may be empty)

template <class T>
void
LinedStaff<T>::resizeStaffLineRow(int row, double offset, double length)
{
//    kdDebug(KDEBUG_AREA) << "LinedStaff::resizeStaffLineRow: row "
//			 << row << ", offset " << offset << ", length " 
//			 << length << ", pagewidth " << getPageWidth() << endl;


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
/*
    if (m_pageMode && row > 0 && offset == 0.0) {
	offset = (double)m_npf->getBarMargin() / 2;
	length -= offset;
    }
*/
    QCanvasLine *line;
    double lx;
    int ly;

    delete m_staffConnectingLines[row];
    line = 0;
/*
    if (m_pageMode && m_connectingLineHeight > 0.1) {
	line = new QCanvasLine(m_canvas);
	lx = (int)x() + getRowLeftX(row) + offset + length - 1;
	ly = (int)y() + getTopLineOffsetForRow(row);
	line->setPoints(lx, ly, lx,
			ly + getBarLineHeight() + m_connectingLineHeight);
	line->setPen
	    (QPen(RosegardenGUIColours::StaffConnectingTerminatingLine, 1));
	line->setZ(-2);
	line->show();
    }
*/
    m_staffConnectingLines[row] = line;

    while ((int)m_staffLines[row].size() <= getLineCount() * m_lineThickness) {
	m_staffLines[row].push_back(0);
    }

    int lineIndex = 0;

    for (h = 0; h < getLineCount(); ++h) {

	for (j = 0; j < m_lineThickness; ++j) {

	    if (m_staffLines[row][lineIndex] != 0) {
		line = m_staffLines[row][lineIndex];
	    } else {
		line = new QCanvasLine(m_canvas);
	    }

	    lx = getCanvasXForLeftOfRow(row) + offset;
	    ly = getCanvasYForHeight
		(getBottomLineHeight() + getHeightPerLine() * h,
		 getCanvasYForTopLine(row)) + j;

//	    kdDebug(KDEBUG_AREA) << "My coords: " << x() << "," << y()
//				 << "; setting line points to ("
//				 << lx << "," << ly << ") -> ("
//				 << (lx+length-1) << "," << ly << ")" << endl;

	    line->setPoints(lx, ly, lx + length - 1, ly);

//	    if (j > 0) line->setSignificant(false);

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


