/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
 
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
    QDialog(parent),
    m_differentSigsOrTempos(nullptr),
    m_mergeSigsAndTemposLabel(nullptr),
    m_mergeSigsAndTempos(nullptr)
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
    connect(m_mergeLocation,
                static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &FileMergeDialog::slotModified);

    layout->addWidget(m_mergeLocation, row, 1);

    ++row;

    if (timingsDiffer) {
        // Import different time signatures or tempos.
        m_differentSigsOrTempos =
                new QLabel(tr("The file has different time signatures or tempos."));
        layout->addWidget(m_differentSigsOrTempos, row, 0, 1, 2);
        ++row;

        m_mergeSigsAndTemposLabel = new QLabel(tr("Import these as well"));
        layout->addWidget(m_mergeSigsAndTemposLabel, row, 0);
        m_mergeSigsAndTempos = new QCheckBox;
        m_mergeSigsAndTempos->setChecked(false);
        layout->addWidget(m_mergeSigsAndTempos, row, 1);

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
    // If we're merging at the end, it only makes sense to include the
    // sigs and tempos.
    if (getMergeAtEnd())
        return true;

    // We're merging over top.  We need the user's input on whether
    // we should merge the sigs and tempos over top of each other.

    return (m_mergeSigsAndTempos  &&
            m_mergeSigsAndTempos->isChecked());
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

void
FileMergeDialog::slotModified()
{
    // If the sigs and tempos differ
    if (m_differentSigsOrTempos) {
        if (getMergeAtEnd()) {
            // Hide these because we will always merge the sigs and tempos
            // when merging at the end.
            m_differentSigsOrTempos->hide();
            m_mergeSigsAndTemposLabel->hide();
            m_mergeSigsAndTempos->hide();
        } else {
            m_differentSigsOrTempos->show();
            m_mergeSigsAndTemposLabel->show();
            m_mergeSigsAndTempos->show();
        }
    }
}


}
