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


#include "SplitByPitchDialog.h"

#include <klocale.h>
#include "commands/segment/SegmentSplitByPitchCommand.h"
#include "gui/general/ClefIndex.h"
#include "gui/widgets/PitchChooser.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QWidget>
#include <QVBoxLayout>
#include <QLayout>


namespace Rosegarden
{

SplitByPitchDialog::SplitByPitchDialog(QDialogButtonBox::QWidget *parent) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(i18n("Split by Pitch"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vBox = new QWidget(this);
    QVBoxLayout vBoxLayout = new QVBoxLayout;
    metagrid->addWidget(vBox, 0, 0);


    QFrame *frame = new QFrame( vBox );
    vBoxLayout->addWidget(frame);
    vBox->setLayout(vBoxLayout);

    QGridLayout *layout = new QGridLayout(frame, 4, 3, 10, 5);

    m_pitch = new PitchChooser(i18n("Starting split pitch"), frame, 60);
    layout->addWidget(m_pitch, 0, 0, 0- 0+1, 2- 1, Qt::AlignHCenter);

    m_range = new QCheckBox(i18n("Range up and down to follow music"), frame);
    layout->addMultiCellWidget(m_range,
                               1, 1,  // fromRow, toRow
                               0, 2  // fromCol, toCol
                              );

    m_duplicate = new QCheckBox(i18n("Duplicate non-note events"), frame);
    layout->addWidget(m_duplicate, 2, 0, 0+1, 2- 1);

    layout->addWidget(new QLabel(i18n("Clef handling:"), frame), 3, 0);

    m_clefs = new QComboBox(frame);
    m_clefs->addItem(i18n("Leave clefs alone"));
    m_clefs->addItem(i18n("Guess new clefs"));
    m_clefs->addItem(i18n("Use treble and bass clefs"));
    layout->addWidget(m_clefs, 3, 1, 1, 2);

    m_range->setChecked(true);
    m_duplicate->setChecked(true);
    m_clefs->setCurrentIndex(2);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

int
SplitByPitchDialog::getPitch()
{
    return m_pitch->getPitch();
}

bool
SplitByPitchDialog::getShouldRange()
{
    return m_range->isChecked();
}

bool
SplitByPitchDialog::getShouldDuplicateNonNoteEvents()
{
    return m_duplicate->isChecked();
}

int
SplitByPitchDialog::getClefHandling()
{
    switch (m_clefs->currentIndex()) {
    case 0:
        return (int)SegmentSplitByPitchCommand::LeaveClefs;
    case 1:
        return (int)SegmentSplitByPitchCommand::RecalculateClefs;
    default:
        return (int)SegmentSplitByPitchCommand::UseTrebleAndBassClefs;
    }
}

}
#include "SplitByPitchDialog.moc"
