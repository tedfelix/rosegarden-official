// -*- c-file-style:  "bsd" -*-

#ifndef _FAST_VECTOR_H_
#define _FAST_VECTOR_H_

#include <iterator>
#include <vector>
#include <cstdlib> /* for malloc, realloc, free */
#include <cstring> /* for memmove */

#ifndef NDEBUG
#include <iostream>
#endif

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

    class iterator : public
    std::iterator<std::random_access_iterator_tag, T, size_type>
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
	friend class FastVector<T>;
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


template <class T>
void *operator new(size_t, FastVector<T> *, void *space)
{
    return space;
}

template <class T>
FastVector<T>::FastVector(const FastVector<T> &l) :
    m_items(0), m_count(0), m_gapStart(-1),
    m_gapLength(0), m_size(0)
{
    resize(l.size());
    for (size_type i = 0; i < l.size(); ++i) push_back(l.at(i));
}

template <class T>
FastVector<T>::~FastVector()
{
    clear();
    vectorDestroyed();
    free((void *)m_items);
}

template <class T>
FastVector<T>& FastVector<T>::operator=(const FastVector<T>& l)
{
    if (&l == this) return *this;

    if (m_count > 0) elementsRemoved(0, m_count);
    clear();

    if (l.size() >= m_size) resize(l.size());
    for (size_type i = 0; i < l.size(); ++i) push_back(l.at(i));
    if (m_count > 0) elementsAdded(0, m_count);

    return *this;
}

template <class T>
void FastVector<T>::moveGapTo(size_type index)
{
    // shift some elements left or right so as to line the gap up with
    // the prospective insertion or deletion point.

    assert(m_gapStart >= 0);

    if (m_gapStart < index) {
	// need to move some stuff left to fill the gap
	memmove(&m_items[m_gapStart],
		&m_items[m_gapStart + m_gapLength],
		(index - m_gapStart) * sizeof(T));
	
    } else if (m_gapStart > index) {
	// need to move some stuff right to fill the gap
	memmove(&m_items[index + m_gapLength], &m_items[index],
		(m_gapStart - index) * sizeof(T));
    }

    m_gapStart = index;
}

template <class T>
void FastVector<T>::resize(size_type needed)
{
    size_type newSize = bestNewCount(needed, sizeof(T));

    if (m_items) {
	m_items = (T *)realloc(m_items, newSize * sizeof(T));
    } else {
	m_items = (T *)malloc(newSize * sizeof(T));
    }

    m_size = newSize;
}

template <class T>
void FastVector<T>::remove(size_type index, bool suppressCB)
{
    assert(index >= 0 && index < m_count);
    if (!suppressCB) elementsRemoved(index, 1);

    if (index == m_count - 1) {
	// shorten the list without disturbing an existing gap, unless
	// the item we're taking was the only one after the gap
	m_items[externalToInternal(index)].T::~T();
	if (m_gapStart == index) m_gapStart = -1;
    } else {
	if (m_gapStart >= 0) {
	    // moveGapTo shifts the gap around ready for insertion.
	    // It actually moves the indexed object out of the way, so
	    // that it's now at the end of the gap.  We have to cope.
	    moveGapTo(index);
	    m_items[m_gapStart + m_gapLength].T::~T();
	    ++m_gapLength;
	} else {		// no gap, make one
	    m_gapStart = index;
	    m_items[m_gapStart].T::~T();
	    m_gapLength = 1;
	}
    }

    if (--m_count == 0) m_gapStart = -1;
    if (m_count < m_size/3 && m_size > minSize()) {
	closeGap();
	resize(m_count);	// recover some memory
    }
}

template <class T>
void FastVector<T>::insert(size_type index, const T&t)
{
    assert(index >= 0 && index <= m_count);

    if (index == m_count) {

	// Appending.  No need to disturb the gap, if there is one --
	// we'd rather waste a bit of memory than bother closing it up

	if (externalToInternal(m_count) >= m_size || !m_items) {
	    resize(m_size + 1);
	}

	(void) new (this, &m_items[externalToInternal(index)]) T(t);

    } else if (m_gapStart < 0) {

	// Inserting somewhere, when there's no gap we can use.

	if (m_count >= m_size) resize(m_size + 1);

	// I think it's going to be more common to insert elements
	// at the same point repeatedly than at random points.
	// So, if we can make a gap here ready for more insertions
	// *without* exceeding the m_size limit (i.e. if we've got
	// slack left over from a previous gap), then let's.  But
	// not too much -- ideally we'd like some space left for
	// appending.  Say half.

	if (m_count < m_size-2) {
	    m_gapStart = index+1;
	    m_gapLength = (m_size - m_count) / 2;
	    memmove(&m_items[m_gapStart + m_gapLength], &m_items[index],
		    (m_count - index) * sizeof(T));
	} else {
	    memmove(&m_items[index + 1], &m_items[index],
		    (m_count - index) * sizeof(T));
	}

	(void) new (this, &m_items[index]) T(t);

    } else {
	
	// There's already a gap, all we have to do is move it (with
	// no need to resize)

	if (index != m_gapStart) moveGapTo(index);
	(void) new (this, &m_items[m_gapStart]) T(t);
	if (--m_gapLength == 0) m_gapStart = -1;
	else ++m_gapStart;
    }

    ++m_count;

    elementsAdded(index, 1);
}

