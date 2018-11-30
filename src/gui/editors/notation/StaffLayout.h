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

#ifndef RG_STAFFLAYOUT_H
#define RG_STAFFLAYOUT_H

#include "base/Event.h"
#include "base/ViewElement.h"
#include <QRect>
#include <utility>
#include <vector>

class QGraphicsScene;
class QGraphicsLineItem;
class QGraphicsItem;

namespace Rosegarden
{

class BarLineItem;
class TimeSignature;
class SnapGrid;
class ViewSegment;
class HorizontalLayoutEngine;
class Event;


/**
 * StaffLayout is a base for classes that display the contents of a
 * Segment on a set of horizontal lines with optional vertical bar
 * lines.  Possible subclasses include the notation and piano-roll
 * staffs.
 *
 * In general, this class handles x coordinates in floating-point,
 * but y-coordinates as integers because of the requirement that
 * staff lines be a precise integral distance apart.
 */

class StaffLayout
{
public:
    typedef std::pair<double, int> StaffLayoutCoords;

    enum PageMode {
        LinearMode = 0,
        ContinuousPageMode,
        MultiPageMode
    };

    enum BarStyle {
        PlainBar = 0,
        DoubleBar,
        HeavyDoubleBar,
        RepeatEndBar,
        RepeatStartBar,
        RepeatBothBar,
        NoVisibleBar
    };

protected:
    /**
     * Create a new StaffLayout for the given ViewSegment, with a
     * linear layout.
     * 
     * \a id is an arbitrary id for the staff in its view,
     *    not used within the StaffLayout implementation but
     *    queryable via getId
     *
     * \a resolution is the number of blank pixels between
     *    staff lines
     *
     * \a lineThickness is the number of pixels thick a
     *    staff line should be
     */
    StaffLayout(QGraphicsScene *, ViewSegment *, SnapGrid *,
                int id, int resolution, int lineThickness);

    /**
     * Create a new StaffLayout for the given ViewSegment, with a
     * page layout.
     * 
     * \a id is an arbitrary id for the staff in its view,
     *    not used within the StaffLayout implementation but
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
     * \a rowsPerPage is the number of rows to a page, or zero
     *    for a single continuous page
     *
     * \a rowSpacing is the distance in pixels between
     *    the tops of consecutive rows on this staff
     */
    StaffLayout(QGraphicsScene *, ViewSegment *, SnapGrid *,
                int id, int resolution, int lineThickness,
                double pageWidth, int rowsPerPage, int rowSpacing);

    /**
     * Create a new StaffLayout for the given Segment, with
     * either page or linear layout.
     */
    StaffLayout(QGraphicsScene *, ViewSegment *, SnapGrid *,
                int id, int resolution, int lineThickness, PageMode pageMode,
                double pageWidth, int rowsPerPage, int rowSpacing);

public:
    virtual ~StaffLayout();

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

    /**
     * Returns the number of bars between bar-line numbers, or zero if
     * bar lines should not be numbered.  For example, if this
     * function returns 5, every 5th bar (starting at bar 5) will be
     * numbered.
     */
    virtual int showBarNumbersEvery() const {
        return 0;
    }

    /**
     * Returns the bar line / repeat style for the start of the given bar.
     */
    virtual BarStyle getBarStyle(int /* barNo */) const {
        return PlainBar;
    }

