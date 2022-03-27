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

#define RG_MODULE_STRING "[ShortcutDialog]"
#define RG_NO_DEBUG_PRINT

#include "ShortcutDialog.h"

#include "gui/general/ActionData.h"
#include "misc/ConfigGroups.h"
#include "misc/Debug.h"
#include "gui/dialogs/ShortcutWarnDialog.h"
#include "gui/dialogs/ShortcutDelegate.h"
#include "gui/general/ResourceFinder.h"

#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QSettings>
#include <QItemSelection>
#include <QKeySequenceEdit>
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

    connect(m_model,
            SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
            this,
            SLOT(dataChanged(const QModelIndex&, const QModelIndex&)));

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
            SIGNAL(selectionChanged(const QItemSelection&,
                                    const QItemSelection&)),
            this,
            SLOT(selectionChanged(const QItemSelection&,
                                  const QItemSelection&)));

    m_filterPatternLineEdit = new QLineEdit;
    m_filterPatternLabel = new QLabel(tr("Filter pattern:"));

    connect(m_filterPatternLineEdit, SIGNAL(textChanged(const QString&)),
            this, SLOT(filterChanged()));

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

    m_defPB = new QPushButton(tr("Reset Selected"));
    connect(m_defPB, SIGNAL(clicked()),
            this, SLOT(defPBClicked()));
    m_defPB->setEnabled(false);
    m_defPB->setToolTip(tr("Reset selected actions' shortcuts to defaults"));
    m_clearPB = new QPushButton(tr("Remove shortcuts"));
    connect(m_clearPB, SIGNAL(clicked()),
            this, SLOT(clearPBClicked()));
    m_clearPB->setEnabled(false);
    m_clearPB->setToolTip(tr("Remove all shortcuts from selected actions"));
    m_clearAllPB = new QPushButton(tr("Reset All"));
    connect(m_clearAllPB, SIGNAL(clicked()),
            this, SLOT(clearAllPBClicked()));
    m_clearAllPB->setEnabled(true);
    m_clearAllPB->setToolTip(tr("Reset all shortcuts for all actions"));

    m_warnLabel = new QLabel(tr("Warnings when:"));
    m_warnSetting = new QComboBox;
    m_warnSetting->addItem(tr("Never"));
    m_warnSetting->addItem(tr("Conflict in same context"));
    m_warnSetting->addItem(tr("Conflict in any context"));
    connect(m_warnSetting, SIGNAL(currentIndexChanged(int)),
            this, SLOT(warnSettingChanged(int)));
    settings.beginGroup(GeneralOptionsConfigGroup);
    m_warnType = (WarningType)(settings.value("shortcut_warnings", 1).toInt());
    m_warnSetting->setCurrentIndex(m_warnType);
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
    connect(m_keyboard, SIGNAL(currentIndexChanged(int)),
            this, SLOT(keyboardChanged(int)));

    hlayout->addStretch();
    hlayout->addWidget(m_defPB);
    hlayout->addStretch();
    hlayout->addWidget(m_clearPB);
    hlayout->addStretch();
    hlayout->addWidget(m_clearAllPB);
    hlayout->addStretch();
    hlayout->addWidget(m_warnLabel);
    hlayout->addWidget(m_warnSetting);
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
    std::set<QKeySequence> ksOld = adata->getShortcuts(key);
    std::set<QKeySequence> ksSet;
    unsigned int editIndex = column - 4;
    RG_DEBUG << "setModelData editIndex:" << editIndex;
    if (editIndex >= ksOld.size() && !ks.isEmpty()) {
        ksSet.insert(ks);
    }
    unsigned int kssIndex = 0;
    QKeySequence toRemove;
    foreach(auto dks, ksOld) {
        if (kssIndex == editIndex) {
            if (! dks.isEmpty()) {
                toRemove = dks;
            }
            if (! ks.isEmpty()) {
                ksSet.insert(ks);
            }
        } else {
            ksSet.insert(dks);
        }
        kssIndex++;
    }
    if (! toRemove.isEmpty()) {
        ksSet.erase(toRemove);
    }
    // debug
    QStringList sl;
    foreach(auto k, ksOld) {
        sl << k.toString();
    }
    RG_DEBUG << "setModelData old:" << sl;
    sl.clear();
    foreach(auto k, ksSet) {
        sl << k.toString();
    }
    RG_DEBUG << "setModelData new:" << sl;
    // debug end
    if (ksSet == ksOld) {
        RG_DEBUG << "setModelData no change";
        return;
    }
    ActionData::DuplicateData duplicates;
    if (m_warnType != None) {
        QString context = "";
        if (m_warnType == SameContext) {
            QStringList klist = key.split(":");
            context = klist[0];
        }
        std::set<QString> keys;
        keys.insert(key);
        bool sameContext = (m_warnType == SameContext);
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
    adata->setUserShortcuts(key, ksSet);
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

void ShortcutDialog::filterChanged()
{
    m_proxyModel->setFilterFixedString(m_filterPatternLineEdit->text());
}

void ShortcutDialog::selectionChanged(const QItemSelection&,
                                      const QItemSelection&)
{
    RG_DEBUG << "selection changed";
    QModelIndexList indexes =
        m_proxyView->selectionModel()->selectedIndexes();

    m_editRows.clear();
    std::set<int> selectedRows;
    foreach(auto index, indexes) {
        QModelIndex srcIndex = m_proxyModel->mapToSource(index);
        int row = srcIndex.row();
        m_editRows.insert(row);
    }
    RG_DEBUG << "selectedRows size" << m_editRows.size();

    editRow();
}

void ShortcutDialog::defPBClicked()
{
    RG_DEBUG << "set shortcut to default";
    ActionData* adata = ActionData::getInstance();
    std::set<QKeySequence> ksSet;
    ActionData::DuplicateData duplicates;
    std::set<QString> keys;
    foreach(auto row, m_editRows) {
        QString key = adata->getKey(row);
        keys.insert(key);
    }
    if (m_warnType != None) {
        QString context = "";
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
    foreach(auto key, keys) {
        adata->removeUserShortcuts(key);
        if (m_warnType != None) {
            // remove the duplicates
            foreach(auto pair, duplicates) {
                const ActionData::DuplicateDataForKey& ddatak = pair.second;
                foreach(auto dpair, ddatak.duplicateMap) {
                    const QKeySequence& dks = dpair.first;
                    const ActionData::KeyDuplicates& kdups = dpair.second;
                    foreach(auto kdup, kdups) {
                        adata->removeUserShortcut(kdup.key, dks);
                    }
                }
            }
        }
    }
    // refresh edit data
    editRow();
}

void ShortcutDialog::clearPBClicked()
{
    RG_DEBUG << "remove shortcuts on selection";
    ActionData* adata = ActionData::getInstance();
    foreach(auto row, m_editRows) {
        QString key = adata->getKey(row);
        std::set<QKeySequence> ksSet;
        adata->setUserShortcuts(key, ksSet);
    }
    // refresh edit data
    editRow();
}

void ShortcutDialog::clearAllPBClicked()
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

void ShortcutDialog::keyboardChanged(int index)
{
    RG_DEBUG << "keyboardChanged" << index;
    ActionData* adata = ActionData::getInstance();
    adata->applyKeyboard(index);
}

void ShortcutDialog::warnSettingChanged(int index)
{
    RG_DEBUG << "warnSettingChanged" << index;
    m_warnType = static_cast<WarningType>(index);
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    settings.setValue("shortcut_warnings", m_warnSetting->currentIndex());
    settings.endGroup();
}

void ShortcutDialog::reject()
{
    // if cancel is pressed we must undo all the changes
    ActionData* adata = ActionData::getInstance();
    adata->undoChanges();
    QDialog::reject();
}

void ShortcutDialog::dataChanged(const QModelIndex& topLeft,
                                 const QModelIndex& bottomRight)
{
    RG_DEBUG << "dataChanged" << topLeft.row() << topLeft.column() <<
        bottomRight.row() << bottomRight.column() <<
        (topLeft == bottomRight);
    if (topLeft != bottomRight) {
        RG_DEBUG << "dataChanged region edit";
    }
}

void ShortcutDialog::editRow()
{
    RG_DEBUG << "editRow:";
    ActionData* adata = ActionData::getInstance();
    m_defPB->setEnabled(false);
    m_clearPB->setEnabled(false);

    foreach(auto row, m_editRows) {
        RG_DEBUG << "edit row" << row;
        QString key = adata->getKey(row);
        RG_DEBUG << "editing key" << key;

        std::set<QKeySequence> ksSet = adata->getShortcuts(key);
        if (! adata->isDefault(key, ksSet)) {
            m_defPB->setEnabled(true);
        }
        if (! ksSet.empty()) {
            m_clearPB->setEnabled(true);
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
