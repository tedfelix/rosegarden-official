/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

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

#include <klineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstddirs.h>
#include <qlayout.h>
#include <qlabel.h>

namespace Rosegarden
{

GuitarChordEditorDialog::GuitarChordEditorDialog(Guitar::Chord& chord, const Guitar::ChordMap& chordMap, QWidget *parent)
    : KDialogBase(parent, "GuitarChordEditor", true, i18n("Guitar Chord Editor"), Ok|Cancel),
      m_chord(chord),
      m_chordMap(chordMap)
{
    QWidget *page = new QWidget(this);
    setMainWidget(page);
    QGridLayout *topLayout = new QGridLayout(page, 7, 2, spacingHint());

    topLayout->addWidget(new QLabel(i18n("Start fret"), page), 0, 1);
    m_startFret = new QSpinBox(1, 24, 1, page);
    topLayout->addWidget(m_startFret, 1, 1);
    
    connect(m_startFret, SIGNAL(valueChanged(int)),
            this, SLOT(slotStartFretChanged(int)));
    
    topLayout->addWidget(new QLabel(i18n("Root"), page), 2, 1);
    m_rootNotesList = new QComboBox(page);
    topLayout->addWidget(m_rootNotesList, 3, 1);
    
    topLayout->addWidget(new QLabel(i18n("Extension"), page), 4, 1);
    m_ext = new QComboBox(true, page);
    topLayout->addWidget(m_ext, 5, 1);

    topLayout->addItem(new QSpacerItem(1, 1), 6, 1);

    m_fingeringBox = new FingeringBox(true, page);
    m_fingeringBox->setFingering(m_chord.getFingering());
    topLayout->addMultiCellWidget(m_fingeringBox, 0, 7, 0, 0);

    NOTATION_DEBUG << "GuitarChordEditorDialog : chord = " << m_chord << endl;


    QStringList rootList = m_chordMap.getRootList();
    if (rootList.count() > 0) {
        m_rootNotesList->insertStringList(rootList);
        m_rootNotesList->setCurrentItem(rootList.findIndex(m_chord.getRoot()));
    }
    
    QStringList extList = m_chordMap.getExtList(m_chord.getRoot());
    if (extList.count() > 0) {
        m_ext->insertStringList(extList);
        m_ext->setCurrentItem(extList.findIndex(m_chord.getExt()));
    }
    
}

void
GuitarChordEditorDialog::slotStartFretChanged(int startFret)
{
    m_fingeringBox->setStartFret(startFret);
}

void
GuitarChordEditorDialog::slotOk()
{
    m_chord.setFingering(m_fingeringBox->getFingering());
    m_chord.setExt(m_ext->currentText());
    m_chord.setRoot(m_rootNotesList->currentText());
    m_chord.setUserChord(true);            
    KDialogBase::slotOk();
}


}

#include "GuitarChordEditorDialog.moc"

