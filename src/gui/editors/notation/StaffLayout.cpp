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

#define RG_MODULE_STRING "[StaffLayout]"
#define RG_NO_DEBUG_PRINT 1

#include "StaffLayout.h"

#include "base/Event.h"
#include "base/LayoutEngine.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/SnapGrid.h"
#include "base/ViewElement.h"
#include "base/ViewSegment.h"
#include "gui/editors/notation/BarLineItem.h"
#include "gui/general/GUIPalette.h"
#include "misc/Debug.h"

#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPen>
#include <QRect>
#include <QString>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QtGlobal>

#include <algorithm>


namespace Rosegarden
{

const int pointerWidth = 3;

StaffLayout::StaffLayout(QGraphicsScene *scene, ViewSegment *viewSegment,
                         SnapGrid *snapGrid, int id,
                         int resolution, int lineThickness) :
    m_scene(scene),
    m_viewSegment(viewSegment),
    m_snapGrid(snapGrid),
    m_id(id),
    m_x(0.0),
    m_y(0),
    m_margin(0.0),
    m_titleHeight(0),
    m_resolution(resolution),
    m_lineThickness(lineThickness),
    m_pageMode(LinearMode),
    m_pageWidth(2000.0),  // fairly arbitrary, but we need something non-zero
    m_rowsPerPage(0),
    m_rowSpacing(0),
    m_connectingLineLength(0),
    m_startLayoutX(0),
    m_endLayoutX(0),
    m_current(false)
{
}

StaffLayout::StaffLayout(QGraphicsScene *scene, ViewSegment *viewSegment,
                         SnapGrid *snapGrid,
                         int id, int resolution, int lineThickness,
                         double pageWidth, int rowsPerPage, int rowSpacing) :
    m_scene(scene),
    m_viewSegment(viewSegment),
    m_snapGrid(snapGrid),
    m_id(id),
    m_x(0.0),
    m_y(0),
    m_margin(0.0),
    m_titleHeight(0),
    m_resolution(resolution),
    m_lineThickness(lineThickness),
    m_pageMode(rowsPerPage ? MultiPageMode : ContinuousPageMode),
    m_pageWidth(pageWidth),
    m_rowsPerPage(rowsPerPage),
    m_rowSpacing(rowSpacing),
    m_connectingLineLength(0),
    m_startLayoutX(0),
    m_endLayoutX(0),
    m_current(false)
{
}

StaffLayout::StaffLayout(QGraphicsScene *scene, ViewSegment *viewSegment,
                         SnapGrid *snapGrid,
                         int id, int resolution, int lineThickness,
                         PageMode pageMode, double pageWidth, int rowsPerPage,
                         int rowSpacing) :
    m_scene(scene),
    m_viewSegment(viewSegment),
    m_snapGrid(snapGrid),
    m_id(id),
    m_x(0.0),
    m_y(0),
    m_margin(0.0),
    m_titleHeight(0),
    m_resolution(resolution),
    m_lineThickness(lineThickness),
    m_pageMode(pageMode),
    m_pageWidth(pageWidth),
    m_rowsPerPage(rowsPerPage),
    m_rowSpacing(rowSpacing),
    m_connectingLineLength(0),
    m_startLayoutX(0),
    m_endLayoutX(0),
    m_current(false)
{
}

StaffLayout::~StaffLayout()
{
    deleteBars();
    for (int i = 0; i < (int)m_staffLines.size(); ++i) clearStaffLineRow(i);
}

void
StaffLayout::setResolution(int resolution)
{
    m_resolution = resolution;
}

void
StaffLayout::setLineThickness(int lineThickness)
{
    m_lineThickness = lineThickness;
}

void
StaffLayout::setPageMode(PageMode pageMode)
{
    m_pageMode = pageMode;
}

void
StaffLayout::setPageWidth(double pageWidth)
{
    m_pageWidth = pageWidth;
}

void
StaffLayout::setRowsPerPage(int rowsPerPage)
{
    m_rowsPerPage = rowsPerPage;
}

void
StaffLayout::setRowSpacing(int rowSpacing)
{
    m_rowSpacing = rowSpacing;
}

void
StaffLayout::setConnectingLineLength(int connectingLineLength)
{
    m_connectingLineLength = connectingLineLength;
}

int
StaffLayout::getId() const
{
    return m_id;
}

void
StaffLayout::setX(double x)
{
    m_x = x;
}

double
StaffLayout::getX() const
{
    return m_x;
}

void
StaffLayout::setY(int y)
{
    m_y = y;
}

int
StaffLayout::getY() const
{
    return m_y;
}

void
StaffLayout::setMargin(double margin)
{
    m_margin = margin;
}

double
StaffLayout::getMargin() const
{
    if (m_pageMode != MultiPageMode)
        return 0;
    return m_margin;
}

void
StaffLayout::setTitleHeight(int titleHeight)
{
    m_titleHeight = titleHeight;
}

int
StaffLayout::getTitleHeight() const
{
    return m_titleHeight;
}

double
StaffLayout::getTotalWidth() const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        return getSceneXForRightOfRow(getRowForLayoutX(m_endLayoutX)) - m_x;

