
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

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
static const QString commentsKeyBase("comments_");

// Strings used in XML to ask for a note popup at startup
static const QString commentsPopup("comments_popup");

// Size of the numeric part of the line key 
static const int keyNumSize(6);

// Make the key associated to a comment line from its line number.
// If the line number is 123 and commentsKeyBase is "comments_" then
// the key is "comments_000123".
static QString lineKey(int lineNumber)
{
    QString number = QString("%1").arg(lineNumber);
    while (number.size() < keyNumSize) number = "0" + number;
    return commentsKeyBase + number;
}

// Get the number of  comment line from its key.
// If the key is "comments_000098" then return 98.
// Return 0 if the string is not a lineKey.
static int lineNumber(QString lineKey)
{
    int baseKeyLength = commentsKeyBase.size();
    if (lineKey.size() != (baseKeyLength + keyNumSize)) return 0;
    if (!lineKey.startsWith(commentsKeyBase)) return 0;
    return lineKey.right(keyNumSize).toInt();
}



MetadataHelper::MetadataHelper(RosegardenDocument *doc) :
    m_doc(doc)
{
}

QString
MetadataHelper::getComments()
{
    Configuration &metadata = (&m_doc->getComposition())->getMetadata();

    // Get all the relevant keys from the metadata
    std::set<QString> keys;
    keys.clear();
    for (Configuration::iterator
            it = metadata.begin(); it != metadata.end(); ++it) {
        QString key = strtoqstr(it->first);
        if (key == commentsPopup) continue;
        if (key.startsWith(commentsKeyBase)) {
            keys.insert(key);
        }
    }

    // Create the text
    QStringList lines;
    if (keys.size()) {
        int lastLine = 0;
        for (std::set<QString>::iterator it = keys.begin(); it != keys.end(); ++it) {
            QString key = *it;
            int currentLine = lineNumber(key);
            if (currentLine == 0) {
                std::cerr << "ERROR: Bad comment key \"" << key << "\"" << std::endl; 
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

    return QString(lines.join("\n"));
}

void
MetadataHelper::setComments(QString text)
{
    Configuration &metadata = m_doc->getComposition().getMetadata();
    const Configuration origmetadata = metadata;

    // Get from the metadata all the keys other than comments
    std::map<QString, QString> notComments;
    notComments.clear();
    for (Configuration::iterator
            it = metadata.begin(); it != metadata.end(); ++it) {
        QString key = strtoqstr(it->first);
        if ((key == commentsPopup) || !key.startsWith(commentsKeyBase)) {
            notComments[key] = strtoqstr(metadata.get<String>(qstrtostr(key)));
        }
    }

    // Clear the metadata
    metadata.clear();
    
    // Add the strings other than comments
    for (std::map<QString, QString>::iterator it = notComments.begin();
            it != notComments.end(); ++it) {
        QString key = it->first;;
        QString value = it->second;
        metadata.set<String>(qstrtostr(key), qstrtostr(value));
    }

    // Add the comments lines
    QStringList lines = text.split("\n", QString::KeepEmptyParts);
    int n = 0;
    for (QStringList::iterator it = lines.begin(); it != lines.end(); ++it) {
        n++;
        QString value = *it;
        if (!value.isEmpty()) {
            QString key = lineKey(n);
            metadata.set<String>(qstrtostr(key), qstrtostr(value));
        }
    }

    if (metadata != origmetadata) {
        m_doc->slotDocumentModified();
    }
}

std::map<QString, QString>
MetadataHelper::getHeaders()
{
    Configuration &metadata = (&m_doc->getComposition())->getMetadata();
    std::map<QString, QString> data;

    data.clear();
    for (Configuration::iterator
            it = metadata.begin(); it != metadata.end(); ++it) {
        QString key = strtoqstr(it->first);
        if (!key.startsWith(commentsKeyBase)) {
            data[key] = strtoqstr(metadata.get<String>(qstrtostr(key)));
        }
    }

    return data;
}

void
MetadataHelper::setHeaders(std::map<QString, QString> data)
{


    Configuration &metadata = m_doc->getComposition().getMetadata();
    const Configuration origmetadata = metadata;

    // Get all the comments from the metadata
    std::map<QString, QString> comments;
    comments.clear();
    for (Configuration::iterator
            it = metadata.begin(); it != metadata.end(); ++it) {
        QString key = strtoqstr(it->first);
        if (key.startsWith(commentsKeyBase)) {
            comments[key] = strtoqstr(metadata.get<String>(qstrtostr(key)));
        }
    }

    // Clear the metadata
    metadata.clear();
    
    // Add the strings other than comments
    for (std::map<QString, QString>::iterator it = data.begin();
            it != data.end(); ++it) {
        QString key = it->first;
        QString value = it->second;
        if (!value.isEmpty()) {
            metadata.set<String>(qstrtostr(key), qstrtostr(value));
        }
    }

    // Add the comments lines
    for (std::map<QString, QString>::iterator it = comments.begin();
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

    metadata.set<String>(qstrtostr(commentsPopup), enabled ? "true" : "false");

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

        if ((key == commentsPopup) && (value == "true")) return true;
    }
    return false;
}

}
