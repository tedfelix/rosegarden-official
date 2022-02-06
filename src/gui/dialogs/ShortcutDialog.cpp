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


#include "ShortcutDialog.h"

#include "gui/general/ActionData.h"
#include "misc/ConfigGroups.h"

#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QSettings>

namespace Rosegarden
{

ShortcutDialog::ShortcutDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Shortcuts"));

    m_model = new QStandardItemModel(0, 4, this);

    ActionData* adata = ActionData::getInstance();
    adata->fillModel(m_model);
    
    proxyModel = new QSortFilterProxyModel;
    proxyModel->setSourceModel(m_model);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(-1);
    
    proxyView = new QTreeView;
    proxyView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    proxyView->setRootIsDecorated(false);
    proxyView->setAlternatingRowColors(true);
    proxyView->setModel(proxyModel);
    proxyView->setSortingEnabled(true);

    filterPatternLineEdit = new QLineEdit;
    filterPatternLabel = new QLabel(tr("Filter pattern:"));
    
    connect(filterPatternLineEdit, SIGNAL(textChanged(const QString&)),
            this, SLOT(filterChanged()));
    
    QGridLayout *proxyLayout = new QGridLayout;
    proxyLayout->addWidget(filterPatternLabel, 0, 0);
    proxyLayout->addWidget(filterPatternLineEdit, 0, 1, 1, 3);
    proxyLayout->addWidget(proxyView, 1, 0, 1, 4);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    
    mainLayout->addLayout(proxyLayout);
    setLayout(mainLayout);
    setWindowTitle(tr("Shortcuts"));
    
    proxyView->sortByColumn(1, Qt::AscendingOrder);

    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    this->restoreGeometry(settings.value("Shortcut_Dialog").toByteArray());
    QStringList columnWidths =
        settings.value("Shortcut_Table_Widths").toStringList();
    settings.endGroup();

    // set column widths (except for last one)
    for (int i = 0; i < columnWidths.size() - 1; i++) {
        proxyView->setColumnWidth(i, columnWidths[i].toInt());
    }
    
}

ShortcutDialog::~ShortcutDialog()
{
    QStringList columnWidths;
    // save column widths (except for last one)
    for (int i = 0; i < m_model->columnCount() - 1; i++) {
        columnWidths << QString::number(proxyView->columnWidth(i));
    }
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    settings.setValue("Shortcut_Dialog", this->saveGeometry());
    settings.setValue("Shortcut_Table_Widths", columnWidths);
    settings.endGroup();
}

void ShortcutDialog::filterChanged()
{
  proxyModel->setFilterFixedString(filterPatternLineEdit->text());
}

}
