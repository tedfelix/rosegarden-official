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

#include "GuitarChordEditorDialog.h"
#include "FingeringBox.h"
#include "Chord.h"
#include "ChordMap.h"

#include <QComboBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QDir>
#include <QLayout>
#include <QLabel>
#include <QDialogButtonBox>

namespace Rosegarden
{

GuitarChordEditorDialog::GuitarChordEditorDialog(Guitar::Chord& chord, const Guitar::ChordMap& chordMap, QWidget *parent)
    : QDialog(parent),
      m_chord(chord),
      m_chordMap(chordMap)
{
    setModal(true);
    setWindowTitle(tr("Guitar Chord Editor"));
    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QWidget *page = new QWidget(this);
    QGridLayout *topLayout = new QGridLayout(page);
    metagrid->addWidget(page, 0, 0);

    topLayout->addWidget(new QLabel(tr("Start fret"), page), 0, 1);
    m_startFret = new QSpinBox(page);
    m_startFret->setRange(1, 24);
    m_startFret->setSingleStep(1);
    topLayout->addWidget(m_startFret, 1, 1);
    
    connect(m_startFret, SIGNAL(valueChanged(int)),
            this, SLOT(slotStartFretChanged(int)));
    
    topLayout->addWidget(new QLabel(tr("Root"), page), 2, 1);
    m_rootNotesList = new QComboBox(page);
    topLayout->addWidget(m_rootNotesList, 3, 1);
    
    topLayout->addWidget(new QLabel(tr("Extension"), page), 4, 1);
    m_ext = new QComboBox(page);

    // This was editable on purpose, but to what end I could never fathom.
    // Creating new extensions on the fly does not seem to work.  I'm not sure
    // if it was supposed to work and didn't, or if it, like the barre symbol
    // whatnots, just never got done.  I'll just un-editable it and move along
    // then.
    //m_ext->setEditable(true);
    topLayout->addWidget(m_ext, 5, 1);

    topLayout->addItem(new QSpacerItem(1, 1), 6, 1);

    m_fingeringBox = new FingeringBox(true, page, true);
    m_fingeringBox->setFingering(m_chord.getFingering());
    topLayout->addWidget(m_fingeringBox, 0, 0, 7- 0+1, 0- 0+1);

    NOTATION_DEBUG << "GuitarChordEditorDialog : chord = " << m_chord;


    QStringList rootList = m_chordMap.getRootList();
    if (rootList.count() > 0) {
        m_rootNotesList->addItems(rootList);
        m_rootNotesList->setCurrentIndex(rootList.indexOf(m_chord.getRoot()));
    }
    
    QStringList extList = m_chordMap.getExtList(m_chord.getRoot());
    if (extList.count() > 0) {
        m_ext->addItems(extList);
        m_ext->setCurrentIndex(extList.indexOf(m_chord.getExt()));
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void
GuitarChordEditorDialog::slotStartFretChanged(int startFret)
{
    m_fingeringBox->setStartFret(startFret);
}

void
GuitarChordEditorDialog::accept()
{
    m_chord.setFingering(m_fingeringBox->getFingering());
    m_chord.setExt(m_ext->currentText());
    m_chord.setRoot(m_rootNotesList->currentText());
    m_chord.setUserChord(true);            
	
    QDialog::accept();
}


}


