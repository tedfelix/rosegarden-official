/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_FAST_VECTOR_H
#define RG_FAST_VECTOR_H

#include <iterator>
#include <cstdlib> /* for malloc, realloc, free */
#include <cstring> /* for memmove */

#include <cassert>


/**
  FastVector is a sequence class with an interface similar to that
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
 
  * BUT: To achieve all this, it cheats.  Objects are moved around
    from place to place in the vector using memmove(), rather than
    deep copy.  If you store objects with internal pointers, they
    will break badly.  Storing simple structures will be no problem,
    and if you just store pointers to objects you'll be fine, but
    it's unwise (for example) to store other containers.
 
  * One other difference from the STL vector: It uses placement new
    with the copy constructor to construct objects, rather than
    the default constructor and assignment.  Thus the copy
    constructor must work on the stored objects, though assignment
    doesn't have to.

  Do not use this class if:

  * You do not require random access (operator[]).  Use the STL
    linked list instead, it'll almost certainly be faster.

  * Your sequence is constructed once at a non-time-critical
    moment, and subsequently is only read.  Use STL vector, as
    it's more standard and lookup is slightly quicker.

  * Your sequence is unlikely to contain more than a dozen objects
    which are only appended (push_back) and you do not require
    prepend (push_front).  Use STL vector, as it's more standard,
    simpler and often quicker in this case.

  * You want to pass sequences to other libraries or return them
    from library functions.  Use a standard container instead.

  * You want to store objects that contain internal pointers or
    that do not have a working copy constructor.

  Chris Cannam, 1996-2001
*/

template <class T>
class FastVector
{ 
public:
    typedef T value_type;
    typedef long size_type;
    typedef long difference_type;

private:
    class iterator_base : public
#if defined(_STL_1997_) || (__GNUC__ > 2) || (__SUNPRO_CC)
    std::iterator<std::random_access_iterator_tag, T, difference_type>
#else
#if defined(__STL_USE_NAMESPACES)
    std::
#endif
    random_access_iterator<T, difference_type>
#endif
    {
    public:
        iterator_base() :
            m_v(0), m_i(-1) {
        }
        iterator_base(const iterator_base &i) :
            m_v(i.m_v), m_i(i.m_i) {
        }
        iterator_base &operator=(const iterator_base &i) {
            if (&i != this) { m_v = i.m_v; m_i = i.m_i; }
            return *this;
        }

        iterator_base &operator--() { --m_i; return *this; }
        iterator_base operator--(int) {
            iterator_base i(*this);
            --m_i;
            return i;
        }
        iterator_base &operator++() { ++m_i; return *this; }
        iterator_base operator++(int) {
            iterator_base i(*this);
            ++m_i;
            return i;
        }

        bool operator==(const iterator_base &i) const {
            return (m_v == i.m_v && m_i == i.m_i);
        }

        bool operator!=(const iterator_base &i) const {
            return (m_v != i.m_v || m_i != i.m_i);
        }

        iterator_base &operator+=(typename FastVector<T>::difference_type i) {
            m_i += i; return *this;
        }
        iterator_base &operator-=(typename FastVector<T>::difference_type i) {
            m_i -= i; return *this;
        }

        iterator_base operator+(typename FastVector<T>::difference_type i) const {
            iterator_base n(*this); n += i; return n;
        }
        iterator_base operator-(typename FastVector<T>::difference_type i) const {
            iterator_base n(*this); n -= i; return n;
        }

        typename FastVector<T>::difference_type operator-(const iterator_base &i) const{
            assert(m_v == i.m_v);
            return m_i - i.m_i;
        }

    protected:
        iterator_base(FastVector<T> *v, size_type i) : m_v(v), m_i(i) { }
        FastVector<T> *m_v;
        size_type m_i;
    };

public:
    // I'm sure these can be simplified

    class iterator : public
    iterator_base
    {
    public:
        iterator() : iterator_base() { }
        iterator(const iterator_base &i) : iterator_base(i) { }
        iterator &operator=(const iterator &i) {
            iterator_base::operator=(i);
            return *this;
        }

        T &operator*() { return iterator_base::m_v->at(iterator_base::m_i); }
        T *operator->() { return &(operator*()); }

        const T &operator*() const { return iterator_base::m_v->at(iterator_base::m_i); }
        const T *operator->() const { return &(operator*()); }

    protected:
        friend class FastVector<T>;
        iterator(FastVector<T> *v, size_type i) : iterator_base(v,i) { }
    };

    class reverse_iterator : public
    iterator_base
    {
    public:
        reverse_iterator() : iterator_base() { }
        reverse_iterator(const iterator_base &i) : iterator_base(i) { }
        reverse_iterator &operator=(const reverse_iterator &i) {
            iterator_base::operator=(i);
            return *this;
        }

        T &operator*() { return iterator_base::m_v->at(iterator_base::m_v->size() - iterator_base::m_i - 1); }
        T *operator->() { return &(operator*()); }

        const T &operator*() const { return iterator_base::m_v->at(iterator_base::m_v->size() - iterator_base::m_i - 1); }
        const T *operator->() const { return &(operator*()); }

    protected:
        friend class FastVector<T>;
        reverse_iterator(FastVector<T> *v, size_type i) : iterator_base(v,i) { }
    };

