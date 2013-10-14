/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2013 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "SelectDialog.h"

// local includes
#include "misc/Debug.h"
#include "misc/ConfigGroups.h"

#include "gui/general/IconLoader.h"

#include "gui/widgets/CheckButton.h"

// Qt includes
#include <QDialog>
#include <QUrl>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolButton>


namespace Rosegarden
{


SelectDialog::SelectDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Search and Select"));
//    setStyleSheet("background: #FFFFFF");

    // master layout
    QVBoxLayout *layout = new QVBoxLayout();
    setLayout(layout);

    // button grid
    QGridLayout *grid = new QGridLayout();
    QWidget *box = new QWidget;
    box->setLayout(grid);
    layout->addWidget(box);

    // INSERT GENERATED CODE AFTER THIS LINE
 

  
    // INSERT GENERATED CODE BEFORE THIS LINE


    // primary buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    layout->addWidget(buttonBox);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(help()));

}

SelectDialog::~SelectDialog()
{
}

void
SelectDialog::help()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:manual-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:manual-search-and-select-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}


}

#include "SelectDialog.moc"
