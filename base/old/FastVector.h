// -*- c-basic-offset: 4 -*-

// -*- c-file-style:  "bsd" -*-
#ifndef _FAST_VECTOR_H_
#define _FAST_VECTOR_H_

#include <iterator>
#include <vector>

/** FastVector is a sequence class with an interface similar to that
    of the STL vector, with several nice properties and one nasty one:
 
  * It allows fast random access, like the STL vector -- although
    access is not quite as fast, as a little arithmetic is required.
 
  * Appending (push_back) and prepending (push_front) are both fast.
 
  * The worst-case behaviour is repeated random inserts and deletes
    of single items, and performance in this case is still as good
    as vector where builtin types are stored, and much better where
    deep-copied objects are stored.
 
  * Performance is not as good as vector for very short sequences
    (where vector's simple implementation excels), but it's not bad.
 
  * You can subclass the iterator so as to request callbacks when
    elements are inserted or deleted (the RobustIteratorVector at
    bottom is a simple example).  This obviously dents performance;
    you get a proportionality to the number of iterators that have
    registered this request.  However, this facility imposes
    virtually no overhead on the non-subclassed, default version.
 
  * BUT: To achieve all this, it cheats.  Objects are moved around
    from place to place in the vector using memmove(), rather than
    deep copy.  If you store objects with internal pointers, they
    will break badly.  Storing simple structures will be no problem,
    and if you just store pointers to objects you'll be fine.
 
  * One other difference from the STL vector: It uses placement new
    with the copy constructor to construct objects, rather than
    the default constructor and assignment.  Thus the copy
    constructor must work on the stored objects, though assignment
    doesn't have to.

  Chris Cannam, 1996-2000
*/

template <class T>
class FastVector
{ 
public:
    typedef T value_type;
    typedef signed long size_type;

    class iterator
#ifdef _STL_1997_
	: public ::std::iterator<::std::random_access_iterator_tag, T, size_type>
#else
        : public 
#ifdef __STL_USE_NAMESPACES
         ::std::
#endif
         random_access_iterator<T, size_type>
#endif
    {
    public:
	iterator() :
	    m_v(0), m_i(-1), m_registered(false) {
	}
	iterator(const iterator &i) :
	    m_v(i.m_v), m_i(i.m_i), m_registered(false) {
	    if (i.m_registered) registerIterator();
	}
	virtual ~iterator() {
	    if (m_registered) unregisterIterator();
	}

	iterator &operator=(const iterator &i) {
	    if (&i != this) {
		if (m_registered) unregisterIterator();
		m_v = i.m_v; m_i = i.m_i; m_registered = false;
		if (i.m_registered) registerIterator();
	    }
	    return *this;
	}

	iterator &operator--() { --m_i; return *this; }
	iterator operator--(int) { iterator i(*this); --m_i; return i; }
	iterator &operator++() { ++m_i; return *this; }
	iterator operator++(int) { iterator i(*this); ++m_i; return i; }

	bool operator==(const iterator &i) const {
	    return (m_v == i.m_v && m_i == i.m_i);
	}

	bool operator!=(const iterator &i) const {
	    return (m_v != i.m_v || m_i != i.m_i);
	}

	iterator &operator+=(size_type i) { m_i += i; return *this; }
	iterator &operator-=(size_type i) { m_i -= i; return *this; }

	iterator operator+(size_type i) const {
	    iterator n(*this); n += i; return n;
	}
	iterator operator-(size_type i) const {
	    iterator n(*this); n -= i; return n;
	}

	size_type operator-(const iterator &i) const {
	    assert(m_v == i.m_v); return m_i - i.m_i;
	}

	T &operator*() { return m_v->at(m_i); }
	const T &operator*() const { return m_v->at(m_i); }

	T *operator->() { return &(operator*()); }
	const T *operator->() const { return &(operator*()); }
	
    protected:
	void registerIterator() {
	    if (!m_v) return;
	    m_v->registerIterator(this);
	    m_registered = true;
	}
	void unregisterIterator() {
	    if (m_v) m_v->unregisterIterator(this);
	    m_registered = false;
	}

	/** You can subclass the iterator, and your subclass can
	    request notifications of changes by calling
	    registerIterator() from its constructors.  The following
	    methods are the ones that get called to notify it; note
	    that the removeCB is called *before* the elements are
	    actually removed, just in case you want to find out
	    anything about the elements before they go */
	virtual void elementsAddedCB(size_type i, size_type n) { }
	virtual void elementsRemovedCB(size_type i, size_type n) { }
	virtual void vectorDestroyedCB() { }
	size_type position() { return m_i; }

    private:
	friend FastVector<T>;
	iterator(FastVector<T> *v, size_type i) :
	    m_v(v), m_i(i), m_registered(false) { }
	FastVector<T> *m_v;
	size_type m_i;
	bool m_registered;
    };

public: 
    FastVector() :
	m_items(0), m_count(0), m_gapStart(-1),
	m_gapLength(0), m_size(0) { }
    FastVector(const FastVector<T> &);
    virtual ~FastVector();

    /** Requires a compiler that supports template methods; gcc-2.7.2
	doesn't, egcs series and gcc-2.9x do */
    template <class InputIterator>
    FastVector(InputIterator first, InputIterator last) :
	m_items(0), m_count(0), m_gapStart(-1),
	m_gapLength(0), m_size(0) {
	insert(begin(), first, last);
    }

    FastVector<T> &operator=(const FastVector<T> &);

    virtual iterator begin() { return iterator(this, 0); }
    virtual iterator end() { return iterator(this, m_count); }

