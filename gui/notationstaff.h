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
#include <string>

#include "qcanvasitemgroup.h"
#include "notepixmapfactory.h"
#include "notationelement.h"

#include "Staff.h"
#include "Quantizer.h"

class QCanvasLineGroupable;
class QCanvasSimpleSprite;
class EventSelection;

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
    
    /**
     * Creates a new NotationStaff for the specified Track
     * \a id is the id of the staff in the NotationView
     */
    NotationStaff(QCanvas*, Rosegarden::Track*, unsigned int id,
                  std::string fontName, int resolution);
    ~NotationStaff();

    /**
     * Changes the resolution of the note pixmap factory and the
     * staff lines, etc; can't change resolution of the actual layout
     * or pixmaps on the staff, the notation view should do that
     */
    void changeFont(std::string fontName, int resolution);

    void setQuantizationDuration(Rosegarden::timeT duration);
    const Rosegarden::Quantizer &getQuantizer() const { return m_quantizer; }

    /**
     * Gets a read-only reference to the pixmap factory used by the
     * staff.  (For use by NotationHLayout, principally.)  This
     * reference isn't const because the NotePixmapFactory maintains
     * too much state for its methods to be const, but you should
     * treat the returned reference as if it were const anyway.
     */
    NotePixmapFactory& getNotePixmapFactory() { return *m_npf; }

    /**
     * Returns the Y coordinate of the specified line on the staff.
     *
     * 0 is the bottom staff-line, 8 is the top one.
     */
    int yCoordOfHeight(int height) const;

    /**
     * Returns the height of a bar line.
     */
    unsigned int getBarLineHeight() const { return m_barLineHeight; }

    /**
     * Returns the margin surrounding a bar line.
     *
     * The bar line is in the middle of the margin.
     */
    unsigned int getBarMargin() const { return m_npf->getBarMargin(); }

    /**
     * Returns the total height of a staff
     */
    unsigned int getStaffHeight() const {
	return (m_resolution + 1) * nbLines + linesOffset * 2 + 1;
    }

    /**
     * Return the id of the staff
     * This will be passed to the NotationTools
     * so they know on which staff a mouse event occurred
     *
     * @see NotationTool#handleMousePress
     * @see NotationView#itemClicked
     */
    unsigned int getId() { return m_id; }

    bool showElements();

    bool showElements(NotationElementList::iterator from,
		      NotationElementList::iterator to,
		      bool positionOnly = false);

    bool showSelection(const EventSelection *selection);


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
    static const int nbLegerLines;   // number of lines above or below
    static const int linesOffset;    // from top of canvas to top line (bad!)

protected:
    /**
     * Return a QCanvasSimpleSprite representing the NotationElement
     * pointed to by the given iterator
     */
    QCanvasSimpleSprite* makeNoteSprite(NotationElementList::iterator);

    int m_id;

    int m_barLineHeight;
    int m_horizLineLength;
    int m_resolution;

    barlines m_barLines;
    barlines m_staffLines;

    QCanvasLineGroupable *m_initialBarA, *m_initialBarB;
    NotePixmapFactory *m_npf;
    Rosegarden::Quantizer m_quantizer;
};

#endif
