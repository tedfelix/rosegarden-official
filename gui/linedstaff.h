
// -*- c-basic-offset: 4 -*-

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

#ifndef LINED_STAFF_H
#define LINED_STAFF_H

#include "Staff.h"

/**
 * LinedStaff is a base class for implementations of Staff that
 * display the contents of a Segment on a set of horizontal lines
 * with optional vertical bar lines plus a StaffRuler at the top.  
 * Likely subclasses include the notation and piano-roll staffs.
 *
 * In general, this class handles x coordinates in floating-point,
 * but y-coordinates as integers because of the requirement that
 * staff lines be a precise integral distance apart.
 */

template <class T>
class LinedStaff : public Rosegarden::Staff<T>,
		   public QCanvasItemGroup
{
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
     */
    LinedStaff(QCanvas *, Rosegarden::Segment *, int id, int resolution);

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
     * \a pageWidth is the width of a window (to determine
     *    when to break lines for page layout)
     *
     * \a lineBreakGap is the distance in pixels between
     *    the tops of consecutive rows on this staff
     */
    LinedStaff(QCanvas *, Rosegarden::Segment *, int id, int resolution);

public:
    virtual ~LinedStaff();

protected:
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

protected:
    /// Subclass may wish to expose this
    virtual void setPageMode(bool pageMode);

    /// Subclass may wish to expose this
    virtual void setPageWidth(double pageWidth);

    /// Subclass may wish to expose this
    virtual void setLineBreakGap(int lineBreakGap);

public:

    /**
     * Return the id of the staff.  This is only useful to external
     * agents, it isn't used by the LinedStaff itself.
     */
    virtual int getId();

    /**
     * Returns the total height of a staff row
     */
    virtual int getStaffHeight() const;
    
    /**
     * Returns true if the given y-coordinate falls within (any of
     * the rows of) this staff.
     */
    virtual bool containsY(int y) const; 

    /**
     * Returns the y coordinate of the specified line on the staff,
     * relative to the top of the staff.  baseY is a canvas y
     * coordinate somewhere on the correct row, or -1 for the default
     * row.
     */
    virtual int getYOfHeight(int height, int baseY = -1) const;

    /**
     * Returns the height-on-staff value nearest to the given
     * y coordinate.
     */
    virtual int getHeightAtY(int y) const;

    /**
     * Return the full width, height and origin of the bar containing
     * the given cooordinates.
     */
    virtual void getBarExtents(int x, int y,
			       int &rx, int &ry, int &rw, int &rh) const;

    /**
     * Generate or re-generate sprites for all the elements between
     * from and to.  See subclasses for specific detailed comments.
     *
     * A very simplistic staff implementation may choose not to
     * implement this (the default implementation is empty) and to
     * do all the rendering work in positionElements.  If rendering
     * elements is slow, however, it makes sense to do it here
     * because this method may be called less often.
     */
    virtual void renderElements(Rosegarden::ViewElementList<T>::iterator from,
				Rosegarden::ViewElementList<T>::iterator to);

    /**
     * Call renderElements(from, to) on the whole staff.
     */
    virtual void renderElements();

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
    virtual void positionElements();

};

#endif
