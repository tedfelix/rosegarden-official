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

#ifndef STAFF_H
#define STAFF_H

#include <vector>

#include "qcanvasitemgroup.h"
#include "notepixmapfactory.h"
#include "notationelement.h"

#include "Staff.h"

class QCanvasLineGroupable;

/**
 * The Staff is a repository for information about the notation
 * representation of a single Track.  This includes all of the
 * NotationElements representing the Events on that Track, the staff
 * lines, as well as basic positional and size data.  This class
 * used to be in gui/staff.h, but it's been moved and renamed
 * following the introduction of the core Staff base class.
 */

class NotationStaff : public Rosegarden::Staff<NotationElement>,
		      public QCanvasItemGroup
{
public:
    typedef std::vector<QCanvasLineGroupable*> barlines;
    
    NotationStaff(QCanvas*, Rosegarden::Track*, int resolution);
    ~NotationStaff();

    // bit dubious, really -- I'd rather have a NotePixmapFactory that
    // worked even through a const reference, but most of its drawing
    // methods are necessarily non-const because it stores so much
    // state internally
    NotePixmapFactory& getNotePixmapFactory() { return m_npf; }

    /**
     * Return the Y coordinate of specified line
     *
     * 0 is bottom staff-line, 8 is top one.
     */
    int yCoordOfHeight(int height) const;

    /**
     * Return the height of a bar line
     */
    unsigned int getBarLineHeight() const { return m_barLineHeight; }

    /**
     * Return the margin surrounding a bar line
     *
     * The bar line is in the middle of the margin
     */
    unsigned int getBarMargin() const { return m_npf.getBarMargin(); }

    /**
     * Return the total height of a staff
     */
    unsigned int getStaffHeight() const {
	return m_resolution * nbLines + linesOffset * 2 + 1;
    }

    /**
     * Insert a bar line at X position \a barPos.
     *
     * If \a correct is true, the bar line ends a correct (timewise)
     * bar.  If false, the bar line ends an incorrect bar (for instance,
     * two minims in 3:4 time), and will be drawn as red
     */
    void insertBar(unsigned int barPos, bool correct);

    /**
     * Delete all bars which are after X position \a fromPos
     */
    void deleteBars(unsigned int fromPos);

    /**
     * Delete all bars
     */
    void deleteBars();

    /**
     * Set the start and end x-coords of the staff lines
     */
    void setLines(double xfrom, double xto);

    static const int nbLines;        // number of main lines on the staff
    static const int linesOffset;    // from top of canvas to top line (bad!)

protected:
    int m_barLineHeight;
    int m_horizLineLength;
    int m_resolution;

    barlines m_barLines;
    barlines m_staffLines;

    QCanvasLineGroupable *m_initialBarA, *m_initialBarB;

    NotePixmapFactory m_npf;
};

#endif
