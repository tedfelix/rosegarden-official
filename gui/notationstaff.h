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

#ifndef NOTATION_STAFF_H
#define NOTATION_STAFF_H

#include <string>

#include "notepixmapfactory.h"
#include "notationelement.h"
#include "FastVector.h"

#include "Staff.h"
#include "StaffLayout.h"

#include "linedstaff.h"
#include "qcanvasgroupableitem.h"

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

    /**
     * Creates a new NotationStaff for the specified Segment
     * \a id is the id of the staff in the NotationView
     */
    NotationStaff(QCanvas *, Rosegarden::Segment *, unsigned int id,
		  bool pageMode, double pageWidth,
                  std::string fontName, int resolution);
    ~NotationStaff();

    /**
     * Changes the resolution of the note pixmap factory and the
     * staff lines, etc; can't change resolution of the actual layout
     * or pixmaps on the staff, the notation view should do that
     */
    void changeFont(std::string fontName, int resolution);

    void setLegatoDuration(Rosegarden::timeT duration);

    void setPageMode(bool pageMode) { m_pageMode = pageMode; }
    void setPageWidth(double pageWidth) { m_pageWidth = pageWidth; }
    void setLineBreakGap(int lineBreakGap) { m_lineBreakGap = lineBreakGap; }
    void setConnectingLineHeight(int clh) { m_connectingLineHeight = clh; }

    /**
     * Gets a read-only reference to the pixmap factory used by the
     * staff.  (For use by NotationHLayout, principally.)  This
     * reference isn't const because the NotePixmapFactory maintains
     * too much state for its methods to be const, but you should
     * treat the returned reference as if it were const anyway.
     */
    NotePixmapFactory& getNotePixmapFactory() { return *m_npf; }

    /**
     * Returns the y coordinate of the specified line on the staff,
     * relative to the top of the staff.  baseY is a canvas y
     * coordinate somewhere on the correct row, or -1 for the default
     * row.
     *
     * 0 is the bottom staff-line, 8 is the top one.
     */
    int getYOfHeight(int height, int baseY = -1) const;

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
     * Returns true if the given y-coordinate falls within (any of
     * the rows of) this staff.
     */
    bool containsY(int y) const; 

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
     * Assign suitable coordinates to the elements on the staff,
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
     * Set the start and end x-coords of the staff lines, and update
     * the canvas size if so requested
     */
    void setLines(double xfrom, double xto, bool resizeCanvas = false);

    int getHeightAtY(int y) const;

    /**
     * Return a rectangle describing the full width and height of the
     * bar containing the given cooordinates.  Used for setting a
     * selection to the scope of a full bar.
     */
    QRect getBarExtents(int x, int y);

    void getClefAndKeyAt(int x, int y,
			 Rosegarden::Clef &clef, Rosegarden::Key &key) const;

    static const int nbLines;        // number of main lines on the staff
    static const int nbLegerLines;   // number of lines above or below

    static const int NoHeight;

protected:

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

    void resizeStaffLines();
    void clearStaffLineRow(int rowNo);
    void resizeStaffLineRow(int rowNo, double offset, double length);

    double getPageWidth() const;
    int	   getRowForLayoutX(double x) const;
    int    getRowForY(int y) const;
    double getXForLayoutX(double x) const;
    int	   getTopOfStaffForRow(int row) const;
    int	   getTopLineOffsetForRow(int row) const;
    int	   getRowCount() const;
    double getRowLeftX(int row) const;
    double getRowRightX(int row) const;

    void   getPageOffsets(NotationElement *,
			  double &xoff, double &yoff) const;

    int heightOfYCoord(int height) const;

    typedef std::pair<int, QCanvasLine *> BarPair;
    typedef std::vector<BarPair> BarLineList;
    BarLineList m_barLines;
    static bool compareBarPos(const BarPair &, const BarPair &);
    static bool compareBarToPos(const BarPair &, unsigned int);

    typedef std::vector<QCanvasLine *> StaffLineList;
    typedef std::vector<StaffLineList> StaffLineListList;

    //--------------- Data members ---------------------------------

    int m_id;

    bool m_pageMode;
    double m_pageWidth;
    int m_lineBreakGap;
    int m_connectingLineHeight;

    double m_horizLineStart;
    double m_horizLineEnd;
    int m_resolution;

    StaffLineListList m_staffLines;
    StaffLineList m_staffConnectingLines;

    typedef std::set<QCanvasSimpleSprite *> SpriteSet;
    SpriteSet m_timeSigs;

    typedef std::pair<int, Rosegarden::Clef> ClefChange;
    FastVector<ClefChange> m_clefChanges;

    typedef std::pair<int, Rosegarden::Key> KeyChange;
    FastVector<KeyChange> m_keyChanges;

    QCanvasLine *m_initialBarA, *m_initialBarB;
    NotePixmapFactory *m_npf;

    bool m_haveSelection;
};

#endif
