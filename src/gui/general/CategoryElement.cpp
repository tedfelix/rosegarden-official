/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[CategoryElement]"

#include "CategoryElement.h"

#include "misc/Debug.h"
#include "PresetElement.h"
#include <QString>


namespace Rosegarden
{

CategoryElement::CategoryElement(QString name) :
        m_name(name)
{}

CategoryElement::~CategoryElement()
{
    // nothing to do
}

void
CategoryElement::addPreset(QString name,
                           int clef,
                           int transpose,
                           int highAm,
                           int lowAm,
                           int highPro,
                           int lowPro)
{
    RG_DEBUG << "CategoryElement::addPreset(...): adding new PresetElement";

    PresetElement e(name, clef, transpose, highAm, lowAm,
                    highPro, lowPro);
    m_categoryPresets.push_back(e);
}

}
