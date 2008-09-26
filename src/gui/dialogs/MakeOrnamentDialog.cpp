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


#include "MakeOrnamentDialog.h"

#include <klocale.h>
#include "gui/widgets/PitchChooser.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>


namespace Rosegarden
{

MakeOrnamentDialog::MakeOrnamentDialog(QDialogButtonBox::QWidget *parent,
                                       QString defaultName,
                                       int defaultBasePitch) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(i18n("Make Ornament"));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);

    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    QGroupBox *nameBox = new QGroupBox(i18n("Name"));
    QVBoxLayout *nameBoxLayout = new QVBoxLayout;
    vboxLayout->addWidget(nameBox);

    nameBoxLayout->addWidget(
        new QLabel(i18n("The name is used to identify both the ornament\n"
                        "and the triggered segment that stores\n"
                        "the ornament's notes."), nameBox));

    QWidget *hbox = new QWidget;
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    nameBoxLayout->addWidget(hbox);
    nameBox->setLayout(nameBoxLayout);

    QLabel *child_3 = new QLabel(i18n("Name:  "));
    hboxLayout->addWidget(child_3);

    m_name = new QLineEdit(defaultName);
    hboxLayout->addWidget(m_name);
    hbox->setLayout(hboxLayout);

    m_pitch = new PitchChooser(i18n("Base pitch"), vbox, defaultBasePitch);
    vboxLayout->addWidget(m_pitch);
    vbox->setLayout(vboxLayout);

    QDialogButtonBox *buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

QString
MakeOrnamentDialog::getName() const
{
    return m_name->text();
}

int
MakeOrnamentDialog::getBasePitch() const
{
    return m_pitch->getPitch();
}

}
#include "MakeOrnamentDialog.moc"
