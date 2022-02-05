/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ACTIONDATA_H
#define RG_ACTIONDATA_H

#include <map>
#include <list>

#include "document/io/XMLHandler.h"

class QXmlStreamAttributes;
class QStandardItemModel;

namespace Rosegarden
{

/// Singleton class for collecting data about all actions
class ActionData : public XMLHandler
{
public:
    static ActionData* getInstance();
    ~ActionData();

    void fillModel(QStandardItemModel *model);
    
 private:
    ActionData();
    ActionData(const ActionData&);
    void operator=(const ActionData&);
    
    // Xml methods
    bool startDocument() override;
    bool startElement(const QString& namespaceURI,
                      const QString& localName,
                      const QString& qName,
                      const QXmlStreamAttributes& atts) override;
    bool endElement(const QString& namespaceURI,
                    const QString& localName,
                    const QString& qName) override;
    bool characters(const QString& ch) override;
    bool endDocument() override;
    bool fatalError(int lineNumber, int columnNumber,
                    const QString& msg) override;


    void loadData(const QString& name);
    
    struct ActionInfo
    {
        QStringList menus;
        QString toolbar;
        QString text;
        QString icon;
        QString shortcut;
        QString tooltip;
    };
    
    struct RCFileData
    {
        std::map<QString, ActionInfo> actionMap;
    };
    
    std::map<QString, RCFileData> m_dataMap;
    
    bool m_inMenuBar;
    bool m_inText;
    bool m_inEnable;
    bool m_inDisable;
    bool m_inVisible;  // Are we inside a State/visible tag?
    bool m_inInvisible;  // Are we inside a State/invisible tag?
    QStringList m_currentMenus;
    QString m_currentToolbar;
    QString m_currentState;
    QString m_currentText;
    QString m_currentFile;
    std::map<QString, QString> m_contextMap;
    
    static ActionData* m_instance;
};
 
}

#endif

