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

#ifndef RG_XMLSUBHANDLER_H
#define RG_XMLSUBHANDLER_H

#include <QString>
#include <qxml.h>

namespace Rosegarden {
    
class XmlSubHandler
{
public:
    XmlSubHandler();
    virtual ~XmlSubHandler();
    
    virtual bool startElement(const QString& namespaceURI,
                              const QString& localName,
                              const QString& qName,
                              const QXmlAttributes& atts) = 0;

    /**
     * @param finished : if set to true on return, means that
     * the handler should be deleted
     */
    virtual bool endElement(const QString& namespaceURI,
                            const QString& localName,
                            const QString& qName,
                            bool& finished) = 0;

    virtual bool characters(const QString& ch) = 0;
};

}

#endif /*RG_XMLSUBHANDLER_H*/
