/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2020 the Rosegarden development team.
 
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

#include <svnversion.h>  // Generated file.  BUILDKEY
#include "rosegarden-version.h"  // for VERSION and CODENAME

namespace Rosegarden
{


AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent, nullptr)
{
    setWindowTitle(tr("About Rosegarden"));
    setModal(true);

    // Layout
    QGridLayout *layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setVerticalSpacing(20);

    // Icon
    QLabel *image = new QLabel;
    image->setAlignment(Qt::AlignTop);
    image->setPixmap(IconLoader::loadPixmap("welcome-icon"));

    layout->addWidget(image, 0, 0);

    // Header Text
    QLabel *label = new QLabel;
    label->setText(tr("<h2>Rosegarden</h2><h3>A sequencer and musical notation editor</h3>"));
    label->setWordWrap(false);

    layout->addWidget(label, 0, 1);

    // Body Text
    QLabel *label2 = new QLabel;
    label2->setText(tr("<p>Copyright 2000-2020 the Rosegarden development team</p><p>Version: %1 &nbsp; \"%4\"<br>Build key: %3<br>Qt version: %2</p><p>Rosegarden was brought to you by a team of volunteers across the world.  For a list of contributors, visit<br><a style=\"color:gold\" href=\"http://www.rosegardenmusic.com/resources/authors\">http://www.rosegardenmusic.com/resources/authors</a></p><p>For more information about Rosegarden, visit<br><a style=\"color:gold\" href=\"http://www.rosegardenmusic.com\">http://www.rosegardenmusic.com</a></p><p>License: GNU General Public License Version 2 or later</p>").arg(VERSION).arg(QT_VERSION_STR).arg(BUILDKEY).arg(CODENAME));
    label2->setWordWrap(true);
    label2->setAlignment(Qt::AlignHCenter);
    label2->setOpenExternalLinks(true);

    layout->addWidget(label2, 1, 0, 1, 2);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted,
                     this, &QDialog::accept);

    layout->addWidget(buttonBox, 2, 0, 1, 2);

    // Display the dialog.
    exec();
}


}
