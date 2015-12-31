
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

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

#include <QWidget>

class QPlainTextEdit;
class QPushButton;

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
                              ConfigureDialogBase *parentDialog = 0);
    void apply();

protected slots:
    void slotShowPopupChanged(int state);
    void slotClear();
    void slotReload();
    void slotResetUndoClearButton();
    void slotResetUndoReloadButton();
 
protected:
    void loadFromMetadata();

    // Set the correct label and tool tip for a button according to it's use
    void setClearButton();
    void setReloadButton();
    void setUndoClearButton();
    void setUndoReloadButton();
    
    RosegardenDocument *m_doc;
    QPlainTextEdit *m_textEdit;
    ConfigureDialogBase *m_parentDialog;
    
    QPushButton *m_clearButton;
    QPushButton *m_reloadButton;
    QString m_saveTextClear;
    QString m_saveTextReload;
    bool m_clearSaved;
    bool m_reloadSaved;
};


}

#endif