    class const_iterator : public
    iterator_base
    {
    public:
        const_iterator() : iterator_base() { }
        const_iterator(const iterator_base &i) : iterator_base(i) { }
        const_iterator &operator=(const const_iterator &i) {
            iterator_base::operator=(i);
            return *this;
        }

        const T &operator*() const { return iterator_base::m_v->at(iterator_base::m_i); }
        const T *operator->() const { return &(operator*()); }

    protected:
        friend class FastVector<T>;
        const_iterator(const FastVector<T> *v, size_type i) :
            iterator_base(const_cast<FastVector<T> *>(v),i) { }
    };

    class const_reverse_iterator : public
    iterator_base
    {
    public:
        const_reverse_iterator() : iterator_base() { }
        const_reverse_iterator(const iterator_base &i) : iterator_base(i) { }
        const_reverse_iterator &operator=(const const_reverse_iterator &i) {
            iterator_base::operator=(i);
            return *this;
        }

        const T &operator*() const { return iterator_base::m_v->at(iterator_base::m_v->size() - iterator_base::m_i - 1); }
        const T *operator->() const { return &(operator*()); }

    protected:
        friend class FastVector<T>;
        const_reverse_iterator(const FastVector<T> *v, size_type i) :
            iterator_base(const_cast<FastVector<T> *>(v),i) { }
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

    virtual const_iterator begin() const { return const_iterator(this, 0); }
    virtual const_iterator end() const { return const_iterator(this, m_count); }

    virtual reverse_iterator rbegin() { return reverse_iterator(this, 0); }
    virtual reverse_iterator rend() { return reverse_iterator(this, m_count); }

    virtual const_reverse_iterator rbegin() const { return const_reverse_iterator(this, 0); }
    virtual const_reverse_iterator rend() const { return const_reverse_iterator(this, m_count); }

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
        return (const_cast<FastVector<T> *>(this))->at(index);
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

protected:
    /// basic insert -- all others call this
    virtual void insert(size_type index, const T&);

    /// basic remove -- erase(), clear() call this
    virtual void remove(size_type index);

private:
    void resize(size_type needed); // needed is internal (i.e. including gap)

    void moveGapTo(size_type index);    // index is external
    void closeGap() {
        if (m_gapStart >= 0) moveGapTo(m_count);
        m_gapStart = -1;
    }

    size_type bestNewCount(size_type n, size_t) const {
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
    size_type m_count;          // not counting gap
    size_type m_gapStart;               // -1 for no gap
    size_type m_gapLength;              // undefined if no gap
    size_type m_size;
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
    free(static_cast<void *>(m_items));
}

template <class T>
FastVector<T>& FastVector<T>::operator=(const FastVector<T>& l)
{
    if (&l == this) return *this;

    clear();

    if (l.size() >= m_size) resize(l.size());
    for (size_type i = 0; i < l.size(); ++i) push_back(l.at(i));

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
        m_items = static_cast<T *>(realloc(m_items, newSize * sizeof(T)));
    } else {
        m_items = static_cast<T *>(malloc(newSize * sizeof(T)));
    }

    m_size = newSize;
}

template <class T>
void FastVector<T>::remove(size_type index)
{
    assert(index >= 0 && index < m_count);

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
        } else {                // no gap, make one
            m_gapStart = index;
            m_items[m_gapStart].T::~T();
            m_gapLength = 1;
        }
    }

    if (--m_count == 0) m_gapStart = -1;
    if (m_count < m_size/3 && m_size > minSize()) {
        closeGap();
        resize(m_count);        // recover some memory
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

        new (this, &m_items[externalToInternal(index)]) T(t);

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

        new (this, &m_items[index]) T(t);

    } else {
        
        // There's already a gap, all we have to do is move it (with
        // no need to resize)

        if (index != m_gapStart) moveGapTo(index);
        new (this, &m_items[m_gapStart]) T(t);
        if (--m_gapLength == 0) m_gapStart = -1;
        else ++m_gapStart;
    }

    ++m_count;
}

template <class T>
template <class InputIterator>
typename FastVector<T>::iterator FastVector<T>::insert
(const typename FastVector<T>::iterator &p, InputIterator &i, InputIterator &j)
{
    size_type n = p.m_i;
    while (i != j) {
        --j;
        insert(n, *j);
    }
    return begin() + n;
}

template <class T>
typename FastVector<T>::iterator FastVector<T>::erase
(const typename FastVector<T>::iterator &i, const typename FastVector<T>::iterator &j)
{
    assert(i.m_v == this && j.m_v == this && j.m_i >= i.m_i);
    for (size_type k = i.m_i; k < j.m_i; ++k) remove(i.m_i);
    return iterator(this, i.m_i);
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

#endif


