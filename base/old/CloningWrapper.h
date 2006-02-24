// -*- c-basic-offset: 4 -*-

// -*- c-file-style:  "bsd" -*-
#ifndef _CLONING_WRAPPER_H_
#define _CLONING_WRAPPER_H_

// This is basically just another smart pointer; it wrappers a
// pointer-type (or some other wacky reference, like a CORBA object
// ref) and deals with aliasing, so that if you copy one of these
// wrappers the object is shared, but if you modify one copy (or at
// least call a non-const accessor) it unshares the data so the other
// one remains unmodified.

#include <cassert>

/* DefaultCloner works for any object that has a clone() method.
   clone() could be virtual, so it works for e.g. lists of pointers
   to various object types with a common base */
   
template <class Tptr>
struct DefaultCloner
{
    Tptr null() const { return 0; }
    Tptr clone(const Tptr t) const { return t->clone(); }
    void release(Tptr t) const { delete t; }
};

/* For CORBA object references, the following cloner works nicely:

template <class T, class Tptr>
struct CorbaCloner
{
    Tptr null() const { return T::_nil(); }
    Tptr clone(const Tptr t) const { return T::_duplicate(t); }
    void release(Tptr t) const { CORBA::release(t); }
};
*/

template <class Tptr, class Cloner = DefaultCloner<Tptr> >
class CloningWrapper
{
public:
    CloningWrapper(Cloner cloner = Cloner()) :
	m_t(cloner.null()), m_count(0), m_cloner(cloner) { }

    CloningWrapper(const Tptr t, Cloner cloner = Cloner()) :
	m_t(m_cloner.clone(t)), m_count(new int(1)), m_cloner(cloner) { }

    CloningWrapper(const CloningWrapper<Tptr, Cloner> &c) :
	m_t(c.m_t), m_count(c.m_count), m_cloner(c.m_cloner)
    {
	if (m_count) ++*m_count;
    }

    virtual ~CloningWrapper()
    {
//	cout << "wrapper dtor on " << this << endl;
	if (m_count && !--*m_count) {
	    m_cloner.release(m_t);
	    delete m_count;
	}
    }

    CloningWrapper &operator=(const CloningWrapper<Tptr, Cloner> &c)
    {
	if (&c != this) {
	    if (m_count && !--*m_count) {
		m_cloner.release(m_t);
		delete m_count;
	    }
	    m_t = c.m_t;
	    m_count = c.m_count;

	    // no -- keep the same cloner; all cloners of the same
	    // type should behave the same anyway, and egcs complains
	    // if you assign an empty object (as cloners tend to be)
//	    m_cloner = c.m_cloner; 

	    if (m_count) ++*m_count;
	}
	return *this;
    }

    virtual bool operator==(const CloningWrapper<Tptr, Cloner> &c) const
    {
	assert(0); // not meaningful: this and c may point to different subclasses
	return false;
    }

    virtual bool operator!=(const CloningWrapper<Tptr, Cloner> &c) const
    {
	assert(0); // not meaningful: this and c may point to different subclasses
	return false;
    }

    virtual bool operator<(const CloningWrapper<Tptr, Cloner> &c) const
    {
	assert(0); // likewise
	return false;
    }

    virtual const Tptr data() const
    {
	return m_t;
    }

    virtual const Tptr operator->() const
    {
	return m_t;
    }
    
    virtual Tptr cloned_data() const	 // caller owns returned value
    {
	return m_cloner.clone(m_t);
    }
    
    // The non-const accessors unshare the data, so that we don't
    // modify all copies.  This isn't completely secure, though -- we
    // could retain the non-const pointer returned from the accessor
    // until after this object's data has become shared again for any
    // reason, and then write through it.  I don't think we can win
    // here without making write-access rather hard work (e.g. with
    // a lock/unlock pair surrounding each access in the client code).

    virtual Tptr data()
    {
	if (m_count && *m_count > 1) {
	    --*m_count;
	    m_t = m_cloner.clone(m_t);
	    m_count = new int(1);
	}
	return m_t;
    }

    virtual Tptr operator->()
    {
	return data();
    }

private:
    Tptr m_t;
    int *m_count;
    Cloner m_cloner;
};

#endif

