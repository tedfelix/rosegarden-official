/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    This file is Copyright 2006-2009
	D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[PresetElement]"

#include "PresetElement.h"
#include "misc/Debug.h"

#include <QString>

//#define DEBUG_CATEGORIES

namespace Rosegarden
{

PresetElement::PresetElement(QString name,
                             int clef,
                             int transpose,
                             int highAm,
                             int lowAm,
                             int highPro,
                             int lowPro) :
        m_name (name),
        m_clef (clef),
        m_transpose (transpose),
        m_highAm (highAm),
        m_lowAm (lowAm),
        m_highPro (highPro),
        m_lowPro (lowPro)
{
#ifdef DEBUG_CATEGORIES
    RG_DEBUG << "PresetElement::PresetElement("
    << "    name = " << name << endl
    << "    clef = " << clef << endl
    << "    trns.= " << transpose << endl
    << "    higH = " << highAm << endl
    << "    lowA = " << lowAm << endl
    << "    higP = " << highPro << endl
    << "    lowP = " << lowPro << ")" << endl;
#endif
}

PresetElement::~PresetElement()
{
    // nothing to do
}

}
