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

#define RG_MODULE_STRING "[ShortcutDelegate]"
#define RG_NO_DEBUG_PRINT

#include "ShortcutDelegate.h"

#include "misc/Debug.h"
#include "gui/dialogs/ShortcutDialog.h"

#include <QKeySequenceEdit>
#include <QKeyEvent>

namespace Rosegarden
{

ShortcutDelegate::ShortcutDelegate(ShortcutDialog *dialog) :
    m_editor(nullptr),
    m_dialog(dialog)
{
}

ShortcutDelegate::~ShortcutDelegate()
{
}

QWidget* ShortcutDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem&,
                                        const QModelIndex&) const
{
    RG_DEBUG << "createEditor";
    m_editor = new QKeySequenceEdit(parent);
    connect(m_editor, SIGNAL(editingFinished()),
            this, SLOT(ksEditFinished()));
    m_editor->setFocusPolicy(Qt::StrongFocus);
    m_editor->setFocus();
    return m_editor;
}

void ShortcutDelegate::setModelData(QWidget *editor,
                                    QAbstractItemModel*,
                                    const QModelIndex &index) const
{
    RG_DEBUG << "setModelData" << index.row() << index.column();
    Q_ASSERT(m_editor == editor);

    QKeySequence ks = m_editor->keySequence();
    // limit to just one shortcut
    if (! ks.isEmpty()) {
        QKeySequence singleKey(ks[0]);
        ks = singleKey;
    }
    m_dialog->setModelData(ks, index);
}

bool ShortcutDelegate::eventFilter(QObject *editor, QEvent *event)
{
    if (event->type()==QEvent::KeyPress) {
        QKeyEvent* key = static_cast<QKeyEvent*>(event);
        if (key->key()==Qt::Key_Escape) {
            return false;
        }
    }
    return QStyledItemDelegate::eventFilter(editor, event);
}

void ShortcutDelegate::ksEditFinished()
{
    RG_DEBUG << "ksEditFinished";
    m_editor->clearFocus();
}

}