    case MultiPageMode:
        return getSceneXForRightOfRow(getRowForLayoutX(m_endLayoutX)) + m_margin - m_x;

    case LinearMode:
    default:
        return getSceneXForLayoutX(m_endLayoutX) - m_x;
    }
}

int
StaffLayout::getTotalHeight() const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        return getSceneYForTopOfStaff(getRowForLayoutX(m_endLayoutX)) +
               getHeightOfRow() - m_y;

    case MultiPageMode:
        return getSceneYForTopOfStaff(m_rowsPerPage - 1) +
               getHeightOfRow() - m_y;

    case LinearMode:
    default:
        return getSceneYForTopOfStaff(0) + getHeightOfRow() - m_y;
    }
}

int
StaffLayout::getHeightOfRow() const
{
    return getTopLineOffset() + getLegerLineCount() * getLineSpacing()
           + getBarLineHeight() + m_lineThickness;
}

bool
StaffLayout::containsSceneCoords(double x, int y) const
{
//    std::cerr << "StaffLayout::containsSceneCoords(" << x << "," << y << ")" << std::endl;

    switch (m_pageMode) {

    case ContinuousPageMode:

        for (int row = getRowForLayoutX(m_startLayoutX);
             row <= getRowForLayoutX(m_endLayoutX); ++row) {

            if (y >= getSceneYForTopOfStaff(row) &&
                y  < getSceneYForTopOfStaff(row) + getHeightOfRow()) {
                return true;
            }
        }

        return false;

    case MultiPageMode:

        for (int row = getRowForLayoutX(m_startLayoutX);
             row <= getRowForLayoutX(m_endLayoutX); ++row) {

            if (y >= getSceneYForTopOfStaff(row) &&
                y  < getSceneYForTopOfStaff(row) + getHeightOfRow() &&
                x >= getSceneXForLeftOfRow(row) &&
                x <= getSceneXForRightOfRow(row)) {
                return true;
            }
        }

        return false;

    case LinearMode:
    default:

        return (y >= getSceneYForTopOfStaff() &&
                y < getSceneYForTopOfStaff() + getHeightOfRow());
    }
}

int
StaffLayout::getSceneYForHeight(int h, double baseX, int baseY) const
{
    int y;

    //    RG_DEBUG << "getSceneYForHeight(" << h << "," << baseY
    //             << ")" << endl;

    if (baseX < 0)
        baseX = getX() + getMargin();

    if (baseY >= 0) {
        y = getSceneYForTopLine(getRowForSceneCoords(baseX, baseY));
    } else {
        y = getSceneYForTopLine();
    }

    y += getLayoutYForHeight(h);

    return y;
}

int
StaffLayout::getLayoutYForHeight(int h) const
{
    int y = ((getTopLineHeight() - h) * getLineSpacing()) / getHeightPerLine();
    if (h < getTopLineHeight() && (h % getHeightPerLine() != 0))
        ++y;

    return y;
}

