/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SETS_H
#define RG_SETS_H

#include <vector>
#include <algorithm>

#include "Event.h"
#include "base/Segment.h"
#include "CompositionTimeSliceAdapter.h"
#include "base/BaseProperties.h"
#include "base/NotationTypes.h"
#include "base/MidiTypes.h"
#include "Quantizer.h"
#include "misc/Debug.h"

namespace Rosegarden
{

class Quantizer;

/**
 * A "set" in Rosegarden terminology is a collection of elements found
 * in a container (indeed, a subset of that container) all of which
 * share a particular property and are located near to one another:
 * generally either contiguous or within the same bar.  The elements
 * are most usually Events and the container most usually a Segment,
 * and although this does not have to be the case (for other examples
 * see gui/notationsets.h), the elements do have to be convertible to
 * Events somehow.
 *
 * To construct a set requires (at least) a container reference plus
 * an iterator into that container.  Derived classes must call
 * initialise().  It has to be done this way otherwise the expected
 * virtual functions wouldn't get called.
 * 
 * The initialise() method scans the surrounding area of the list for
 * the maximal set of contiguous or within-the-same-bar elements
 * before and after the passed-in iterator such that all elements are
 * in the same set (i.e. Chord, BeamedGroup etc) as the one that the
 * passed-in iterator pointed to.
 *
 * The extents of the set within the list can then be discovered via
 * getInitialElement() and getFinalElement().  If the iterator passed
 * in to the constructor was at end() or did not point to an element
 * that could be a member of this kind of set, getInitialElement()
 * will return end(); if the passed-in iterator pointed to the only
 * member of this set, getInitialElement() and getFinalElement() will
 * be equal.
 *
 * These classes are not intended to be stored anywhere; all they
 * contain is iterators into the main container, and those might not
 * persist.  Instead you should create these on-the-fly when you want,
 * for example, to consider a note as part of a chord; and then you
 * should let them expire when you've finished with them.
 */

template <class Element, class Container>
class AbstractSet // abstract base
{
public:
    typedef typename Container::iterator Iterator;

    virtual ~AbstractSet() { }

    /**
     * getInitialElement() returns end() if there are no elements in
     * the set.  getInitialElement() == getFinalElement() if there is
     * only one element in the set
     */
    Iterator getInitialElement() const  { return m_initial;  }
    Iterator getFinalElement() const    { return m_final;    }

    /// only return note elements; will return end() if there are none
    Iterator getInitialNote() const     { return m_initialNote; }
    Iterator getFinalNote() const       { return m_finalNote;   }

    /**
     * only elements with duration > 0 are candidates for shortest and
     * longest; these will return end() if there are no such elements
     */
    Iterator getLongestElement() const  { return m_longest;  }
    Iterator getShortestElement() const { return m_shortest; }

    /// these will return end() if there are no note elements in the set
    Iterator getHighestNote() const     { return m_highest;  }
    Iterator getLowestNote() const      { return m_lowest;   }

    virtual bool contains(const Iterator &) const = 0;

    /// Return the pointed-to element, in Event form (public to work around gcc-2.95 bug)
    static Event *getAsEvent(const Iterator &i);

protected:
    AbstractSet(Container &c, Iterator elementInSet, const Quantizer *);
    void initialise();

    // Finish initialization specifically for a derived class
    virtual void initialiseFinish() = 0;

    /// Return true if this element is not definitely beyond bounds of set
    virtual bool test(const Iterator &i) = 0;

    /// Return true if this element, known to test() true, is a set member
    virtual bool sample(const Iterator &i, bool goingForwards);

    Container &getContainer() const { return m_container; }
    const Quantizer &getQuantizer() const { return *m_quantizer; }

    // Data members:

