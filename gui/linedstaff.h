
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

#ifndef LINED_STAFF_H
#define LINED_STAFF_H

#include "Staff.h"
#include "FastVector.h"

#include <qrect.h>

#include <string>
#include <vector>

namespace Rosegarden { class SnapGrid; class HorizontalLayoutEngine; }
class QCanvas;
class QCanvasLine;
class QCanvasItem;

/**
 * LinedStaffManager is a trivial abstract base for classes that own
 * and position sets of LinedStaffs, as a convenient API to permit
 * clients (such as canvas implementations) to discover which staff
 * lies where.
 * 
 * LinedStaffManager is not used by LinedStaff.
 */

class LinedStaff;

class LinedStaffManager
{
public:
    virtual LinedStaff *getStaffForCanvasCoords(int x, int y) const = 0;
};


/**
 * LinedStaff is a base class for implementations of Staff that
 * display the contents of a Segment on a set of horizontal lines
 * with optional vertical bar lines.  
 * Likely subclasses include the notation and piano-roll staffs.
 *
 * In general, this class handles x coordinates in floating-point,
 * but y-coordinates as integers because of the requirement that
 * staff lines be a precise integral distance apart.
 */

class LinedStaff : public Rosegarden::Staff
{
public:
    typedef std::pair<double, int> LinedStaffCoords;

    enum PageMode {
	LinearMode = 0,
	ContinuousPageMode,
	MultiPageMode
    };

protected:
    /**
     * Create a new LinedStaff for the given Segment, with a
     * linear layout.
     * 
     * \a id is an arbitrary id for the staff in its view,
     *    not used within the LinedStaff implementation but
     *    queryable via getId
     *
     * \a resolution is the number of blank pixels between
     *    staff lines
     *
     * \a lineThickness is the number of pixels thick a
     *    staff line should be
     */
    LinedStaff(QCanvas *, Rosegarden::Segment *, Rosegarden::SnapGrid *,
               int id, int resolution, int lineThickness);

    /**
     * Create a new LinedStaff for the given Segment, with a
     * page layout.
     * 
     * \a id is an arbitrary id for the staff in its view,
     *    not used within the LinedStaff implementation but
     *    queryable via getId
     *
     * \a resolution is the number of blank pixels between
     *    staff lines
     *
     * \a lineThickness is the number of pixels thick a
     *    staff line should be
     *
     * \a pageWidth is the width of a page, to determine
     *    when to break lines for page layout
     *
     * \a pageHeight is the height of a page, or zero for
     *    a single continuous page
     *
     * \a rowSpacing is the distance in pixels between
     *    the tops of consecutive rows on this staff
     */
    LinedStaff(QCanvas *, Rosegarden::Segment *, Rosegarden::SnapGrid *,
               int id, int resolution, int lineThickness,
	       double pageWidth, int pageHeight, int rowSpacing);

    /**
     * Create a new LinedStaff for the given Segment, with
     * either page or linear layout.
     */
    LinedStaff(QCanvas *, Rosegarden::Segment *, Rosegarden::SnapGrid *,
               int id, int resolution, int lineThickness,
	       PageMode pageMode, double pageWidth, int pageHeight, int rowSpacing);

public:
    virtual ~LinedStaff();

protected:
    // Methods required to define the type of staff this is

    /**
     * Returns the number of visible staff lines
     */
    virtual int getLineCount() const = 0;

    /**
     * Returns the number of invisible staff lines
     * to leave space for above (and below) the visible staff
     */
    virtual int getLegerLineCount() const = 0;

    /**
     * Returns the height-on-staff value for
     * the bottom visible staff line (a shorthand means for
     * referring to staff lines)
     */
    virtual int getBottomLineHeight() const = 0;

    /**
     * Returns the difference between the height-on-
     * staff value of one visible staff line and the next one
     * above it
     */
    virtual int getHeightPerLine() const = 0;

    /**
     * Returns the height-on-staff value for the top visible
     * staff line.  This is deliberately not virtual.
     */
    int getTopLineHeight() const {
	return getBottomLineHeight() +
	    (getLineCount() - 1) * getHeightPerLine();
    }

