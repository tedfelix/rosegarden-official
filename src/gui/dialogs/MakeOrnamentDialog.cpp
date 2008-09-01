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

MakeOrnamentDialog::MakeOrnamentDialog(QDialogButtonBox::QWidget *parent, QString defaultName,
                                       int defaultBasePitch) :
        KDialogBase(parent, "makeornamentdialog", true, i18n("Make Ornament"),
                    Ok | Cancel, Ok)
{
    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *nameBox = new QGroupBox(2, Vertical, i18n("Name"), vbox);

    new QLabel(i18n("The name is used to identify both the ornament\nand the triggered segment that stores\nthe ornament's notes."), nameBox);

    QWidget *hbox = new QWidget(nameBox);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    QLabel *child_3 = new QLabel(i18n("Name:  "), hbox );
    hboxLayout->addWidget(child_3);
    m_name = new QLineEdit(defaultName, hbox );
    hboxLayout->addWidget(m_name);
    hbox->setLayout(hboxLayout);

    m_pitch = new PitchChooser(i18n("Base pitch"), vbox, defaultBasePitch);
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
