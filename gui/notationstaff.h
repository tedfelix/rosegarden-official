// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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
#include "linedstaff.h"

class QCanvasSimpleSprite;
class EventSelection;

/**
 * The Staff is a repository for information about the notation
 * representation of a single Segment.  This includes all of the
 * NotationElements representing the Events on that Segment, the staff
 * lines, as well as basic positional and size data.  This class
 * used to be in gui/staff.h, but it's been moved and renamed
 * following the introduction of the core Staff base class, and
 * much of the functionality has been extracted into the LinedStaff
 * base class.
 */

class NotationStaff : public LinedStaff<NotationElement>
{
public:

    /**
     * Creates a new NotationStaff for the specified Segment
     * \a id is the id of the staff in the NotationView
     */
    NotationStaff(QCanvas *, Rosegarden::Segment *, int id,
		  bool pageMode, double pageWidth,
                  std::string fontName, int resolution);
    virtual ~NotationStaff();

    /**
     * Changes the resolution of the note pixmap factory and the
     * staff lines, etc
     */
    virtual void changeFont(std::string fontName, int resolution);

    virtual void setLegatoDuration(Rosegarden::timeT duration);

    LinedStaff<NotationElement>::setPageMode;
    LinedStaff<NotationElement>::setPageWidth;
    LinedStaff<NotationElement>::setRowSpacing;
    LinedStaff<NotationElement>::setConnectingLineLength;

    /**
     * Gets a read-only reference to the pixmap factory used by the
     * staff.  (For use by NotationHLayout, principally.)  This
     * reference isn't const because the NotePixmapFactory maintains
     * too much state for its methods to be const, but you should
     * treat the returned reference as if it were const anyway.
     */
    virtual NotePixmapFactory& getNotePixmapFactory() { return *m_npf; }

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
    virtual void renderElements(NotationElementList::iterator from,
				NotationElementList::iterator to);

    
    virtual void renderElements() {
	LinedStaff<NotationElement>::renderElements();
    }


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
    virtual void positionElements(Rosegarden::timeT from = -1,
				  Rosegarden::timeT to = -1);
    
    /**
     * Insert time signature at x-coordinate \a x.
     */
    virtual void insertTimeSignature(double layoutX,
				     const Rosegarden::TimeSignature &timeSig);

    /**
     * Delete all time signatures
     */
    virtual void deleteTimeSignatures();

    /**
     * Return the clef and key in force at the given canvas
     * coordinates
     */
    virtual void getClefAndKeyAtCanvasCoords(double x, int y,
					     Rosegarden::Clef &clef,
					     Rosegarden::Key &key) const;

    /**
     * Return the note name (C4, Bb3, whatever) corresponding to the
     * given canvas coordinates
     */
    virtual std::string getNoteNameAtCanvasCoords
    (double x, int y,
     Rosegarden::Accidental accidental =
     Rosegarden::Accidentals::NoAccidental) const;

    /**
     * Find the NotationElement whose canvas coords are closest to
     * (x,y).
     *
     * If notesAndRestsOnly is true, will return the closest note
     * or rest but will never return any other kind of element.
     * 
     * If the closest event is further than \a proximityThreshold
     * horizontally away from (x,y), in pixels, end() is returned.
     * (If proximityThreshold is negative, there will be no limit
     * to the distances that will be considered.)
     *
     * Also returns the time signature, clef and key in force
     * at the given coordinates.
     */
    virtual NotationElementList::iterator getClosestElementToCanvasCoords
    (double x, int y,
     Rosegarden::Event *&timeSignature,
     Rosegarden::Event *&clef,
     Rosegarden::Event *&key,
     bool notesAndRestsOnly = false,
     int proximityThreshold = 10);

    /**
     * Find the NotationElement whose layout x-coord is closest to x,
     * without regard to its y-coord.
     *
     * If notesAndRestsOnly is true, will return the closest note
     * or rest but will never return any other kind of element.
     * 
     * If the closest event is further than \a proximityThreshold
     * horizontally away from x, in pixels, end() is returned.
     * (If proximityThreshold is negative, there will be no limit
     * to the distances that will be considered.)
     *
     * Also returns the time signature, clef and key in force
     * at the given coordinate.
     */
    virtual NotationElementList::iterator getClosestElementToLayoutX
    (double x,
     Rosegarden::Event *&timeSignature,
     Rosegarden::Event *&clef,
     Rosegarden::Event *&key,
     bool notesAndRestsOnly = false,
     int proximityThreshold = 10);

    /**
     * Overridden from Rosegarden::Staff<T>.
     * We want to avoid wrapping really short rests
     */
    virtual bool wrapEvent(Rosegarden::Event *);

protected:

    // definition of staff
    virtual int getLineCount() const         { return 5; }
    virtual int getLegerLineCount() const    { return 8; }
    virtual int getBottomLineHeight() const  { return 0; }
    virtual int getHeightPerLine() const     { return 2; }

    /** 
     * Assign a suitable sprite to the given element (the clef is
     * needed in case it's a key event, in which case we need to judge
     * the correct pitch for the key)
     */
    virtual void renderSingleElement(NotationElement *, 
				     const Rosegarden::Clef &,
				     bool selected);

    /**
     * Return a QCanvasSimpleSprite representing the given note event
     */
    virtual QCanvasSimpleSprite *makeNoteSprite(NotationElement *);

    /**
     * Return true if the element has a canvas item that is already
     * at the correct coordinates
     */
    virtual bool elementNotMoved(NotationElement *);

    /**
     * Return true if the element has a canvas item that is already
     * at the correct y-coordinate
     */
    virtual bool elementNotMovedInY(NotationElement *);

    /**
     * Returns true if the item at the given iterator appears to have
     * moved horizontally without the spacing around it changing.
     * 
     * In practice, calculates the offset between the intended layout
     * and current canvas coordinates of the item at the given
     * iterator, and returns true if this offset is equal to those of
     * all other following iterators at the same time as well as the
     * first iterator found at a greater time.
     */
    virtual bool elementShiftedOnly(NotationElementList::iterator);

    typedef std::set<QCanvasSimpleSprite *> SpriteSet;
    SpriteSet m_timeSigs;

    typedef std::pair<int, Rosegarden::Clef> ClefChange;
    FastVector<ClefChange> m_clefChanges;

    typedef std::pair<int, Rosegarden::Key> KeyChange;
    FastVector<KeyChange> m_keyChanges;

    NotePixmapFactory *m_npf;
};

#endif
