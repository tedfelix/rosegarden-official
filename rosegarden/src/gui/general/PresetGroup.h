
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    This file is Copyright 2006
	D. Michael McIntyre <dmmcintyr@users.sourceforge.net>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_PRESETGROUP_H_
#define _RG_PRESETGROUP_H_

#include "base/Exception.h"
#include <qstring.h>
#include <qxml.h>


class QXmlParseException;
class QXmlAttributes;


namespace Rosegarden
{

/*
 * Read presets.xml from disk and store a collection of PresetElement objects
 * which can then be used to populate and run the chooser GUI
 */
class PresetGroup : public QXmlDefaultHandler
{
public:
    typedef Exception PresetFileReadFailed;

    PresetGroup(); // load and parse the XML mapping file
    ~PresetGroup();

    CategoriesContainer  getCategories() { return m_categories; }
    //CategoryElement getCategoryByIndex(int index) { return m_categories [index]; }

    // Xml handler methods:

    virtual bool startElement (const QString& namespaceURI, const QString& localName,
                               const QString& qName, const QXmlAttributes& atts);

    bool error(const QXmlParseException& exception);
    bool fatalError(const QXmlParseException& exception);

    // I don't think I have anything to do with this, but it must return true?
//    bool characters(const QString &) { return true; }

private:

    //--------------- Data members ---------------------------------
    CategoriesContainer m_categories;

    // For use when reading the XML file:
    QString m_errorString;
    QString m_presetDirectory;

    QString m_elCategoryName;
    QString m_elInstrumentName;
    int m_elClef;
    int m_elTranspose;
    int m_elLowAm;
    int m_elHighAm;
    int m_elLowPro;
    int m_elHighPro;

    int m_lastCategory;
    int m_currentCategory;
    int m_lastInstrument;
    int m_currentInstrument;

    bool m_name;
    bool m_clef;
    bool m_transpose;
    bool m_amateur;
    bool m_pro;

}; // PresetGroup


}

#endif