    Container &m_container;
    Iterator m_initial, m_final, m_initialNote, m_finalNote;
    Iterator m_shortest, m_longest, m_highest, m_lowest;
    Iterator m_baseIterator;
    const Quantizer *m_quantizer;
};


/**
 * Chord is subclassed from a vector of iterators; this vector
 * contains iterators pointing at all the notes in the chord, in
 * ascending order of pitch.  You can also track through all the
 * events in the chord by iterating from getInitialElement() to
 * getFinalElement(), but this will only get them in the order in
 * which they appear in the original container.
 *
 * However, the notes in a chord might not be contiguous events in the
 * container, as there could be other zero-duration events such as
 * controllers (or even conceivably some short rests) between notes in
 * the same chord, depending on the quantization settings.  The Chord
 * itself only contains iterators pointing at the notes, so if you
 * want to iterate through all events spanned by the Chord, iterate
 * from getInitialElement() to getFinalElement() instead.
 *
 * This class can tell you various things about the chord it
 * describes, but not everything.  It can't tell you whether the
 * chord has a stem, for example, because that depends on the
 * notation style and system in use.  See gui/notationsets.h
 * for a NotationChord class (subclassed from this) that can.
 */

template <class Element, class Container, bool singleStaff>
class GenericChord : public AbstractSet<Element, Container>,
                     public std::vector<typename Container::iterator>
{
public:
    typedef typename Container::iterator Iterator;

    /* You only need to provide the clef and key if the notes
       making up your chord lack HEIGHT_ON_STAFF properties, in
       which case this constructor will write those properties
       in to the chord for you */
    GenericChord(Container &c,
                 Iterator elementInChord,
                 const Quantizer *quantizer,
                 PropertyName stemUpProperty = PropertyName::EmptyPropertyName);

    ~GenericChord() override;

    virtual int getMarkCountForChord() const;
    virtual std::vector<Mark> getMarksForChord() const;
    virtual std::vector<int> getPitches() const;
    bool contains(const Iterator &) const override;

    void initialiseFinish() override;

    /**
     * Return an iterator pointing to the previous note before this
     * chord, or container's end() if there is no previous note.
     */
    virtual Iterator getPreviousNote();

    /**
     * Return an iterator pointing to the next note after this chord,
     * or container's end() if there is no next note.  Remember this
     * class can't know about Segment end marker times, so if your
     * container is a Segment, check the returned note is actually
     * before the end marker.
     */
    virtual Iterator getNextNote();

    /**
     * It's possible for a chord to surround (in the segment) elements
     * that are not members of the chord.  This function returns an
     * iterator pointing to the first of those after the iterator that
     * was passed to the chord's constructor.  If there are none, it
     * returns the container's end().
     */
    virtual Iterator getFirstElementNotInChord();

    virtual int getSubOrdering() { return m_subordering; }

protected:
    bool test(const Iterator&) override;
    bool sample(const Iterator&, bool goingForwards) override;

    class PitchGreater {
    public:
        bool operator()(const Iterator &a, const Iterator &b);
    };

    void copyGroupProperties(Event *e0, Event *e1) const;

    //--------------- Data members ---------------------------------

    PropertyName m_stemUpProperty;
    timeT m_time;
    int m_subordering;
    Iterator m_firstReject;
};

// These class definitions exist just to call `initialise' in
// derived ctors.
class Chord : public GenericChord<Event, Segment, true>
{
    typedef Event    Element;
    typedef Segment  Container;
    static const bool singleStaff = true;

 public:
    Chord(Container &c,
          Iterator elementInChord,
          const Quantizer *quantizer,
          PropertyName stemUpProperty =
          PropertyName::EmptyPropertyName)
        : GenericChord<Element, Container, singleStaff>
              (c, elementInChord, quantizer, stemUpProperty)
        { initialise(); }
};

class GlobalChord : public GenericChord<Event, CompositionTimeSliceAdapter, false>
{
    typedef Event                        Element;
    typedef CompositionTimeSliceAdapter  Container;
    static const bool singleStaff = false;

