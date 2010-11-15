/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "InsertTupletDialog.h"
#include <QLayout>

#include "base/NotationTypes.h"
#include "gui/widgets/LineEdit.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QObject>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <sstream>

namespace Rosegarden
{

InsertTupletDialog::InsertTupletDialog(QWidget *parent, unsigned int untupledCount,
        unsigned int tupledCount) :
        QDialog(parent)
{
    std::ostringstream untupledCountStr, tupledCountStr;
    untupledCountStr << untupledCount;
    tupledCountStr << tupledCount;

    //setHelp("nv-tuplets");
    setModal(true);
    setWindowTitle(tr("Tuplet"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vbox->setLayout(vboxLayout);
    metagrid->addWidget(vbox, 0, 0);


    QGroupBox *timingBox = new QGroupBox( tr("New timing for tuplet group"), vbox );
    timingBox->setContentsMargins(5, 5, 5, 5);
    QGridLayout *timingLayout = new QGridLayout;
    timingBox->setLayout(timingLayout);
    timingLayout->setSpacing(5);
    vboxLayout->addWidget(timingBox);


    timingLayout->addWidget(new QLabel(tr("Play "), timingBox), 0, 0);
    m_untupledLine = new LineEdit(QString((untupledCountStr.str()).c_str()),QString("99"),timingBox);
    timingLayout->addWidget(m_untupledLine, 0, 1);

    timingLayout->addWidget(new QLabel(tr("in the time of  "), timingBox), 1, 0);
    m_tupledLine = new LineEdit(QString((tupledCountStr.str()).c_str()),QString("99"),timingBox);
    timingLayout->addWidget(m_tupledLine, 1, 1);



    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(slotHelpRequested()));
}



unsigned int
InsertTupletDialog::getUntupledCount() const
{
    bool isNumeric = true;
    int count = m_untupledLine->text().toInt(&isNumeric);
    if (count == 0 || !isNumeric)
        return 1;
    else
        return count;
}

unsigned int
InsertTupletDialog::getTupledCount() const
{
    bool isNumeric = true;
    int count = m_tupledLine->text().toInt(&isNumeric);
    if (count == 0 || !isNumeric)
        return 1;
    else
        return count;
}



}
#include "InsertTupletDialog.moc"