int
StaffLayout::getWeightedHeightAtSceneCoords(int originalHeight, double x, int y)
{
    RG_DEBUG << "getWeightedHeightAtSceneCoords: originalHeight: "
                   << originalHeight << " non-weighted height: "
                   << getHeightAtSceneCoords(x, y)
                   << endl;
    
    // return the non-weighted height if it already matches (ie. the user
    // clicked pretty close to the center of the note head)
    int nonWeightedHeight = getHeightAtSceneCoords(x, y);

    if (originalHeight == nonWeightedHeight) return nonWeightedHeight;

    // if no match, calculate an approximate height
    int row = getRowForSceneCoords(x, y);
    int approximateHeight = (y - getSceneYForTopLine(row)) * getHeightPerLine() / getLineSpacing();
    approximateHeight = getTopLineHeight() - approximateHeight;

    RG_DEBUG << "approximateHeight: " << approximateHeight
                   << " originalHeight: " << originalHeight;

    int difference = approximateHeight - originalHeight;
    if (difference < 0) difference *= -1;

    // the approximate height is very coarse, so let's try using it as a rough
    // measure of how far the new height differs from the original, and return
    // the original if it's inside the range of "close enough"
    if (difference > 1) return nonWeightedHeight;
    else return originalHeight;
}

int
StaffLayout::getHeightAtSceneCoords(double x, int y) const
{
    //!!! the lazy route: approximate, then get the right value
    // by calling getSceneYForHeight a few times... ugh

    //    RG_DEBUG << "\nNotationStaff::heightOfYCoord: y = " << y
    //                         << ", getTopLineOffset() = " << getTopLineOffset()
    //                         << ", getLineSpacing() = " << m_npf->getLineSpacing()
    //                         << endl;

    if (x < 0)
        x = getX() + getMargin();

    int row = getRowForSceneCoords(x, y);
    int ph = (y - getSceneYForTopLine(row)) * getHeightPerLine() /
             getLineSpacing();
    ph = getTopLineHeight() - ph;

    int i;
    int mi = -2;
    int md = getLineSpacing() * 2;

    int testi = -2;
    int testMd = 1000;

    for (i = -1; i <= 1; ++i) {
        int d = y - getSceneYForHeight(ph + i, x, y);
        if (d < 0) {
            d = -d;
        }
        if (d < md) {
            md = d;
            mi = i;
        }
        if (d < testMd) {
            testMd = d;
            testi = i;
        }
    }

    if (mi > -2) {
        //         RG_DEBUG << "StaffLayout::getHeightAtSceneCoords: " << y
        //                              << " -> " << (ph + mi) << " (mi is " << mi << ", distance "
        //                              << md << ")" << endl;
        //         if (mi == 0) {
        //             RG_DEBUG << "GOOD APPROXIMATION";
        //         } else {
        //             RG_DEBUG << "BAD APPROXIMATION";
        //         }
        return ph + mi;
    } else {
        RG_DEBUG << "StaffLayout::getHeightAtSceneCoords: heuristic got " << ph << ", nothing within range (closest was " << (ph + testi) << " which is " << testMd << " away)";
        return 0;
    }
}

QRect
StaffLayout::getBarExtents(double x, int y) const
{
    int row = getRowForSceneCoords(x, y);

    RG_DEBUG << "getBarExtents(" << x << "," << y << "), row " << row << ", have " << m_barLines.size() << " bar records";

    for (BarLineList::const_iterator i = m_barLines.begin();
         i != m_barLines.end(); ++i) {

        BarLineItem *line = *i;

        double layoutX = line->getLayoutX();
        int barRow = getRowForLayoutX(layoutX);

        RG_DEBUG << "bar layoutX " << layoutX << ", row " << barRow << ", page mode " << m_pageMode << ", x " << line->x();

        if (m_pageMode != LinearMode && (barRow < row)) continue;

        if (line->x() <= x) continue;
        if (i == m_barLines.begin()) continue;

        BarLineList::const_iterator j = i;
        --j;
        BarLineItem *prevline = *j;

        QRect r = QRect(int(prevline->x()),
                        getSceneYForTopOfStaff(barRow),
                        int(line->x() - prevline->x()),
                        getHeightOfRow());
        RG_DEBUG << "Returning rect " << r;
        return r;
    }

    // failure
    return QRect(int(getX() + getMargin()), getSceneYForTopOfStaff(),
                 4, getHeightOfRow());
}

double
StaffLayout::getSceneXForLayoutX(double x) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        return m_x + x - (m_pageWidth * getRowForLayoutX(x));

    case MultiPageMode: {
            int pageNo = getRowForLayoutX(x) / getRowsPerPage();
            double cx = m_x + x - (m_pageWidth * getRowForLayoutX(x));
            cx += m_margin + (m_margin * 2 + m_pageWidth) * pageNo;
            return cx;
        }

    case LinearMode:
    default:
        return m_x + x;
    }
}

