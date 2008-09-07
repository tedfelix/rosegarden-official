/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ExportDeviceDialog.h"

#include <klocale.h>
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

ExportDeviceDialog::ExportDeviceDialog(QDialogButtonBox::QWidget *parent, QString deviceName) :
        KDialogBase(parent, "exportdevicedialog", true, i18n("Export Devices..."),
                    Ok | Cancel, Ok)
{
    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *bg = new QGroupBox(1, Qt::Horizontal,
                                        i18n("Export devices"),
                                        vbox);
    m_exportAll = new QRadioButton(i18n("Export all devices"), bg);
    m_exportOne = new QRadioButton(i18n("Export selected device only"), bg);
    new QLabel(i18n("         (\"%1\")", deviceName), bg);

    m_exportOne->setChecked(true);
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
