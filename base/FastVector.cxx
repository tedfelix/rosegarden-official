// -*- c-file-style:  "bsd" -*-
#ifndef _FAST_VECTOR_CXX_
#define _FAST_VECTOR_CXX_

#include "FastVector.h"
#include <cstdlib> /* for malloc, realloc, free */
#include <cstring> /* for memmove */

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

    cerr << "Warning: duplicate unregistration of FastVector "
	 << "iterator" << endl;
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
	cerr << "Warning: FastVector destroyed with "
	     << m_registeredIterators.size() << " iterators still registered"
	     << endl;
	IteratorSet::iterator itr(m_registeredIterators.begin());
	while (itr != m_registeredIterators.end()) {
	    if (*itr) (*itr)->vectorDestroyedCB();
	    ++itr;
	}
    }
    m_registeredIterators.erase(m_registeredIterators.begin(),
				m_registeredIterators.end());
}

#endif

