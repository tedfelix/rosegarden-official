/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[ShortcutDialog]"
#define RG_NO_DEBUG_PRINT

#include "ShortcutDialog.h"

#include "gui/general/ActionData.h"
#include "misc/ConfigGroups.h"
#include "misc/Debug.h"
#include "gui/dialogs/ShortcutWarnDialog.h"
#include "gui/dialogs/ShortcutDelegate.h"

#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QSettings>
//#include <QItemSelection>
#include <QPushButton>
#include <QComboBox>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QKeyEvent>


namespace Rosegarden
{


ShortcutDialog::ShortcutDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Keyboard Shortcuts"));
    ActionData* adata = ActionData::getInstance();
    adata->resetChanges();
    m_model = adata->getModel();

    connect(m_model, &QStandardItemModel::dataChanged,
            this, &ShortcutDialog::slotDataChanged);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);

    m_proxyView = new QTreeView;
    m_proxyView->setRootIsDecorated(false);
    m_proxyView->setAlternatingRowColors(true);
    m_proxyView->setModel(m_proxyModel);
    m_proxyView->setSortingEnabled(true);
    m_delegate = new ShortcutDelegate(this);
    m_proxyView->setItemDelegate(m_delegate);
    m_proxyView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    connect(m_proxyView->selectionModel(),
                    &QItemSelectionModel::selectionChanged,
            this, &ShortcutDialog::slotSelectionChanged);

    m_filterPatternLineEdit = new QLineEdit;
    m_filterPatternLineEdit->setClearButtonEnabled(true);
    m_filterPatternLabel = new QLabel(tr("Filter pattern:"));

    connect(m_filterPatternLineEdit, &QLineEdit::textChanged,
            this, &ShortcutDialog::slotFilterChanged);

    QGridLayout *proxyLayout = new QGridLayout;
    proxyLayout->addWidget(m_filterPatternLabel, 0, 0);
    proxyLayout->addWidget(m_filterPatternLineEdit, 0, 1, 1, 3);
    proxyLayout->addWidget(m_proxyView, 1, 0, 1, 4);

    QVBoxLayout *mainLayout = new QVBoxLayout;

    setLayout(mainLayout);

    QLabel* helpLabel = new QLabel;
    helpLabel->setWordWrap(true);
    helpLabel->setText(tr("<p>Select an action in the table below then double click one of the four <b>Shortcut</b> fields in the table.  Press the new shortcut key.  A shortcut can be removed by pressing Shift in the <b>Shortcut</b> field.</p><p><i>Actions marked with a <span style=\"background-color:cyan; color:black\">light blue background</span> are global and valid for all windows.</i></p>"));

    mainLayout->addWidget(helpLabel);

    m_proxyView->sortByColumn(0, Qt::AscendingOrder);

    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    this->restoreGeometry(settings.value("Shortcut_Dialog").toByteArray());
    QStringList columnWidths =
        settings.value("Shortcut_Table_Widths").toStringList();
    settings.endGroup();

    // set column widths
    for (int i = 0; i < columnWidths.size(); i++) {
        m_proxyView->setColumnWidth(i, columnWidths[i].toInt());
    }

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->setContentsMargins(0, 10, 0, 10);

    m_resetSelected = new QPushButton(tr("Reset Selected"));
    connect(m_resetSelected, &QPushButton::clicked,
            this, &ShortcutDialog::slotResetSelectedClicked);
    m_resetSelected->setEnabled(false);
    m_resetSelected->setToolTip(tr("Reset selected actions' shortcuts to defaults"));
    m_removeShortcuts = new QPushButton(tr("Remove shortcuts"));
    connect(m_removeShortcuts, &QPushButton::clicked,
            this, &ShortcutDialog::slotRemoveShortcutsClicked);
    m_removeShortcuts->setEnabled(false);
    m_removeShortcuts->setToolTip(tr("Remove all shortcuts from selected actions"));
    m_resetAll = new QPushButton(tr("Reset All"));
    connect(m_resetAll, &QPushButton::clicked,
            this, &ShortcutDialog::slotResetAllClicked);
    m_resetAll->setEnabled(true);
    m_resetAll->setToolTip(tr("Reset all shortcuts for all actions"));

    m_warnLabel = new QLabel(tr("Warnings when:"));
    m_warningsWhen = new QComboBox;
    m_warningsWhen->addItem(tr("Never"));
    m_warningsWhen->addItem(tr("Conflict in same context"));
    m_warningsWhen->addItem(tr("Conflict in any context"));
    connect(m_warningsWhen, static_cast<void(QComboBox::*)(int)>(
                    &QComboBox::currentIndexChanged),
            this, &ShortcutDialog::slotWarningsWhenChanged);
    settings.beginGroup(GeneralOptionsConfigGroup);
    m_warnType = (WarningType)(settings.value("shortcut_warnings", 1).toInt());
    m_warningsWhen->setCurrentIndex(m_warnType);
    settings.endGroup();

    m_keyboardLabel = new QLabel(tr("Keyboard:"));
    m_keyboardLabel->setToolTip(tr("Apply changes for your keyboard layout."));
    m_keyboard = new QComboBox;
    m_keyboard->setToolTip(tr("Apply changes for your keyboard layout."));
    std::list<QString> keyboards;
    int kbIndex = adata->getKeyboards(keyboards);
    foreach(QString kb, keyboards) {
        m_keyboard->addItem(kb);
    }
    m_keyboard->setCurrentIndex(kbIndex);
    connect(m_keyboard, static_cast<void(QComboBox::*)(int)>(
                    &QComboBox::currentIndexChanged),
            this, &ShortcutDialog::slotKeyboardChanged);

    hlayout->addStretch();
    hlayout->addWidget(m_resetSelected);
    hlayout->addStretch();
    hlayout->addWidget(m_removeShortcuts);
    hlayout->addStretch();
    hlayout->addWidget(m_resetAll);
    hlayout->addStretch();
    hlayout->addWidget(m_warnLabel);
    hlayout->addWidget(m_warningsWhen);
    hlayout->addStretch();
    hlayout->addWidget(m_keyboardLabel);
    hlayout->addWidget(m_keyboard);
    hlayout->addStretch();

    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QDialogButtonBox *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted,
                     this, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected,
                     this, &ShortcutDialog::reject);

    mainLayout->addLayout(hlayout);
    mainLayout->addWidget(line);
    mainLayout->addLayout(proxyLayout);
    mainLayout->addWidget(buttonBox);
}

