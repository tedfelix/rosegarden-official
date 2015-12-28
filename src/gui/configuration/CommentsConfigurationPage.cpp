
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
#include "document/MetadataHelper.h"
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
#include <QCheckBox>
#include <QLayout>
#include <QPushButton>
#include <QString>
#include <QPlainTextEdit>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QFont>

#include <set>


namespace Rosegarden
{

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
    m_textEdit->setToolTip(tr("<qt>Notes inserted here will be stored in the .rg "
                              "file along with the composition</qt>"));

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    layout->addLayout(buttonsLayout);
    
    QCheckBox *checkBox = new QCheckBox;
    buttonsLayout->addWidget(checkBox);
    checkBox->setText(tr("Show at startup"));
    MetadataHelper mh(m_doc);
    checkBox->setChecked(mh.popupWanted());
    
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
        connect(checkBox, SIGNAL(stateChanged(int)),
                m_parentDialog, SLOT(slotActivateApply()));
    }

    connect (checkBox, SIGNAL(stateChanged(int)),
             this, SLOT(slotShowPopupChanged(int)));
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

void
CommentsConfigurationPage::apply()
{
    MetadataHelper mh(m_doc);
    QString text = m_textEdit->toPlainText();
    mh.setComments(m_textEdit->toPlainText());
}

void
CommentsConfigurationPage::loadFromMetadata()
{
    MetadataHelper mh(m_doc);
    QString text = mh.getComments();
    m_textEdit->setPlainText(text);
}

void
CommentsConfigurationPage::slotShowPopupChanged(int state)
{
    MetadataHelper mh(m_doc);
    mh.setPopupWanted(state != Qt::Unchecked);
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