 public:
    GlobalChord(Container &c,
          Iterator elementInChord,
          const Quantizer *quantizer,
          PropertyName stemUpProperty =
          PropertyName::EmptyPropertyName)
        : GenericChord<Element, Container, singleStaff>
              (c, elementInChord, quantizer, stemUpProperty)
        { initialise(); }
};

///
/// Implementation only from here on.
///

// forward declare hack functions -- see Sets.C for an explanation

extern long
get__Int(Event *e, const PropertyName &name);

extern bool
get__Bool(Event *e, const PropertyName &name);

extern std::string
get__String(Event *e, const PropertyName &name);

extern bool
get__Int(Event *e, const PropertyName &name, long &ref);

extern bool
get__Bool(Event *e, const PropertyName &name, bool &ref);

extern bool
get__String(Event *e, const PropertyName &name, std::string &ref);

extern bool
isPersistent__Bool(Event *e, const PropertyName &name);

extern void
setMaybe__Int(Event *e, const PropertyName &name, long value);

extern void
setMaybe__String(Event *e, const PropertyName &name, const std::string &value);



template <class Element, class Container>
AbstractSet<Element, Container>::AbstractSet(Container &c,
                                             Iterator i, const Quantizer *q):
    m_container(c),
    m_initial(c.end()),
    m_final(c.end()),
    m_initialNote(c.end()),
    m_finalNote(c.end()),
    m_shortest(c.end()),
    m_longest(c.end()),
    m_highest(c.end()),
    m_lowest(c.end()),
    m_baseIterator(i),
    m_quantizer(q)
{
    // ...
}

template <class Element, class Container>
void
AbstractSet<Element, Container>::initialise()
{
    if (m_baseIterator == getContainer().end() || !test(m_baseIterator)) return;

    m_initial = m_baseIterator;
    m_final = m_baseIterator;
    sample(m_baseIterator, true);

    if (AbstractSet::getAsEvent(m_baseIterator)->isa(Note::EventType)) {
        m_initialNote = m_baseIterator;
        m_finalNote = m_baseIterator;
    }

    Iterator i, j;

    // first scan back to find an element not in the desired set,
    // sampling everything as far back as the one after it

    for (i = j = m_baseIterator; i != getContainer().begin() && test(--j); i = j){
        if (sample(j, false)) {
            m_initial = j;
            if (AbstractSet::getAsEvent(j)->isa(Note::EventType)) {
		m_initialNote = j;
		if (m_finalNote == getContainer().end()) {
		    m_finalNote = j;
		}
	    }
        }
    }

    j = m_baseIterator;

    // then scan forwards to find an element not in the desired set,
    // sampling everything as far forward as the one before it

    for (i = j = m_baseIterator; ++j != getContainer().end() && test(j); i = j) {
        if (sample(j, true)) {
            m_final = j;
            if (AbstractSet::getAsEvent(j)->isa(Note::EventType)) {
		m_finalNote = j;
		if (m_initialNote == getContainer().end()) {
		    m_initialNote = j;
		}
	    }
        }
    }

    // And do specific things for the derived class
    initialiseFinish();
}

template <class Element, class Container>
bool
AbstractSet<Element, Container>::sample(const Iterator &i, bool)
{
    const Quantizer &q(getQuantizer());
    Event *e = AbstractSet::getAsEvent(i);
    timeT d(q.getQuantizedDuration(e));
    
    if (e->isa(Note::EventType) || d > 0) {
        if (m_longest == getContainer().end() ||
            d > q.getQuantizedDuration(AbstractSet::getAsEvent(m_longest))) {
//          std::cerr << "New longest in set at duration " << d << " and time " << e->getAbsoluteTime() << std::endl;
            m_longest = i;
        }
        if (m_shortest == getContainer().end() ||
            d < q.getQuantizedDuration(AbstractSet::getAsEvent(m_shortest))) {
//          std::cerr << "New shortest in set at duration " << d << " and time " << e->getAbsoluteTime() << std::endl;
            m_shortest = i;
        }
    }

    if (e->isa(Note::EventType)) {
        long p = get__Int(e, BaseProperties::PITCH);

        if (m_highest == getContainer().end() ||
            p > get__Int(AbstractSet::getAsEvent(m_highest), BaseProperties::PITCH)) {
//          std::cerr << "New highest in set at pitch " << p << " and time " << e->getAbsoluteTime() << std::endl;
            m_highest = i;
        }
        if (m_lowest == getContainer().end() ||
            p < get__Int(AbstractSet::getAsEvent(m_lowest), BaseProperties::PITCH)) {
//          std::cerr << "New lowest in set at pitch " << p << " and time " << e->getAbsoluteTime() << std::endl;
            m_lowest = i;
        }
    }

    return true;
}


//////////////////////////////////////////////////////////////////////
 
template <class Element, class Container, bool singleStaff>
GenericChord<Element, Container, singleStaff>::GenericChord(Container &c,
                                                            Iterator i,
                                                            const Quantizer *q,
                                                            PropertyName stemUpProperty) :
    AbstractSet<Element, Container>(c, i, q),
    m_stemUpProperty(stemUpProperty),
    m_time(q->getQuantizedAbsoluteTime(GenericChord::getAsEvent(i))),
    m_subordering(GenericChord::getAsEvent(i)->getSubOrdering()),
    m_firstReject(c.end())
{
    // initialise must be called in individual derived classes.  If we
    // called it here, it wouldn't use the right
    // virtual functions.
}

template <class Element, class Container, bool singleStaff>
GenericChord<Element, Container, singleStaff>::~GenericChord()
{
}

template <class Element, class Container, bool singleStaff>
void
GenericChord<Element, Container, singleStaff>::
initialiseFinish()
{
    if (std::vector<typename Container::iterator>::size() > 1) {
        std::stable_sort(std::vector<typename Container::iterator>::begin(),
			 std::vector<typename Container::iterator>::end(),
			 PitchGreater());
    }
 }

template <class Element, class Container, bool singleStaff>
bool
GenericChord<Element, Container, singleStaff>::test(const Iterator &i)
{
    Event *e = GenericChord::getAsEvent(i);
    if (AbstractSet<Element, Container>::
	getQuantizer().getQuantizedAbsoluteTime(e) != m_time) {
	return false;
    }
    if (e->getSubOrdering() != m_subordering) {
	return false;
    }

    // We permit note or rest events etc here, because if a chord is a
    // little staggered (for performance reasons) then it's not at all
    // unlikely we could get other events (even rests) in the middle
    // of it.  So long as sample() only permits notes, we should be
    // okay with this.
    //
    // (We're really only refusing things like clef and key events
    // here, though it's slightly quicker [since most things are
    // notes] and perhaps a bit safer to do it by testing for
    // inclusion rather than exclusion.)

    std::string type(e->getType());
    return (type == Note::EventType ||
            type == Note::EventRestType ||
            type == Text::EventType ||
            type == Indication::EventType ||
            type == PitchBend::EventType ||
            type == Controller::EventType ||
            type == KeyPressure::EventType ||
            type == ChannelPressure::EventType);
}

template <class Element, class Container, bool singleStaff>
bool
GenericChord<Element, Container, singleStaff>::sample(const Iterator &i,
                                                      bool goingForwards)
{
    Event *e1 = GenericChord::getAsEvent(i);
    if (!e1->isa(Note::EventType)) {
        if (goingForwards && m_firstReject == AbstractSet<Element, Container>::getContainer().end()) m_firstReject = i;
        return false;
    }

    if (singleStaff) {

        // Two notes that would otherwise be in a chord but are
        // explicitly in different groups, or have stems pointing in
        // different directions by design, or have substantially
        // different x displacements, count as separate chords.
        
        // Per #930473 ("Inserting notes into beamed chords is
        // broken"), if one note is in a group and the other isn't,
        // that's no problem.  In fact we should actually modify the
        // one that isn't so as to suggest that it is.

        if (AbstractSet<Element, Container>::m_baseIterator != AbstractSet<Element, Container>::getContainer().end()) {

            Event *e0 = GenericChord::getAsEvent(AbstractSet<Element, Container>::m_baseIterator);

            if (!(m_stemUpProperty == PropertyName::EmptyPropertyName)) {

                if (e0->has(m_stemUpProperty) &&
                    e1->has(m_stemUpProperty) &&
                    isPersistent__Bool(e0, m_stemUpProperty) &&
                    isPersistent__Bool(e1, m_stemUpProperty) &&
                    get__Bool(e0, m_stemUpProperty) !=
                    get__Bool(e1, m_stemUpProperty)) {

                    if (goingForwards && m_firstReject == AbstractSet<Element, Container>::getContainer().end())
                        m_firstReject = i;
                    return false;
                }
            }

            long dx0 = 0, dx1 = 0;
            get__Int(e0, BaseProperties::DISPLACED_X, dx0);
            get__Int(e1, BaseProperties::DISPLACED_X, dx1);
            if (abs(dx0 - dx1) >= 700) {
                if (goingForwards && m_firstReject == AbstractSet<Element, Container>::getContainer().end())
                    m_firstReject = i;
                return false;
            }

            if (e0->has(BaseProperties::BEAMED_GROUP_ID)) {
                if (e1->has(BaseProperties::BEAMED_GROUP_ID)) {
                    if (get__Int(e1, BaseProperties::BEAMED_GROUP_ID) !=
                        get__Int(e0, BaseProperties::BEAMED_GROUP_ID)) {
                        if (goingForwards && m_firstReject == AbstractSet<Element, Container>::getContainer().end())
                            m_firstReject = i;
                        return false;
                    }
                } else {
                    copyGroupProperties(e0, e1); // #930473
                }
            } else {
                if (e1->has(BaseProperties::BEAMED_GROUP_ID)) {
                    copyGroupProperties(e1, e0); // #930473
                }
            }
        }
    }

    AbstractSet<Element, Container>::sample(i, goingForwards);
    this->push_back(i);
    return true;
}

template <class Element, class Container, bool singleStaff>
void
GenericChord<Element, Container, singleStaff>::copyGroupProperties(Event *e0,
                                                                   Event *e1) const
{
    if (e0->has(BaseProperties::BEAMED_GROUP_TYPE)) {
        setMaybe__String(e1, BaseProperties::BEAMED_GROUP_TYPE,
                         get__String(e0, BaseProperties::BEAMED_GROUP_TYPE));
    }
    if (e0->has(BaseProperties::BEAMED_GROUP_ID)) {
        setMaybe__Int(e1, BaseProperties::BEAMED_GROUP_ID,
                      get__Int(e0, BaseProperties::BEAMED_GROUP_ID));
    }
    if (e0->has(BaseProperties::BEAMED_GROUP_TUPLET_BASE)) {
        setMaybe__Int(e1, BaseProperties::BEAMED_GROUP_TUPLET_BASE,
                      get__Int(e0, BaseProperties::BEAMED_GROUP_TUPLET_BASE));
    }
    if (e0->has(BaseProperties::BEAMED_GROUP_TUPLED_COUNT)) {
        setMaybe__Int(e1, BaseProperties::BEAMED_GROUP_TUPLED_COUNT,
                      get__Int(e0, BaseProperties::BEAMED_GROUP_TUPLED_COUNT));
    }
    if (e0->has(BaseProperties::BEAMED_GROUP_UNTUPLED_COUNT)) {
        setMaybe__Int(e1, BaseProperties::BEAMED_GROUP_UNTUPLED_COUNT,
                      get__Int(e0, BaseProperties::BEAMED_GROUP_UNTUPLED_COUNT));
    }
}


template <class Element, class Container, bool singleStaff>
int
GenericChord<Element, Container, singleStaff>::getMarkCountForChord() const
{
    // need to weed out duplicates

    std::set<Mark> cmarks;

    for (unsigned int i = 0; i < std::vector<typename Container::iterator>::size(); ++i) {

        Event *e = GenericChord::getAsEvent((*this)[i]);
        std::vector<Mark> marks(Marks::getMarks(*e));

        for (std::vector<Mark>::iterator j = marks.begin(); j != marks.end(); ++j) {
            cmarks.insert(*j);
        }
    }

    return cmarks.size();
}


template <class Element, class Container, bool singleStaff>
std::vector<Mark>
GenericChord<Element, Container, singleStaff>::getMarksForChord() const
{
    std::vector<Mark> cmarks;

    for (unsigned int i = 0; i < std::vector<typename Container::iterator>::size(); ++i) {

        Event *e = GenericChord::getAsEvent((*this)[i]);
        std::vector<Mark> marks(Marks::getMarks(*e));


        for (std::vector<Mark>::iterator j = marks.begin(); j != marks.end(); ++j) {

            // We permit multiple identical fingering marks per chord,
            // but not any other sort
            if (Marks::isFingeringMark(*j) ||
                std::find(cmarks.begin(), cmarks.end(), *j) == cmarks.end()) {
                cmarks.push_back(*j);
            }
        }
    }

    return cmarks;
}


template <class Element, class Container, bool singleStaff>
std::vector<int>
GenericChord<Element, Container, singleStaff>::getPitches() const
{
    std::vector<int> pitches;

    for (typename std::vector<typename Container::iterator>::const_iterator
             i = std::vector<typename Container::iterator>::begin(); i != std::vector<typename Container::iterator>::end(); ++i) {
        if (GenericChord::getAsEvent(*i)->has(BaseProperties::PITCH)) {
            int pitch = get__Int
                (GenericChord::getAsEvent(*i), BaseProperties::PITCH);
            if (pitches.size() > 0 && pitches[pitches.size()-1] == pitch) 
                continue;
            pitches.push_back(pitch);
        }
    }

    return pitches;
}


template <class Element, class Container, bool singleStaff>
bool
GenericChord<Element, Container, singleStaff>::contains(const Iterator &itr) const
{
    for (typename std::vector<typename Container::iterator>::const_iterator
             i = std::vector<typename Container::iterator>::begin();
         i != std::vector<typename Container::iterator>::end(); ++i) {
        if (*i == itr) return true;
    }
    return false;
}


template <class Element, class Container, bool singleStaff>
typename GenericChord<Element, Container, singleStaff>::Iterator
GenericChord<Element, Container, singleStaff>::getPreviousNote()
{
    Iterator i(AbstractSet<Element, Container>::getInitialElement());
    while (1) {
        if (i == AbstractSet<Element, Container>::getContainer().begin()) return AbstractSet<Element, Container>::getContainer().end();
        --i;
        if (GenericChord::getAsEvent(i)->isa(Note::EventType)) {
            return i;
        }
    }
}


template <class Element, class Container, bool singleStaff>
typename GenericChord<Element, Container, singleStaff>::Iterator
GenericChord<Element, Container, singleStaff>::getNextNote()
{
    Iterator i(AbstractSet<Element, Container>::getFinalElement());
    while (  i != AbstractSet<Element, Container>::getContainer().end() &&
           ++i != AbstractSet<Element, Container>::getContainer().end()) {
        if (GenericChord::getAsEvent(i)->isa(Note::EventType)) {
            return i;
        }
    }
    return AbstractSet<Element, Container>::getContainer().end();
}


template <class Element, class Container, bool singleStaff>
typename GenericChord<Element, Container, singleStaff>::Iterator
GenericChord<Element, Container, singleStaff>::getFirstElementNotInChord()
{
    return m_firstReject;
}

        
template <class Element, class Container, bool singleStaff>     
bool
GenericChord<Element, Container, singleStaff>::PitchGreater::operator()(const Iterator &a,
                                                           const Iterator &b)
{
    try {
        long ap = get__Int(GenericChord::getAsEvent(a), BaseProperties::PITCH);
        long bp = get__Int(GenericChord::getAsEvent(b), BaseProperties::PITCH);
        return (ap < bp);
    } catch (const Event::NoData &) {
        RG_WARNING << "Bad karma: PitchGreater failed to find one or both pitches";
        return false;
    }
}


}


#endif

