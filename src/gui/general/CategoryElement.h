
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_CATEGORYELEMENT_H_
#define _RG_CATEGORYELEMENT_H_

#include "PresetElement.h"
#include <QString>




namespace Rosegarden
{


/*
 * A container class for storing a collection of PresetElement objects grouped
 * into the same musical category (eg. Flutes, Clarinets, Strings)
 */
class CategoryElement
{
public:
    CategoryElement(QString name);
    ~CategoryElement();

    void addPreset(QString name,
                  int clef,
                  int transpose,
                  int highAm,
                  int lowAm,
                  int highPro,
                  int lowPro);

    QString getName() { return m_name; }

    ElementContainer getPresets() { return m_categoryPresets; }
    PresetElement getPresetByIndex(int index) { return m_categoryPresets [index]; }

private:
    QString m_name;
    ElementContainer m_categoryPresets;
}; // CategoryElement

typedef std::vector<CategoryElement> CategoriesContainer;

}

#endif
