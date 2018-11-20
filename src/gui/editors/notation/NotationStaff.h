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

#ifndef RG_NOTATIONSTAFF_H
#define RG_NOTATIONSTAFF_H

#include "base/ViewSegment.h"
#include "base/ViewElement.h"
#include "StaffLayout.h"
#include <map>
#include <set>
#include <string>
#include <utility>
#include "base/Event.h"
#include "NotationElement.h"


class QPainter;
class QGraphicsItem;
class StaffLayoutCoords;


namespace Rosegarden
{

class ViewElement;
class TimeSignature;
class SnapGrid;
class Segment;
class NotePixmapParameters;
class NotePixmapFactory;
class Note;
class NotationScene;
class NotationProperties;
class Key;
class Event;
class Clef;


class NotationStaff : public ViewSegment,
                      public StaffLayout,
                      public QObject  // Just for tr().  Could be cleaned up.
{
    //Q_OBJECT
public:
    NotationStaff(NotationScene *, Segment *, SnapGrid *,
                  int id,
                  NotePixmapFactory *normalFactory,
                  NotePixmapFactory *smallFactory);
    virtual ~NotationStaff();

    void setNotePixmapFactories(NotePixmapFactory *normal,
                                NotePixmapFactory *small);

    void setLegerLineCount(int legerLineCount) {
        if (legerLineCount == -1) m_legerLineCount = 8;
        else m_legerLineCount = legerLineCount;
    }

    void setBarNumbersEvery(int barNumbersEvery) {
        m_barNumbersEvery = barNumbersEvery;
    }

    using StaffLayout::setPageMode;
    using StaffLayout::setPageWidth;
    using StaffLayout::setRowsPerPage;
    using StaffLayout::setRowSpacing;
    using StaffLayout::setConnectingLineLength;

    SegmentRefreshStatus &getRefreshStatus() const;
    void resetRefreshStatus();

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

    void regenerate(timeT from, timeT to, bool secondary);

    /**
     * Generate or re-generate items for all the elements between
     * from and to.  Call this when you've just made a change,
     * specifying the extents of the change in the from and to
     * parameters.
     * 
     * This method does not reposition any elements outside the given
     * range -- so after any edit that may change the visible extents
     * of a range, you will then need to call positionElements for the
     * changed range and the entire remainder of the staff.
     */
    void renderElements(NotationElementList::iterator from,
                                NotationElementList::iterator to) override;

    virtual void renderElements(timeT from, timeT to);

    
    /**
     * Assign suitable coordinates to the elements on the staff,
     * based entirely on the layout X and Y coordinates they were
     * given by the horizontal and vertical layout processes.
     *
     * This is necessary because the items that are being positioned
     * may have been created either after the layout process completed
     * (by renderElements) or before (by the previous renderElements
     * call, if the items are unchanged but have moved) -- so
     * neither the layout nor renderElements can authoritatively set
     * their final positions.
     *
     * This method also updates the selected-ness of any elements it
     * sees (i.e. it turns the selected ones blue and the unselected
     * ones black), and re-generates items for any elements for
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
    void positionElements(timeT from,
                                  timeT to) override;

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
     * Use a gray color if \a grayed is true.
     */
    void insertTimeSignature(double layoutX,
                                     const TimeSignature &timeSig, bool grayed) override;

    /**
     * Delete all time signatures
     */
    void deleteTimeSignatures() override;
    
    /**
     * Insert repeated clef and key at start of new line, at x-coordinate \a x.
     */
    void insertRepeatedClefAndKey(double layoutX, int barNo) override;

    /**
     * Delete all repeated clefs and keys.
     */
    void deleteRepeatedClefsAndKeys() override;

    /**
     * (Re)draw the staff name from the track's current name
     */
    void drawStaffName() override;

    /**
     * Return true if the staff name as currently drawn is up-to-date
     * with that in the composition
     */
    virtual bool isStaffNameUpToDate();

    /**
     * Return the clef and key in force at the given canvas
     * coordinates
     */
    virtual void getClefAndKeyAtSceneCoords(double x, int y,
                                             Clef &clef,
                                             ::Rosegarden::Key &key) const;

    /**
     * Return the note name (C4, Bb3, whatever) corresponding to the
     * given scene coordinates
     */
    QString getNoteNameAtSceneCoords(double x, int y,
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
/*!!!
    virtual ViewElementList::iterator getClosestElementToLayoutX
    (double x, Event *&clef, Event *&key,
     bool notesAndRestsOnly = false,
     int proximityThreshold = 10);
*/

    /**
     * Find the NotationElement "under" the given layout x-coord,
     * without regard to its y-coord.
     *
     * Also returns the clef and key in force at the given coordinates.
     */
    ViewElementList::iterator getElementUnderLayoutX
    (double x, Event *&clef, Event *&key) override;

    /**
     * Draw a note on the staff to show an insert position prior to
     * an insert. 
     */
    virtual void showPreviewNote(double layoutX, int heightOnStaff,
                                 const Note &note, bool grace,
                                 Accidental accidental, bool cautious,
                                 QColor color);

    /**
     * Remove any visible preview note.
     */
    virtual void clearPreviewNote();

    /**
     * Overridden from Staff<T>.
     * We want to avoid wrapping things like controller events, if
     * our showUnknowns preference is off
     */
    bool wrapEvent(Event *) override;

    /**
     * Override from Staff<T>
     * Let tools know if their current element has gone
     */
    void eventRemoved(const Segment *, Event *) override;

    /**
     * Return the view-local PropertyName definitions for this staff's view
     */
    const NotationProperties &getProperties() const;

    double getBarInset(int barNo, bool isFirstBarInRow) const override;

    /**
     * Return the time at the given scene coordinates
     */
    timeT getTimeAtSceneCoords(double x, int y) const;

    bool includesTime(timeT t);
    
protected:

    ViewElement* makeViewElement(Event*) override;

    // definition of staff
    int getLineCount() const         override { return 5; }
    int getLegerLineCount() const    override { return m_legerLineCount; }
    int getBottomLineHeight() const  override { return 0; }
    int getHeightPerLine() const     override { return 2; }
    int showBarNumbersEvery() const  override { return m_barNumbersEvery; }

    BarStyle getBarStyle(int barNo) const override;

    /** 
     * Assign a suitable item to the given element (the clef is
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
     * Set an item representing the given note event to the given notation element
     */
    virtual void renderNote(ViewElementList::iterator &);

    /**
     * Return true if the element has a scene item that is already
     * at the correct y-coordinate
     */
    virtual bool elementNotMovedInY(NotationElement *);

    /**
     * Returns true if the item at the given iterator appears to have
     * moved horizontally without the spacing around it changing.
     * 
     * In practice, calculates the offset between the intended layout
     * and current scene coordinates of the item at the given
     * iterator, and returns true if this offset is equal to those of
     * all other following iterators at the same time as well as the
     * first iterator found at a greater time.
     */
    virtual bool elementShiftedOnly(NotationElementList::iterator);

    virtual bool elementNeedsRegenerating(NotationElementList::iterator);

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
     * or zero if no crop was necessary.  The scene coords at which
     * the object should subsequently be drawn are returned in coords.
     *
     * This function calls painter.save(), and the caller must call
     * painter.restore() after use.
     */
    virtual double setPainterClipping(QPainter *, double layoutX, int layoutY,
                                      double dx, double w, StaffLayoutCoords &coords,
                                      FitPolicy policy);

    /**
     * Set a single item to a notation element, or split it into bits
     * if it overruns the end of a row and can be split, and set the
     * bits separately.
     */
    virtual void setItem(NotationElement *, QGraphicsItem *, int z,
                         FitPolicy policy);

    bool isSelected(NotationElementList::iterator);

    typedef std::set<QGraphicsItem *> ItemSet;
    ItemSet m_timeSigs;
    ItemSet m_repeatedClefsAndKeys;

    typedef std::pair<int, Clef> ClefChange;
    std::vector<ClefChange> m_clefChanges;

    typedef std::pair<int, ::Rosegarden::Key> KeyChange;
    std::vector<KeyChange> m_keyChanges;

    void truncateClefsAndKeysAt(int);

    /**
     * Verify that a possible Clef or Key in bar is already inserted
     * in m_clefChange or m_keyChange.
     * If not, do the insertion.
     */
    void checkAndCompleteClefsAndKeys(int bar);

    NotePixmapFactory *m_notePixmapFactory; // I do not own this
    NotePixmapFactory *m_graceNotePixmapFactory; // I do not own this
    QGraphicsItem *m_previewItem;
    QGraphicsItem *m_staffName;
    std::string m_staffNameText;
    NotationScene *m_notationScene;
    int m_legerLineCount;
    int m_barNumbersEvery;
    bool m_colourQuantize;
    bool m_showUnknowns;
    bool m_showRanges;
    bool m_showCollisions;
    bool m_hideRedundance;
    bool m_distributeVerses;
    int m_keySigCancelMode;

    QPainter *m_printPainter;

    unsigned int m_refreshStatusId;
};


}

#endif
