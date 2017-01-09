/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

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
#include "misc/Strings.h"
#include "NotationStrings.h"
#include "misc/Debug.h"
#include "gui/general/ResourceFinder.h"
#include "NoteStyle.h"

#include <QFileInfo>
#include <QDir>



namespace Rosegarden {


NoteStyleFileReader::NoteStyleFileReader(QString name) :
    m_style(new NoteStyle(name)),
    m_haveNote(false)
{
/*!!!
	IconLoader il;
//     QString styleDirectory =
//	KGlobal::dirs()->findResource("appdata", "styles/");
	QString styleDirectory = il.getResourcePath( "styles" );
	
    QString styleFileName =
	QString("%1/%2.xml").arg(styleDirectory).arg(name);
*/
    QString styleFileName = ResourceFinder().getResourcePath
        ("styles", QString("%1.xml").arg(name));

    QFileInfo fileInfo(styleFileName);

    if (styleFileName == "" || !fileInfo.isReadable()) {
        throw StyleFileReadFailed(tr("Can't open style file \"%1\" for style \"%2\"").arg(styleFileName).arg(name));
    }

    QFile styleFile(styleFileName);

    QXmlInputSource source(&styleFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    bool ok = reader.parse(source);
    styleFile.close();

    if (!ok) {
	throw StyleFileReadFailed(m_errorString);
    }
}

bool
NoteStyleFileReader::startElement(const QString &, const QString &,
				  const QString &qName,
				  const QXmlAttributes &attributes)
{
    QString lcName = qName.toLower();

    if (lcName == "rosegarden-note-style") {

	QString s = attributes.value("base-style");
	if ( !s.isEmpty() ) m_style->setBaseStyle(s);

    } else if (lcName == "note") {

	m_haveNote = true;
	
	QString s = attributes.value("type");
	if (s.isEmpty() ) {
	    m_errorString = tr("type is a required attribute of note");
	    return false;
	}
	
	try {
	    Note::Type type = NotationStrings::getNoteForName(s).getNoteType();
	    if (!setFromAttributes(type, attributes)) return false;

	} catch (NotationStrings::MalformedNoteName n) {
	    m_errorString = tr("Unrecognised note name %1").arg(s);
	    return false;
	}

    } else if (lcName == "global") {

	if (m_haveNote) {
	    m_errorString = tr("global element must precede note elements");
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
	if (!s.isEmpty() ) {
	m_style->setShape(type, s.toLower());
	haveShape = true;
    }
    
    s = attributes.value("charname");
	if (!s.isEmpty() ) {
	if (haveShape) {
	    m_errorString = tr("global and note elements may have shape "
				 "or charname attribute, but not both");
	    return false;
	}
	m_style->setShape(type, NoteStyle::CustomCharName);
	m_style->setCharName(type, s);
    }

    s = attributes.value("filled");
	if (!s.isEmpty() ) m_style->setFilled(type, s.toLower() == "true");
    
    s = attributes.value("stem");
	if (!s.isEmpty() ) m_style->setStem(type, s.toLower() == "true");
    
    s = attributes.value("flags");
	if (!s.isEmpty() ) m_style->setFlagCount(type, s.toInt());
    
    s = attributes.value("slashes");
	if (!s.isEmpty() ) m_style->setSlashCount(type, s.toInt());

    NoteStyle::HFixPoint hfix;
    NoteStyle::VFixPoint vfix;
    m_style->getStemFixPoints(type, hfix, vfix);
    bool haveHFix = false;
    bool haveVFix = false;

    s = attributes.value("hfixpoint");
	if (!s.isEmpty() ) {
	s = s.toLower();
	haveHFix = true;
	if (s == "normal") hfix = NoteStyle::Normal;
	else if (s == "central") hfix = NoteStyle::Central;
	else if (s == "reversed") hfix = NoteStyle::Reversed;
	else haveHFix = false;
    }

    s = attributes.value("vfixpoint");
	if (!s.isEmpty() ) {
	s = s.toLower();
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

