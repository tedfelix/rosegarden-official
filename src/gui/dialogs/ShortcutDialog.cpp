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

namespace Rosegarden
{

ShortcutDialog::ShortcutDialog(QWidget *parent) :
    QDialog(parent),
    m_editRow(-1)
{
    setModal(true);
    setWindowTitle(tr("Shortcuts"));
    
    m_model = new QStandardItemModel(0, 4, this);
    
    ActionData* adata = ActionData::getInstance();
    adata->fillModel(m_model);
    
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
    
    m_proxyView->sortByColumn(0, Qt::AscendingOrder);

    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    this->restoreGeometry(settings.value("Shortcut_Dialog").toByteArray());
    QStringList columnWidths =
        settings.value("Shortcut_Table_Widths").toStringList();
    settings.endGroup();

    // set column widths (except for last one)
    for (int i = 0; i < columnWidths.size() - 1; i++) {
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
    m_defPB->setEnabled(false);

    hlayout3->addStretch();
    hlayout3->addWidget(m_setPB);
    hlayout3->addStretch();
    hlayout3->addWidget(m_defPB);
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
    QModelIndexList indexes = selected.indexes();
    if (indexes.empty()) {
        foreach(QKeySequenceEdit* ksEdit, m_ksEditList) {
            ksEdit->setEnabled(false);
        }
        m_setPB->setEnabled(false);
        m_defPB->setEnabled(false);
        m_editRow = -1;
        m_editKey = "";
        return;
    }
    
    QModelIndex index = indexes.first();
    int row = index.row();
    int column = index.column();
    RG_DEBUG << "row" << row << column << "selected";
    QModelIndex rindex = m_proxyModel->index(row, 0);
    m_editRow = row;
    QModelIndex srcIndex = m_proxyModel->mapToSource(rindex);
    RG_DEBUG << "src row" << srcIndex.row();
    ActionData* adata = ActionData::getInstance();
    m_editKey = adata->getKey(srcIndex.row());
    RG_DEBUG << "editing key" << m_editKey;
    QModelIndex i0 = m_proxyModel->index(row, 0);
    QString ctext = m_proxyModel->data(i0, Qt::DisplayRole).toString();
    m_clabel->setText(ctext);
    QModelIndex i1 = m_proxyModel->index(row, 1);
    QString atext = m_proxyModel->data(i1, Qt::DisplayRole).toString();
    m_alabel->setText(atext);
    QModelIndex i2 = m_proxyModel->index(row, 2);
    QVariant imagev = m_proxyModel->data(i2, Qt::DecorationRole);
    QIcon icon = imagev.value<QIcon>();
    if (! icon.isNull()) {
        QPixmap pixmap =
            icon.pixmap(icon.availableSizes().first());
        int w = m_ilabel->width();
        int h = m_ilabel->height();
        m_ilabel->setPixmap(pixmap.scaled(w, h, Qt::KeepAspectRatio));
    }
    
    QModelIndex i3 = m_proxyModel->index(row, 3);
    QString shortcuts =  m_proxyModel->data(i3, Qt::DisplayRole).toString();
    RG_DEBUG << "got shortcutlist" << shortcuts;
    QStringList shortcutList = shortcuts.split(", ");
    int kindex = 0;
    std::set<QKeySequence> ksSet;
    foreach(QKeySequenceEdit* ksEdit, m_ksEditList) {
        ksEdit->setEnabled(true);
        QString scString = "";
        if (kindex < shortcutList.size()) {
            scString = shortcutList.at(kindex);
        }
        RG_DEBUG << "set keysequence" << scString;
        QKeySequence ks(scString);
        ksSet.insert(ks);
        ksEdit->setKeySequence(ks);
        kindex++;
    }
    if (! adata->isDefault(m_editKey, ksSet)) {
        m_defPB->setEnabled(true);
    }
}

void ShortcutDialog::keySequenceEdited()
{
    RG_DEBUG << "key sequence edited row:" << m_editRow <<
        "key: " << m_editKey;
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
    adata->setUserShortcuts(m_editKey, ksSet);
}

}
