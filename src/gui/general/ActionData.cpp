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

#define RG_MODULE_STRING "[ActionData]"
#define RG_NO_DEBUG_PRINT

#include "ActionData.h"

#include "misc/Debug.h"
#include "ResourceFinder.h"
#include "document/io/XMLReader.h"
#include "gui/general/IconLoader.h"
#include "misc/ConfigGroups.h"

#include <QFile>
#include <QStandardItemModel>
#include <QSettings>

namespace Rosegarden
{

ActionData* ActionData::m_instance = 0;

ActionData* ActionData::getInstance()
{
    if (m_instance == 0) {
        m_instance = new ActionData();
    }
    return m_instance;
}

ActionData::~ActionData()
{
}

QStandardItemModel* ActionData::getModel()
{
    if (! m_model) {
        m_model = new QStandardItemModel;
    }
    fillModel();
    return m_model;
}

QString ActionData::getKey(int row) const
{
    return m_keyStore[row];
}

bool ActionData::isDefault(const QString& key,
                           const std::set<QKeySequence>& ksSet) const
{
    auto it = m_actionMap.find(key);
    if (it == m_actionMap.end()) {
        // action not found
        return true;
    }
    ActionInfo ainfo = (*it).second;
    return (ksSet == ainfo.shortcuts);
}

void ActionData::saveUserShortcuts()
{
    QSettings settings;
    settings.beginGroup(UserShortcutsConfigGroup);
    settings.remove("");
    for (auto it = m_userShortcuts.begin(); it != m_userShortcuts.end(); it++) {
        const QString& key = (*it).first;
        const KeySet& keySet = (*it).second;
        if (keySet.empty()) {
            QString skey = key + QString::number(0);
            RG_DEBUG << "settings set" << skey << "";
            settings.setValue(skey, "");
        }
        int index = 0;
        foreach(auto keyseq, keySet) {
            QString skey = key + QString::number(index);
            RG_DEBUG << "settings set" << skey << keyseq.toString();
            settings.setValue(skey, keyseq.toString());
            index++;
        }
    }
    settings.endGroup();

    settings.beginGroup(ShortcutKeyboardConfigGroup);
    settings.setValue("actualkeyboard", m_actKb);
    settings.endGroup();
}

void ActionData::setUserShortcuts(const QString& key,
                                  const std::set<QKeySequence>& ksSet)
{
    QStringList kssl;
    foreach(auto ks, ksSet) {
        kssl << ks.toString(QKeySequence::NativeText);
    }
    QString scString = kssl.join(", ");
    RG_DEBUG << "setUserShortcuts:" << key << scString;
    if (ksSet == m_actionMap[key].shortcuts) {
        // setting back to default
        m_userShortcuts.erase(key);
    } else {
        m_userShortcuts[key] = ksSet;
    }
    updateModel(key);
}

void ActionData::addUserShortcut(const QString& key,
                                 const QKeySequence& ks)
{
    RG_DEBUG << "add shortcut" << ks << "to" << key;
    std::set<QKeySequence> ksSet = getShortcuts(key);
    ksSet.insert(ks);
    setUserShortcuts(key, ksSet);
}

void ActionData::removeUserShortcut(const QString& key,
                                    const QKeySequence& ks)
{
    RG_DEBUG << "remove shortcut" << ks << "from" << key;
    std::set<QKeySequence> ksSet = getShortcuts(key);
    ksSet.erase(ks);
    setUserShortcuts(key, ksSet);
}

void ActionData::removeUserShortcuts(const QString& key)
{
    RG_DEBUG << "removeUserShortcuts for" << key;
    auto usiter = m_userShortcuts.find(key);
    if (usiter != m_userShortcuts.end()) {
        RG_DEBUG << "remove user shortcuts for" << key;
        m_userShortcuts.erase(key);
        updateModel(key);
    }
}

void ActionData::removeAllUserShortcuts()
{
    RG_DEBUG << "remove all user shortcuts";
    m_userShortcuts.clear();
    updateModel(""); // key empty updates the whole model
}

std::set<QKeySequence> ActionData::getShortcuts(const QString& key) const {
    std::set<QKeySequence> ret;
    auto it = m_actionMap.find(key);
    if (it == m_actionMap.end()) {
        // action not found
        return ret;
    }
    ActionInfo ainfo = (*it).second;
    auto usiter = m_userShortcuts.find(key);
    if (usiter != m_userShortcuts.end()) {
        // take the user shortcuts
        const KeySet& keySet = (*usiter).second;
        ret = keySet;
    } else {
        // no user shortcuts - use the defaults
        ret = ainfo.shortcuts;
    }
    return ret;
}

void ActionData::getDuplicateShortcuts(const std::set<QString>& keys,
                                       std::set<QKeySequence> ksSet,
                                       bool resetToDefault,
                                       bool sameContext,
                                       DuplicateData& duplicates) const
{
    // the ksSet is only relevant setting for a single key (keys.size() == 1)

    duplicates.clear();
    foreach(auto key, keys) {
        RG_DEBUG << "Check duplicates for" << key;
        ActionInfo ainfo = m_actionMap.at(key);
        QString etextAdj = ainfo.text;
        QStringList eklist = key.split(":");
        QString efile = eklist[0];
        etextAdj.remove("&");
        DuplicateDataForKey ddatak;
        ddatak.editActionText = etextAdj;
        ddatak.editContext = m_contextMap.at(efile);
        std::set<QKeySequence> newKS;
        std::set<QKeySequence> oldKS = getShortcuts(key);
        if (resetToDefault) {
            // the new shortcuts are the defaults old ones are the present ones
            newKS = ainfo.shortcuts;
        } else {
            // ksSet is the new shortcut set the old one is the present one
            newKS = ksSet;
        }

        std::set<QKeySequence> addedKS;
        std::set_difference(newKS.begin(), newKS.end(),
                            oldKS.begin(), oldKS.end(),
                            std::inserter(addedKS, addedKS.begin()));

        foreach(auto ks, addedKS) {
            RG_DEBUG << "getDuplicateShortcuts added" << ks;
            KeyDuplicates kdups;
            for (auto i = m_actionMap.begin(); i != m_actionMap.end(); i++) {
                const QString& mkey = (*i).first;
                const ActionInfo& mainfo = m_actionMap.at(mkey);
                // ignore passed key
                if (mkey == key) continue;
                QStringList mklist = mkey.split(":");
                QString mcontext = m_contextMap.at(mklist[0]);
                bool contextRelevant;
                if (sameContext) {
                    contextRelevant = (mcontext == ddatak.editContext);
                } else {
                    contextRelevant = true;
                }
                if (mainfo.global) contextRelevant = true;
                if (! contextRelevant) continue;
                std::set<QKeySequence> mKSL = getShortcuts(mkey);
                if (mKSL.find(ks) != mKSL.end()) {
                    // this entry also uses the KeySequence ks
                    RG_DEBUG << "found duplicate for" << ks << mkey;
                    KeyDuplicate kdup;
                    kdup.key = mkey;
                    QString textAdj = mainfo.text;
                    textAdj.remove("&");
                    kdup.actionText = textAdj;
                    QStringList mklist = mkey.split(":");
                    QString mfile = mklist[0];
                    kdup.context = m_contextMap.at(mfile);
                    kdups.push_back(kdup);
                }
            }
            if (! kdups.empty()) {
                ddatak.duplicateMap[ks] = kdups;
            }
        }
        if (! ddatak.duplicateMap.empty()) {
            duplicates[key] = ddatak;
        }
    }
}

void ActionData::resetChanges()
{
    m_actKbCopy = m_actKb;
    m_userShortcutsCopy = m_userShortcuts;
}

bool ActionData::hasDataChanged() const
{
    if (m_actKb != m_actKbCopy) {
        // keyboard has changed
        return true;
    }
    return (m_userShortcutsCopy != m_userShortcuts);
}

void ActionData::undoChanges()
{
    m_actKb = m_actKbCopy;
    m_userShortcuts = m_userShortcutsCopy;
}

int ActionData::getKeyboards(std::list<QString>& keyboards)
{
    keyboards.clear();
    int akIndex = 0;
    foreach(auto ktpair, m_keyboardTranslations) {
        int index = ktpair.first;
        const QString& name = ktpair.second.kbName;
        RG_DEBUG << "getKeyboards append" << name;
        keyboards.push_back(m_translatedKeyboardNames[name]);
        if (index == m_actKb) akIndex = index;
    }
    return akIndex;
}

void ActionData::applyKeyboard(int index)
{
    RG_DEBUG << "applyKeyboard" << index;
    m_actKb = index;
    // first reset to original
    m_actionMap = m_actionMapOriginal;
    KeyboardTranslation kbt = m_keyboardTranslations[index];
    foreach(auto pair, kbt.translation) {
        const QString& ksSrc = pair.first;
        const QString& ksDest = pair.second;
        RG_DEBUG << "applyTranslation" << ksSrc << ksDest;
        for (auto i = m_actionMap.begin(); i != m_actionMap.end(); i++) {
            const QString& mkey = (*i).first;
            const ActionInfo& mainfo = m_actionMap.at(mkey);
            const std::set<QKeySequence>& defaultShortcuts = mainfo.shortcuts;
            if (defaultShortcuts.find(ksSrc) == defaultShortcuts.end()) {
                // ksSrc not in default set
                continue;
            }

            std::set<QKeySequence> scs = m_actionMapOriginal[mkey].shortcuts;
            auto diter = scs.find(ksDest);
            if (diter != scs.end()) {
                // no ksDest is already in the set - done
                continue;
            }
            // so ksSrc is in the default set and ksDest is not in the shortcuts
            // apply change
            RG_DEBUG << "applyTranslation applying for" << mkey;
            scs.erase(ksSrc);
            scs.insert(ksDest);
            m_actionMap[mkey].shortcuts = scs;
        }
    }
    updateModel("");
}

ActionData::ActionData() :
    m_model(0)
{
    loadData("audiomanager.rc");
    loadData("bankeditor.rc");
    loadData("clefinserter.rc");
    loadData("controleditor.rc");
    loadData("eventlist.rc");
    loadData("guitarchordinserter.rc");
    loadData("markereditor.rc");
    loadData("markerruler.rc");
    loadData("matrixeraser.rc");
    loadData("matrixmover.rc");
    loadData("matrixpainter.rc");
    loadData("matrix.rc");
    loadData("matrixresizer.rc");
    loadData("matrixselector.rc");
    loadData("matrixvelocity.rc");
    loadData("midimixer.rc");
    loadData("mixer.rc");
    loadData("notationeraser.rc");
    loadData("notation.rc");
    loadData("notationselector.rc");
    loadData("noterestinserter.rc");
    loadData("rosegardenmainwindow.rc");
    loadData("segmenttool.rc");
    loadData("symbolinserter.rc");
    loadData("temporuler.rc");
    loadData("tempoview.rc");
    loadData("textinserter.rc");
    loadData("triggermanager.rc");

    m_contextMap["audiomanager.rc"] = QObject::tr("Audio manager");
    m_contextMap["bankeditor.rc"] = QObject::tr("Bank editor");
    m_contextMap["clefinserter.rc"] = QObject::tr("Clef inserter");
    m_contextMap["controleditor.rc"] = QObject::tr("Control editor");
    m_contextMap["eventlist.rc"] = QObject::tr("Event list");
    m_contextMap["guitarchordinserter.rc"] = QObject::tr("Guitar chord inserter");
    m_contextMap["markereditor.rc"] = QObject::tr("Marker editor");
    m_contextMap["markerruler.rc"] = QObject::tr("Marker ruler");
    m_contextMap["matrixeraser.rc"] = QObject::tr("Matrix eraser");
    m_contextMap["matrixmover.rc"] = QObject::tr("Matrix mover");
    m_contextMap["matrixpainter.rc"] = QObject::tr("Matrix painter");
    m_contextMap["matrix.rc"] = QObject::tr("Matrix");
    m_contextMap["matrixresizer.rc"] = QObject::tr("Matrix resizer");
    m_contextMap["matrixselector.rc"] = QObject::tr("Matrix selector");
    m_contextMap["matrixvelocity.rc"] = QObject::tr("Matrix velocity");
    m_contextMap["midimixer.rc"] = QObject::tr("Midi mixer");
    m_contextMap["mixer.rc"] = QObject::tr("Mixer");
    m_contextMap["notationeraser.rc"] = QObject::tr("Notation eraser");
    m_contextMap["notation.rc"] = QObject::tr("Notation");
    m_contextMap["notationselector.rc"] = QObject::tr("Notation selector");
    m_contextMap["noterestinserter.rc"] = QObject::tr("Note/rest inserter");
    m_contextMap["rosegardenmainwindow.rc"] = QObject::tr("Rosegarden main window");
    m_contextMap["segmenttool.rc"] = QObject::tr("Segment tool");
    m_contextMap["symbolinserter.rc"] = QObject::tr("Symbol inserter");
    m_contextMap["temporuler.rc"] = QObject::tr("Tempo ruler");
    m_contextMap["tempoview.rc"] = QObject::tr("Tempo view");
    m_contextMap["textinserter.rc"] = QObject::tr("Text inserter");
    m_contextMap["triggermanager.rc"] = QObject::tr("Trigger manager");

    // read user shortcuts
    QSettings settings;
    settings.beginGroup(UserShortcutsConfigGroup);
    QStringList settingsKeys = settings.childKeys();
    foreach(auto settingsKey, settingsKeys) {
        QString dataKey = settingsKey;
        dataKey.chop(1);
        QVariant keyValue = settings.value(settingsKey);
        QKeySequence keySeq = keyValue.value<QKeySequence>();
        RG_DEBUG << "user defined shortcuts - found:" << dataKey << keySeq;
        m_userShortcuts[dataKey].insert(keySeq);
    }
    settings.endGroup();
    m_userShortcutsCopy = m_userShortcuts;

    readKeyboardShortcuts();
    settings.beginGroup(UserShortcutsConfigGroup);
    m_actKb = settings.value("acualkeyboard", 0).toInt();
    settings.endGroup();
    m_actKbCopy = m_actKb;
    applyKeyboard(m_actKb);
}

bool ActionData::startDocument()
{
    return true;
}

bool ActionData::startElement(const QString&,
                              const QString&,
                              const QString& qName,
                              const QXmlStreamAttributes& atts)
{
    QString name = qName.toLower();

    if (name == "menubar") {

        m_inMenuBar = true;

    } else if (name == "menu") {

        QString menuName = atts.value("name").toString();
        if (menuName == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): No menu name provided in menu element";
        }

        m_currentMenus.push_back(menuName);

    } else if (name == "toolbar") {

        QString toolbarName = atts.value("name").toString();
        if (toolbarName == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): No toolbar name provided in toolbar element";
        }
        m_currentToolbar = toolbarName;

    } else if (name == "text") {

        // used to provide label for menu or title for toolbar, but
        // text comes from characters()

        if (!m_currentMenus.empty() || m_currentToolbar != "") {
            m_inText = true;
            m_currentText = "";
        }

    } else if (name == "action") {

        QString actionName = atts.value("name").toString();
        if (actionName == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): No action name provided in action element";
        }

        if (m_currentMenus.empty() && m_currentToolbar == "" &&
            (m_currentState == "" || (!m_inEnable && !m_inDisable
            && !m_inVisible && !m_inInvisible))) {
            RG_WARNING << "WARNING: startElement("
                       << m_currentFile << "): Action \"" << actionName
                       << "\" appears outside (valid) menu, toolbar or state "
                       << "enable/disable/visible/invisible element";
        }

        QString text = atts.value("text").toString();
        QString icon = atts.value("icon").toString();
        QString shortcut = atts.value("shortcut").toString();
        QString shortcutContext = atts.value("shortcut-context").toString();
        QString tooltip = atts.value("tooltip").toString();
        QString group = atts.value("group").toString();
        QString checked = atts.value("checked").toString();

        if (!m_currentMenus.empty() || m_currentToolbar != "") {
            QString key = m_currentFile + ":" + actionName;
            auto it = m_actionMap.find(key);
            if (it == m_actionMap.end()) {
                // new action
                ActionInfo ainfo;
                ainfo.global = false;
                m_actionMap[key] = ainfo;
            }
            ActionInfo& ainfo = m_actionMap[key];
            if (! m_currentMenus.empty()) ainfo.menus = m_currentMenus;
            if (m_currentToolbar != "") ainfo.toolbar = m_currentToolbar;
            if (text != "") {
                ainfo.text = translate(text);
                RG_DEBUG << "text:" << key << text << ainfo.text;
            }
            if (icon != "") ainfo.icon = icon;
            if (shortcut != "") {
                RG_DEBUG << "read xml xmlshortcut" << shortcut;
                QStringList shortcuts = shortcut.split(", ");
                for (int i = 0; i < shortcuts.size(); i++) {
                    RG_DEBUG << "read xml shortcut for" << key <<
                        QKeySequence(shortcuts.at(i));
                    ainfo.shortcuts.insert(QKeySequence(shortcuts.at(i)));
                }
                RG_DEBUG << "read xml number of shortcuts" << key
                         << ainfo.shortcuts.size();
            }
            if (shortcutContext == "application") ainfo.global = true;
            if (tooltip != "") ainfo.tooltip = tooltip;
            RG_DEBUG << "data" << m_currentMenus << m_currentToolbar <<
                actionName << icon <<
                shortcut << tooltip;
            RG_DEBUG << "map size" << m_currentFile << m_actionMap.size();
        }
    } else if (name == "state") {

        QString stateName = atts.value("name").toString();
        if (stateName == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): No state name provided in state element";
        }
        m_currentState = stateName;

    } else if (name == "enable") {

        if (m_currentState == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): Enable element appears outside state element";
        } else {
            m_inEnable = true;
        }

    } else if (name == "disable") {

        if (m_currentState == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): Disable element appears outside state element";
        } else {
            m_inDisable = true;
        }
    } else if (name == "visible") {

        if (m_currentState == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): Visible element appears outside state element";
        } else {
            m_inVisible = true;
        }
    } else if (name == "invisible") {

        if (m_currentState == "") {
            RG_WARNING << "WARNING: startElement(" << m_currentFile << "): Invisible element appears outside state element";
        } else {
            m_inInvisible = true;
        }
    }

    // make a copy of the unmodified map - it will be modified by
    // keyboard layout shortcuts
    m_actionMapOriginal = m_actionMap;
    return true;
}

