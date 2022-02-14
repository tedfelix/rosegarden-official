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
//#define RG_NO_DEBUG_PRINT

#include "ShortcutDialog.h"

#include "gui/general/ActionData.h"
#include "misc/ConfigGroups.h"
#include "misc/Debug.h"
#include "gui/dialogs/ShortcutWarnDialog.h"

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

namespace Rosegarden
{

ShortcutDialog::ShortcutDialog(QWidget *parent) :
    QDialog(parent),
    m_editRow(-1),
    m_selectionChanged(false)
{
    setModal(true);
    setWindowTitle(tr("Shortcuts"));
    
    ActionData* adata = ActionData::getInstance();
    m_model = adata->getModel();
    
    m_proxyModel = new QSortFilterProxyModel;
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1);
    
    m_proxyView = new QTreeView;
    m_proxyView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_proxyView->setRootIsDecorated(false);
    m_proxyView->setAlternatingRowColors(true);
    m_proxyView->setModel(m_proxyModel);
    m_proxyView->setSortingEnabled(true);
    
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
    setWindowTitle(tr("Shortcuts"));

    QLabel* helpLabel = new QLabel;
    helpLabel->setWordWrap(true);
    helpLabel->setText(tr("To edit a shortcut select the action in the table below. To change a shortcut click on the shortcut and press the new key sequence. To reomve a shortcut press and release ctrl alt or shift. Press \"set Shortcuts\" to make the change. Changes will take effect affter restarting Rosegarden"));
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
    m_clabel = new QLabel;
    m_alabel = new QLabel;
    QFont font = m_clabel->font();
    font.setPointSize(20);
    font.setBold(true);
    m_clabel->setFont(font);
    m_alabel->setFont(font);
    m_ilabel = new QLabel;
    m_ilabel->setStyleSheet("QLabel { background-color : white }");
    hlayout->addWidget(m_clabel);
    hlayout->addWidget(m_alabel);
    hlayout->addWidget(m_ilabel);
    
    QHBoxLayout *hlayout2 = new QHBoxLayout;
    // up to 4  shortcuts
    m_ksEditList.push_back(new QKeySequenceEdit);
    m_ksEditList.push_back(new QKeySequenceEdit);
    m_ksEditList.push_back(new QKeySequenceEdit);
    m_ksEditList.push_back(new QKeySequenceEdit);
    foreach(QKeySequenceEdit* ksEdit, m_ksEditList) {
        hlayout2->addWidget(ksEdit);
        connect(ksEdit, SIGNAL(editingFinished()),
                this, SLOT(keySequenceEdited()));
        ksEdit->setEnabled(false);
    }
    
    QHBoxLayout *hlayout3 = new QHBoxLayout;
    m_setPB = new QPushButton(tr("Set Shortcuts"));
    connect(m_setPB, SIGNAL(clicked()),
            this, SLOT(setPBClicked()));
    m_setPB->setEnabled(false);
    m_defPB = new QPushButton(tr("Reset to Defaults"));
    connect(m_defPB, SIGNAL(clicked()),
            this, SLOT(defPBClicked()));
    m_defPB->setEnabled(false);

    m_warnLabel = new QLabel(tr("Warnings when:"));
    m_warnSetting = new QComboBox;
    m_warnSetting->addItem(tr("Never"));
    m_warnSetting->addItem(tr("Duplicate in same context"));
    m_warnSetting->addItem(tr("Duplicate in any context"));
    connect(m_warnSetting, SIGNAL(currentIndexChanged(int)),
            this, SLOT(warnSettingChanged(int)));
    settings.beginGroup(GeneralOptionsConfigGroup);
    settings.value("ShortcutWarnings");
    m_warnSetting->setCurrentIndex
        (settings.value("shortcut_warnings", 1).toInt());
    settings.endGroup();
    
    hlayout3->addStretch();
    hlayout3->addWidget(m_setPB);
    hlayout3->addStretch();
    hlayout3->addWidget(m_defPB);
    hlayout3->addStretch();
    hlayout3->addWidget(m_warnLabel);
    hlayout3->addWidget(m_warnSetting);
    hlayout3->addStretch();
    
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    
    mainLayout->addLayout(hlayout);
    mainLayout->addLayout(hlayout2);
    mainLayout->addLayout(hlayout3);
    mainLayout->addWidget(line);
    mainLayout->addLayout(proxyLayout);
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
}

void ShortcutDialog::filterChanged()
{
    m_proxyModel->setFilterFixedString(m_filterPatternLineEdit->text());
}

void ShortcutDialog::selectionChanged(const QItemSelection& selected,
                                      const QItemSelection&)
{
    RG_DEBUG << "selection changed" << selected;
    m_selectionChanged = true;
    QModelIndexList indexes = selected.indexes();
    if (indexes.empty()) {
        foreach(QKeySequenceEdit* ksEdit, m_ksEditList) {
            ksEdit->setEnabled(false);
        }
        m_setPB->setEnabled(false);
        m_defPB->setEnabled(false);
        m_editRow = -1;
        m_editKey = "";
        editRow(); // to reset the edit data
        return;
    }
    
    QModelIndex index = indexes.first();
    int row = index.row();
    int column = index.column();
    RG_DEBUG << "row" << row << column << "selected";
    m_editRow = row;
    editRow();
}

void ShortcutDialog::keySequenceEdited()
{
    RG_DEBUG << "key sequence edited row:" << m_editRow <<
        "key: " << m_editKey;
    // limit to just one shortcut
    foreach(QKeySequenceEdit* ksEdit, m_ksEditList) {
        QKeySequence ks = ksEdit->keySequence();
        if (! ks.isEmpty()) {
            QKeySequence singleKey(ks[0]);
            ksEdit->setKeySequence(singleKey);
        }
    }
    m_setPB->setEnabled(true);
}

