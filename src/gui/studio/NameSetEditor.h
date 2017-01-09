
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NAMESETEDITOR_H
#define RG_NAMESETEDITOR_H

#include "gui/widgets/LineEdit.h"

#include <QString>
#include <QGroupBox>
#include <QToolButton>
#include <QPushButton>
#include <QTreeWidgetItem>
#include <QLabel>
#include <QFrame>
#include <QGridLayout>

#include <vector>


namespace Rosegarden
{

class BankEditorDialog;


class NameSetEditor : public QGroupBox
{
    Q_OBJECT
public:
    virtual void clearAll() = 0;

    virtual void populate(QTreeWidgetItem *) = 0;
    virtual void reset() = 0;

public slots:
    virtual void slotNameChanged(const QString&) = 0;
    virtual void slotKeyMapButtonPressed() = 0;
    void slotToggleInitialLabel();

protected:
    NameSetEditor(BankEditorDialog *bankEditor,
                  QString title,
                  QWidget *parent,
                  const char *name,
                  QString headingPrefix = "",
                  bool showKeyMapButtons = false);

    QToolButton *getKeyMapButton(int n) { return m_keyMapButtons[n]; }
    const QToolButton *getKeyMapButton(int n) const { return m_keyMapButtons[n]; }

    QGridLayout              *m_mainLayout;
    BankEditorDialog         *m_bankEditor;
    QStringList               m_completions;
    QPushButton              *m_initialLabel;
    std::vector<QLabel*>      m_labels;
    std::vector<LineEdit*>    m_names;
    QFrame                   *m_mainFrame;
    QLabel                   *m_librarian;
    QLabel                   *m_librarianEmail;
    std::vector<QToolButton*> m_keyMapButtons;
};


}

#endif