bool ActionData::endElement(const QString&,
                            const QString&,
                            const QString& qName)
{
    QString name = qName.toLower();

    if (name == "menubar") {
        m_inMenuBar = false;
    } else if (name == "menu") {
        m_currentMenus.pop_back();
    } else if (name == "toolbar") {
        m_currentToolbar = "";
    } else if (name == "text") {
        if (m_inText) {
            m_inText = false;
        }
    } else if (name == "state") {
        m_currentState = "";
    } else if (name == "enable") {
        m_inEnable = false;
    } else if (name == "disable") {
        m_inDisable = false;
    } else if (name == "visible") {
        m_inVisible = false;
    } else if (name == "invisible") {
        m_inInvisible = false;
    }

    return true;
}

bool ActionData::characters(const QString& ch)
{
    if (m_inText) m_currentText += ch;
    return true;
}

bool ActionData::endDocument()
{
    return true;
}

bool ActionData::fatalError(int lineNumber, int columnNumber,
                            const QString& msg)
{
    QString errorString =
        QString("FATAL ERROR: %1 at line %2, column %3 in file %4")
        .arg(msg)
        .arg(lineNumber)
        .arg(columnNumber)
        .arg(m_currentFile);

    RG_WARNING << errorString.toLocal8Bit().data();

    return false;
}

