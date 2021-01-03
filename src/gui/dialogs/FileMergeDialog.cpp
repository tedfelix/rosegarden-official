/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
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
    setModal(true);
    setWindowTitle(tr("Merge File"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);


    QWidget *hbox = new QWidget;
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hbox->setLayout(hboxLayout);
    layout->addWidget(hbox);
    hboxLayout->addWidget(new QLabel(tr("Merge new file  ")));

    m_choice = new QComboBox;
    hboxLayout->addWidget(m_choice);
    hbox->setLayout(hboxLayout);
    m_choice->addItem(tr("At start of existing composition"));
    m_choice->addItem(tr("From end of existing composition"));
    m_useTimings = nullptr;

    if (timingsDiffer) {
        layout->addWidget(new QLabel(tr("The file has different time signatures or tempos.")));
        m_useTimings = new QCheckBox(tr("Import these as well"));
        layout->addWidget(m_useTimings);
        m_useTimings->setChecked(false);
    }
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested, this, &FileMergeDialog::slotHelpRequested);
    layout->addWidget(buttonBox);
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