ShortcutDialog::~ShortcutDialog()
{
    QStringList columnWidths;
    // save column widths (except for last one)
    for (int i = 0; i < m_model->columnCount() - 1; i++) {
        columnWidths << QString::number(m_proxyView->columnWidth(i));
    }
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    settings.setValue("Shortcut_Dialog", this->saveGeometry());
    settings.setValue("Shortcut_Table_Widths", columnWidths);
    settings.endGroup();

    // save any changes to settings
    ActionData* adata = ActionData::getInstance();
    adata->saveUserShortcuts();
    if (adata->hasDataChanged()) {
        QMessageBox::information(this,
                                 tr("Shortcuts Changed"),
                                 tr("You must restart Rosegarden for shortcut changes to take effect."));
    }
    delete m_delegate;
}

void ShortcutDialog::setModelData(const QKeySequence ks,
                                  const QModelIndex &index)
{
    // index is an index for the proxy model
    QModelIndex srcIndex = m_proxyModel->mapToSource(index);
    int row = srcIndex.row();
    int column = srcIndex.column();
    ActionData* adata = ActionData::getInstance();
    QString key = adata->getKey(row);
    RG_DEBUG << "setModelData" << key << ks;
    KeyList ksOld = adata->getShortcuts(key);
    if (! ks.isEmpty()) {
        // check for duplicates
        for (const QKeySequence& kso : ksOld) {
            if (kso == ks) {
                // duplicate - nothing to do
                return;
            }
        }
    }
    KeyList ksList;
    unsigned int editIndex = column - 4;
    RG_DEBUG << "setModelData editIndex:" << editIndex;
    unsigned int kssIndex = 0;
    QKeySequence toRemove;
    foreach(auto dks, ksOld) {
        if (kssIndex == editIndex) {
            if (! dks.isEmpty()) {
                toRemove = dks;
            }
            if (! ks.isEmpty()) {
                ksList.push_back(ks);
            }
        } else {
            ksList.push_back(dks);
        }
        kssIndex++;
    }
    if (editIndex >= ksOld.size() && !ks.isEmpty()) {
        ksList.push_back(ks);
    }
    if (! toRemove.isEmpty()) {
        ksList.remove(toRemove);
    }
    // debug
    QStringList sl;
    foreach(auto k, ksOld) {
        sl << k.toString();
    }
    RG_DEBUG << "setModelData old:" << sl;
    sl.clear();
    foreach(auto k, ksList) {
        sl << k.toString();
    }
    RG_DEBUG << "setModelData new:" << sl;
    // debug end
    if (ksList == ksOld) {
        RG_DEBUG << "setModelData no change";
        return;
    }
    ActionData::DuplicateData duplicates;
    if (m_warnType != None) {
#if 0
        QString context = "";
        if (m_warnType == SameContext) {
            QStringList klist = key.split(":");
            context = klist[0];
        }
#endif
        std::set<QString> keys;
        keys.insert(key);
        bool sameContext = (m_warnType == SameContext);
        std::set<QKeySequence> ksSet;
        for (auto& val : ksList) {
            ksSet.insert(val);
        }
        adata->getDuplicateShortcuts(keys, ksSet, false,
                                     sameContext, duplicates);
        if (! duplicates.empty()) {
            // ask the user
            ShortcutWarnDialog warnDialog(duplicates);
            if (warnDialog.exec() != QDialog::Accepted) {
                RG_DEBUG << "setModelData warnDialog rejected";
                // do nothing
                return;
            }
        }
    }

    // set the shortcuts
    adata->setUserShortcuts(key, ksList);
    if (m_warnType != None) {
        // remove the duplicates
        foreach(auto pair, duplicates[key].duplicateMap) {
            const QKeySequence& dks = pair.first;
            const ActionData::KeyDuplicates& kdups = pair.second;
            foreach(auto kdup, kdups) {
                adata->removeUserShortcut(kdup.key, dks);
            }
        }
    }
    // refresh edit data
    editRow();
}