void ActionData::loadData(const QString& name)
{
    QString file = ResourceFinder().getResourcePath("rc", name);
    if (file == "") {
        RG_WARNING << "load(): Failed to find RC file \"" << name << "\"";
        return;
    }

    m_currentFile = name;

    QFile f(file);
    XMLReader reader;
    reader.setHandler(this);
    reader.parse(f);
}

QString ActionData::translate(QString text, QString disambiguation) const
{
    // These translations are extracted from data/ui/*.rc files via
    // scripts/extract*.pl and pulled into the QObject translation context in
    // one great lump.

    if (!disambiguation.isEmpty()) return QObject::tr(text.toStdString().c_str(), disambiguation.toStdString().c_str());
    else return QObject::tr(text.toStdString().c_str());
}

void ActionData::fillModel()
{
    m_keyStore.clear();
    m_model->clear();
    m_model->insertColumns(0, 8);

    m_model->setHeaderData(0, Qt::Horizontal, QObject::tr("Context"));
    m_model->setHeaderData(1, Qt::Horizontal, QObject::tr("Action"));
    m_model->setHeaderData(2, Qt::Horizontal, QObject::tr("Icon"));
    m_model->setHeaderData(3, Qt::Horizontal, QObject::tr("User defined"));
    m_model->setHeaderData(4, Qt::Horizontal, QObject::tr("Shortcut 1"));
    m_model->setHeaderData(5, Qt::Horizontal, QObject::tr("Shortcut 2"));
    m_model->setHeaderData(6, Qt::Horizontal, QObject::tr("Shortcut 3"));
    m_model->setHeaderData(7, Qt::Horizontal, QObject::tr("Shortcut 4"));

    QPixmap udPixmap = QPixmap(IconLoader::loadPixmap("button-record"));
    for (auto i = m_actionMap.begin(); i != m_actionMap.end(); i++) {
        const QString& key = (*i).first;
        const ActionInfo& ainfo = (*i).second;
        QString textAdj = ainfo.text;
        QStringList klist = key.split(":");
        QString file = klist[0];
        QString aname = klist[1];
        textAdj.remove("&");
        if (textAdj == "") {
            textAdj = QObject::tr(aname.toStdString().c_str());
        }
        m_model->insertRow(0);
        m_keyStore.push_front(key);
        m_model->setData(m_model->index(0, 0), m_contextMap[file]);
        m_model->item(0, 0)->setEditable(false);
        m_model->setData(m_model->index(0, 1), textAdj);
        m_model->item(0, 1)->setEditable(false);
        if (ainfo.icon != "") {
            QIcon icon = IconLoader::load(ainfo.icon);
            if (! icon.isNull()) {
                QPixmap pixmap =
                    icon.pixmap(icon.availableSizes().first());
                QStandardItem* item = new QStandardItem;
                item->setIcon(pixmap);
                m_model->setItem(0, 2, item);
            }
        }
        m_model->setData(m_model->index(0, 2), "");
        m_model->item(0, 2)->setEditable(false);
        auto usiter = m_userShortcuts.find(key);
        bool userDefined = false;
        KeySet kset = ainfo.shortcuts;
        if (usiter != m_userShortcuts.end()) {
            // user shortcut
            userDefined = true;
            kset = (*usiter).second;
        }
        QStringList kssl;
        foreach(auto ks, kset) {
            kssl << ks.toString(QKeySequence::NativeText);
        }

        for(int i=0; i<4; i++) {
            if(i < kssl.length()) {
                m_model->setData(m_model->index(0, 4 + i), kssl[i]);
            } else {
                m_model->setData(m_model->index(0, 4 + i), "");
            }
        }
        QStandardItem* item = new QStandardItem;
        if (userDefined) {
            item->setIcon(udPixmap);
            item->setText(" "); // for sorting
        } else {
            item->setText("");
        }
        if (ainfo.shortcuts != m_actionMapOriginal[key].shortcuts) {
            // keyboard translation change
            item->setText("k");
        }
        m_model->setItem(0, 3, item);
        m_model->item(0, 3)->setEditable(false);
        if (ainfo.global) {
            QVariant bg(QBrush(Qt::cyan));
            for (int col=0; col<8; col++) {
                m_model->setData(m_model->index(0, col),
                                 bg, Qt::BackgroundRole);
            }
        }

    }
}

