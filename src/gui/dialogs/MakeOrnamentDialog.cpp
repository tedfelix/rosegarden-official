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


#include "MakeOrnamentDialog.h"

#include "gui/widgets/PitchChooser.h"
#include "gui/widgets/LineEdit.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFontMetrics>


namespace Rosegarden
{

MakeOrnamentDialog::MakeOrnamentDialog(QWidget *parent,
                                       QString defaultName,
                                       int defaultBasePitch) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Make Ornament"));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);

    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);

    QGroupBox *nameBox = new QGroupBox(tr("Name"));
    QVBoxLayout *nameBoxLayout = new QVBoxLayout;
    vboxLayout->addWidget(nameBox);

    QLabel *lbl =  new QLabel(tr("<qt>The name is used to identify both the ornament "
                                 "and the triggered segment that stores "
                                 "the ornament's notes.</qt>"), nameBox);
    lbl->setWordWrap(true);                                 
    nameBoxLayout->addWidget(lbl);

    QWidget *hbox = new QWidget;
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    nameBoxLayout->addWidget(hbox);
    nameBox->setLayout(nameBoxLayout);

    QLabel *child_3 = new QLabel(tr("Name:  "));
    hboxLayout->addWidget(child_3);

    m_name = new LineEdit(defaultName);
    QFontMetrics metrics(m_name->font());
    int width30 = metrics.width("123456789012345678901234567890");
    m_name->setFixedWidth(width30);
    hboxLayout->addWidget(m_name);
    hbox->setLayout(hboxLayout);

    m_pitch = new PitchChooser(tr("Base pitch"), vbox, defaultBasePitch);
    vboxLayout->addWidget(m_pitch);
    vbox->setLayout(vboxLayout);

    QDialogButtonBox *buttonBox
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
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
