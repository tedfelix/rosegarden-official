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

#ifndef RG_SHORTCUTDIALOG_H
#define RG_SHORTCUTDIALOG_H

#include <QDialog>

#include <map>
#include <set>

class QGroupBox;
class QTreeView;
class QLabel;
class QLineEdit;
class QAbstractItemModel;
class QSortFilterProxyModel;
class QStandardItemModel;
class QItemSelection;
class QKeySequenceEdit;
class QPushButton;
class QComboBox;

namespace Rosegarden
{

class ShortcutDelegate;

/// Keyboard Shortcuts dialog
class ShortcutDialog : public QDialog
{
    Q_OBJECT

 public:
    explicit ShortcutDialog(QWidget *parent);
    ~ShortcutDialog();

    void setModelData(const QKeySequence ks, const QModelIndex &index);

 private slots:
    void filterChanged();
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected);
    void defPBClicked();
    void clearPBClicked();
    void clearAllPBClicked();
    void keyboardChanged(int index);
    void warnSettingChanged(int index);
    void reject() override;
    void dataChanged(const QModelIndex&, const QModelIndex&);

 private:
    enum WarningType { None, SameContext, AllContexts };

    void editRow();
    void keyPressEvent(QKeyEvent *event) override;

    QSortFilterProxyModel *m_proxyModel;

    QTreeView *m_proxyView;
    QLabel *m_filterPatternLabel;
    QLineEdit *m_filterPatternLineEdit;
    QStandardItemModel *m_model;
    QPushButton *m_defPB;
    QPushButton *m_clearPB;
    QPushButton *m_clearAllPB;
    QLabel *m_warnLabel;
    QComboBox *m_warnSetting;
    QLabel *m_keyboardLabel;
    QComboBox *m_keyboard;
    std::set<int> m_editRows;
    WarningType m_warnType;
    ShortcutDelegate *m_delegate;
    std::map<int, QString> m_indexMap;
};

}

#endif