template <class T>
template <class InputIterator>
FastVector<T>::iterator FastVector<T>::insert
(const FastVector<T>::iterator &p, InputIterator &i, InputIterator &j) {
    size_type n = p.m_i;
    while (i != j) {
	--j;
	insert(n, *j);
    }
    return begin() + n;
}

template <class T>
FastVector<T>::iterator FastVector<T>::erase
(const FastVector<T>::iterator &i, const FastVector<T>::iterator &j)
{
    assert(i.m_v == this && j.m_v == this && j.m_i >= i.m_i);

    size_type ip = ::std::distance(begin(), i);
    size_type n = ::std::distance(i, j);
    elementsRemoved(ip, n);

    for (size_type k = i.m_i; k < j.m_i; ++k) remove(i.m_i, true);
    return FastVector<T>::iterator(this, i.m_i);
}

template <class T>
void FastVector<T>::clear()
{
    // Use erase(), which uses remove() -- a subclass that overrides
    // remove() will not want to have to provide this method as well
    erase(begin(), end());
}

template <class T>
T* FastVector<T>::array(size_type index, size_type count)
{
    assert(index >= 0 && count > 0 && index + count <= m_count);

    if (m_gapStart < 0 || index + count <= m_gapStart) {
	return m_items + index;
    } else if (index >= m_gapStart) {
	return m_items + index + m_gapLength;
    } else {
	closeGap();
	return m_items + index;
    }
}

template <class T>
bool FastVector<T>::operator==(const FastVector<T> &v) const
{
    if (size() != v.size()) return false;
    for (size_type i = 0; i < m_count; ++i) {
	if (at(i) != v.at(i)) return false;
    }
    return true;
}

template <class T>
void FastVector<T>::registerIterator(iterator *const i)
{
    m_registeredIterators.push_back(i);
}

template <class T>
void FastVector<T>::unregisterIterator(iterator *const i)
{
    for (IteratorSet::iterator itr = m_registeredIterators.end();
	 itr != m_registeredIterators.begin(); ) {
	--itr;
	if (*itr == i) {
	    m_registeredIterators.erase(itr);
	    return;
	}
    }

#ifndef NDEBUG
    std::cerr << "Warning: duplicate unregistration of FastVector "
              << "iterator" << std::endl;
#endif
}

template <class T>
void FastVector<T>::elementsAdded(size_type i, size_type n) const
{
    IteratorSet::const_iterator itr(m_registeredIterators.begin());
    while (itr != m_registeredIterators.end()) {
	if (*itr) (*itr)->elementsAddedCB(i, n);
	++itr;
    }
}

template <class T>
void FastVector<T>::elementsRemoved(size_type i, size_type n) const
{
    IteratorSet::const_iterator itr(m_registeredIterators.begin());
    while (itr != m_registeredIterators.end()) {
	if (*itr) (*itr)->elementsRemovedCB(i, n);
	++itr;
    }
}

template <class T>
void FastVector<T>::vectorDestroyed()
{
    if (m_registeredIterators.size() > 0) {
#ifndef NDEBUG
	std::cerr << "Warning: FastVector destroyed with "
                  << m_registeredIterators.size()
                  << " iterators still registered" << std::endl;
#endif
	IteratorSet::iterator itr(m_registeredIterators.begin());
	while (itr != m_registeredIterators.end()) {
	    if (*itr) (*itr)->vectorDestroyedCB();
	    ++itr;
	}
    }
    m_registeredIterators.erase(m_registeredIterators.begin(),
				m_registeredIterators.end());
}




/** RobustIteratorVector is an example of a class that provides an
    iterator derived from the basic one and requesting notifications.
    This one keeps each iterator pointing at the same element, by
    moving it to track when other elements are inserted or deleted
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