void ShortcutDialog::slotFilterChanged(const QString &text)
{
    m_proxyModel->setFilterFixedString(text);
}

void ShortcutDialog::slotSelectionChanged(const QItemSelection&,
                                      const QItemSelection&)
{
    RG_DEBUG << "selection changed";
    QModelIndexList indexes =
        m_proxyView->selectionModel()->selectedIndexes();

    m_editRows.clear();
    //std::set<int> selectedRows;
    foreach(auto index, indexes) {
        QModelIndex srcIndex = m_proxyModel->mapToSource(index);
        int row = srcIndex.row();
        m_editRows.insert(row);
    }
    RG_DEBUG << "selectedRows size" << m_editRows.size();

    editRow();
}

void ShortcutDialog::slotResetSelectedClicked(bool)
{
    RG_DEBUG << "set shortcut to default";
    ActionData* adata = ActionData::getInstance();
    ActionData::DuplicateData duplicates;
    std::set<QString> keys;
    foreach(auto row, m_editRows) {
        QString key = adata->getKey(row);
        keys.insert(key);
    }
    if (m_warnType != None) {
        //QString context = "";
        std::set<QKeySequence> ksSet;
        bool sameContext = (m_warnType == SameContext);
        adata->getDuplicateShortcuts(keys, ksSet, true,
                                     sameContext, duplicates);
        if (! duplicates.empty()) {
            // ask the user
            ShortcutWarnDialog warnDialog(duplicates);
            if (warnDialog.exec() != QDialog::Accepted) {
                RG_DEBUG << "setModelData warnDialog rejected";
                // do nothing
                return;
            }
        }
    }
    for (const QString &key: keys) {
        adata->removeUserShortcuts(key);
        if (m_warnType != None) {
            // remove the duplicates
            for (auto pair : duplicates) {
                const ActionData::DuplicateDataForKey& ddatak = pair.second;
                for (auto dpair : ddatak.duplicateMap) {
                    const QKeySequence& dks = dpair.first;
                    const ActionData::KeyDuplicates& kdups = dpair.second;
                    for (auto kdup : kdups) {
                        adata->removeUserShortcut(kdup.key, dks);
                    }
                }
            }
        }
    }
    // refresh edit data
    editRow();
}