void ShortcutDialog::setPBClicked()
{
    RG_DEBUG << "set shortcut";
    ActionData* adata = ActionData::getInstance();
    std::set<QKeySequence> ksSet;
    foreach(QKeySequenceEdit* ksEdit, m_ksEditList) {
        QKeySequence ks = ksEdit->keySequence();
        if (! ks.isEmpty()) {
            ksSet.insert(ks);
        }
    }
    if (m_warnType != None) {
        QString context = "";
        if (m_warnType == SameContext) {
            QStringList klist = m_editKey.split(":");
            context = klist[0];
        }
        ActionData::DuplicateData duplicates;
        adata->getDuplicateShortcuts(m_editKey, ksSet, false,
                                     context, duplicates);
        if (! duplicates.empty()) {
            // ask the user
            ShortcutWarnDialog warnDialog(duplicates);
            if (warnDialog.exec() == QDialog::Accepted) {
                RG_DEBUG << "defPBClicked warnDialog accepted";
            } else {
                RG_DEBUG << "defPBClicked warnDialog rejected";
                // do nothing
                return;
            }
        }
    }
    m_setPB->setEnabled(false);
    m_selectionChanged = false;
    adata->setUserShortcuts(m_editKey, ksSet);

    // If the selection has not changed - refresh edit data
    if (! m_selectionChanged) editRow();
}

void ShortcutDialog::defPBClicked()
{
    RG_DEBUG << "set shortcut to default";
    ActionData* adata = ActionData::getInstance();
    std::set<QKeySequence> ksSet;
    if (m_warnType != None) {
        QString context = "";
        if (m_warnType == SameContext) {
            QStringList klist = m_editKey.split(":");
            context = klist[0];
        }
        ActionData::DuplicateData duplicates;
        adata->getDuplicateShortcuts(m_editKey, ksSet, true,
                                     context, duplicates);
        if (! duplicates.empty()) {
            // ask the user
            ShortcutWarnDialog warnDialog(duplicates);
            if (warnDialog.exec() == QDialog::Accepted) {
                RG_DEBUG << "defPBClicked warnDialog accepted";
            } else {
                RG_DEBUG << "defPBClicked warnDialog rejected";
                // do nothing
                return;
            }
        }
    }
    m_defPB->setEnabled(false);
    m_selectionChanged = false;
    adata->removeUserShortcuts(m_editKey);

    // If the selection has not changed - refresh edit data
    if (! m_selectionChanged) editRow();
}

void ShortcutDialog::warnSettingChanged(int index)
{
    RG_DEBUG << "warnSettingChanged" << index;
    m_warnType = static_cast<WarningType>(index);
    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    //settings.value("shortcut_warnings");
    settings.setValue("shortcut_warnings", m_warnSetting->currentIndex());
    settings.endGroup();
}
            
void ShortcutDialog::editRow()
{
    RG_DEBUG << "editRow:" << m_editRow;
    if (m_editRow == -1) {
        m_clabel->setText("");
        m_alabel->setText("");
        QPixmap noPixmap;
        m_ilabel->setPixmap(noPixmap);
        return;
    }
    QModelIndex rindex = m_proxyModel->index(m_editRow, 0);
    QModelIndex srcIndex = m_proxyModel->mapToSource(rindex);
    RG_DEBUG << "src row" << srcIndex.row();
    if (srcIndex.row() == -1) {
        m_clabel->setText("");
        m_alabel->setText("");
        QPixmap noPixmap;
        m_ilabel->setPixmap(noPixmap);
        return;
    }
    ActionData* adata = ActionData::getInstance();
    m_editKey = adata->getKey(srcIndex.row());
    RG_DEBUG << "editing key" << m_editKey;
    QModelIndex i0 = m_proxyModel->index(m_editRow, 0);
    QString ctext = m_proxyModel->data(i0, Qt::DisplayRole).toString();
    m_clabel->setText(ctext);
    QModelIndex i1 = m_proxyModel->index(m_editRow, 1);
    QString atext = m_proxyModel->data(i1, Qt::DisplayRole).toString();
    m_alabel->setText(atext);
    QModelIndex i2 = m_proxyModel->index(m_editRow, 2);
    QVariant imagev = m_proxyModel->data(i2, Qt::DecorationRole);
    QIcon icon = imagev.value<QIcon>();
    if (! icon.isNull()) {
        QPixmap pixmap =
            icon.pixmap(icon.availableSizes().first());
        int w = m_ilabel->width();
        int h = m_ilabel->height();
        m_ilabel->setPixmap(pixmap.scaled(w, h, Qt::KeepAspectRatio));
    } else {
        QPixmap noPixmap;
        m_ilabel->setPixmap(noPixmap);
    }
    
    std::set<QKeySequence> ksSet = adata->getShortcuts(m_editKey);
    auto ksiter = ksSet.begin();
    foreach(QKeySequenceEdit* ksEdit, m_ksEditList) {
        ksEdit->setEnabled(true);
        QKeySequence ks;
        if (ksiter != ksSet.end()) {
            ks = (*ksiter);
            ksiter++;
        }
        RG_DEBUG << "set keysequence" << ks;
        ksEdit->setKeySequence(ks);
    }
    RG_DEBUG << "editRow is default:" << adata->isDefault(m_editKey, ksSet);
    if (adata->isDefault(m_editKey, ksSet)) {
        m_defPB->setEnabled(false);
    } else {
        m_defPB->setEnabled(true);
    }
}

}
