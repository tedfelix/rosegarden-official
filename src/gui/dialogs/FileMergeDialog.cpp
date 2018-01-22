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


#include "FileMergeDialog.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QLabel>
#include <QString>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QUrl>
#include <QDesktopServices>
#include "document/RosegardenDocument.h"


namespace Rosegarden
{

FileMergeDialog::FileMergeDialog(QWidget *parent,
                                 QString /*fileName*/,
                                 bool timingsDiffer) :
        QDialog(parent)
{
    //###
    //&&&
    // This one will take some thought.  We're going to have to figure out some
    // new help system.  This //setHelp() call is from KDialog, which we are no
    // longer using.  Until we have the new help system sorted, I don't see
    // anything useful to do with this, so I'm disabling it for now.
    //
    ////setHelp("file-merge");

    setModal(true);
    setWindowTitle(tr("Merge File"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout;
    metagrid->addWidget(vbox, 0, 0);


    QWidget *hbox = new QWidget( vbox );
    vboxLayout->addWidget(hbox);
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    QLabel *child_3 = new QLabel(tr("Merge new file  "), hbox );
    hboxLayout->addWidget(child_3);

    m_choice = new QComboBox( hbox );
    hboxLayout->addWidget(m_choice);
    hbox->setLayout(hboxLayout);
    m_choice->addItem(tr("At start of existing composition"));
    m_choice->addItem(tr("From end of existing composition"));
    m_useTimings = 0;

    if (timingsDiffer) {
        new QLabel(tr("The file has different time signatures or tempos."), vbox);
        m_useTimings = new QCheckBox(tr("Import these as well"), vbox );
        vboxLayout->addWidget(m_useTimings);
        vbox->setLayout(vboxLayout);
        m_useTimings->setChecked(false);
    }
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(buttonBox, SIGNAL(helpRequested()), this, SLOT(slotHelpRequested()));
}

int
FileMergeDialog::getMergeOptions()
{
    int options = MERGE_KEEP_OLD_TIMINGS | MERGE_IN_NEW_TRACKS;

    if (m_choice->currentIndex() == 1) {
        options |= MERGE_AT_END;
    }

    if (m_useTimings && m_useTimings->isChecked()) {
        options |= MERGE_KEEP_NEW_TIMINGS;
    }

    return options;
}


void
FileMergeDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:fileMergeDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:fileMergeDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}
}
