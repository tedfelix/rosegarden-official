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

#include <string>

#include "qcanvasgroupableitem.h"
#include "notepixmapfactory.h"
#include "notationelement.h"
#include "FastVector.h"

#include "Staff.h"
#include "StaffLayout.h"

class QCanvasLineGroupable;
class QCanvasSimpleSprite;
class EventSelection;

typedef Rosegarden::StaffLayout<NotationElement> NotationStaffLayout;

/**
 * The Staff is a repository for information about the notation
 * representation of a single Segment.  This includes all of the
 * NotationElements representing the Events on that Segment, the staff
 * lines, as well as basic positional and size data.  This class
 * used to be in gui/staff.h, but it's been moved and renamed
 * following the introduction of the core Staff base class.
 *
 * For various wacky reasons, many of the x-coordinate measurements
 * are in floating-point whereas the y-coordinate ones are integers.
 */

class NotationStaff : public Rosegarden::Staff<NotationElement>,
		      public QCanvasItemGroup
{
public:
    typedef std::vector<QCanvasLine *> LineList;
    typedef std::vector<LineList> LineListList;
    typedef std::set<QCanvasSimpleSprite *> SpriteSet;
    
    /**
     * Creates a new NotationStaff for the specified Segment
     * \a id is the id of the staff in the NotationView
     */
    NotationStaff(QCanvas *, Rosegarden::Segment *, unsigned int id,
		  bool pageMode, double pageWidth, int lineBreakGap,
                  std::string fontName, int resolution);
    ~NotationStaff();

    /**
     * Changes the resolution of the note pixmap factory and the
     * staff lines, etc; can't change resolution of the actual layout
     * or pixmaps on the staff, the notation view should do that
     */
    void changeFont(std::string fontName, int resolution);

    void setLegatoDuration(Rosegarden::timeT duration);

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
     * Returns the height of the nearest line on the staff to the
     * specified Y coordinate.
     *
     * 0 is the bottom staff-line, 8 is the top one.
     */
    int heightOfYCoord(int height) const;

    /**
     * Returns the difference between the y-coord of one visible line
     * and that of its neighbour
     */
    int getLineSpacing() const {
	return m_npf->getLineSpacing();
    }

    /**
     * Returns the height of a bar line.
     */
    int getBarLineHeight() const {
	return getLineSpacing() * (nbLines - 1);
    }

    /**
     * Returns the total height of a staff
     */
    int getStaffHeight() const {
	return getTopLineOffset() * 2 + getBarLineHeight()
	    + m_npf->getStaffLineThickness();
    }

    /**
     * Returns the space between the top of the staff object and the
     * first visible line on the staff.  (This is the same as the
     * space between the last visible line and the bottom of the staff
     * object.)
     */
    int getTopLineOffset() const {
	return getLineSpacing() * nbLegerLines;
    }
 
    /**
     * Return the id of the staff
     * This will be passed to the NotationTools
     * so they know on which staff a mouse event occurred
     *
     * @see NotationTool#handleMousePress
     * @see NotationView#itemPressed
     */
    int getId() { return m_id; }


    /**
     * Generate or re-generate sprites for all the elements between
     * from and to.  Call this when you've just created a staff and
     * done the layout on it, and now you want to draw it; or when
     * you've just made a change, in which case you can specify the
     * extents of the change in the from and to parameters.
     * 
     * This method does not reposition any elements outside the given
     * range -- so after any edit that may change the visible extents
     * of a range, you will then need to call positionElements for the
     * changed range and the entire remainder of the staff.
     */
    void renderElements(NotationElementList::iterator from,
			NotationElementList::iterator to);

    /**
     * Call renderElements(from, to) on the whole staff.
     */
    void renderElements();


    /**
     * Assign suitable coordinates to all the elements on the staff,
     * based entirely on the layout X and Y coordinates they were
     * given by the horizontal and vertical layout processes.
     *
     * This is necessary because the sprites that are being positioned
     * may have been created either after the layout process completed
     * (by renderElements) or before (by the previous renderElements
     * call, if the sprites are unchanged but have moved) -- so
     * neither the layout nor renderElements can authoritatively set
     * their final positions.  Also, this operates on the entire staff
     * so that it can update its record of key and clef changes during
     * the course of the staff, which is needed to support the
     * getClefAndKeyAtX() method.
     *
     * This method also updates the selected-ness of any elements it
     * sees (i.e. it turns the selected ones blue and the unselected
     * ones black).  As a result -- and for other implementation
     * reasons -- it may actually re-generate some sprites.
     *
     * The from and to arguments are used to indicate the extents of a
     * changed area within the staff.  The actual area within which the
     * elements end up being repositioned will begin at the start of
     * the bar containing the changed area's start, and will end at the
     * start of the next bar whose first element hasn't moved, after
     * the changed area's end.
     *
     * Call this after renderElements, or after changing the selection,
     * passing from and to arguments corresponding to the times of those
     * passed to renderElements.
     */
    void positionElements(Rosegarden::timeT from, Rosegarden::timeT to);
    
    /**
     * Call positionElements(from, to) on the whole staff.
     */
    void positionElements();

    /**
     * Insert a bar line at x-coordinate \a barPos.
     *
     * If \a correct is true, the bar line ends a correct (timewise)
     * bar.  If false, the bar line ends an incorrect bar (for instance,
     * two minims in 3:4 time), and will be drawn in red
     */
    void insertBar(unsigned int barPos, bool correct);

    /**
     * Return a rectangle describing the full width and height of the
     * bar containing the given x-cooordinate.  Used for setting a
     * selection to the scope of a full bar.
     */
    QRect getBarExtents(unsigned int x);

    /**
     * Insert time signature at x-coordinate \a x.
     */
    void insertTimeSignature(unsigned int x,
			     const Rosegarden::TimeSignature &timeSig);

    /**
     * Delete all bars which are after X position \a fromPos
     */
    void deleteBars(unsigned int fromPos);

    /**
     * Delete all bars
     */
    void deleteBars();

    /**
     * Delete all time signatures
     */
    void deleteTimeSignatures();

    /**
     * Set the start and end x-coords of the staff lines
     */
    void setLines(double xfrom, double xto);

    void getClefAndKeyAtX(int x, Rosegarden::Clef &clef, Rosegarden::Key &key)
	const;

    static const int nbLines;        // number of main lines on the staff
    static const int nbLegerLines;   // number of lines above or below

    static const int NoHeight;

protected:

    enum RefreshType { FullRefresh, PositionRefresh, SelectionRefresh };

    /** 
     * Assign a suitable sprite to the given element (the clef is
     * needed in case it's a key event, in which case we need to judge
     * the correct pitch for the key)
     */
    void renderSingleElement(NotationElement *, const Rosegarden::Clef &,
			     bool selected);

    /**
     * Return a QCanvasSimpleSprite representing the given note event
     */
    QCanvasSimpleSprite *makeNoteSprite(NotationElement *);

    /**
     * Return true if the element has a canvas item that is already
     * at the correct layout coordinates
     */
    bool elementNotMoved(NotationElement *);

    int m_id;

    bool m_pageMode;
    double m_pageWidth;
    int m_lineBreakGap;

    double m_horizLineStart;
    double m_horizLineEnd;
    int m_resolution;

    void resizeStaffLines();
    void clearStaffLineRow(int rowNo);
    void resizeStaffLineRow(int rowNo, double offset, double length);

    double getPageWidth();
    int	   getRowForLayoutX(double x);
    double getXForLayoutX(double x);
    int	   getTopOfStaffForRow(int row);
    int	   getTopLineOffsetForRow(int row);
    int	   getRowCount();
    double getRowLeftX(int row);
    double getRowRightX(int row);

    void   getPageOffsets(NotationElement *,
			  double &xoff, double &yoff);

    LineList m_barLines;
    LineListList m_staffLines;
    SpriteSet m_timeSigs;

    typedef std::pair<int, Rosegarden::Clef> ClefChange;
    FastVector<ClefChange> m_clefChanges;

    typedef std::pair<int, Rosegarden::Key> KeyChange;
    FastVector<KeyChange> m_keyChanges;

    QCanvasLineGroupable *m_initialBarA, *m_initialBarB;
    NotePixmapFactory *m_npf;

    bool m_haveSelection;
};

#endif
