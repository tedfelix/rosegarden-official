/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_CATEGORYELEMENT_H
#define RG_CATEGORYELEMENT_H

#include "PresetElement.h"

#include <QString>


namespace Rosegarden
{


/**
 * A container class for storing a collection of PresetElement objects grouped
 * into the same musical category (eg. Flutes, Clarinets, Strings)
 *
 * \author D. Michael McIntyre
 */
class CategoryElement
{
public:
    /** Create a category of \c name */
    CategoryElement(QString name);

    /** Destroy the category */
    ~CategoryElement();

    /** Add a PresetElement to the CategoryElement 
     *
     * \sa { PresetElement, PresetGroup }
     */
    void addPreset(QString name,
                  int clef,
                  int transpose,
                  int highAm,
                  int lowAm,
                  int highPro,
                  int lowPro);

    /** Return the category name */
    QString getName() { return m_name; }

    /** Return all the instrument presets for this category
     *
     * \sa { PresetElement, PresetGroup, ElementContainer }
     */
    ElementContainer getPresets() { return m_categoryPresets; }

    /** Return the instrument preset at index \c index */
    PresetElement getPresetByIndex(int index) { return m_categoryPresets [index]; }

private:
    QString m_name;
    ElementContainer m_categoryPresets;
}; // CategoryElement

/** A container for storing a collection of CategoryElement objects */
typedef std::vector<CategoryElement> CategoriesContainer;

}

#endif
