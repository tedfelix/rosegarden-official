/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
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

#define RG_MODULE_STRING "[PresetGroup]"

#include "PresetGroup.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "gui/general/ClefIndex.h"
#include "base/Exception.h"
#include "CategoryElement.h"
#include "gui/general/ResourceFinder.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QString>
#include <QLocale>

//#define DEBUG_CATEGORIES

namespace Rosegarden
{

PresetGroup::PresetGroup() :
        m_errorString(tr("unknown error")),
        m_elCategoryName(""),
        m_elInstrumentName(""),
        m_elClef(0),
        m_elTranspose(0),
        m_elLowAm(0),
        m_elHighAm(0),
        m_elLowPro(0),
        m_elHighPro(0),
        m_lastCategory( -1),
        m_currentCategory( -1),
        m_lastInstrument( -1),
        m_currentInstrument( -1),
        m_name(false),
        m_clef(false),
        m_transpose(false),
        m_amateur(false),
        m_pro(false)
{
    // NOTE: the following code was simplified to remove three layers of trying
    // different ways to find presets.xml which were left over from the old
    // translation mechanism of having multiple presets-XX.xml to choose from.
    // Once cleaned of this language property, all statements distilled down to
    // the same statement, so I removed the redundant code.
    ResourceFinder rf;
    QString presetFileName = rf.getResourcePath
        ("presets", QString("presets.xml"));

    if (presetFileName == "" || !QFileInfo(presetFileName).isReadable()) {

        RG_DEBUG << "Failed to open " << presetFileName << endl;

        throw PresetFileReadFailed
        (qstrtostr(tr("Can't open preset file %1").arg(presetFileName)));
    }

    QFile presetFile(presetFileName);

    QXmlInputSource source(&presetFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    bool ok = reader.parse(source);
    presetFile.close();

    if (!ok) {
        throw PresetFileReadFailed(qstrtostr(m_errorString));
    }
}

PresetGroup::~PresetGroup()
{
    //!!! do I have anything to do here?
}

bool
PresetGroup::startElement(const QString &, const QString &,
                          const QString &qName,
                          const QXmlAttributes &attributes)
{
    QString lcName = qName.toLower();

    //    RG_DEBUG << "PresetGroup::startElement: processing starting element: " << lcName << endl;

    if (lcName == "category") {

        QString s = attributes.value("name");
        if (!s.isEmpty()) {
            m_elCategoryName = s;
            // increment the current category number
            m_lastCategory = m_currentCategory;
            m_currentCategory++;

            // reset the instrument counter going into the new category
            m_lastInstrument = -1;
            m_currentInstrument = -1;

#ifdef DEBUG_CATEGORIES
            RG_DEBUG << "PresetGroup::startElement: adding category " << m_elCategoryName << " last: "
            << m_lastCategory << " curr: " << m_currentCategory << endl;
#endif

            // add new CategoryElement to m_categories, in order to contain
            // subsequent PresetElements
            CategoryElement ce(m_elCategoryName);
            m_categories.push_back(ce);
        }

    } else if (lcName == "instrument") {

        QString s = attributes.value("name");
        if (!s.isEmpty()) {
            m_elInstrumentName = s;
            m_name = true;

            // increment the current instrument number
            m_lastInstrument = m_currentInstrument;
            m_currentInstrument++;
        }

    } else if (lcName == "clef") {
        QString s = attributes.value("type");
        if (!s.isEmpty()) {
        	m_elClef = clefNameToClefIndex(s);
            m_clef = true;
        }
    } else if (lcName == "transpose") {
        QString s = attributes.value("value");
        if (!s.isEmpty()) {
            m_elTranspose = s.toInt();
            m_transpose = true;
        }

    } else if (lcName == "range") {
        QString s = attributes.value("class");

        if (s == "amateur") {
            s = attributes.value("low");
            if (!s.isEmpty()) {
                m_elLowAm = s.toInt();
                m_amateur = true;
            }

            s = attributes.value("high");
            if (!s.isEmpty() && m_amateur) {
                m_elHighAm = s.toInt();
            } else {
                return false;
            }

        } else if (s == "professional") {
            s = attributes.value("low");
            if (!s.isEmpty()) {
                m_pro = true;
                m_elLowPro = s.toInt();
            }

            s = attributes.value("high");
            if (!s.isEmpty() && m_pro) {
                m_elHighPro = s.toInt();
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

    // once we have assembled all the bits, create a new PresetElement
    if (m_name && m_clef && m_transpose && m_amateur && m_pro) {
        m_categories[m_currentCategory].addPreset(m_elInstrumentName,
                m_elClef,
                m_elTranspose,
                m_elHighAm,
                m_elLowAm,
                m_elHighPro,
                m_elLowPro);
        // increment the current instrument
        //!!! (is this ever going to be needed?)
        m_lastInstrument = m_currentInstrument;
        m_currentInstrument++;

        // reset the "do we have a whole preset yet?" flags
        m_name = false;
        m_clef = false;
        m_transpose = false;
        m_amateur = false;
        m_pro = false;
    }

    return true;

} // startElement

bool
PresetGroup::error(const QXmlParseException& exception)
{
    RG_DEBUG << "PresetGroup::error(): jubilation and glee, we have an error, whee!" << endl;

    m_errorString = QString("%1 at line %2, column %3: %4")
                    .arg(exception.message())
                    .arg(exception.lineNumber())
                    .arg(exception.columnNumber())
                    .arg(m_errorString);
    return QXmlDefaultHandler::error(exception);
}

bool
PresetGroup::fatalError(const QXmlParseException& exception)
{
    RG_DEBUG << "PresetGroup::fatalError(): double your jubilation, and triple your glee, a fatal error doth it be!" << endl;
    m_errorString = QString("%1 at line %2, column %3: %4")
                    .arg(exception.message())
                    .arg(exception.lineNumber())
                    .arg(exception.columnNumber())
                    .arg(m_errorString);
    return QXmlDefaultHandler::fatalError(exception);
}

}