StaffLayout::StaffLayoutCoords
StaffLayout::getLayoutCoordsForSceneCoords(double x, int y) const
{
    int row = getRowForSceneCoords(x, y);
    return StaffLayoutCoords
           ((row * m_pageWidth) + x - getSceneXForLeftOfRow(row),
            y - getSceneYForTopOfStaff(row));
}

StaffLayout::StaffLayoutCoords
StaffLayout::getSceneCoordsForLayoutCoords(double x, int y) const
{
    int row = getRowForLayoutX(x);
    return StaffLayoutCoords
           (getSceneXForLayoutX(x), getSceneYForTopLine(row) + y);
}

int
StaffLayout::getRowForSceneCoords(double x, int y) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        return ((y - m_y) / m_rowSpacing);

    case MultiPageMode: {
            int px = int(x - m_x - m_margin);
            int pw = int(m_margin * 2 + m_pageWidth);
            if (px < pw)
                y -= m_titleHeight;
            return (getRowsPerPage() * (px / pw)) + ((y - m_y) / m_rowSpacing);
        }

    case LinearMode:
    default:
        return (int)((x - m_x) / m_pageWidth);
    }
}

int
StaffLayout::getSceneYForTopOfStaff(int row) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        if (row <= 0)
            return m_y;
        else
            return m_y + (row * m_rowSpacing);

    case MultiPageMode:
        if (row <= 0)
            return m_y + m_titleHeight;
        else if (row < getRowsPerPage())
            return m_y + ((row % getRowsPerPage()) * m_rowSpacing) + m_titleHeight;
        else
            return m_y + ((row % getRowsPerPage()) * m_rowSpacing);

    case LinearMode:
    default:
        return m_y;
    }
}

double
StaffLayout::getSceneXForLeftOfRow(int row) const
{
    switch (m_pageMode) {

    case ContinuousPageMode:
        return m_x;

    case MultiPageMode:
        return m_x + m_margin +
               (m_margin*2 + m_pageWidth) * (row / getRowsPerPage());

    case LinearMode:
    default:
        return m_x + (row * m_pageWidth);
    }
}

void
StaffLayout::sizeStaff(HorizontalLayoutEngine &layout)
{
    Profiler profiler("StaffLayout::sizeStaff", true);

    deleteBars();
    deleteRepeatedClefsAndKeys();
    deleteTimeSignatures();

    //    RG_DEBUG << "StaffLayout::sizeStaff";

    int lastBar = layout.getLastVisibleBarOnViewSegment(*m_viewSegment);

    double xleft = 0, xright = 0;
    bool haveXLeft = false;

    xright = layout.getBarPosition(lastBar) - 1;

    TimeSignature currentTimeSignature;

    for (int barNo = layout.getFirstVisibleBarOnViewSegment(*m_viewSegment);
            barNo <= lastBar; ++barNo) {

        double x = layout.getBarPosition(barNo);

        if (!haveXLeft) {
            xleft = x;
            haveXLeft = true;
        }

        double timeSigX = 0;
        TimeSignature timeSig;
        bool isNew = layout.getTimeSignaturePosition
            (*m_viewSegment, barNo, timeSig, timeSigX);


        if (isNew && barNo < lastBar) {
            currentTimeSignature = timeSig;
            // Ask for a gray color if the segment is a repetition
            bool grayed = m_viewSegment->getSegment().isTmp();
            insertTimeSignature(timeSigX, currentTimeSignature, grayed);
            RG_DEBUG << "StaffLayout[" << this << "]::sizeStaff: bar no " << barNo << " has time signature at " << timeSigX;
        }

        RG_DEBUG << "StaffLayout::sizeStaff: inserting bar at " << x << " on staff " << this << " (isNew " << isNew << ", timeSigX " << timeSigX << ")";

        bool showBarNo =
            (showBarNumbersEvery() > 0 &&
             ((barNo + 1) % showBarNumbersEvery()) == 0);

        insertBar(x,
                  ((barNo == lastBar) ? 0 :
                   (layout.getBarPosition(barNo + 1) - x)),
                  layout.isBarCorrectOnViewSegment(*m_viewSegment, barNo - 1),
                  currentTimeSignature,
                  barNo,
                  showBarNo);
    }

    m_startLayoutX = xleft;
    m_endLayoutX = xright;

    drawStaffName();
    resizeStaffLines();
}