    /**
     * Returns true if elements fill the spaces between lines,
     * false if elements can fall on lines.  If true, the lines
     * will be displaced vertically by half a line spacing.
     */
    virtual bool elementsInSpaces() const {
	return false;
    }

    /**
     * Returns true if the staff should draw a faint vertical line at
     * each beat, in between the (darker) bar lines.
     */
    virtual bool showBeatLines() const {
	return false;
    }

protected:
    /// Subclass may wish to expose this
    virtual void setResolution(int resolution);

    /// Subclass may wish to expose this
    virtual void setLineThickness(int lineThickness);

    /// Subclass may wish to expose this
    virtual void setPageMode(PageMode pageMode);

    /// Subclass may wish to expose this
    virtual void setPageWidth(double pageWidth);

    /// Subclass may wish to expose this
    virtual void setPageHeight(int pageHeight);

    /// Subclass may wish to expose this
    virtual void setRowSpacing(int rowSpacing);

    /// Subclass may wish to expose this.  Default is zero
    virtual void setConnectingLineLength(int length);

public:
    /**
     * Return the id of the staff.  This is only useful to external
     * agents, it isn't used by the LinedStaff itself.
     */
    virtual int getId() const;

    /**
     * Set the canvas x-coordinate of the left-hand end of the staff.
     * This does not move any canvas items that have already been
     * created; it should be called before the sizeStaff/positionElements
     * procedure begins.
     */
    virtual void setX(double x);

    /**
     * Get the canvas x-coordinate of the left-hand end of the staff.
     */
    virtual double getX() const;

    /**
     * Set the canvas y-coordinate of the top of the first staff row.
     * This does not move any canvas items that have already been
     * created; it should be called before the sizeStaff/positionElements
     * procedure begins.
     */
    virtual void setY(int y);

    /**
     * Get the canvas y-coordinate of the top of the first staff row.
     */
    virtual int getY() const;

    /**
     * Set the canvas width of the margin to left and right of the
     * staff on each page (used only in MultiPageMode).  Each staff
     * row will still be pageWidth wide (that is, the margin is in
     * addition to the pageWidth, not included in it).  This does not
     * move any canvas items that have already been created; it should
     * be called before the sizeStaff/positionElements procedure
     * begins.
     */
    virtual void setMargin(double m);
    
    /**
     * Get the canvas width of the left and right margins.
     */
    virtual double getMargin() const;
    
    /**
     * Returns the width of the entire staff after layout.  Call
     * this only after you've done the full sizeStaff/positionElements
     * procedure.
     */
    virtual double getTotalWidth() const;

    /**
     * Returns the height of the entire staff after layout.  Call
     * this only after you've done the full sizeStaff/positionElements
     * procedure.  If there are multiple rows, this will be the
     * height of all rows, including any space between rows that
     * is used to display other staffs.
     */
    virtual int getTotalHeight() const;

    /**
     * Returns the total number of pages used by the staff.
     */
    int getPageCount() const {
	if (m_pageMode != MultiPageMode) return 1;
	else return 1 + (getRowForLayoutX(m_endLayoutX) / getRowsPerPage());
    }

    /**
     * Returns the difference between the y coordinates of
     * neighbouring visible staff lines.  Deliberately non-virtual
     */
    int getLineSpacing() const {
	return m_resolution + m_lineThickness;
    }

    /**
     * Returns the total height of a single staff row, including ruler
     */
    virtual int getHeightOfRow() const;
    
    /**
     * Returns true if the given canvas coordinates fall within
     * (any of the rows of) this staff.  False if they fall in the
     * gap between two rows.
     */
    virtual bool containsCanvasCoords(double canvasX, int canvasY) const; 

    /**
     * Returns the canvas y coordinate of the specified line on the
     * staff.  baseY is a canvas y coordinate somewhere on the
     * correct row, or -1 for the default row.
     */
    virtual int getCanvasYForHeight(int height, int baseY = -1) const;

    /**
     * Returns the y coordinate of the specified line on the
     * staff, relative to the top of the row.
     */
    virtual int getLayoutYForHeight(int height) const;

    /**
     * Returns the height-on-staff value nearest to the given
     * canvas y coordinate.
     */
    virtual int getHeightAtCanvasY(int y) const;

    /**
     * Return the full width, height and origin of the bar containing
     * the given canvas cooordinates.
     */
    virtual QRect getBarExtents(double x, int y) const;

