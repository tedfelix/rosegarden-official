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

#define RG_MODULE_STRING "[ShowSequencerStatusDialog]"
#define RG_NO_DEBUG_PRINT

#include "ShowSequencerStatusDialog.h"

#include "misc/Debug.h"
#include "sound/Audit.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QSettings>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>


namespace Rosegarden
{


ShowSequencerStatusDialog::ShowSequencerStatusDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Sequencer status"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);

    int row = 0;

    // Sequencer Status (Audit Log)
    QTextEdit *text = new QTextEdit;
    text->setReadOnly(true);

    text->setPlainText(strtoqstr(AUDIT.str()));

    layout->addWidget(text, row);
    layout->setStretch(row, 1);

    ++row;

    // Button Box

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    buttonBox->setContentsMargins(10,10,10,10);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(buttonBox, row);

    ++row;

    setMinimumSize(500, 200);

    // Restore window geometry and toolbar/dock state
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    restoreGeometry(settings.value("Sequencer_Status_Dialog_Geometry").toByteArray());

}

ShowSequencerStatusDialog::~ShowSequencerStatusDialog()
{
    // Save window geometry and header state.
    QSettings settings;
    settings.beginGroup(WindowGeometryConfigGroup);
    settings.setValue("Sequencer_Status_Dialog_Geometry", saveGeometry());
}


}