void
StaffLayout::deleteBars()
{
    for (BarLineList::iterator i = m_barLines.begin();
         i != m_barLines.end(); ++i) {
        delete *i;
    }

    for (LineRecList::iterator i = m_beatLines.begin();
         i != m_beatLines.end(); ++i) {
        delete i->second;
    }

    for (LineRecList::iterator i = m_barConnectingLines.begin();
         i != m_barConnectingLines.end(); ++i) {
        delete i->second;
    }

    for (ItemList::iterator i = m_barNumbers.begin();
         i != m_barNumbers.end(); ++i) {
        delete *i;
    }

    m_barLines.clear();
    m_beatLines.clear();
    m_barConnectingLines.clear();
    m_barNumbers.clear();
}

void
StaffLayout::insertBar(double layoutX, double width, bool isCorrect,
                      const TimeSignature &timeSig,
                      int barNo, bool showBarNo)
{
//    RG_DEBUG << "insertBar: " << layoutX << ", " << width
//                   << ", " << isCorrect << endl;

    int barThickness = m_lineThickness * 5 / 4;

    // hack to ensure the bar line appears on the correct row in
    // notation page layouts, with a conditional to prevent us from
    // moving the bar and beat lines in the matrix
    if (!showBeatLines()) {
        if (width > 0.01) { // not final bar in staff
            layoutX += 1;
        } else {
            layoutX -= 1;
        }
    }

    int row = getRowForLayoutX(layoutX);
    double x = getSceneXForLayoutX(layoutX);
    int y = getSceneYForTopLine(row);

    bool firstBarInRow = false, lastBarInRow = false;

    if (m_pageMode != LinearMode &&
            (getRowForLayoutX(layoutX) >
             getRowForLayoutX(layoutX - getMargin() - 2)))
        firstBarInRow = true;

    if (m_pageMode != LinearMode &&
            width > 0.01 &&  // width == 0 for final bar in staff
            (getRowForLayoutX(layoutX) <
             getRowForLayoutX(layoutX + width + getMargin() + 2)))
        lastBarInRow = true;

    BarStyle style = getBarStyle(barNo);

    if (style == RepeatBothBar && firstBarInRow)
        style = RepeatStartBar;

    if (firstBarInRow)
        insertRepeatedClefAndKey(layoutX, barNo);

    // If we're supposed to be hiding bar lines, we do just that --
    // create them as normal, then hide them.  We can't simply not
    // create them because we rely on this to find bar extents for
    // things like double-click selection in notation.
    bool hidden = false;
    if (style == PlainBar && timeSig.hasHiddenBars())
        hidden = true;

    double inset = 0.0;
    if (style == RepeatStartBar || style == RepeatBothBar) {
        inset = getBarInset(barNo, firstBarInRow);
    }

    BarLineItem *line = new BarLineItem(layoutX, getBarLineHeight(),
                                        barThickness, getLineSpacing(),
                                        (int)inset, style);

    m_scene->addItem(line);
    line->setPos(int(x), y);

    if (isCorrect) {
        line->setColour(GUIPalette::getColour(GUIPalette::BarLine));
    } else {
        line->setColour(GUIPalette::getColour(GUIPalette::BarLineIncorrect));
    }

    line->setZValue( -1);
    if (hidden)
        line->hide();
    else
        line->show();

    // The bar lines have to be in order of layout-x (there's no
    // such interesting stipulation for beat or connecting lines)
    BarLineList::iterator insertPoint = lower_bound
        (m_barLines.begin(), m_barLines.end(), line, compareBars);
    m_barLines.insert(insertPoint, line);

    if (lastBarInRow) {

        double xe = x + width - barThickness;
        style = getBarStyle(barNo + 1);
        if (style == RepeatBothBar)
            style = RepeatEndBar;

        BarLineItem *eline = new BarLineItem(layoutX, getBarLineHeight(),
                                             barThickness, getLineSpacing(),
                                             0, style);
        m_scene->addItem(eline);
        eline->setPos(int(xe) + .5, y + .5);

        eline->setColour(GUIPalette::getColour(GUIPalette::BarLine));

        eline->setZValue( -1);
        if (hidden)
            eline->hide();
        else
            eline->show();

        BarLineList::iterator insertPoint = lower_bound
            (m_barLines.begin(), m_barLines.end(), eline, compareBars);
        m_barLines.insert(insertPoint, eline);
    }

    if (showBarNo) {

        QFont font;
        font.setPixelSize(m_resolution * 3 / 2);
        QFontMetrics metrics(font);
        QString text = QString("%1").arg(barNo + 1);

        QGraphicsSimpleTextItem *barNoText = new QGraphicsSimpleTextItem(text);
        barNoText->setFont(font);
        barNoText->setPos(x, y - metrics.height() - m_resolution * 2);
        barNoText->setZValue( -1);
        m_scene->addItem(barNoText);
        if (hidden)
            barNoText->hide();
        else
            barNoText->show();

        m_barNumbers.push_back(barNoText);
    }

    QGraphicsRectItem *rect = 0;

    if (showBeatLines()) {

        double gridLines; // number of grid lines per bar may be fractional

        // If the snap time is zero we default to beat markers
        //
        if (m_snapGrid && m_snapGrid->getSnapTime(x))
            gridLines = double(timeSig.getBarDuration()) /
                        double(m_snapGrid->getSnapTime(x));
        else
            gridLines = timeSig.getBeatsPerBar();

        double dx = width / gridLines;

        for (int gridLine = hidden ? 0 : 1; gridLine < gridLines; ++gridLine) {

            rect = new QGraphicsRectItem
                (0, 0, barThickness, getBarLineHeight());
            m_scene->addItem(rect);

            rect->setPos(x + gridLine * dx + .5, y + .5);

            double currentGrid = gridLines / double(timeSig.getBeatsPerBar());

            rect->setPen(GUIPalette::getColour(GUIPalette::BeatLine));
            rect->setBrush(GUIPalette::getColour(GUIPalette::BeatLine));

            // Reset to SubBeatLine colour if we're not a beat line - avoid div by zero!
            //
            if (currentGrid > 1.0 && double(gridLine) / currentGrid != gridLine / int(currentGrid)) {
                rect->setPen(GUIPalette::getColour(GUIPalette::SubBeatLine));
                rect->setBrush(GUIPalette::getColour(GUIPalette::SubBeatLine));
            }

            rect->setZValue( -1);
            rect->show(); // show beat lines even if the bar lines are hidden

            LineRec beatLine(layoutX + gridLine * dx, rect);
            m_beatLines.push_back(beatLine);
        }
    }

    if (m_connectingLineLength > 0) {

        rect = new QGraphicsRectItem
               (0, 0, barThickness, m_connectingLineLength);
        m_scene->addItem(rect);

        rect->setPos(x + .5, y + .5);

        rect->setPen(GUIPalette::getColour(GUIPalette::StaffConnectingLine));
        rect->setBrush(GUIPalette::getColour(GUIPalette::StaffConnectingLine));
        rect->setZValue( -3);
        if (hidden)
            rect->hide();
        else
            rect->show();

        LineRec connectingLine(layoutX, rect);
        m_barConnectingLines.push_back(connectingLine);
    }
}

