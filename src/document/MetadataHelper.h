
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

#ifndef RG_METADATAHELPER_H
#define RG_METADATAHELPER_H

#include <QString>

#include <map>

namespace Rosegarden
{

class RosegardenDocument;

class MetadataHelper
{

public:

    MetadataHelper(RosegardenDocument *doc);

    /**
     * Get the comments text from the metadata
     */
    QString getComments();

    /**
     * Replace the comments text in the metadata with the argument string
     */
    void setComments(QString text);

    /**
     * Get all the metadata which are not comments into a map
     */
    std::map<QString, QString> getHeaders();

    /**
     * Replace all the metadata which are not comments with the argument map
     */
    void setHeaders(std::map<QString, QString>);
    
    void setPopupWanted(bool enabled);
    
    bool popupWanted();

 
protected:

    RosegardenDocument *m_doc;
    
};


}

#endif