    size_type size() const { return m_count; }
    bool empty() const { return m_count == 0; }

    /// not all of these are defined yet
    void swap(FastVector<T> &v);
    bool operator==(const FastVector<T> &) const;
    bool operator!=(const FastVector<T> &v) const { return !operator==(v); }
    bool operator<(const FastVector<T> &) const;
    bool operator>(const FastVector<T> &) const;
    bool operator<=(const FastVector<T> &) const;
    bool operator>=(const FastVector<T> &) const;

    T& at(size_type index) {
	assert(index >= 0 && index < m_count);
	return m_items[externalToInternal(index)];
    }
    const T& at(size_type index) const {
	return ((FastVector<T> *)this)->at(index);
    }
 
    T &operator[](size_type index) {
	return at(index);
    }
    const T &operator[](size_type index) const {
	return at(index);
    }

    virtual T* array(size_type index, size_type count); 

    /** We *guarantee* that push methods etc modify the FastVector
	only through a call to insert(size_type, T), and that erase
	etc modify it only through a call to remove(size_type).  This
	is important because subclasses only need to override those
	functions to catch all mutations */
    virtual void push_front(const T& item) { insert(0, item); }
    virtual void push_back(const T& item) { insert(m_count, item); }

    virtual iterator insert(const iterator &p, const T &t) {
	insert(p.m_i, t);
	return p;
    }

    /** Requires a compiler that supports template methods; gcc-2.7.2
	doesn't, egcs series and gcc-2.9x do */
    template <class InputIterator>
    iterator insert(const iterator &p, InputIterator &i, InputIterator &j);
    
    virtual iterator erase(const iterator &i) {
	assert(i.m_v == this);
	remove(i.m_i);
	return iterator(this, i.m_i);
    }

    virtual iterator erase(const iterator &i, const iterator &j);
    virtual void clear();

    /// conceptually private for call from the iterator class
    void registerIterator(iterator *const);

    /// conceptually private for call from the iterator class
    void unregisterIterator(iterator *const);

protected:
    /// basic insert -- all others call this
    virtual void insert(size_type index, const T&);

    /// basic remove -- erase(), clear() call this
    virtual void remove(size_type index, bool suppressCB = false);

private:
    void resize(size_type needed); // needed is internal (i.e. including gap)

    void moveGapTo(size_type index);	// index is external
    void closeGap() {
	if (m_gapStart >= 0) moveGapTo(m_count);
	m_gapStart = -1;
    }

    size_type bestNewCount(size_type n, size_t s) const {
	if (m_size == 0) {
	    if (n < 8) return 8;
	    else return n;
	} else {
	    // double up each time -- it's faster than just incrementing
	    size_type s(m_size);
	    if (s > n*2) return s/2;
	    while (s <= n) s *= 2;
	    return s;
	}
    }

    size_type externalToInternal(size_type index) const {
	return ((index < m_gapStart || m_gapStart < 0) ?
		index : index + m_gapLength);
    } 

    size_type minSize() const { return 8; }
    size_t minBlock() const {
	return minSize() * sizeof(T) > 64 ? minSize() * sizeof(T) : 64;
    }

    T* m_items;
    size_type m_count;		// not counting gap
    size_type m_gapStart;		// -1 for no gap
    size_type m_gapLength;		// undefined if no gap
    size_type m_size;

    void elementsAdded(size_type i, size_type n) const;
    void elementsRemoved(size_type i, size_type n) const;
    void vectorDestroyed();

    // vector is much faster than set in this situation, because we
    // frequently have to traverse the entire set and callback on
    // every element, but we only relatively rarely insert into or
    // delete from it.  I suspect also that the most common delete is
    // the one we just inserted, so reverse search should work well too.
    // Can be an object not a pointer; an empty vector is small and fast
    typedef ::std::vector<iterator *> IteratorSet;
    IteratorSet m_registeredIterators;
};  

template <class T> void *operator new(size_t size, FastVector<T> *list,
				      void *space);


/** RobustIteratorVector is an example of a class that provides an
    iterator derived from the basic one and requesting notifications.
    This one keeps each iterator pointing at the same element, by
    moving it to segment when other elements are inserted or deleted
    before it.
*/

template <class T>
class RobustIteratorVector : public FastVector<T>
{
public:
    class iterator : public FastVector<T>::iterator
    {
    public:
	iterator() : FastVector<T>::iterator() {
	    registerIterator();
	}
	iterator(const iterator &i) : FastVector<T>::iterator(i) {
	    registerIterator();
	}

	/** This is what makes begin() and end() work -- we can't
	    override them, as they don't return a ref so we'd need
	    covariant return */
	iterator(const FastVector<T>::iterator &i) :
	    FastVector<T>::iterator(i) {
	    registerIterator();
	}

	virtual ~iterator() { }

    protected:

	virtual void elementsAddedCB(size_type i, size_type n) {
	    if (i <= position()) operator+=(n);
	}
	virtual void elementsRemovedCB(size_type i, size_type n) {
	    if (i < position()) operator-=(n);
	}
	virtual void vectorDestroyedCB() {
	    // nothing to see here
	}
    };

public:
    RobustIteratorVector() : FastVector<T>() { }
    RobustIteratorVector(const RobustIteratorVector<T> &l) : FastVector<T>(l) { }
    virtual ~RobustIteratorVector() { }

    template <class InputIterator>
    RobustIteratorVector(InputIterator first, InputIterator last) :
	FastVector<T>(first, last) { }
};


#endif

// For gcc-2.7.2 this was a terrible inefficiency, but newer compilers
// seem to expect it:
#include "FastVector.cxx"