bool
StaffLayout::compareBars(const BarLineItem *barLine1, const BarLineItem *barLine2)
{
    return (barLine1->getLayoutX() < barLine2->getLayoutX());
}

bool
StaffLayout::compareBarToLayoutX(const BarLineItem *barLine1, int x)
{
    return (barLine1->getLayoutX() < x);
}

void
StaffLayout::deleteTimeSignatures()
{
    // default implementation is empty
}

void
StaffLayout::insertTimeSignature(double, const TimeSignature &, bool)
{
    // default implementation is empty
}

void
StaffLayout::deleteRepeatedClefsAndKeys()
{
    // default implementation is empty
}

void
StaffLayout::insertRepeatedClefAndKey(double, int)
{
    // default implementation is empty
}

void
StaffLayout::drawStaffName()
{
    // default implementation is empty
}

void
StaffLayout::resizeStaffLines()
{
    Profiler profiler("StaffLayout::resizeStaffLines");

    int firstRow = getRowForLayoutX(m_startLayoutX);
    int lastRow = getRowForLayoutX(m_endLayoutX);

    RG_DEBUG << "StaffLayout::resizeStaffLines: firstRow "
             << firstRow << ", lastRow " << lastRow
             << " (startLayoutX " << m_startLayoutX
             << ", endLayoutX " << m_endLayoutX << ")" << endl;

    Q_ASSERT(lastRow >= firstRow);

    int i;
    while ((int)m_staffLines.size() <= lastRow) {
        m_staffLines.push_back(ItemList());
        m_staffConnectingLines.push_back(0);
    }

    // Remove all the staff lines that precede the start of the staff

    for (i = 0; i < firstRow; ++i) clearStaffLineRow(i);

    // now i == firstRow

    while (i <= lastRow) {

        double x0;
        double x1;

        if (i == firstRow) {
            x0 = getSceneXForLayoutX(m_startLayoutX);
        } else {
            x0 = getSceneXForLeftOfRow(i);
        }

        if (i == lastRow) {
            x1 = getSceneXForLayoutX(m_endLayoutX);
        } else {
            x1 = getSceneXForRightOfRow(i);
        }

        resizeStaffLineRow(i, x0, x1 - x0);

        ++i;
    }

    // now i == lastRow + 1

    while (i < (int)m_staffLines.size()) clearStaffLineRow(i++);
}

