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

#include "ActionFileParser.h"

#include <iostream>

namespace Rosegarden
{
   
ActionFileParser::ActionFileParser(QObject *actionOwner)
{
}

ActionFileParser::~ActionFileParser()
{
}

bool
ActionFileParser::load(QString actionRcFile)
{
    QFile f(actionRcFile); //!!! find it
    QXmlInputSource is(&f);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    return reader.parse(is);
}

bool
ActionFileParser::startDocument()
{
    return false;
}

bool
ActionFileParser::startElement(const QString& namespaceURI,
                               const QString& localName,
                               const QString& qName,
                               const QXmlAttributes& atts)
{
    return false;
}

bool
ActionFileParser::endElement(const QString& namespaceURI,
                             const QString& localName,
                             const QString& qName)
{
    return false;
}

bool
ActionFileParser::characters(const QString &ch)
{
    return false;
}

bool
ActionFileParser::endDocument()
{
    return false;
}

bool
ActionFileParser::error(const QXmlParseException &exception)
{
    QString errorString =
	QString("ERROR: ActionFileParser: %1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    std::cerr << errorString.toLocal8Bit().data() << std::endl;
    return QXmlDefaultHandler::error(exception);
}

bool
ActionFileParser::fatalError(const QXmlParseException &exception)
{
    QString errorString =
	QString("FATAL ERROR: ActionFileParser: %1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    std::cerr << errorString.toLocal8Bit().data() << std::endl;
    return QXmlDefaultHandler::fatalError(exception);
}

bool
ActionFileParser::setActionText(QString actionName, QString text)
{
    return false;
}

bool
ActionFileParser::setActionIcon(QString actionName, QString icon)
{
    return false;
}

bool
ActionFileParser::setActionShortcut(QString actionName, QString shortcut)
{
    return false;
}

bool
ActionFileParser::setActionGroup(QString actionName, QString group)
{
    return false;
}

bool
ActionFileParser::setMenuText(QString name, QString text)
{
    return false;
}

bool
ActionFileParser::addMenuToMenu(QString parent, QString child)
{
    return false;
}

bool
ActionFileParser::addActionToMenu(QString menuName, QString actionName)
{
    return false;
}

bool
ActionFileParser::addSeparatorToMenu(QString menuName)
{
    return false;
}

bool
ActionFileParser::setToolbarText(QString name, QString text)
{
    return false;
}

bool
ActionFileParser::addActionToToolbar(QString toolbarName, QString actionName)
{
    return false;
}

bool
ActionFileParser::addSeparatorToToolbar(QString toolbarName)
{
    return false;
}

bool
ActionFileParser::addState(QString name)
{
    return false;
}

bool
ActionFileParser::enableActionInState(QString stateName, QString actionName)
{
    return false;
}

bool
ActionFileParser::disableActionInState(QString stateName, QString actionName)
{
    return false;
}

}

