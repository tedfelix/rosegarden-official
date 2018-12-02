/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MetadataHelper]"

#include "MetadataHelper.h"

#include "document/RosegardenDocument.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/configuration/CommentsConfigurationPage.h"

#include <QString>
#include <QStringList>

#include <set>
#include <map>


namespace Rosegarden
{


// Strings used in XML to embed the comments inside the metadata
static const QString commentsKeyBase() { return "comments_"; }

// Strings used in XML to ask for a note popup at startup
static const QString commentsPopup() { return "comments_popup"; }

// Size of the numeric part of the line key 
static const int keyNumSize(6);

// Make the key associated to a comment line from its line number and page name.
// If the line number is 123, the page name is "es" and commentsKeyBase is
// "comments_" then the key is "comments_es_000123".
// If there in no page name (reference page) then the key is "comments_000123".
static QString lineKey(const QString &pageName, int lineNumber)
{
    QString number = QString::number(lineNumber);
    while (number.size() < keyNumSize)
        number.prepend(QLatin1Char('0'));
    const QString sep = pageName.isEmpty() ? "" : "_";
    return commentsKeyBase() + pageName + sep + number;
}

MetadataHelper::CommentsKey::CommentsKey(QString keyString) :
    m_key(keyString),
    m_isOK(false),
    m_pageName("")
{
    int baseKeyLength = commentsKeyBase().size();

    // A valid comment key has a minimum length 
    m_isOK = keyString.size() >= (baseKeyLength + keyNumSize);
 
    // A valid comment key starts with the comment key base string
    m_isOK = m_isOK && keyString.startsWith(commentsKeyBase());

    // The character preceding the final line number must be "_"
    m_isOK = m_isOK && (keyString.mid(baseKeyLength - 1,
                            keyString.size() - keyNumSize -
                            baseKeyLength + 1).right(1) == "_");
           
    if (!m_isOK) return;         // key is not a comments key

    // Get the page of a comment line from its key.
    // If the key is "comments_es_000098" then pageName is "es".
    // If the key is "comments_000098" then pageName is "".
    m_pageName = keyString.mid(baseKeyLength - 1,
                               keyString.size() - baseKeyLength - keyNumSize);
    if (!m_pageName.isEmpty()) m_pageName.remove(0, 1);

}

// Get the number of the comment line from its key.
// If the key is "comments_es_000098" then lineNumber 98.
// Line number 0 is used to store a time stamp.
// Return -1 if the key is not related to a comment.
int
MetadataHelper::CommentsKey::lineNumber() 
{
    if (!m_isOK) return -1;
    return m_key.rightRef(keyNumSize).toInt();
}


MetadataHelper::MetadataHelper(RosegardenDocument *doc) :
    m_doc(doc)
{
}

MetadataHelper::CommentsMap
MetadataHelper::getComments()
{
    Configuration &metadata = (&m_doc->getComposition())->getMetadata();

    // Get all the relevant keys and pages from the metadata
    // (keys[page] is the ordered list of keys related to page)
   std::map< QString, std::set<QString> > keys;
    keys.clear();
    for (Configuration::iterator
            it = metadata.begin(); it != metadata.end(); ++it) {
        CommentsKey ck(strtoqstr(it->first));
        if (!ck.isOK()) continue;
        keys[ck.pageName()].insert(ck.key());
    }

    // Create the texts
    CommentsMap comments;
    comments.clear();
    
    for (std::map< QString, std::set<QString> >::iterator it0 = keys.begin();
             it0 != keys.end(); ++it0) {
        
        std::set<QString> keySet = it0->second;
        QStringList lines;
        if (keySet.size()) {
            int lastLine = 0;
            for (std::set<QString>::iterator it = keySet.begin();
                    it != keySet.end(); ++it) {
                QString key = *it;
                CommentsKey ck(key);
                int currentLine = ck.lineNumber();
                if (currentLine == -1) {
                    RG_WARNING << "ERROR: Bad comment key \"" << key << "\"";
                    continue;
                }
                if (currentLine == 0) {
                    // Line 0 is the time stamp
                    comments[it0->first].timeStamp =
                        strtoqstr(metadata.get<String>(qstrtostr(key)));
                    continue;
                }
                lastLine++;
                for (int i = lastLine; i < currentLine; i++) {
                    // Insert blank line
                    lines << "";
                }
                // Insert currentLine
                lines << strtoqstr(metadata.get<String>(qstrtostr(key)));
                lastLine = currentLine;
            }
        }
        
        comments[it0->first].text = lines.join("\n");
    }

    return comments;
}

void
MetadataHelper::setComments(CommentsMap comments)
{
    Configuration &metadata = m_doc->getComposition().getMetadata();
    const Configuration origmetadata = metadata;

    // Get from the metadata all the keys other than comments
    HeadersMap notComments;
    notComments.clear();
    for (Configuration::iterator
            it = metadata.begin(); it != metadata.end(); ++it) {
        QString key = strtoqstr(it->first);
        if ((key == commentsPopup()) || !key.startsWith(commentsKeyBase())) {
            notComments[key] = strtoqstr(metadata.get<String>(qstrtostr(key)));
        }
    }

    // Clear the metadata
    metadata.clear();
    
    // Add the strings other than comments
    for (HeadersMap::iterator it = notComments.begin();
            it != notComments.end(); ++it) {
        QString key = it->first;;
        QString value = it->second;
        metadata.set<String>(qstrtostr(key), qstrtostr(value));
    }

    // Add the comments lines
    for (CommentsMap::iterator it = comments.begin();
            it != comments.end(); ++it) {
        QString page = it->first;
        QString timeStamp = it->second.timeStamp;
        QString text = it->second.text;
        // Add the text lines
        QStringList lines = text.split("\n", QString::KeepEmptyParts);
        bool textExists = false;
        int n = 0;
        for (QStringList::iterator it = lines.begin(); it != lines.end(); ++it) {
            n++;
            QString value = *it;
            if (!value.isEmpty()) {
                QString key = lineKey(page, n);
                metadata.set<String>(qstrtostr(key), qstrtostr(value));
                textExists = true;
            }
        }
        // Add the time stamp if any and if there is some text
        if (textExists && !timeStamp.isEmpty()) {
            QString key = lineKey(page, 0);   // Time stamp is stored as line 0
            metadata.set<String>(qstrtostr(key), qstrtostr(timeStamp));
        }
    }

    if (metadata != origmetadata) {
        m_doc->slotDocumentModified();
    }
}

MetadataHelper::HeadersMap
MetadataHelper::getHeaders()
{
    HeadersMap data;

    Configuration &metadata = m_doc->getComposition().getMetadata();

    // For each Composition Property
    for (Configuration::iterator it = metadata.begin();
         it != metadata.end();
         ++it) {
        QString key = strtoqstr(it->first.getName());

        // If it's a comment, try the next one.
        if (key.startsWith(commentsKeyBase()))
            continue;

        // This should never happen.  See r14693.
        if (!metadata.has(qstrtostr(key))) {
            RG_WARNING << "getHeaders() WARNING: Key " << key << "not found";
            continue;
        }

        data[key] = strtoqstr(metadata.get<String>(qstrtostr(key)));
    }

    return data;
}

void
MetadataHelper::setHeaders(HeadersMap data)
{


    Configuration &metadata = m_doc->getComposition().getMetadata();
    const Configuration origmetadata = metadata;

    // Get all the comments from the metadata (keep them in raw form)
    HeadersMap comments;
    comments.clear();
    for (Configuration::iterator
            it = metadata.begin(); it != metadata.end(); ++it) {
        QString key = strtoqstr(it->first);
        if (key.startsWith(commentsKeyBase())) {
            comments[key] = strtoqstr(metadata.get<String>(qstrtostr(key)));
        }
    }

    // Clear the metadata
    metadata.clear();
    
    // Add the strings other than comments to the metadata
    for (HeadersMap::iterator it = data.begin();
            it != data.end(); ++it) {
        QString key = it->first;
        QString value = it->second;
        if (!value.isEmpty()) {
            metadata.set<String>(qstrtostr(key), qstrtostr(value));
        }
    }

    // Add the comments lines to the metadata
    for (HeadersMap::iterator it = comments.begin();
            it != comments.end(); ++it) {
        QString key = it->first;;
        QString value = it->second;
        metadata.set<String>(qstrtostr(key), qstrtostr(value));
    }

    if (metadata != origmetadata) {
        m_doc->slotDocumentModified();
    }
}

void
MetadataHelper::setPopupWanted(bool enabled)
{
    Configuration &metadata = (&m_doc->getComposition())->getMetadata();
    const Configuration origmetadata = metadata;

    metadata.set<String>(qstrtostr(commentsPopup()), enabled ? "true" : "false");

    if (metadata != origmetadata) {
        m_doc->slotDocumentModified();
    }
}
    
bool
MetadataHelper::popupWanted()
{
    Configuration &metadata = (&m_doc->getComposition())->getMetadata();
    for (Configuration::iterator
            it = metadata.begin(); it != metadata.end(); ++it) {
        QString key = strtoqstr(it->first);
        QString value = strtoqstr(metadata.get<String>(it->first));

        if ((key == commentsPopup()) && (value == "true")) return true;
    }
    return false;
}

}
