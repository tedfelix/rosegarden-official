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

#ifndef _STAFF_LAYOUT_H_
#define _STAFF_LAYOUT_H_

namespace Rosegarden {

template <class T>
class Staff;

/**
 * Abstract base for classes that provide mappings from display
 * coordinates onto staffs and the elements on those staffs (and vice
 * versa).
 *
 * Where the LayoutEngine implementations determine the positions of
 * elements on staffs, the implementation of StaffLayout is likely to
 * determine the positions of staffs on a canvas (although one could
 * equally implement this class by simply delegating to other classes
 * that know better, and it's likely that some methods will ultimately
 * delegate to the implementation of Staff itself).
 * 
 * Any parameters called "x" or "y" are canvas coordinates.  They
 * correspond directly to the positions of events or canvas items on
 * the canvas.
 * 
 * We have to be fairly strict about this API, because it has to work
 * for many possibly layouts of staffs and so some methods need to be
 * given more inputs than would be expected of a simple linear layout.
 * (For example, getYOfHeightAtTime needs the time parameter in order
 * to cope with page-layout staffs that may be divided vertically as
 * well as horizontally.)
 */

template <class T>
class StaffLayout
{
public: 
    StaffLayout() { }
    virtual ~StaffLayout() { }

    /**
     * Find the Staff whose Y coord range includes y, and return a
     * pointer to that Staff.  If no Staff is suitable, return 0.
     */
    virtual Staff<T> *getStaffAtY(int y) const = 0;

    /**
     * Return the "height-on-staff" of the given y-coordinate, on
     * whichever staff is at this y (see getStaffAtY).  The meaning
     * of height-on-staff depends on the staff implementation, but
     * for example on a notation staff it's zero for the bottom line
     * and counts up in spaces and lines from there (with 8 being
     * the top line, of five).
     */
    virtual int getHeightAtY(int y) const = 0;

    /**
     * Return the absolute time of the point at the given x and y
     * coordinates, on whichever staff is at this y (see getStaffAtY).
     * This may be the time of the nearest element on the staff (for a
     * staff in which elements are not spaced linearly, like the
     * notation staff) or it may just be a value proportional to the
     * distance from the start of the staff.
     */
    virtual timeT getTimeAtCoordinates(int x, int y) const = 0;

    /**
     * Return the y coordinate of the given height-on-staff at the
     * given time.  (We need the time in order to handle staffs that
     * wrap around at the right-hand end of the screen.)
     */
    virtual int getYOfHeightAtTime(Staff<T> *staff,
				   int height, timeT time) const = 0;

    /**
     * Return the y coordinate of the nearest height-on-staff to the
     * given y at the given x coordinate
     */
    virtual int getYSnappedToLine(int y) const = 0;

    /**
     * Return the note name (C4, Bb3, whatever) corresponding to the
     * given coordinates
     */
    virtual std::string getNoteNameAtCoordinates(int x, int y) const = 0;
};

}


#endif
