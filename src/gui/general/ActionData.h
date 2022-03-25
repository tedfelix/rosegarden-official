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
#include <set>
#include <deque>
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

    struct KeyDuplicate
    {
        QString key;
        QString actionText;
        QString context;
    };

    typedef std::list<KeyDuplicate> KeyDuplicates;

    struct DuplicateDataForKey
    {
        QString editActionText;
        QString editContext;
        std::map<QKeySequence, KeyDuplicates> duplicateMap;
    };

    typedef std::map<QString, DuplicateDataForKey> DuplicateData;

    QStandardItemModel* getModel();
    QString getKey(int row) const;
    bool isDefault(const QString& key,
                   const std::set<QKeySequence>& ksSet) const;
    void saveUserShortcuts();
    void setUserShortcuts(const QString& key,
                          const std::set<QKeySequence>& ksSet);
    void addUserShortcut(const QString& key,
                         const QKeySequence& ks);
    void removeUserShortcut(const QString& key,
                            const QKeySequence& ks);
    void removeUserShortcuts(const QString& key);
    void removeAllUserShortcuts();
    std::set<QKeySequence> getShortcuts(const QString& key) const;
    void getDuplicateShortcuts(const std::set<QString>& keys,
                               std::set<QKeySequence> ksSet,
                               bool resetToDefault,
                               bool sameContext,
                               DuplicateData& duplicates) const;

    void resetChanges();
    bool hasDataChanged() const;
    void undoChanges();
    // return value is the index of the active keyboard
    int getKeyboards(std::list<QString>& keyboards);
    void applyKeyboard(int keyboard);

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
    QString translate(QString text, QString disambiguation = "") const;
    void fillModel();
    void updateModel(const QString& changedKey);
    void readKeyboardShortcuts();

    struct ActionInfo
    {
        QStringList menus;
        QString toolbar;
        QString text;
        QString icon;
        std::set<QKeySequence> shortcuts;
        QString tooltip;
        bool global;
    };

    typedef std::set<QKeySequence> KeySet;

    std::map<QString, ActionInfo> m_actionMap;
    std::map<QString, ActionInfo> m_actionMapOriginal;

    struct KeyboardTranslation
    {
        QString kbName;
        std::map<QString, QString> translation;
    };
    typedef std::map<int, KeyboardTranslation> KeyboardTranslations;

    bool m_inMenuBar;
    bool m_inText;
    bool m_inEnable;
    bool m_inDisable;
    bool m_inVisible;  // Are we inside a State/visible tag?
    bool m_inInvisible;  // Are we inside a State/invisible tag?
    QStringList m_currentMenus;
    QStringList m_currentMenuNames;
    QString m_currentToolbar;
    QString m_currentToolbarName;
    QString m_currentState;
    QString m_currentText;
    QString m_currentFile;
    std::map<QString, QString> m_contextMap;
    std::deque<QString> m_keyStore;
    std::map<QString, KeySet> m_userShortcuts;
    std::map<QString, KeySet> m_userShortcutsCopy;
    QStandardItemModel* m_model;
    KeyboardTranslations m_keyboardTranslations;
    std::map<QString, QString> m_translatedKeyboardNames;
    int m_actKb;
    int m_actKbCopy;

    static ActionData* m_instance;
};

}

#endif