    /**
     * Set whether this is the current staff or not.  A staff that is
     * current will differ visually from non-current staffs.
     * 
     * The owner of the staffs should normally ensure that one staff
     * is current (the default is non-current, even if there only is
     * one staff) and that only one staff is current at once.
     */
    virtual void setCurrent(bool current);

    /**
     * Move the playback pointer to the layout-X coordinate
     * corresponding to the given time, and show it.
     */
    virtual void setPointerPosition
    (Rosegarden::HorizontalLayoutEngine&, Rosegarden::timeT);

    /**
     * Move the playback pointer to the layout-X coordinate
     * corresponding to the given canvas coordinates, and show it.
     */
    virtual void setPointerPosition(double x, int y);

    /**
     * Returns the layout-X coordinate corresponding to the current
     * position of the playback pointer.
     */
    virtual double getLayoutXOfPointer() const;

    /**
     * Hide the playback pointer.
     */
    virtual void hidePointer();

    /**
     * Move the insertion cursor to the layout-X coordinate
     * corresponding to the given time, and show it.
     */
    virtual void setInsertCursorPosition
    (Rosegarden::HorizontalLayoutEngine&, Rosegarden::timeT);

    /**
     * Move the insertion cursor to the layout-X coordinate
     * corresponding to the given canvas coordinates, and show it.
     */
    virtual void setInsertCursorPosition(double x, int y);

    /**
     * Returns the layout-X coordinate corresponding to the current
     * position of the insertion cursor.  Returns -1 if this staff
     * is not current or there is some other problem.
     */
    virtual double getLayoutXOfInsertCursor() const;

    /**
     * Hide the insert cursor.
     */
    virtual void hideInsertCursor();

    /**
     * Query the given horizontal layout object (which is assumed to
     * have just completed its layout procedure) to determine the
     * required extents of the staff and the positions of the bars,
     * and create the bars and staff lines accordingly.  It may be
     * called either before or after renderElements and/or
     * positionElements.
     * 
     * No bars or staff lines will appear unless this method has
     * been called.
     */
    virtual void sizeStaff(Rosegarden::HorizontalLayoutEngine& layout);

    /**
     * Generate or re-generate sprites for all the elements between
     * from and to.  See subclasses for specific detailed comments.
     *
     * A very simplistic staff subclass may choose not to
     * implement this (the default implementation is empty) and to
     * do all the rendering work in positionElements.  If rendering
     * elements is slow, however, it makes sense to do it here
     * because this method may be called less often.
     */
    virtual void renderElements(Rosegarden::ViewElementList::iterator from,
				Rosegarden::ViewElementList::iterator to);

    /**
     * Call renderElements(from, to) on the whole staff.
     */
    virtual void renderAllElements();

    /**
     * Assign suitable coordinates to the elements on the staff
     * between the start and end times, based entirely on the layout
     * X and Y coordinates they were given by the horizontal and
     * vertical layout processes.
     *
     * The implementation is free to render any elements it
     * chooses in this method as well.
     */
    virtual void positionElements(Rosegarden::timeT from,
				  Rosegarden::timeT to) = 0;
 
    /**
     * Call positionElements(from, to) on the whole staff.
     */
    virtual void positionAllElements();
    

public:
    // This should not really be public -- it should be one of the
    // protected methods below -- but we have some code that needs
    // it and hasn't been supplied with a proper way to do without.
    // Please try to avoid calling this method.
    //!!! fix NotationView::doDeferredCursorMove
    double getCanvasXForLayoutX(double x) const;

    // This should not really be public -- it should be one of the
    // protected methods below -- but we have some code that needs
    // it and hasn't been supplied with a proper way to do without.
    // Please try to avoid calling this method.
    //!!! fix NotationView::getStaffForCanvasCoords
    LinedStaffCoords
    getLayoutCoordsForCanvasCoords(double x, int y) const;

    // This should not really be public -- it should be one of the
    // protected methods below -- but we have some code that needs
    // it and hasn't been supplied with a proper way to do without.
    // Please try to avoid calling this method.
    //!!! fix NotationView::doDeferredCursorMove
    LinedStaffCoords
    getCanvasCoordsForLayoutCoords(double x, int y) const;

