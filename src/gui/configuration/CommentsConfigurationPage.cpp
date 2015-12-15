
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

#define RG_MODULE_STRING "[CommentsConfigurationPage]"

#include "CommentsConfigurationPage.h"

#include "misc/ConfigGroups.h"
#include "document/RosegardenDocument.h"
#include "document/io/LilyPondExporter.h"
#include "gui/widgets/CollapsingFrame.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "gui/widgets/LineEdit.h"
#include "gui/configuration/DocumentMetaConfigurationPage.h"
#include "gui/dialogs/ConfigureDialogBase.h"

#include <QApplication>
#include <QSettings>
#include <QListWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QGroupBox>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QPalette>
#include <QString>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QFont>

#include <set>


namespace Rosegarden
{

// Strings used in XML to embed the comments inside the metadata
const QString CommentsConfigurationPage::commentsKeyBase("comments_");

// Size of the numeric part of the line key 
static const int keyNumSize(6);


CommentsConfigurationPage::CommentsConfigurationPage(
        QWidget *parent, RosegardenDocument *doc,
        ConfigureDialogBase *parentDialog) :
    QWidget(parent),
    m_doc(doc),
    m_textEdit(0),
    m_parentDialog(parentDialog),
    m_clearButton(0),
    m_saveTextClear(""),
    m_saveTextReload(""),
    m_clearSaved(false),
    m_reloadSaved(false)
{
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QString localStyle("QPlainTextEdit {background-color: #E8E8E8; color: #000000;}");

    m_textEdit = new QPlainTextEdit(this);
    layout->addWidget(m_textEdit);
    m_textEdit->setBackgroundVisible(true);
    m_textEdit->setStyleSheet(localStyle);
    m_textEdit->setToolTip(tr("<qt>Notes inserted here will be stored in .rg "
                              "file along with the composition</qt>"));

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    layout->addLayout(buttonsLayout);

    m_clearButton = new QPushButton();
    buttonsLayout->addWidget(m_clearButton);
    setClearButton();

    m_reloadButton = new QPushButton();
    buttonsLayout->addWidget(m_reloadButton);
    setReloadButton();

    connect(m_clearButton, SIGNAL(clicked()),
            this, SLOT(slotClear()));

    connect(m_reloadButton, SIGNAL(clicked()),
            this, SLOT(slotReload()));

    // Read the comments from the document metadata
    loadFromMetadata();

    if (m_parentDialog) {
        connect(m_textEdit, SIGNAL(textChanged()),
                m_parentDialog, SLOT(slotActivateApply()));
    }
}

void
CommentsConfigurationPage::slotClear()
{
    if (!m_clearSaved) {
        m_saveTextClear = m_textEdit->toPlainText();
        m_textEdit->setPlainText("");
        setUndoClearButton();
        connect(m_textEdit, SIGNAL(textChanged()),
                this, SLOT(slotResetUndoClearButton()));
    } else {
        m_textEdit->setPlainText(m_saveTextClear);
        m_saveTextClear = "";
        setClearButton();
    }
}

void
CommentsConfigurationPage::slotReload()
{
    if (!m_reloadSaved) {
        m_saveTextReload = m_textEdit->toPlainText();
        loadFromMetadata();
        setUndoReloadButton();
        connect(m_textEdit, SIGNAL(textChanged()),
                this, SLOT(slotResetUndoReloadButton()));
   } else {
        m_textEdit->setPlainText(m_saveTextReload);
        m_saveTextReload = "";
        setReloadButton();
    }
}

void
CommentsConfigurationPage::slotResetUndoClearButton()
{
    setClearButton();
    disconnect(m_textEdit, SIGNAL(textChanged()),
                this, SLOT(slotResetUndoClearButton()));
    m_saveTextClear = "";
}

void
CommentsConfigurationPage::slotResetUndoReloadButton()
{
    setReloadButton();
    disconnect(m_textEdit, SIGNAL(textChanged()),
                this, SLOT(slotResetUndoReloadButton()));
    m_saveTextReload = "";
}

// Make the key associated to a comment line from its line number.
// If the line number is 123 and commentsKeyBase is "comments_" then
// the key is "comments_000123".
static QString lineKey(int lineNumber)
{
    QString number = QString("%1").arg(lineNumber);
    while (number.size() < keyNumSize) number = "0" + number;
    return CommentsConfigurationPage::commentsKeyBase + number;
}

// Get the number of  comment line from its key.
// If the key is "comments_000098" then return 98.
// Return 0 if the string is not a lineKey.
static int lineNumber(QString lineKey)
{
    int baseKeyLength = CommentsConfigurationPage::commentsKeyBase.size();
    if (lineKey.size() != (baseKeyLength + keyNumSize)) return 0;
    if (!lineKey.startsWith(CommentsConfigurationPage::commentsKeyBase)) return 0;
    return lineKey.right(keyNumSize).toInt();
}

void
CommentsConfigurationPage::apply()
{
    // Should only be called from DocumentMetaConfigurationPage::apply()

    Configuration &metadata = m_doc->getComposition().getMetadata();
    QString value = m_textEdit->toPlainText();
    QStringList lines = value.split("\n", QString::KeepEmptyParts);
    
    int n = 0;

    for (QStringList::iterator i = lines.begin(); i != lines.end(); ++i) {
        n++;
        QString value = *i;
        if (!value.isEmpty()) {
            QString key = lineKey(n);
            metadata.set<String>(qstrtostr(key), qstrtostr(value));
        }
    }  
}

void
CommentsConfigurationPage::loadFromMetadata()
{
    m_textEdit->setPlainText("");

    Configuration &metadata = (&m_doc->getComposition())->getMetadata();

    // Get all the relevant keys from the metadata
    std::set<QString> keys;
    keys.clear();
    for (Configuration::iterator
            it = metadata.begin(); it != metadata.end(); ++it) {
        QString key = strtoqstr(it->first);
        if (key.startsWith(CommentsConfigurationPage::commentsKeyBase)) {
            keys.insert(key);
        }
    }

    // Create the text
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
                m_textEdit->appendPlainText("");
            }
            // Insert currentLine
            QString value = strtoqstr(metadata.get<String>(qstrtostr(key)));
            m_textEdit->appendPlainText(value);
            lastLine = currentLine;
        }
    }
}

void
CommentsConfigurationPage::setClearButton()
{
    m_clearSaved = false;
    m_clearButton->setText(tr("Clear", "Button label"));
    m_clearButton->setToolTip(tr("Clear text", "Button tool tip"));
}

void
CommentsConfigurationPage::setReloadButton()
{
    m_reloadSaved = false;
    m_reloadButton->setText(tr("Reload", "Button label"));
    m_reloadButton->setToolTip(tr("<qt>Reload text from the document (come"
                                  " back to the last time apply was pressed)"
                                  "</qt>", "Button tool tip"));
}

void
CommentsConfigurationPage::setUndoClearButton()
{
    m_clearSaved = true;
    m_clearButton->setText(tr("Undo last clear", "Button label"));
    m_clearButton->setToolTip(tr("<qt>Restore to the last text before "
                                 "clear</qt>", "Button tool tip"));
}

void
CommentsConfigurationPage::setUndoReloadButton()
{
    m_reloadSaved = true;
    m_reloadButton->setText(tr("Undo last reload", "Button label"));
    m_reloadButton->setToolTip(tr("<qt>Restore to the last text before "
                                  "reload</qt>", "Button tool tip"));
}

}
