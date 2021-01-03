
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

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

#include "document/RosegardenDocument.h"
#include "document/MetadataHelper.h"
#include "misc/Debug.h"
#include "gui/widgets/InputDialog.h"
#include "gui/dialogs/ConfigureDialogBase.h"

#include <QAction>
#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QString>
#include <QToolTip>
#include <QVBoxLayout>
#include <QWidget>



namespace Rosegarden
{

CommentsConfigurationPage::CommentsConfigurationPage(
        QWidget *parent, RosegardenDocument *doc,
        ConfigureDialogBase *parentDialog) :
    QWidget(parent),
    m_doc(doc),
    m_page(""),             // The reference (default) page of the comments
    m_pageLabel(nullptr),
    m_pageButton(nullptr),
    m_textEdit(nullptr),
    m_parentDialog(parentDialog),
    m_checkBox(nullptr),
    m_clearButton(nullptr),
    m_reloadButton(nullptr),
    m_saveTextClear(""),
    m_saveTextReload(""),
    m_clearSaved(false),
    m_reloadSaved(false)
{

    // Top widgets
    m_pageLabel = new QLabel();
    QLabel *filler = new QLabel("");
    m_pageButton = new QPushButton();


    // Text editor widget
    m_textEdit = new QPlainTextEdit(this);
    m_textEdit->setBackgroundVisible(true);
    QPalette palette = m_textEdit->palette();
    palette.setColor(QPalette::Base, QColor(0xE8, 0xE8, 0xE8));
    m_textEdit->setPalette(palette);
    m_textEdit->setToolTip(tr("<qt>Notes inserted here will be stored in the .rg "
                              "file along with the composition</qt>"));



    // Bottom widgets
    m_checkBox = new QCheckBox;
    m_checkBox->setText(tr("Show at startup"));
    m_checkBox->setToolTip(tr("<p>If checked, these notes or their translation"
                              " into the local language will pop up the next"
                              " time the document is loaded</p>"));
    MetadataHelper mh(m_doc);
    m_checkBox->setChecked(mh.popupWanted());
    m_clearButton = new QPushButton();
    setClearButton();
    m_reloadButton = new QPushButton();
    setReloadButton();
    


    // Define the page layout
    
    // Main layout
    // Contains top widgets, text editor and bottom widgets
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    // Top widgets layout
    QHBoxLayout *pageNameLayout = new QHBoxLayout;
    layout->addLayout(pageNameLayout);
    pageNameLayout->addWidget(m_pageLabel);
    pageNameLayout->addWidget(filler);
    pageNameLayout->addWidget(m_pageButton);
    
    // Editor 
    layout->addWidget(m_textEdit);
    
    // Bottom widgets layout
    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    layout->addLayout(buttonsLayout);
    buttonsLayout->addWidget(m_checkBox);
    buttonsLayout->addWidget(m_clearButton);
    buttonsLayout->addWidget(m_reloadButton);

    

    // Read the comments from the document metadata
    m_comments = loadFromMetadata();

    // Display the reference page
    m_textEdit->setPlainText(m_comments[m_page].text);
    
    // Set the pages related labels
    if (m_comments.size() == 1) {
        m_pageLabel->setText(tr(""));
        m_pageButton->setText(tr("Create another page"));
    } else {
        m_pageLabel->setText(tr("<h3>Main page</h3>"));
        m_pageButton->setText(tr("Change page"));
        m_pageButton->setToolTip(tr("<p>Display another existing page "
                                    "or create a new one.</p>"
                                    "<p>If the page name is a language code"
                                    " name (as \"es\" or \"de\") the page"
                                    " should be a translation of the main page"
                                    " and may be displayed at startup if"
                                    " matching the local language.</p>"));
    }


    // Set up the connexions

    connect(m_clearButton, &QAbstractButton::clicked,
            this, &CommentsConfigurationPage::slotClear);

    connect(m_reloadButton, &QAbstractButton::clicked,
            this, &CommentsConfigurationPage::slotReload);

    if (m_parentDialog) {
        connect(m_textEdit, &QPlainTextEdit::textChanged,
                m_parentDialog, &ConfigureDialogBase::slotActivateApply);
        connect(m_checkBox, &QCheckBox::stateChanged,
                m_parentDialog, &ConfigureDialogBase::slotActivateApply);
    }

    connect (m_checkBox, &QCheckBox::stateChanged,
             this, &CommentsConfigurationPage::slotShowPopupChanged);

    connect(m_pageButton, &QAbstractButton::clicked, this, &CommentsConfigurationPage::slotShowPagesMenu);
}

void
CommentsConfigurationPage::createPage()
{
    bool ok;
    QString pageName = InputDialog::getText(this,
                    tr("Create a new page"), tr("Page name:"),
                    LineEdit::Normal, "", &ok);

    if (ok && !pageName.isEmpty()) {
        cacheEditedCommentPage();
        if (m_comments.find(m_page) == m_comments.end()) {
            m_comments[pageName].text = "";
        }
        showPage(pageName);
    }
}

void
CommentsConfigurationPage::showPage(QString pageName)
{
    cacheEditedCommentPage();
    m_page = pageName;
    m_textEdit->setPlainText(m_comments[m_page].text);
    if (m_page.isEmpty()) {
        m_pageLabel->setText(tr("<h3>Main page</h3>"));
    } else {
        m_pageLabel->setText(tr("<h3>Page \"%1\"</h3>").arg(m_page));
    }
    m_pageButton->setText(tr("Change page"));
}

void
CommentsConfigurationPage::slotShowPagesMenu()
{
    if (m_comments.size() == 1) {
        // There is only one page which is already displayed.
        // So we only can create a new page.
        createPage();
        return;
    }

    // Create a popup menu
    QMenu menu("Page menu");  // Text never shown (?) so translation not needed

    // First mandatory entry : Create a new page.
    QAction * action = menu.addAction(tr("Create a new page"));
    action->setData(1);

    // Second (optional) entry : Go to the main page.
    if (!m_page.isEmpty()) {
        action = menu.addAction(tr("Go to the main page"));
        action->setData("");
    }

    // Add to the menu one entry for each existing page.
    for (MetadataHelper::CommentsMap::iterator it = m_comments.begin();
         it != m_comments.end(); ++it) {
        QString page = it->first;
        if (!page.isEmpty() && (page != m_page)) {
            action = menu.addAction(tr("Go to page \"%1\"").arg(page));
            action->setData(page);
        }
    }

    // Pop up the menu then execute the chosen action
    action = menu.exec(QCursor::pos());
    if (action) {
        if (action->data().userType() == QMetaType::Int) {
            if (action->data().toInt() == 1) {
                createPage();
            }
        } else {
            showPage(action->data().toString());
        }
    }
}

void
CommentsConfigurationPage::slotClear()
{
    if (!m_clearSaved) {
        m_saveTextClear = m_textEdit->toPlainText();
        m_textEdit->setPlainText("");
        setUndoClearButton();
        connect(m_textEdit, &QPlainTextEdit::textChanged,
                this, &CommentsConfigurationPage::slotResetUndoClearButton);
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
        MetadataHelper::CommentsMap comments = loadFromMetadata();
        m_comments[m_page] = comments[m_page];
        m_textEdit->setPlainText(m_comments[m_page].text);
        setUndoReloadButton();
        connect(m_textEdit, &QPlainTextEdit::textChanged,
                this, &CommentsConfigurationPage::slotResetUndoReloadButton);
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
    disconnect(m_textEdit, &QPlainTextEdit::textChanged,
                this, &CommentsConfigurationPage::slotResetUndoClearButton);
    m_saveTextClear = "";
}

void
CommentsConfigurationPage::slotResetUndoReloadButton()
{
    setReloadButton();
    disconnect(m_textEdit, &QPlainTextEdit::textChanged,
                this, &CommentsConfigurationPage::slotResetUndoReloadButton);
    m_saveTextReload = "";
}

void
CommentsConfigurationPage::apply()
{
    cacheEditedCommentPage();
    MetadataHelper mh(m_doc);
    mh.setComments(m_comments);
}

// Return the current UTC date and time using the format "yyyy-MM-dd hh:mm:ss"
static QString
currentTime()
{
    QDateTime t(QDateTime::currentDateTime().toUTC());
    return t.toString("yyyy-MM-dd hh:mm:ss");
}

void
CommentsConfigurationPage::cacheEditedCommentPage()
{
    if (m_textEdit->toPlainText() != m_comments[m_page].text) {
        m_comments[m_page].text = m_textEdit->toPlainText();
        if (m_page.isEmpty()) {
            // The reference page has changed : get a new time stamp.
            m_comments[m_page].timeStamp = currentTime();
        } else {
            // The page has changed and is not the reference one : it get
            // the time stamp of the reference page.
            m_comments[m_page].timeStamp = m_comments[""].timeStamp;
        }
    }
}


MetadataHelper::CommentsMap
CommentsConfigurationPage::loadFromMetadata()
{
    MetadataHelper mh(m_doc);
    MetadataHelper::CommentsMap comments = mh.getComments();
    if (comments.find("") == comments.end()) {
        comments[""].text = "";
    }
    return comments;
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
