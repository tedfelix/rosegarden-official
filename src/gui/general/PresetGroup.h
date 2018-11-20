/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

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

#ifndef RG_PRESETGROUP_H
#define RG_PRESETGROUP_H

#include "base/Exception.h"
#include "CategoryElement.h"

#include <QString>
#include <qxml.h>

#include <QCoreApplication>


class QXmlParseException;
class QXmlAttributes;


namespace Rosegarden
{

/**
 * Read \c presets.xml from disk and store a collection of PresetElement objects
 * which can then be used to populate and run the chooser GUI
 *
 * The file \c data/presets.xml is generated from \c
 * data/presets/presets-editable.conf using the \c regenerate-presets script.
 * \c presets-editable.conf is a human-editable plain text file whose format
 * should be evident upon reading it.  This extensive database was supplied
 * almost entirely by E. Magnus Johannson
 *
 * The file is loaded and parsed into a \c CategoriesContainer full of
 * categories (eg. Strings, Brass, Single Reeds, Double Reeds) and the
 * individual PresetElements therein contained.
 */
class PresetGroup : public QXmlDefaultHandler
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::PresetGroup)

public:
    /** A typedef used to indicate that reading the file failed */
    typedef Exception PresetFileReadFailed;

    /** Load and parse the XML file into a PresetGroup object */
    PresetGroup();

    /** Destroy the PresetGroup object */
    ~PresetGroup();

    /** Return a \c CategoriesContainer that comprises a list of the categories
     * discovered in the XML file */
    CategoriesContainer  getCategories() { return m_categories; }
    //CategoryElement getCategoryByIndex(int index) { return m_categories [index]; }

    // Xml handler methods:
    /** Handle XML starting elements */
    bool startElement (const QString& namespaceURI, const QString& localName,
                               const QString& qName, const QXmlAttributes& atts) override;

    /** Handle non-fatal parsing errors */
    bool error(const QXmlParseException& exception) override;

    /** Handle fatal parsing errors */
    bool fatalError(const QXmlParseException& exception) override;

    // I don't think I have anything to do with this, but it must return true?
//    bool characters(const QString &) { return true; }

private:

    //--------------- Data members ---------------------------------
    CategoriesContainer m_categories;

    // For use when reading the XML file:
    QString m_errorString;

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