void ShortcutDialog::slotRemoveShortcutsClicked(bool)
{
    RG_DEBUG << "remove shortcuts on selection";
    ActionData* adata = ActionData::getInstance();
    foreach(auto row, m_editRows) {
        QString key = adata->getKey(row);
        KeyList ksList;
        adata->setUserShortcuts(key, ksList);
    }
    // refresh edit data
    editRow();
}

void ShortcutDialog::slotResetAllClicked(bool)
{
    RG_DEBUG << "remove all shortcuts";
    QMessageBox::StandardButton reply =
        QMessageBox::warning(
                             0,
                             tr("Rosegarden"),
                             tr("This will reset all shortcuts for all actions across all contexts. Are you sure?"),
                             QMessageBox::Yes | QMessageBox::No,
                             QMessageBox::Yes);

    // If not, return.
    if (reply == QMessageBox::No)
        return;

    ActionData* adata = ActionData::getInstance();
    adata->removeAllUserShortcuts();
}

void ShortcutDialog::slotKeyboardChanged(int index)
{
    RG_DEBUG << "keyboardChanged" << index;
    ActionData* adata = ActionData::getInstance();
    adata->applyKeyboard(index);
}

void ShortcutDialog::slotWarningsWhenChanged(int index)
{
    RG_DEBUG << "warnSettingChanged" << index;
    m_warnType = static_cast<WarningType>(index);
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    settings.setValue("shortcut_warnings", m_warningsWhen->currentIndex());
    settings.endGroup();
}

void ShortcutDialog::reject()
{
    // if cancel is pressed we must undo all the changes
    ActionData* adata = ActionData::getInstance();
    adata->undoChanges();
    QDialog::reject();
}

void ShortcutDialog::slotDataChanged(const QModelIndex& topLeft,
                                 const QModelIndex& bottomRight)
{
    // debugging only

    RG_DEBUG << "dataChanged" << topLeft.row() << topLeft.column() << bottomRight.row() << bottomRight.column() << (topLeft == bottomRight);
    if (topLeft != bottomRight) {
        RG_DEBUG << "dataChanged region edit";
    }
}

void ShortcutDialog::editRow()
{
    RG_DEBUG << "editRow:";
    const ActionData *adata = ActionData::getInstance();
    m_resetSelected->setEnabled(false);
    m_removeShortcuts->setEnabled(false);

    foreach(auto row, m_editRows) {
        RG_DEBUG << "edit row" << row;
        QString key = adata->getKey(row);
        RG_DEBUG << "editing key" << key;

        KeyList ksList = adata->getShortcuts(key);
        if (! adata->isDefault(key, ksList)) {
            m_resetSelected->setEnabled(true);
        }
        if (! ksList.empty()) {
            m_removeShortcuts->setEnabled(true);
        }
    }
}

void ShortcutDialog::keyPressEvent(QKeyEvent *event)
{
    // catch enter and return to avoid accepting the dialog with these keys
    if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return) {
        return;
    }
    QDialog::keyPressEvent(event);
}

}
