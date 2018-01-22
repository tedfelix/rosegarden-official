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


#include "AboutDialog.h"

#include "gui/general/IconLoader.h"
#include <QGridLayout>
#include <QLabel>
#include <QDialogButtonBox>

#include <svnversion.h> // generated file
#include "rosegarden-version.h"

namespace Rosegarden
{

AboutDialog::AboutDialog(QWidget *parent): QDialog(parent, 0)
{
    this->setModal(true);
    this->setWindowTitle(tr("About Rosegarden"));
    QGridLayout *metagrid = new QGridLayout;
    this->setLayout(metagrid);

    QWidget *hb = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout;
    QHBoxLayout *headerLayout = new QHBoxLayout;
    metagrid->addWidget(hb, 0, 0);
    QLabel *image = new QLabel;
    headerLayout->addWidget(image);
    image->setAlignment(Qt::AlignTop);

    image->setPixmap(IconLoader().loadPixmap("welcome-icon"));

    QLabel *label = new QLabel;
    headerLayout->addWidget(label);
    label->setText(tr("<h2>Rosegarden - \"%1\"</h2><h3>A sequencer and musical notation editor</h3>").arg(CODENAME));
    label->setWordWrap(false);

    mainLayout->addLayout(headerLayout);

    QLabel *label2 = new QLabel;
    label2->setText(tr("<p>Copyright 2000-2018 the Rosegarden development team</p><p>Version: %1 &nbsp; Qt version: %2<br>Build key: %3</p><p>Rosegarden was brought to you by a team of volunteers across the world.  For a list of contributors, visit <a style=\"color:gold\" href=\"http://www.rosegardenmusic.com/resources/authors\">http://www.rosegardenmusic.com/resources/authors</a>.<br>For more information about Rosegarden, visit <a style=\"color:gold\" href=\"http://www.rosegardenmusic.com\">http://www.rosegardenmusic.com</a>.</p><p>License: GNU General Public License Version 2 or later</p>").arg(VERSION).arg(QT_VERSION_STR).arg(BUILDKEY));
    label2->setWordWrap(true);
    label2->setAlignment(Qt::AlignHCenter);
    label2->setOpenExternalLinks(true);
    mainLayout->addWidget(label2);
    hb->setLayout(mainLayout);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    this->exec();
}
}
 

