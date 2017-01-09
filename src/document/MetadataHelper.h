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

#ifndef RG_METADATAHELPER_H
#define RG_METADATAHELPER_H

#include <QString>

#include <map>

namespace Rosegarden
{

class RosegardenDocument;

/*
 * This class helps to discriminate beetween headers and comments which
 * are stored together in the document metadata.
 */
class MetadataHelper
{

public:
    
    /**
     * A class to embed a time stamp inside a comment page.
     * The time stamp is used to recognize up to date translation when the
     * comment page is a translation.
     *
     * The time stamp if the last modification date of the main page.
     * It uses the UTC time and its format is "YYYY-MM-DD hh:mm:ss".
     */
    struct Comment {
        Comment() : text(""), timeStamp("") { }

        QString text;
        QString timeStamp;
    };
    
    typedef std::map<QString, Comment> CommentsMap;
    
    typedef std::map<QString, QString> HeadersMap;


    MetadataHelper(RosegardenDocument *doc);

    /**
     * Get the comments text from the metadata.
     * Return the comments indexed by pageName.
     */
    CommentsMap getComments();

    /**
     * Replace the comments text in the metadata with the argument strings.
     * The argument is a map of the comments indexed by pageName.
     */
    void setComments(CommentsMap comments);

    /**
     * Get all the metadata which are not comments into a map.
     * The values of the headers are returned indexed by header names.
     */
    HeadersMap getHeaders();

    /**
     * Replace all the metadata which are not comments with the argument map.
     * The map contains the values of the headers indexed by header names.
     */
    void setHeaders(HeadersMap data);
    
    /**
     * Add or modify in the metadata the special header used to ask for
     * a comment being popped up when the file is loaded.
     */
    void setPopupWanted(bool enabled);
    
    /**
     * Return from the metadata the value carried by the special header used to
     * ask for a comment being popped up when the file is loaded.
     */
    bool popupWanted();

private :

    /**
     * Used to recognize the key string of a comment then extract from it the
     * line number and the page name.
     * 
     * The key string of a comment has the form "comments_es_000098" where
     *    "comments_" is the commentsKeyBase constant (see MetadataHelper.cpp)
     *    "es" is the page name
     *    "000098" is the line number
     * 
     * The main comment page has an empty page name and its key has the
     * form "comments_999999" (only one underscore).
     *
     * Line number 0 is used to store the time stamp.
     *
     * The line number has a fixed size defined with the keyNumSize constant
     * (see MetadataHelper.cpp)
     * 
     */
    class CommentsKey
    {
        public:
            CommentsKey(QString keyString);
            
            QString key() { return m_key; }
            
            /*
             * Return true if the key is a comment key
             */
            bool isOK() { return m_isOK; }

            /**
             * Get the number of the comment line from its key.
             * If the key is "comments_es_000098" then lineNumber is 98.
             * Return -1 if the key is not related to a comment.
             */
            int lineNumber();

            QString pageName() { return m_pageName; }
            
        private:
            QString m_key;
            bool m_isOK;
            QString m_pageName;
    };


protected:

    RosegardenDocument *m_doc;
    
};


}

#endif
