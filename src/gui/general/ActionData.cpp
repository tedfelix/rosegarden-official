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
//#define RG_NO_DEBUG_PRINT

#include "ActionData.h"

#include "misc/Debug.h"
#include "ResourceFinder.h"
#include "document/io/XMLReader.h"
#include "gui/general/IconLoader.h"

#include <QFile>
#include <QStandardItemModel>

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

void ActionData::fillModel(QStandardItemModel *model)
{
    m_keyStore.clear();
    
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("Context"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Action"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Icon"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Shortcuts"));

    for (auto i = m_dataMap.begin(); i != m_dataMap.end(); i++) {
        QString file = (*i).first;
        RCFileData& fdata = (*i).second;
        for (auto mit = fdata.actionMap.begin();
             mit != fdata.actionMap.end();
             mit++) {
            const QString& aname = (*mit).first;
            const ActionInfo& ainfo = (*mit).second;
            QString textAdj = ainfo.text;
            textAdj.remove("&");
            model->insertRow(0);
            QString key = file + ":" + aname;
            m_keyStore.push_front(key);
            model->setData(model->index(0, 0), m_contextMap[file]);
            model->setData(model->index(0, 1), textAdj);
            if (ainfo.icon != "") {
                QIcon icon = IconLoader::load(ainfo.icon);
                if (! icon.isNull()) {
                    QPixmap pixmap =
                        icon.pixmap(icon.availableSizes().first());
                    QStandardItem* item = new QStandardItem;
                    item->setIcon(pixmap);
                    model->setItem(0, 2, item);
                } else {
                    model->setData(model->index(0, 2), "");
                }
            }
            model->setData(model->index(0, 3), ainfo.shortcut);
        }
    }
}

QString ActionData::getKey(int row)
{
    return m_keyStore[row];
}
    
ActionData::ActionData()
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
            std::map<QString, ActionInfo>& amap =
                m_dataMap[m_currentFile].actionMap;
            auto it = amap.find(actionName);
            if (it == amap.end()) {
                // new action
                ActionInfo ainfo;
                amap[actionName] = ainfo;
            }
            ActionInfo& ainfo = amap[actionName];
            if (! m_currentMenus.empty()) ainfo.menus = m_currentMenus;
            if (m_currentToolbar != "") ainfo.toolbar = m_currentToolbar;
            if (text != "") ainfo.text = translate(text);
            if (icon != "") ainfo.icon = icon;
            QStringList shortcuts = shortcut.split(", ");
            QStringList shortcuts_trans;
            for (int i = 0; i < shortcuts.size(); i++) {
                // Keyboard shortcuts require the disambiguation
                // "keyboard shortcut" and this must match
                // scripts/extract_menu_tr_strings.pl
                shortcuts_trans.append(translate(shortcuts.at(i),
                                                 "keyboard shortcut"));
            }
            QString shortcut_trans = shortcuts_trans.join(", ");
    
            if (shortcut != "") ainfo.shortcut = shortcut_trans;
            if (tooltip != "") ainfo.tooltip = tooltip;
            RG_DEBUG << "data" << m_currentMenus << m_currentToolbar <<
                actionName << icon <<
                shortcut << tooltip;
            RG_DEBUG << "map size" << m_currentFile << amap.size();
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

QString ActionData::translate(QString text, QString disambiguation)
{
    // These translations are extracted from data/ui/*.rc files via
    // scripts/extract*.pl and pulled into the QObject translation context in
    // one great lump.
    
    if (!disambiguation.isEmpty()) return QObject::tr(text.toStdString().c_str(), disambiguation.toStdString().c_str());
    else return QObject::tr(text.toStdString().c_str());
}
    
}