void
StaffLayout::clearStaffLineRow(int row)
{
    for (int h = 0; h < (int)m_staffLines[row].size(); ++h) {
        delete m_staffLines[row][h];
    }
    m_staffLines[row].clear();

    delete m_staffConnectingLines[row];
    m_staffConnectingLines[row] = 0;
}

void
StaffLayout::resizeStaffLineRow(int row, double x, double length)
{
    Profiler profiler("StaffLayout::resizeStaffLineRow");

    //    RG_DEBUG << "StaffLayout::resizeStaffLineRow: row "
    //       << row << ", x " << x << ", length "
    //       << length << endl;


    // If the resolution is 8 or less, we want to reduce the blackness
    // of the staff lines somewhat to make them less intrusive

    int level = 0;
    // int z = 2;
    if (m_resolution < 6) {
        // z = -1;
        level = (9 - m_resolution) * 32;
        if (level > 200)
            level = 200;
    }

    // As a result of a bug Michael reported where the staff changes color after
    // bar N, I decided to check the values. For m_resolution of 4 the level = 160.
    // However, if the previous value was < 12, the line remained black, i.e. level = 0.
    // If the previous value was 12 and it was reduced to 4, the line indeed was
    // gray, but it was UGLY. The reason for this is the notes remain black and
    // there are these gray lines through the notes. Since it doesn't always work,
    // and since it is ugly when it does work - kill it. 6 Nov 2009, Ilan
    level = 0;
    QColor lineColour(level, level, level);

    int h;

    /*!!! No longer really good enough. But we could potentially use the
      bar positions to sort this out
     
        if (m_pageMode && row > 0 && offset == 0.0) {
            offset = (double)m_npf->getBarMargin() / 2;
            length -= offset;
        }
    */

    int y;

    delete m_staffConnectingLines[row];

    if (m_pageMode != LinearMode && m_connectingLineLength > 0.1) {

        // rather arbitrary (dup in insertBar)
        int barThickness = m_resolution / 12 + 1;
        y = getSceneYForTopLine(row);
        QGraphicsRectItem *line = new QGraphicsRectItem
            (int(x + length) + .5, y + .5, barThickness, m_connectingLineLength);
        m_scene->addItem(line);
        line->setPen(GUIPalette::getColour(GUIPalette::StaffConnectingTerminatingLine));
        line->setBrush(GUIPalette::getColour(GUIPalette::StaffConnectingTerminatingLine));
        line->setZValue( -2);
        line->show();
        m_staffConnectingLines[row] = line;

    } else {
        m_staffConnectingLines[row] = 0;
    }

    while ((int)m_staffLines[row].size() <= getLineCount() * m_lineThickness) {
        m_staffLines[row].push_back(0);
    }

    int lineIndex = 0;

    for (h = 0; h < getLineCount(); ++h) {

        y = getSceneYForHeight
            (getBottomLineHeight() + getHeightPerLine() * h,
             x, getSceneYForTopLine(row));

        if (elementsInSpaces()) {
            y -= getLineSpacing() / 2 + 1;
        }

        //      RG_DEBUG << "StaffLayout: drawing line from ("
        //                           << x << "," << y << ") to (" << (x+length-1)
        //                           << "," << y << ")" << endl;

        if (m_lineThickness > 1) {
        
            QGraphicsRectItem *line = dynamic_cast<QGraphicsRectItem *>
                (m_staffLines[row][lineIndex]);

            if (!line) {
                delete m_staffLines[row][lineIndex];
                m_staffLines[row][lineIndex] = line = new QGraphicsRectItem;
                line->setPen(QPen(lineColour, 0));
                line->setBrush(lineColour);
                m_scene->addItem(line);
            }

            line->setRect(int(x) + .5, y + .5, int(length), m_lineThickness);
            line->show();

        } else {
        
            QGraphicsLineItem *line = dynamic_cast<QGraphicsLineItem *>
                (m_staffLines[row][lineIndex]);

            if (!line) {
                delete m_staffLines[row][lineIndex];
                m_staffLines[row][lineIndex] = line = new QGraphicsLineItem;
                line->setPen(QPen(lineColour, 0));
                m_scene->addItem(line);
            }

            line->setLine(int(x) + .5, y + .5, int(x + length) + .5, y + .5);
            line->show();
        }

        ++lineIndex;
    }

    while (lineIndex < (int)m_staffLines[row].size()) {
        delete m_staffLines[row][lineIndex];
        m_staffLines[row][lineIndex] = 0;
        ++lineIndex;
    }
}