void ActionData::updateModel(const QString& changedKey)
{
    RG_DEBUG << "updateModel for key" << changedKey;
    if (!m_model)
        {
            RG_DEBUG << "updateModel - no model yet";
            return;
        }
    QPixmap udPixmap = QPixmap(IconLoader::loadPixmap("button-record"));
    QPixmap noPixmap;
    int row = m_model->rowCount() - 1;
    for (auto i = m_actionMap.begin(); i != m_actionMap.end(); i++) {
        const QString& key = (*i).first;
        const ActionInfo& ainfo = (*i).second;

        if (key == changedKey || changedKey == "") {
            auto usiter = m_userShortcuts.find(key);
            KeySet kset = ainfo.shortcuts;
            bool userDefined = false;
            if (usiter != m_userShortcuts.end()) {
                // user shortcut
                userDefined = true;
                kset = (*usiter).second;
            }
            QStringList kssl;
            foreach(auto ks, kset) {
                kssl << ks.toString(QKeySequence::NativeText);
            }
            RG_DEBUG << "updateModel" << row << key << kssl;
            for(int i=0; i<4; i++) {
                if(i < kssl.length()) {
                    m_model->setData(m_model->index(row, 4 + i), kssl[i]);
                } else {
                    m_model->setData(m_model->index(row, 4 + i), "");
                }
            }
            QStandardItem* item = m_model->item(row, 3);
            if (! item) {
                item = new QStandardItem;
                m_model->setItem(row, 3, item);
            }
            if (userDefined) {
                item->setIcon(udPixmap);
                item->setText(" "); // for sorting
            } else {
                item->setIcon(noPixmap);
                item->setText("");
            }
            if (ainfo.shortcuts != m_actionMapOriginal[key].shortcuts) {
                // keyboard translation change
                item->setText("k");
            }
        }
        row -= 1;
    }
}

