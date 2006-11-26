
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_NOTATIONSTAFF_H_
#define _RG_NOTATIONSTAFF_H_

#include "base/FastVector.h"
#include "base/Staff.h"
#include "base/ViewElement.h"
#include "gui/general/LinedStaff.h"
#include "gui/general/ProgressReporter.h"
#include <map>
#include <set>
#include <string>
#include <utility>
#include "base/Event.h"
#include "NotationElement.h"


class QPainter;
class QCanvasPixmap;
class QCanvasItem;
class QCanvas;
class LinedStaffCoords;


namespace Rosegarden
{

class ViewElement;
class TimeSignature;
class SnapGrid;
class Segment;
class QCanvasSimpleSprite;
class NotePixmapParameters;
class NotePixmapFactory;
class Note;
class NotationView;
class NotationProperties;
class Key;
class Event;
class Clef;


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

class NotationStaff : public ProgressReporter, public LinedStaff
{
public:

    /**
     * Creates a new NotationStaff for the specified Segment
     * \a id is the id of the staff in the NotationView
     */
    NotationStaff(QCanvas *, Segment *, SnapGrid *,
                  int id, NotationView *view,
                  std::string fontName, int resolution);
    virtual ~NotationStaff();

    /**
     * Changes the resolution of the note pixmap factory and the
     * staff lines, etc
     */
    virtual void changeFont(std::string fontName, int resolution);

    void setLegerLineCount(int legerLineCount) {
        if (legerLineCount == -1) m_legerLineCount = 8;
        else m_legerLineCount = legerLineCount;
    }

    void setBarNumbersEvery(int barNumbersEvery) {
        m_barNumbersEvery = barNumbersEvery;
    }

    LinedStaff::setPageMode;
    LinedStaff::setPageWidth;
    LinedStaff::setRowsPerPage;
    LinedStaff::setRowSpacing;
    LinedStaff::setConnectingLineLength;

    /**
     * Gets a read-only reference to the pixmap factory used by the
     * staff.  (For use by NotationHLayout, principally.)  This
     * reference isn't const because the NotePixmapFactory maintains
     * too much state for its methods to be const, but you should
     * treat the returned reference as if it were const anyway.
     */
    virtual NotePixmapFactory& getNotePixmapFactory(bool grace) {
        return grace ? *m_graceNotePixmapFactory : *m_notePixmapFactory;
    }

    /**
     * Generate or re-generate sprites for all the elements between
     * from and to.  Call this when you've just made a change,
     * specifying the extents of the change in the from and to
     * parameters.
     * 
     * This method does not reposition any elements outside the given
     * range -- so after any edit that may change the visible extents
     * of a range, you will then need to call positionElements for the
     * changed range and the entire remainder of the staff.
     */
    virtual void renderElements(NotationElementList::iterator from,
                                NotationElementList::iterator to);

    
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
     * their final positions.
     *
     * This method also updates the selected-ness of any elements it
     * sees (i.e. it turns the selected ones blue and the unselected
     * ones black), and re-generates sprites for any elements for
     * which it seems necessary.  In general it will only notice a
     * element needs regenerating if its position has changed, not if
     * the nature of the element has changed, so this is no substitute
     * for calling renderElements.
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
    virtual void positionElements(timeT from,
                                  timeT to);

    /**
     * Re-render and position elements as necessary, based on the
     * given extents and any information obtained from calls to
     * markChanged().  This provides a render-on-demand mechanism.  If
     * you are going to use this rendering mechanism, it's generally
     * wise to avoid explicitly calling
     * renderElements/positionElements as well.
     *
     * Returns true if something needed re-rendering.
     */
    virtual bool checkRendered(timeT from,
                               timeT to);

    /**
     * Find something between the given times that has not yet been
     * rendered, and render a small amount of it.  Return true if it
     * found something to do.  This is to be used as a background work
     * procedure for rendering not-yet-visible areas of notation.
     */
    virtual bool doRenderWork(timeT from,
                              timeT to);

    /**
     * Mark a region of staff as changed, for use by the on-demand
     * rendering mechanism.  If fromBar == toBar == -1, mark the
     * entire staff as changed (and recover the memory used for its
     * elements).  Pass movedOnly as true to indicate that elements
     * have not changed but only been repositioned, for example as a
     * consequence of a modification on another staff that caused a
     * relayout.
     */
    virtual void markChanged(timeT from = 0,
                             timeT to = 0,
                             bool movedOnly = false);

    /**
     * Set a painter as the printer output.  If this painter is
     * non-null, subsequent renderElements calls will only render
     * those elements that cannot be rendered directly to a print
     * painter; those that can, will be rendered by renderPrintable()
     * instead.
     */
    virtual void setPrintPainter(QPainter *painter);

    /**
     * Render to the current print painter those elements that can be
     * rendered directly to a print painter.  If no print painter is
     * set, do nothing.
     */
    virtual void renderPrintable(timeT from,
                                 timeT to);
    
    /**
     * Insert time signature at x-coordinate \a x.
     */
    virtual void insertTimeSignature(double layoutX,
                                     const TimeSignature &timeSig);

    /**
     * Delete all time signatures
     */
    virtual void deleteTimeSignatures();
    
    /**
     * Insert repeated clef and key at start of new line, at x-coordinate \a x.
     */
    virtual void insertRepeatedClefAndKey(double layoutX, int barNo);

    /**
     * Delete all repeated clefs and keys.
     */
    virtual void deleteRepeatedClefsAndKeys();

    /**
     * (Re)draw the staff name from the track's current name
     */
    virtual void drawStaffName();

    /**
     * Return true if the staff name as currently drawn is up-to-date
     * with that in the composition
     */
    virtual bool isStaffNameUpToDate();