    // This should not really be public -- it should be one of the
    // protected methods below -- but we have some code that needs
    // it and hasn't been supplied with a proper way to do without.
    // Please try to avoid calling this method.
    //!!! fix NotationView::print
    int getRowSpacing() { return m_rowSpacing; }

protected:

    // Methods that the subclass may (indeed, should) use to convert
    // between the layout coordinates of elements and their canvas
    // coordinates.  These are deliberately not virtual.

    // Note that even linear-layout staffs have multiple rows; their
    // rows all have the same y coordinate but increasing x
    // coordinates, instead of the other way around.  (The only reason
    // for this is that it seems to be more efficient from the QCanvas
    // perspective to create and manipulate many relatively short
    // canvas lines rather than a smaller number of very long ones.)

    int getTopLineOffset() const {
	return getLineSpacing() * getLegerLineCount();
    }

    int getBarLineHeight() const {
	return getLineSpacing() * (getLineCount() - 1) + m_lineThickness;
    }

    int getRowForLayoutX(double x) const {
	return (int)(x / m_pageWidth);
    }

    int getRowForCanvasCoords(double x, int y) const;

    int getCanvasYForTopOfStaff(int row = -1) const;

    int getCanvasYForTopLine(int row = -1) const {
	return getCanvasYForTopOfStaff(row) + getTopLineOffset();
    }

    double getCanvasXForLeftOfRow(int row) const;

    double getCanvasXForRightOfRow(int row) const {
	return getCanvasXForLeftOfRow(row) + m_pageWidth;
    }

    LinedStaffCoords
    getCanvasOffsetsForLayoutCoords(double x, int y) const {
	LinedStaffCoords cc = getCanvasCoordsForLayoutCoords(x, y);
	return LinedStaffCoords(cc.first - x, cc.second - y);
    }

    int getRowsPerPage() const {
	int rows = 0;
	if (m_pageMode == MultiPageMode) {
	    rows = m_pageHeight / m_rowSpacing;
	    if (rows < 1) rows = 1;
	}
	return rows;
    }

protected:
    // Actual implementation methods.  The default implementation
    // shows staff lines, connecting lines (where appropriate) and bar
    // lines, but does not show time signatures.  To see time
    // signatures, override the deleteTimeSignatures and
    // insertTimeSignature methods.

    virtual void resizeStaffLines();
    virtual void clearStaffLineRow(int row);
    virtual void resizeStaffLineRow(int row, double offset, double length);

    virtual void deleteBars();
    virtual void insertBar(double layoutX, double width, bool isCorrect,
			   const Rosegarden::TimeSignature &);

    // The default implementations of the following two are empty.
    virtual void deleteTimeSignatures();
    virtual void insertTimeSignature(double layoutX,
				     const Rosegarden::TimeSignature &);

    // The default implementation of the following is empty.  The
    // subclass is presumed to know what the staff's name is and
    // where to put it; this is simply called at some point during
    // the staff-drawing process.
    virtual void drawStaffName();

    void initCursors();

protected:

    //--------------- Data members ---------------------------------

    QCanvas *m_canvas;
    Rosegarden::SnapGrid *m_snapGrid;

    int	     m_id;

    double   m_x;
    int	     m_y;
    double   m_margin;
    int	     m_resolution;
    int	     m_lineThickness;
    
    PageMode m_pageMode;
    double   m_pageWidth;
    int      m_pageHeight;
    int	     m_rowSpacing;
    int	     m_connectingLineLength;

    double   m_startLayoutX;
    double   m_endLayoutX;

    bool     m_current;

    typedef std::vector<QCanvasItem *> LineList;
    typedef std::vector<LineList> LineMatrix;
    LineMatrix m_staffLines;
    LineList m_staffConnectingLines;

    typedef std::pair<double, QCanvasItem *> BarLine; // layout-x, line
    typedef FastVector<BarLine> BarLineList;
    static bool compareBars(const BarLine &, const BarLine &);
    static bool compareBarToLayoutX(const BarLine &, int);
    BarLineList m_barLines;
    BarLineList m_beatLines;
    BarLineList m_barConnectingLines;

    QCanvasLine *m_pointer;
    QCanvasLine *m_insertCursor;
};

#endif
