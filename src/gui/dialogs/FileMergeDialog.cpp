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

#define RG_MODULE_STRING "[FileMergeDialog]"

#include "FileMergeDialog.h"

#include "misc/Debug.h"
#include "document/RosegardenDocument.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QUrl>
#include <QDesktopServices>


namespace Rosegarden
{


FileMergeDialog::FileMergeDialog(QWidget *parent,
                                 bool timingsDiffer) :
    QDialog(parent)
{
    setWindowTitle(tr("Merge File"));
    setModal(true);

    QGridLayout *layout = new QGridLayout;
    setLayout(layout);

    int row = 0;

    // Merge new file at
    layout->addWidget(new QLabel(tr("Merge new file")), row, 0);
    m_mergeLocation = new QComboBox;
    m_mergeLocation->addItem(tr("At start of existing composition"));
    m_mergeLocation->addItem(tr("From end of existing composition"));
    layout->addWidget(m_mergeLocation, row, 1);

    ++row;

    m_mergeTimesAndTempos = nullptr;

    if (timingsDiffer) {
        // Import different time signatures or tempos.
        layout->addWidget(
                new QLabel(tr("The file has different time signatures or tempos.")),
                row, 0, 1, 2);
        ++row;

        layout->addWidget(new QLabel(tr("Import these as well")), row, 0);
        m_mergeTimesAndTempos = new QCheckBox;
        m_mergeTimesAndTempos->setChecked(false);
        layout->addWidget(m_mergeTimesAndTempos, row, 1);

        ++row;
    }

    // Spacer
    layout->setRowMinimumHeight(row, 8);

    ++row;

    // Button Box
    QDialogButtonBox *buttonBox =
            new QDialogButtonBox(QDialogButtonBox::Ok |
                                 QDialogButtonBox::Cancel |
                                 QDialogButtonBox::Help);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested,
            this, &FileMergeDialog::slotHelpRequested);
    layout->addWidget(buttonBox, row, 0, 1, 2);
}

bool
FileMergeDialog::getMergeAtEnd()
{
    return (m_mergeLocation->currentIndex() == 1);
}

bool
FileMergeDialog::getMergeTimesAndTempos()
{
    return (m_mergeTimesAndTempos  &&
            m_mergeTimesAndTempos->isChecked());
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
