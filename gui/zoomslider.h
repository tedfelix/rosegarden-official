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

#ifndef ZOOM_SLIDER_H
#define ZOOM_SLIDER_H

#include <vector>
#include <qslider.h>

template <class T>
class ZoomSlider : public QSlider
{
public:

    /**
     * Construct a ZoomSlider offering a selection from the
     * given set of sizes.
     *
     * A ZoomSlider is a not-very-well-named slider widget that
     * offers the user an integral range of values (say, 1..3)
     * but maps those values internally onto a list of "sizes",
     * which may be any values of any type (for example the
     * strings "small", "medium" or "large" or the doubles 1.0,
     * 1.2 and 1.5).  It may be useful where a GUI wants to
     * offer a fairly limited range of sizes or options that
     * may actually be arbitrary values chosen because they
     * work well for some internal reason but that should appear
     * to the user as a nice continuous range.
     */
    ZoomSlider(const std::vector<T> &sizes, T initialValue,
	       Orientation, QWidget * parent, const char * name=0);

    virtual ~ZoomSlider();
    
    void reinitialise(const std::vector<T> &sizes, T initialValue);

    const T &getCurrentSize() const;
        
protected:
    static int getIndex(const std::vector<T> &, T size);
    std::vector<T> m_sizes;
};


template<class T>
ZoomSlider<T>::ZoomSlider(const vector<T> &sizes,
			  T initialSize, Orientation o,
			  QWidget *parent, const char *name) :
    QSlider(0, sizes.size()-1, 1,
            getIndex(sizes, initialSize), o, parent, name),
    m_sizes(sizes)
{
    setTracking(false);
    setFixedWidth(150);
    setFixedHeight(15);
    setLineStep(1);
    setTickmarks(Below);
}

template<class T>
ZoomSlider<T>::~ZoomSlider() { }

template<class T>
int
ZoomSlider<T>::getIndex(const vector<T> &sizes, T size)
{
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == size) return i;
    }
    return sizes.size()/2;
}

template<class T>
void
ZoomSlider<T>::reinitialise(const vector<T> &sizes, T size)
{ 
    m_sizes = sizes;
    setMinValue(0);
    setMaxValue(sizes.size()-1);
    setValue(getIndex(sizes, size));
    setLineStep(1);
    setTickmarks(Below);
}

template <class T>
const T &
ZoomSlider<T>::getCurrentSize() const
{
    return m_sizes[value()];
}


#endif

