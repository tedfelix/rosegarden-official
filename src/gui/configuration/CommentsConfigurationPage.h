
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

#ifndef RG_COMMENTSCONFIGURATIONPAGE_H
#define RG_COMMENTSCONFIGURATIONPAGE_H

#include "document/MetadataHelper.h"

#include <QWidget>

#include <map>

class QPlainTextEdit;
class QPushButton;
class QCheckBox;
class QLabel;

namespace Rosegarden
{

class RosegardenDocument;
class ConfigureDialogBase;

class CommentsConfigurationPage : public QWidget
{
    Q_OBJECT

public:
    // parentDialog is only used to allow enabling the apply button of
    // the dialog.
    CommentsConfigurationPage(QWidget *parent, RosegardenDocument *doc,
                              ConfigureDialogBase *parentDialog = nullptr);
    void apply();

protected slots:
    void slotShowPopupChanged(int state);
    void slotClear();
    void slotReload();
    void slotResetUndoClearButton();
    void slotResetUndoReloadButton();
    void slotShowPagesMenu();
    
    // Store the currently edited page in m_comments and update its time stamp 
    void cacheEditedCommentPage();
 
protected:
    MetadataHelper::CommentsMap loadFromMetadata();
    void showPage(QString pageName);
    void createPage();

    // Set the correct label and tool tip for a button according to it's use
    // ("Clear" button may be used as "Undo Clear" button and "Reload" button
    // may be used as "Undo Reload" button).
    void setClearButton();
    void setReloadButton();
    void setUndoClearButton();
    void setUndoReloadButton();
    
    RosegardenDocument *m_doc;
    MetadataHelper::CommentsMap m_comments;
    QString m_page;         // Current page
    
    QLabel *m_pageLabel;
    QPushButton *m_pageButton;
    QPlainTextEdit *m_textEdit;
    ConfigureDialogBase *m_parentDialog;
    QCheckBox *m_checkBox;
    
    QPushButton *m_clearButton;
    QPushButton *m_reloadButton;
    QString m_saveTextClear;
    QString m_saveTextReload;
    bool m_clearSaved;
    bool m_reloadSaved;
};


}

#endif
