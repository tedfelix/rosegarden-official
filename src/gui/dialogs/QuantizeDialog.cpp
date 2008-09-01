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


#include "QuantizeDialog.h"

#include <klocale.h>
#include "base/Quantizer.h"
#include "gui/widgets/QuantizeParameters.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QWidget>
#include <QVBoxLayout>


namespace Rosegarden
{

QuantizeDialog::QuantizeDialog(QDialogButtonBox::QWidget *parent, bool inNotation) :
        QDialog(parent)
{
    setHelp("quantization");

    setModal(true);
    setWindowTitle(i18n("Quantize"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    m_quantizeFrame = new QuantizeParameters( vbox , 0);
    vboxLayout->addWidget(m_quantizeFrame);
    vbox->setLayout(vboxLayout);

    setButtonText(Details, i18n("Advanced"));
    setDetailsWidget(m_quantizeFrame->getAdvancedWidget());
    m_quantizeFrame->getAdvancedWidget()->hide();

    m_quantizeFrame->adjustSize();
    vbox->adjustSize();
    adjustSize();
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Details | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

Quantizer *
QuantizeDialog::getQuantizer() const
{
    return m_quantizeFrame->getQuantizer();
}

}
#include "QuantizeDialog.moc"
