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

#ifndef RG_SHORTCUTDELEGATE_H
#define RG_SHORTCUTDELEGATE_H

#include <QStyledItemDelegate>

class QKeySequenceEdit;

namespace Rosegarden
{

class ShortcutDialog;

/// Keyboard Shortcuts delegate
class ShortcutDelegate : public QStyledItemDelegate
{
    Q_OBJECT

 public:
    explicit ShortcutDelegate(ShortcutDialog *dialog);
    ~ShortcutDelegate();

    QWidget* createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override;

    bool eventFilter(QObject *editor, QEvent *event) override;

 private slots:
    void ksEditFinished();

 private:
    mutable QKeySequenceEdit* m_editor;
    ShortcutDialog* m_dialog;
};

}

#endif