void ActionData::readKeyboardShortcuts()
{
    ResourceFinder().unbundleResource("locale", "keyboard_shortcuts.xml");
    QString keyboardsXml =
        ResourceFinder().getResourcePath("locale", "keyboard_shortcuts.xml");
    if (keyboardsXml == "") {
        RG_DEBUG << "keyboard_shortcuts.xml not found";
    }
    RG_DEBUG << "keyboard_shortcuts.xml found" << keyboardsXml;
    QFile infile(keyboardsXml);

    if (infile.open(QIODevice::ReadOnly) ) {
        QXmlStreamReader stream(&infile);

        stream.readNextStartElement();
        if (stream.name().toString() != "rosegarden_keyboards") {
            RG_DEBUG << keyboardsXml << "invalid file";
            return;
        }
        QString kbName;
        KeyboardTranslation trans;
        int index = 0;
        while(!stream.atEnd()) {
            // Read to the next element delimiter
            do {
                stream.readNext();
            } while(!stream.isStartElement() &&
                    !stream.isEndElement() &&
                    !stream.atEnd());

            if (stream.atEnd()) break;

            if (stream.isStartElement()) {
                if (stream.name().toString() == "name") {
                    kbName = stream.readElementText();
                } else if (stream.name().toString() == "shortcut") {
                    QString src =
                        stream.attributes().value("src").toString();
                    QString dest =
                        stream.attributes().value("dest").toString();
                    trans.translation[src] = dest;
                } else {
                    RG_DEBUG << "start element" << stream.name();
                }
            }

            if (stream.isEndElement()) {
                if (stream.name().toString() == "keyboard") {
                    // Save the keyboard data
                    RG_DEBUG << "save keyboard data for" << kbName;
                    trans.kbName = kbName;
                    m_keyboardTranslations[index] = trans;
                    index++;
                    trans.translation.clear();
                    m_translatedKeyboardNames[kbName] =
                        QObject::tr(kbName.toStdString().c_str());
                } else {
                    RG_DEBUG << "end element" << stream.name();
                }
            }
        }
        infile.close();
    }
}

}