    /**
     * Returns the distance the opening (repeat) bar is inset from the
     * nominal barline position.  This is to accommodate the situation
     * where a repeat bar has to appear after the clef and key.
     */
    virtual double getBarInset(int /* barNo */, bool /* isFirstBarInRow */) const {
        return 0;
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
    virtual void setRowsPerPage(int rowsPerPage);

    /// Subclass may wish to expose this
    virtual void setRowSpacing(int rowSpacing);

    /// Subclass may wish to expose this.  Default is zero
    virtual void setConnectingLineLength(int length);

public:
    /**
     * Return the id of the staff.  This is only useful to external
     * agents, it isn't used by the StaffLayout itself.
     */
    virtual int getId() const;

    /**
     * Set the scene x-coordinate of the left-hand end of the staff.
     * This does not move any scene items that have already been
     * created; it should be called before the sizeStaff/positionElements
     * procedure begins.
     */
    virtual void setX(double x);

    /**
     * Get the scene x-coordinate of the left-hand end of the staff.
     */
    virtual double getX() const;

    /**
     * Set the scene y-coordinate of the top of the first staff row.
     * This does not move any scene items that have already been
     * created; it should be called before the sizeStaff/positionElements
     * procedure begins.
     */
    virtual void setY(int y);

    /**
     * Get the scene y-coordinate of the top of the first staff row.
     */
    virtual int getY() const;

    /**
     * Set the scene width of the margin to left and right of the
     * staff on each page (used only in MultiPageMode).  Each staff
     * row will still be pageWidth wide (that is, the margin is in
     * addition to the pageWidth, not included in it).  This does not
     * move any scene items that have already been created; it should
     * be called before the sizeStaff/positionElements procedure
     * begins.
     */
    virtual void setMargin(double m);
    
    /**
     * Get the scene width of the left and right margins.
     */
    virtual double getMargin() const;

    /**
     * Set the scene height of the area at the top of the first page
     * reserved for the composition title and composer's name (used
     * only in MultiPageMode).
     */
    virtual void setTitleHeight(int h);

    /**
     * Get the scene height of the title area.
     */
    virtual int getTitleHeight() const;
    
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
     * Returns true if the given scene coordinates fall within
     * (any of the rows of) this staff.  False if they fall in the
     * gap between two rows.
     */
    virtual bool containsSceneCoords(double sceneX, int sceneY) const; 

    /**
     * Returns the scene y coordinate of the specified line on the
     * staff.  baseX/baseY are a scene coordinates somewhere on the
     * correct row, or -1 for the default row.
     */
    virtual int getSceneYForHeight(int height, double baseX = -1, int baseY = -1) const;

    /**
     * Returns the y coordinate of the specified line on the
     * staff, relative to the top of the row.
     */
    virtual int getLayoutYForHeight(int height) const;


    /**
     * Returns the height-on-staff value nearest to the given scene coordinates,
     * weighted toward the height specified in originalHeight.  This is used
     * when we are comparing a new height calculation to an existing one, and
     * want the new calculation to err on the side of matching the original
     * value unless the difference is strong enough.
     */
    virtual int getWeightedHeightAtSceneCoords(int originalHeight, double x, int y);

    /**
     * Returns the height-on-staff value nearest to the given
     * scene coordinates.
     */
    virtual int getHeightAtSceneCoords(double x, int y) const;

    /**
     * Return the full width, height and origin of the bar containing
     * the given scene cooordinates.
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
    virtual void sizeStaff(HorizontalLayoutEngine& layout);

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
    virtual void renderElements(ViewElementList::iterator from,
                                ViewElementList::iterator to);

    /**
     * Assign suitable coordinates to the elements on the staff
     * between the start and end times, based entirely on the layout
     * X and Y coordinates they were given by the horizontal and
     * vertical layout processes.
     *
     * The implementation is free to render any elements it
     * chooses in this method as well.
     */
    virtual void positionElements(timeT from,
                                  timeT to) = 0;
 
    /* Some optional methods for the subclass. */

    /**
     * Return an iterator pointing to the nearest view element to the
     * given scene coordinates.
     * 
     * If notesAndRestsOnly is true, do not return any view element
     * other than a note or rest.
     *
     * If the closest view element is further away than
     * proximityThreshold pixels in either x or y axis, return end().
     * If proximityThreshold is less than zero, treat it as infinite.
     *
     * Also return the clef and key in force at these coordinates.
     *
     * The default implementation should suit for subclasses that only
     * show a single element per layout X coordinate.
     */
/*
    virtual ViewElementList::iterator getClosestElementToSceneCoords
    (double x, int y, 
     Event *&clef, Event *&key,
     bool notesAndRestsOnly = false, int proximityThreshold = 10) {
        StaffLayoutCoords layoutCoords = getLayoutCoordsForSceneCoords(x, y);
        return getClosestElementToLayoutX
            (layoutCoords.first, clef, key,
             notesAndRestsOnly, proximityThreshold);
    }
*/
    /**
     * Return an iterator pointing to the nearest view element to the
     * given layout x-coordinate.
     * 
     * If notesAndRestsOnly is true, do not return any view element
     * other than a note or rest.
     *
     * If the closest view element is further away than
     * proximityThreshold pixels in either x or y axis, return end().
     * If proximityThreshold is less than zero, treat it as infinite.
     *
     * Also return the clef and key in force at these coordinates.
     */
/*
    virtual ViewElementList::iterator getClosestElementToLayoutX
    (double x,
     Event *&clef, Event *&key,
     bool notesAndRestsOnly = false, int proximityThreshold = 10) = 0;
*/
    /**
     * Return an iterator pointing to the element "under" the given
     * scene coordinates.
     *
     * Return end() if there is no such element.
     *
     * Also return the clef and key in force at these coordinates.
     *
     *
     * The default implementation should suit for subclasses that only
     * show a single element per layout X coordinate.
     */
    virtual ViewElementList::iterator getElementUnderSceneCoords
    (double x, int y, Event *&clef, Event *&key) {
        StaffLayoutCoords layoutCoords = getLayoutCoordsForSceneCoords(x, y);
        return getElementUnderLayoutX(layoutCoords.first, clef, key);
    }

    /**
     * Return an iterator pointing to the element "under" the given
     * scene coordinates.
     *
     * Return end() if there is no such element.
     *
     * Also return the clef and key in force at these coordinates.
     */
    virtual ViewElementList::iterator getElementUnderLayoutX
    (double x, Event *&clef, Event *&key) = 0;

    // The default implementation of the following is empty.  The
    // subclass is presumed to know what the staff's name is and
    // where to put it; this is simply called at some point during
    // the staff-drawing process.
    virtual void drawStaffName();

    /**
     * Return the smaller rectangle (in scene coords) enclosing the
     * whole segment area.
     */
    virtual QRectF getSceneArea();

public:
    // This should not really be public -- it should be one of the
    // protected methods below -- but we have some code that needs
    // it and hasn't been supplied with a proper way to do without.
    // Please try to avoid calling this method.
    //!!! fix NotationView::doDeferredCursorMove

    // This should not really be public -- it should be one of the
    // protected methods below -- but we have some code that needs
    // it and hasn't been supplied with a proper way to do without.
    // Please try to avoid calling this method.
    //!!! fix NotationView::getStaffForSceneCoords
    StaffLayoutCoords
    getLayoutCoordsForSceneCoords(double x, int y) const;

    // This should not really be public -- it should be one of the
    // protected methods below -- but we have some code that needs
    // it and hasn't been supplied with a proper way to do without.
    // Please try to avoid calling this method.
    //!!! fix NotationView::scrollToTime
    StaffLayoutCoords
    getSceneCoordsForLayoutCoords(double x, int y) const;//!!!

    // This should not really be public -- it should be one of the
    // protected methods below -- but we have some code that needs
    // it and hasn't been supplied with a proper way to do without.
    // Please try to avoid calling this method.
    //!!! fix NotationView::print etc
    int getRowSpacing() { return m_rowSpacing; }

protected:
    // Methods that the subclass may (indeed, should) use to convert
    // between the layout coordinates of elements and their scene
    // coordinates.  These are deliberately not virtual.

    // Note that even linear-layout staffs have multiple rows; their
    // rows all have the same y coordinate but increasing x
    // coordinates, instead of the other way around.  (The only reason
    // for this is that it seems to be more efficient from the canvas
    // perspective to create and manipulate many relatively short
    // lines rather than a smaller number of very long ones.)
    //!!! review that for qgraphicsview

    int getTopLineOffset() const {
        return getLineSpacing() * getLegerLineCount();
    }

    int getBarLineHeight() const {
        return getLineSpacing() * (getLineCount() - 1) + m_lineThickness;
    }

    int getRowForLayoutX(double x) const {
        return (int)(x / m_pageWidth);
    }

    int getRowForSceneCoords(double x, int y) const;

    int getSceneYForTopOfStaff(int row = -1) const;

    int getSceneYForTopLine(int row = -1) const {
        return getSceneYForTopOfStaff(row) + getTopLineOffset();
    }

    double getSceneXForLeftOfRow(int row) const;

    double getSceneXForRightOfRow(int row) const {
        return getSceneXForLeftOfRow(row) + m_pageWidth;
    }

    StaffLayoutCoords
    getSceneOffsetsForLayoutCoords(double x, int y) const {
        StaffLayoutCoords cc = getSceneCoordsForLayoutCoords(x, y);
        return StaffLayoutCoords(cc.first - x, cc.second - y);
    }

    double getSceneXForLayoutX(double x) const;

    int getRowsPerPage() const {
        return m_rowsPerPage;
    }

protected:
    // Actual implementation methods.  The default implementation
    // shows staff lines, connecting lines (where appropriate) and bar
    // lines, but does not show time signatures.  To see time
    // signatures, override the deleteTimeSignatures and
    // insertTimeSignature methods.  For repeated clefs and keys at
    // the start of each row, override deleteRepeatedClefsAndKeys
    // and insertRepeatedClefAndKey, but note that your layout class
    // will need to allot the space for them separately.

    virtual void resizeStaffLines();
    virtual void clearStaffLineRow(int row);
    virtual void resizeStaffLineRow(int row, double offset, double length);

    virtual void deleteBars();
    virtual void insertBar(double layoutX, double width, bool isCorrect,
                           const TimeSignature &,
                           int barNo, bool showBarNo);

    // The default implementations of the following two are empty.
    virtual void deleteTimeSignatures();
    virtual void insertTimeSignature(double layoutX,
                                     const TimeSignature &, bool grayed);

    // The default implementations of the following two are empty.
    virtual void deleteRepeatedClefsAndKeys();
    virtual void insertRepeatedClefAndKey(double layoutX, int barNo);

    QGraphicsScene *getScene() { return m_scene; }

    void initCursors();

protected:

    //--------------- Data members ---------------------------------

    QGraphicsScene *m_scene;
    ViewSegment *m_viewSegment;
    SnapGrid *m_snapGrid;

    int      m_id;

    double   m_x;
    int      m_y;
    double   m_margin;
    int      m_titleHeight;
    int      m_resolution;
    int      m_lineThickness;
    
    PageMode m_pageMode;
    double   m_pageWidth;
    int      m_rowsPerPage;
    int      m_rowSpacing;
    int      m_connectingLineLength;

    double   m_startLayoutX;
    double   m_endLayoutX;

    bool     m_current;

    typedef std::vector<QGraphicsItem *> ItemList;
    typedef std::vector<ItemList> ItemMatrix;
    ItemMatrix m_staffLines;
    ItemList m_staffConnectingLines;

    struct BarLineComparator {
        bool operator()(const BarLineItem *a, const BarLineItem *b) const {
            return compareBars(a, b);
        }
    };

    typedef std::pair<double, QGraphicsItem *> LineRec; // layout-x, line
    typedef std::vector<LineRec> LineRecList;
    typedef std::multiset<BarLineItem *, BarLineComparator> BarLineList;
    static bool compareBars(const BarLineItem *, const BarLineItem *);
    static bool compareBarToLayoutX(const BarLineItem *, int);
    BarLineList m_barLines;
    LineRecList m_beatLines;
    LineRecList m_barConnectingLines;
    ItemList m_barNumbers;
};


}

#endif
