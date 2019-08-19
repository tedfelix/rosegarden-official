/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "QuantizeDialog.h"

#include "base/Quantizer.h"
#include "gui/widgets/QuantizeParameters.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>

namespace Rosegarden
{

QuantizeDialog::QuantizeDialog(QWidget *parent, bool inNotation) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Quantize"));
    setContentsMargins(5, 5, 5, 5);

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    setLayout(vboxLayout);

    m_quantizeFrame = new QuantizeParameters(this , inNotation ?
            QuantizeParameters::Notation : QuantizeParameters::Grid,
            false);
    vboxLayout->addWidget(m_quantizeFrame);

    m_quantizeFrame->adjustSize();
    adjustSize();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    vboxLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

Quantizer *
QuantizeDialog::getQuantizer() const
{
    return m_quantizeFrame->getQuantizer();
}


}