    /**
     * Return the clef and key in force at the given canvas
     * coordinates
     */
    virtual void getClefAndKeyAtCanvasCoords(double x, int y,
                                             Clef &clef,
                                             ::Rosegarden::Key &key) const;

    /**
     * Return the note name (C4, Bb3, whatever) corresponding to the
     * given canvas coordinates
     */
    virtual std::string getNoteNameAtCanvasCoords
    (double x, int y,
     Accidental accidental =
     Accidentals::NoAccidental) const;

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
     * Also returns the clef and key in force at the given coordinate.
     */
    virtual ViewElementList::iterator getClosestElementToLayoutX
    (double x, Event *&clef, Event *&key,
     bool notesAndRestsOnly = false,
     int proximityThreshold = 10);

    /**
     * Find the NotationElement "under" the given layout x-coord,
     * without regard to its y-coord.
     *
     * Also returns the clef and key in force at the given coordinates.
     */
    virtual ViewElementList::iterator getElementUnderLayoutX
    (double x, Event *&clef, Event *&key);

    /**
     * Draw a note on the staff to show an insert position prior to
     * an insert. 
     */
    virtual void showPreviewNote(double layoutX, int heightOnStaff,
                                 const Note &note);

    /**
     * Remove any visible preview note.
     */
    virtual void clearPreviewNote();

    /**
     * Overridden from Staff<T>.
     * We want to avoid wrapping things like controller events, if
     * our showUnknowns preference is off
     */
    virtual bool wrapEvent(Event *);

    /**
     * Override from Staff<T>
     * Let tools know if their current element has gone
     */
    virtual void eventRemoved(const Segment *, Event *);

    /**
     * Return the view-local PropertyName definitions for this staff's view
     */
    const NotationProperties &getProperties() const;

    virtual double getBarInset(int barNo, bool isFirstBarInRow) const;

protected:

    virtual ViewElement* makeViewElement(Event*);

    // definition of staff
    virtual int getLineCount() const         { return 5; }
    virtual int getLegerLineCount() const    { return m_legerLineCount; }
    virtual int getBottomLineHeight() const  { return 0; }
    virtual int getHeightPerLine() const     { return 2; }
    virtual int showBarNumbersEvery() const  { return m_barNumbersEvery; }

    virtual BarStyle getBarStyle(int barNo) const;

    /** 
     * Assign a suitable sprite to the given element (the clef is
     * needed in case it's a key event, in which case we need to judge
     * the correct pitch for the key)
     */
    virtual void renderSingleElement(ViewElementList::iterator &,
                                     const Clef &,
                                     const ::Rosegarden::Key &,
                                     bool selected);

    bool isDirectlyPrintable(ViewElement *elt);

    void setTuplingParameters(NotationElement *, NotePixmapParameters &);

    /**
     * Set a sprite representing the given note event to the given notation element
     */
    virtual void renderNote(ViewElementList::iterator &);

    /**
     * Return a NotationElementList::iterator pointing to the
     * start of a bar prior to the given time that doesn't appear
     * to have been affected by any changes around that time
     */
    NotationElementList::iterator findUnchangedBarStart(timeT);

    /**
     * Return a NotationElementList::iterator pointing to the
     * end of a bar subsequent to the given time that doesn't appear
     * to have been affected by any changes around that time
     */
    NotationElementList::iterator findUnchangedBarEnd(timeT);

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

    enum FitPolicy {
        PretendItFittedAllAlong = 0,
        MoveBackToFit,
        SplitToFit
    };

    /**
     * Prepare a painter to draw an object of logical width w at
     * layout-x coord x, starting at offset dx into the object, by
     * setting the painter's clipping so as to crop the object at the
     * right edge of the row if it would otherwise overrun.  The
     * return value is the amount of the object visible on this row
     * (i.e. the increment in offset for the next call to this method)
     * or zero if no crop was necessary.  The canvas coords at which
     * the object should subsequently be drawn are returned in coords.
     *
     * This function calls painter.save(), and the caller must call
     * painter.restore() after use.
     */
    virtual double setPainterClipping(QPainter *, double layoutX, int layoutY,
                                      double dx, double w, LinedStaffCoords &coords,
                                      FitPolicy policy);

    /**
     * Set a single pixmap to a notation element, or split it into
     * bits if it overruns the end of a row and set the bits
     * separately.
     */
    virtual void setPixmap(NotationElement *, QCanvasPixmap *, int z,
                           FitPolicy policy);

    bool isSelected(NotationElementList::iterator);

    typedef std::set<QCanvasSimpleSprite *> SpriteSet;
    SpriteSet m_timeSigs;

    typedef std::set<QCanvasItem *> ItemSet;
    ItemSet m_repeatedClefsAndKeys;

    typedef std::pair<int, Clef> ClefChange;
    FastVector<ClefChange> m_clefChanges;

    typedef std::pair<int, ::Rosegarden::Key> KeyChange;
    FastVector<KeyChange> m_keyChanges;

    void truncateClefsAndKeysAt(int);

    NotePixmapFactory *m_notePixmapFactory;
    NotePixmapFactory *m_graceNotePixmapFactory;
    QCanvasSimpleSprite *m_previewSprite;
    QCanvasSimpleSprite *m_staffName;
    std::string m_staffNameText;
    NotationView *m_notationView;
    int m_legerLineCount;
    int m_barNumbersEvery;
    bool m_colourQuantize;
    bool m_showUnknowns;
    bool m_showRanges;
    int m_keySigCancelMode;

    QPainter *m_printPainter;

    enum BarStatus { UnRendered = 0, Rendered, Positioned };
    typedef std::map<int, BarStatus> BarStatusMap;
    BarStatusMap m_status;
    std::pair<int, int> m_lastRenderCheck;
    bool m_ready;
};


}

#endif
