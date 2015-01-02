/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ZOOMSLIDER_H
#define RG_ZOOMSLIDER_H

#include <QSlider>
#include <vector>


class T;
class QWidget;


namespace Rosegarden
{



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
    ZoomSlider(const std::vector<T> &sizes, T defaultValue,
               Qt::Orientation, QWidget * parent, const char * name=0);

    virtual ~ZoomSlider();
    
    void reinitialise(const std::vector<T> &sizes, T defaultValue);

    const T &getCurrentSize() const;
    const T &getDefault() const;

public slots:
    void setToDefault(); // restore the initial value
    void setSize(T size);
    void increment();
    void decrement();
        
protected:
    static int getIndex(const std::vector<T> &, T size);
    std::vector<T> m_sizes;
    T m_defaultValue;
};


template<class T>
ZoomSlider<T>::ZoomSlider(const std::vector<T> &sizes,
                          T initialSize, Qt::Orientation o,
                          QWidget *parent, const char *name) :
    QSlider(o, parent),
    m_sizes(sizes),
    m_defaultValue(initialSize)
{
    setObjectName(name);
    setMinimum(0);
    setMaximum(sizes.size() - 1);
    setPageStep(1);
    setValue(getIndex(sizes, initialSize));
    setTracking(false);
    setFixedWidth(150);
    setFixedHeight(15);
    setSingleStep(1);
    setTickPosition(TicksBelow);
}

template<class T>
ZoomSlider<T>::~ZoomSlider() { }

template<class T>
int
ZoomSlider<T>::getIndex(const std::vector<T> &sizes, T size)
{
    for (unsigned int i = 0; i < sizes.size(); ++i) {
        if (sizes[i] == size) return i;
    }
    return sizes.size()/2;
}

template<class T>
void
ZoomSlider<T>::reinitialise(const std::vector<T> &sizes, T size)
{ 
    m_sizes = sizes;
    setMinimum(0);
    setMaximum(sizes.size()-1);
    setValue(getIndex(sizes, size));
    setSingleStep(1);
    setTickPosition(TicksBelow);
}

template<class T>
void
ZoomSlider<T>::setToDefault()
{
    setValue(getIndex(m_sizes, m_defaultValue));
}

template <class T>
const T &
ZoomSlider<T>::getCurrentSize() const
{
    return m_sizes[value()];
}

template <class T>
void
ZoomSlider<T>::setSize(T size)
{
    setValue(getIndex(m_sizes, size));
}

template <class T>
void
ZoomSlider<T>::increment()
{
    if (value() + 1 >= int(m_sizes.size())) return;
    setValue(value() + 1);
}

template <class T>
void
ZoomSlider<T>::decrement()
{
    if (value() <= 0) return;
    setValue(value() - 1);
}

template <class T>
const T &
ZoomSlider<T>::getDefault() const
{
    return m_defaultValue;
}




}

#endif
