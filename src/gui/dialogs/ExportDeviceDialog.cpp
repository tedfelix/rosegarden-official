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


#include "ExportDeviceDialog.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

ExportDeviceDialog::ExportDeviceDialog(QWidget *parent, QString deviceName) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Export Devices..."));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);

    QGroupBox *bg = new QGroupBox("Export devices");
    QVBoxLayout *bgLayout = new QVBoxLayout;
    m_exportAll = new QRadioButton(tr("Export all devices"));
    bgLayout->addWidget(m_exportAll);
    m_exportOne = new QRadioButton(tr("Export selected device only"));
    bgLayout->addWidget(m_exportOne);
    bgLayout->addWidget(new QLabel(tr("         (\"%1\")").arg(deviceName)));
    bg->setLayout(bgLayout);

    m_exportOne->setChecked(true);

    metagrid->addWidget(bg, 0, 0);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                     | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

ExportDeviceDialog::ExportType

ExportDeviceDialog::getExportType()
{
    if (m_exportAll->isChecked())
        return ExportAll;
    else
        return ExportOne;
}

}
