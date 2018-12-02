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


#include "SplitByPitchDialog.h"

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

SplitByPitchDialog::SplitByPitchDialog(QWidget *parent) :
        QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Split by Pitch"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *vBox = new QWidget(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    metagrid->addWidget(vBox, 0, 0);


    QFrame *frame = new QFrame( vBox );
    vBoxLayout->addWidget(frame);
    vBox->setLayout(vBoxLayout);

    frame->setContentsMargins(10, 10, 10, 10);
    QGridLayout *layout = new QGridLayout(frame);
    layout->setSpacing(5);

    m_pitch = new PitchChooser(tr("Starting split pitch"), frame, 60);
    layout->addWidget(m_pitch, 0, 0, 0- 0+1, 2-0+ 1, Qt::AlignHCenter);

    m_strategy = new QComboBox(frame);
    m_strategy->addItem(tr("Always split at this pitch"));
    m_strategy->addItem(tr("Range up and down to follow music"));
    m_strategy->addItem(tr("Split the lowest tone from each chord"));
    m_strategy->addItem(tr("Split the highest tone from each chord"));
    m_strategy->addItem(tr("Split all chords at the same relative tone"));
    layout->addWidget(m_strategy,
                               1, 0,  // fromRow, fromCol
                               1, 3   // rowSpan, colSpan
                              );

    m_duplicate = new QCheckBox(tr("Duplicate non-note events"), frame);
    layout->addWidget(m_duplicate, 2, 0, 0+1, 2- 0+1);

    layout->addWidget(new QLabel(tr("Clef handling:"), frame), 3, 0);

    m_clefs = new QComboBox(frame);
    m_clefs->addItem(tr("Leave clefs alone"));
    m_clefs->addItem(tr("Guess new clefs"));
    m_clefs->addItem(tr("Use treble and bass clefs"));
    layout->addWidget(m_clefs, 3, 1, 1, 2);

    m_strategy->setCurrentIndex(2);
    m_duplicate->setChecked(true);
    m_clefs->setCurrentIndex(2);
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

int
SplitByPitchDialog::getPitch()
{
    return m_pitch->getPitch();
}

int
SplitByPitchDialog::getStrategy()
{
    switch (m_strategy->currentIndex()) {
    case 0:
    default:
        return (int)SegmentSplitByPitchCommand::ConstantPitch;
    case 1:
        return (int)SegmentSplitByPitchCommand::Ranging;
    case 2:
        return (int)SegmentSplitByPitchCommand::LowestTone;
    case 3:
        return (int)SegmentSplitByPitchCommand::HighestTone;
    case 4:
        return (int)SegmentSplitByPitchCommand::ChordToneOfInitialPitch;
    }
    /* NOTREACHED */ 
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
