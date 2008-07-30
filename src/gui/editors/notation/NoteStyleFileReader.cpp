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

#include "NoteStyleFileReader.h"

#include <string>
#include "NoteStyle.h"
#include <qfileinfo.h>
#include <qdir.h>

#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>

#include "misc/Strings.h"
#include "NotationStrings.h"
#include "misc/Debug.h"

namespace Rosegarden {


NoteStyleFileReader::NoteStyleFileReader(std::string name) :
    m_style(new NoteStyle(name)),
    m_haveNote(false)
{
    QString styleDirectory =
	KGlobal::dirs()->findResource("appdata", "styles/");

    QString styleFileName =
	QString("%1/%2.xml").arg(styleDirectory).arg(strtoqstr(name));

    QFileInfo fileInfo(styleFileName);

    if (!fileInfo.isReadable()) {
        throw StyleFileReadFailed
	    (qstrtostr(i18n("Can't open style file %1").arg(styleFileName)));
    }

    QFile styleFile(styleFileName);

    QXmlInputSource source(styleFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    bool ok = reader.parse(source);
    styleFile.close();

    if (!ok) {
	throw StyleFileReadFailed(qstrtostr(m_errorString));
    }
}

bool
NoteStyleFileReader::startElement(const QString &, const QString &,
				  const QString &qName,
				  const QXmlAttributes &attributes)
{
    QString lcName = qName.lower();

    if (lcName == "rosegarden-note-style") {

	QString s = attributes.value("base-style");
	if (s) m_style->setBaseStyle(qstrtostr(s));

    } else if (lcName == "note") {

	m_haveNote = true;
	
	QString s = attributes.value("type");
	if (!s) {
	    m_errorString = i18n("type is a required attribute of note");
	    return false;
	}
	
	try {
	    Note::Type type = NotationStrings::getNoteForName(s).getNoteType();
	    if (!setFromAttributes(type, attributes)) return false;

	} catch (NotationStrings::MalformedNoteName n) {
	    m_errorString = i18n("Unrecognised note name %1").arg(s);
	    return false;
	}

    } else if (lcName == "global") {

	if (m_haveNote) {
	    m_errorString = i18n("global element must precede note elements");
	    return false;
	}
	    
	for (Note::Type type = Note::Shortest; type <= Note::Longest; ++type) {
	    if (!setFromAttributes(type, attributes)) return false;
	}
    }

    return true;
}


bool
NoteStyleFileReader::setFromAttributes(Note::Type type,
				       const QXmlAttributes &attributes)
{
    QString s;
    bool haveShape = false;

    s = attributes.value("shape");
    if (s) {
	m_style->setShape(type, qstrtostr(s.lower()));
	haveShape = true;
    }
    
    s = attributes.value("charname");
    if (s) {
	if (haveShape) {
	    m_errorString = i18n("global and note elements may have shape "
				 "or charname attribute, but not both");
	    return false;
	}
	m_style->setShape(type, NoteStyle::CustomCharName);
	m_style->setCharName(type, qstrtostr(s));
    }

    s = attributes.value("filled");
    if (s) m_style->setFilled(type, s.lower() == "true");
    
    s = attributes.value("stem");
    if (s) m_style->setStem(type, s.lower() == "true");
    
    s = attributes.value("flags");
    if (s) m_style->setFlagCount(type, s.toInt());
    
    s = attributes.value("slashes");
    if (s) m_style->setSlashCount(type, s.toInt());

    NoteStyle::HFixPoint hfix;
    NoteStyle::VFixPoint vfix;
    m_style->getStemFixPoints(type, hfix, vfix);
    bool haveHFix = false;
    bool haveVFix = false;

    s = attributes.value("hfixpoint");
    if (s) {
	s = s.lower();
	haveHFix = true;
	if (s == "normal") hfix = NoteStyle::Normal;
	else if (s == "central") hfix = NoteStyle::Central;
	else if (s == "reversed") hfix = NoteStyle::Reversed;
	else haveHFix = false;
    }

    s = attributes.value("vfixpoint");
    if (s) {
	s = s.lower();
	haveVFix = true;
	if (s == "near") vfix = NoteStyle::Near;
	else if (s == "middle") vfix = NoteStyle::Middle;
	else if (s == "far") vfix = NoteStyle::Far;
	else haveVFix = false;
    }

    if (haveHFix || haveVFix) {
	m_style->setStemFixPoints(type, hfix, vfix);
	// otherwise they inherit from base style, so avoid setting here
    }

    return true;
}


}

