/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This file is Copyright 2003
        Mark Hymers         <markh@linuxfromscratch.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COLOUR_H
#define RG_COLOUR_H

#include <QColor>
#include <QDebug>

#include <rosegardenprivate_export.h>

namespace Rosegarden 
{

/**
 * ??? This class should be removed.  QColor should be used instead everywhere
 *     this is used.  The one feature that this class offers
 *     (dataToXmlString()) should be inlined into its caller.
 */
class Colour
{
public:
    Colour() :
        m_r(0),
        m_g(0),
        m_b(0)
    {
    }

    Colour(int red, int green, int blue) :
        m_r(red),
        m_g(green),
        m_b(blue)
    {
    }

    int red() const  { return m_r; }
    int green() const  { return m_g; }
    int blue() const  { return m_b; }

    operator QColor() const  { return QColor(m_r, m_g, m_b); }

private:
    int m_r;
    int m_g;
    int m_b;
};


}

//ROSEGARDENPRIVATE_EXPORT QDebug &operator<<(QDebug &, const Rosegarden::Colour &);

#endif
