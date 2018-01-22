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

/*
    This file is derived from

    Sonic Visualiser
    An audio file viewer and annotation editor.
    Centre for Digital Music, Queen Mary, University of London.
    This file copyright 2006 Chris Cannam.
    
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SAMPLE_WINDOW_H
#define RG_SAMPLE_WINDOW_H

#include <cmath>
#include <cstdlib>
#include <map>

namespace Rosegarden 
{

template <typename T>
class SampleWindow
{
public:
    enum Type {
        Rectangular,
        Bartlett,
        Hamming,
        Hanning,
        Blackman,
        Gaussian,
        Parzen,
        Nuttall,
        BlackmanHarris
    };

    /**
     * Construct a windower of the given type.
     */
    SampleWindow(Type type, size_t size) : m_type(type), m_size(size) { encache(); }
    SampleWindow(const SampleWindow &w) : m_type(w.m_type), m_size(w.m_size) { encache(); }
    SampleWindow &operator=(const SampleWindow &w) {
	if (&w == this) return *this;
	m_type = w.m_type;
	m_size = w.m_size;
	encache();
	return *this;
    }
    virtual ~SampleWindow() { delete[] m_cache; }
    
    void cut(T *src) const { cut(src, src); }
    void cut(T *src, T *dst) const {
	for (size_t i = 0; i < m_size; ++i) dst[i] = src[i] * m_cache[i];
    }

    T getArea() { return m_area; }
    T getValue(size_t i) { return m_cache[i]; }

    Type getType() const { return m_type; }
    size_t getSize() const { return m_size; }

protected:
    Type m_type;
    size_t m_size;
    T *m_cache;
    T m_area;
    
    void encache();
    void cosinewin(T *, T, T, T, T);
};

template <typename T>
void SampleWindow<T>::encache()
{
    int n = int(m_size);
    T *mult = new T[n];
    int i;
    for (i = 0; i < n; ++i) mult[i] = 1.0;

    switch (m_type) {
		
    case Rectangular:
	for (i = 0; i < n; ++i) {
	    mult[i] *= 0.5;
	}
	break;
	    
    case Bartlett:
	for (i = 0; i < n/2; ++i) {
	    mult[i] *= (i / T(n/2));
	    mult[i + n/2] *= (1.0 - (i / T(n/2)));
	}
	break;
	    
    case Hamming:
        cosinewin(mult, 0.54, 0.46, 0.0, 0.0);
	break;
	    
    case Hanning:
        cosinewin(mult, 0.50, 0.50, 0.0, 0.0);
	break;
	    
    case Blackman:
        cosinewin(mult, 0.42, 0.50, 0.08, 0.0);
	break;
	    
    case Gaussian:
	for (i = 0; i < n; ++i) {
            mult[i] *= pow(2, - pow((i - (n-1)/2.0) / ((n-1)/2.0 / 3), 2));
	}
	break;
	    
    case Parzen:
    {
        int N = n-1;
        for (i = 0; i < N/4; ++i) {
            T m = 2 * pow(1.0 - (T(N)/2 - i) / (T(N)/2), 3);
            mult[i] *= m;
            mult[N-i] *= m;
        }
        for (i = N/4; i <= N/2; ++i) {
            int wn = i - N/2;
            T m = 1.0 - 6 * pow(wn / (T(N)/2), 2) * (1.0 - abs(wn) / (T(N)/2));
            mult[i] *= m;
            mult[N-i] *= m;
        }            
        break;
    }

    case Nuttall:
        cosinewin(mult, 0.3635819, 0.4891775, 0.1365995, 0.0106411);
	break;

    case BlackmanHarris:
        cosinewin(mult, 0.35875, 0.48829, 0.14128, 0.01168);
        break;
    }
	
    m_cache = mult;

    m_area = 0;
    for (int i = 0; i < n; ++i) {
        m_area += m_cache[i];
    }
    m_area /= n;
}

template <typename T>
void SampleWindow<T>::cosinewin(T *mult, T a0, T a1, T a2, T a3)
{
    int n = int(m_size);
    for (int i = 0; i < n; ++i) {
        mult[i] *= (a0
                    - a1 * cos(2 * M_PI * i / n)
                    + a2 * cos(4 * M_PI * i / n)
                    - a3 * cos(6 * M_PI * i / n));
    }
}

}

#endif
