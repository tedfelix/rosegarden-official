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
#include <QVBoxLayout>
#include <QHBoxLayout>
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

    // ??? Switch to grid layout.
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QWidget *hbox = new QWidget;
    QHBoxLayout *hboxLayout = new QHBoxLayout;
    hbox->setLayout(hboxLayout);
    layout->addWidget(hbox);

    // Merge new file at
    hboxLayout->addWidget(new QLabel(tr("Merge new file  ")));
    m_mergeLocation = new QComboBox;
    hboxLayout->addWidget(m_mergeLocation);
    hbox->setLayout(hboxLayout);
    m_mergeLocation->addItem(tr("At start of existing composition"));
    m_mergeLocation->addItem(tr("From end of existing composition"));

    // Import different time signatures or tempos.
    m_importTimeSignaturesAndTempos = nullptr;

    if (timingsDiffer) {
        layout->addWidget(new QLabel(tr("The file has different time signatures or tempos.")));
        m_importTimeSignaturesAndTempos = new QCheckBox(tr("Import these as well"));
        layout->addWidget(m_importTimeSignaturesAndTempos);
        m_importTimeSignaturesAndTempos->setChecked(false);
    }

    // Button Box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Help);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttonBox, &QDialogButtonBox::helpRequested,
            this, &FileMergeDialog::slotHelpRequested);
    layout->addWidget(buttonBox);
}

int
FileMergeDialog::getMergeOptions()
{
    int options = 0;

    if (m_mergeLocation->currentIndex() == 1)
        options |= MERGE_AT_END;

    if (m_importTimeSignaturesAndTempos  &&
        m_importTimeSignaturesAndTempos->isChecked())
        options |= MERGE_KEEP_NEW_TIMINGS;

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
