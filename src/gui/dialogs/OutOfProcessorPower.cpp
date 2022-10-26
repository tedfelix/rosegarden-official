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

#define RG_MODULE_STRING "[OutOfProcessorPower]"

#include "OutOfProcessorPower.h"

#include "misc/Debug.h"
#include "misc/Preferences.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>


namespace Rosegarden
{


OutOfProcessorPower::OutOfProcessorPower(QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(tr("Out Of Processor Power"));
    setModal(true);
    setContentsMargins(10, 10, 10, 10);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(10);

    // Label
    layout->addWidget(
            new QLabel(tr("Out of processor power for real-time audio processing.  Cannot continue.")));

    // Don't show this again.
    m_dontShow = new QCheckBox(tr("Don't display this dialog again.  (Restart required.)"));
    m_dontShow->setChecked(!Preferences::getJACKLoadCheck());
    layout->addWidget(m_dontShow);

    // Button Box
    QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    layout->addWidget(buttonBox);
}

void OutOfProcessorPower::accept()
{
    Preferences::setJACKLoadCheck(!m_dontShow->isChecked());

    QDialog::accept();
}


}