void
StaffLayout::setCurrent(bool current)
{
    m_current = current;
}

void
StaffLayout::renderElements(ViewElementList::iterator,
                            ViewElementList::iterator)
{
    // nothing -- we assume rendering will be done by the implementation
    // of positionElements
}

void
StaffLayout::renderAllElements()
{
    renderElements(m_viewSegment->getViewElementList()->begin(),
                   m_viewSegment->getViewElementList()->end());
}

void
StaffLayout::positionAllElements()
{
    positionElements(m_viewSegment->getSegment().getStartTime(),
                     m_viewSegment->getSegment().getEndTime());
}

QRectF
StaffLayout::getSceneArea()
{
    double top, bottom;
    double left, right;
    int firstRow, lastRow;

    switch (m_pageMode) {

    case ContinuousPageMode:

        firstRow = getRowForLayoutX(m_startLayoutX);
        lastRow = getRowForLayoutX(m_endLayoutX);

        if (lastRow != firstRow) {
            left = m_x;
            right = m_x + m_pageWidth;
        } else {
            left = getSceneXForLayoutX(m_startLayoutX);
            right = getSceneXForLayoutX(m_endLayoutX);
        }

        top = getSceneYForTopOfStaff(firstRow);        
        bottom = getSceneYForTopOfStaff(lastRow) + getHeightOfRow();

        break;

    case MultiPageMode:

        firstRow = getRowForLayoutX(m_startLayoutX);
        lastRow = getRowForLayoutX(m_endLayoutX);

        if (lastRow == firstRow) {
            left = getSceneXForLayoutX(m_startLayoutX);
            right = getSceneXForLayoutX(m_endLayoutX);
            top = getSceneYForTopOfStaff(firstRow);        
            bottom = getSceneYForTopOfStaff(lastRow) + getHeightOfRow();
        } else {

            int firstPage = firstRow / getRowsPerPage();
            int lastPage = lastRow / getRowsPerPage();

            if (lastPage == firstPage) {
                left = getSceneXForLeftOfRow(firstRow);
                right = getSceneXForRightOfRow(lastRow);

                top = getSceneYForTopOfStaff(firstRow);
                bottom = getSceneYForTopOfStaff(lastRow) + getHeightOfRow();
            } else {

              /// TODO : Two special cases should be processed here 
              ///          1 - Only one row on the first page
              ///          2 - Only one row on the last page

                left = getSceneXForLeftOfRow(firstRow);
                right = getSceneXForRightOfRow(lastRow);

                top = m_y;
                bottom = m_y + getHeightOfRow()
                         + getRowsPerPage() * m_rowSpacing;
            }
        }

        break;

    case LinearMode:
    default:
      
        left = m_startLayoutX;
        right = m_endLayoutX;
        top = getSceneYForTopOfStaff();
        bottom = top + getHeightOfRow();
    }

    return QRectF(left, top, right - left, bottom - top);
}

}
