/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#include "GuitarChordSelectorDialog.h"
#include "GuitarChordEditorDialog.h"
#include "ChordXmlHandler.h"
#include "FingeringBox.h"
#include "FingeringListBoxItem.h"

#include "misc/Debug.h"
#include <qlistbox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstddirs.h>

namespace Rosegarden
{

GuitarChordSelectorDialog::GuitarChordSelectorDialog(QWidget *parent)
    : KDialogBase(parent, "GuitarChordSelector", true, i18n("Guitar Chord Selector"), Ok|Cancel)
{
    QWidget *page = new QWidget(this);
    setMainWidget(page);
    QGridLayout *topLayout = new QGridLayout(page, 3, 4, spacingHint());
    
    topLayout->addWidget(new QLabel(i18n("Root"), page), 0, 0);
    m_rootNotesList = new QListBox(page);
    topLayout->addWidget(m_rootNotesList, 1, 0);
    
    topLayout->addWidget(new QLabel(i18n("Extension"), page), 0, 1);
    m_chordExtList = new QListBox(page);
    topLayout->addWidget(m_chordExtList, 1, 1);
    
    m_newFingeringButton = new QPushButton(i18n("New"), page);
    m_deleteFingeringButton = new QPushButton(i18n("Delete"), page);
    m_editFingeringButton = new QPushButton(i18n("Edit"), page);
    
    QVBoxLayout* vboxLayout = new QVBoxLayout(page, 5);
    topLayout->addLayout(vboxLayout, 2, 2);
    vboxLayout->addStretch(10);
    vboxLayout->addWidget(m_newFingeringButton); 
    vboxLayout->addWidget(m_deleteFingeringButton); 
    vboxLayout->addWidget(m_editFingeringButton); 
    
    connect(m_newFingeringButton, SIGNAL(clicked()),
            this, SLOT(slotNewFingering()));
    connect(m_deleteFingeringButton, SIGNAL(clicked()),
            this, SLOT(slotDeleteFingering()));
    connect(m_editFingeringButton, SIGNAL(clicked()),
            this, SLOT(slotEditFingering()));
    
    topLayout->addWidget(new QLabel(i18n("Fingerings"), page), 0, 3);
    m_fingeringsList = new QListBox(page);
    topLayout->addMultiCellWidget(m_fingeringsList, 1, 2, 3, 3);
    
    m_fingeringBox = new FingeringBox(false, page);
    topLayout->addMultiCellWidget(m_fingeringBox, 2, 2, 0, 1);
    
    connect(m_rootNotesList, SIGNAL(highlighted(int)),
            this, SLOT(slotRootHighlighted(int)));
    connect(m_chordExtList, SIGNAL(highlighted(int)),
            this, SLOT(slotChordExtHighlighted(int)));
    connect(m_fingeringsList, SIGNAL(highlighted(QListBoxItem*)),
            this, SLOT(slotFingeringHighlighted(QListBoxItem*)));
}

void
GuitarChordSelectorDialog::init()
{
    // populate the listboxes
    //
    std::vector<QString> chordFiles = getAvailableChordFiles();
    
    parseChordFiles(chordFiles);

//    m_chordMap.debugDump();
    
    populate();
}

void
GuitarChordSelectorDialog::populate()
{    
    QStringList rootList = m_chordMap.getRootList();
    if (rootList.count() > 0) {
        m_rootNotesList->insertStringList(rootList);

        QStringList extList = m_chordMap.getExtList(rootList.first());
        populateExtensions(extList);
        
        Guitar::ChordMap::chordarray chords = m_chordMap.getChords(rootList.first(), extList.first());
        populateFingerings(chords);

        m_chord.setRoot(rootList.first());
        m_chord.setExt(extList.first());
    }
    
    m_rootNotesList->sort();
    
    m_rootNotesList->setCurrentItem(0);
    m_chordExtList->setCurrentItem(0);
    m_fingeringsList->setCurrentItem(0);
}

void
GuitarChordSelectorDialog::clear()
{
    m_rootNotesList->clear();
    m_chordExtList->clear();
    m_fingeringsList->clear();    
}

void
GuitarChordSelectorDialog::refresh()
{
    clear();
    populate();
}

void
GuitarChordSelectorDialog::slotRootHighlighted(int i)
{
    NOTATION_DEBUG << "GuitarChordSelectorDialog::slotRootHighlighted " << i << endl;

    m_chord.setRoot(m_rootNotesList->text(i));

    QStringList extList = m_chordMap.getExtList(m_chord.getRoot());
    populateExtensions(extList);    
}

void
GuitarChordSelectorDialog::slotChordExtHighlighted(int i)
{
    NOTATION_DEBUG << "GuitarChordSelectorDialog::slotChordExtHighlighted " << i << endl;

    Guitar::ChordMap::chordarray chords = m_chordMap.getChords(m_chord.getRoot(), m_chordExtList->text(i));
    populateFingerings(chords);
    
    m_fingeringsList->setCurrentItem(0);        
}

void
GuitarChordSelectorDialog::slotFingeringHighlighted(QListBoxItem* listBoxItem)
{
    NOTATION_DEBUG << "GuitarChordSelectorDialog::slotFingeringHighlighted\n";
    
    FingeringListBoxItem* fingeringItem = dynamic_cast<FingeringListBoxItem*>(listBoxItem);
    if (fingeringItem) {
        m_chord = fingeringItem->getChord();
        m_fingeringBox->setFingering(m_chord.getFingering());
    }
}

void
GuitarChordSelectorDialog::slotNewFingering()
{
    Guitar::Chord newChord;
    newChord.setRoot(m_chord.getRoot());
    newChord.setExt(m_chord.getExt());
    
    GuitarChordEditorDialog* chordEditorDialog = new GuitarChordEditorDialog(newChord, m_chordMap, this);
    
    if (chordEditorDialog->exec() == QDialog::Accepted) {
        m_chordMap.insert(newChord);
        // populate lists
        //
        if (!m_rootNotesList->findItem(newChord.getRoot(), Qt::ExactMatch)) {
            m_rootNotesList->insertItem(newChord.getRoot());
            m_rootNotesList->sort();
        }
        
        if (!m_chordExtList->findItem(newChord.getExt(), Qt::ExactMatch)) {
            m_chordExtList->insertItem(newChord.getExt());
            m_chordExtList->sort();
        }
    }    

    delete chordEditorDialog;
    
    refresh();
}

void
GuitarChordSelectorDialog::slotDeleteFingering()
{
    m_chordMap.remove(m_chord);
    delete m_fingeringsList->selectedItem();
}

void
GuitarChordSelectorDialog::slotEditFingering()
{
    Guitar::Chord newChord = m_chord;
    GuitarChordEditorDialog* chordEditorDialog = new GuitarChordEditorDialog(newChord, m_chordMap, this);
    
    if (chordEditorDialog->exec() == QDialog::Accepted) {
        NOTATION_DEBUG << "GuitarChordSelectorDialog::slotEditFingering() - current map state :\n";
        m_chordMap.debugDump();
        m_chordMap.substitute(m_chord, newChord);
        NOTATION_DEBUG << "GuitarChordSelectorDialog::slotEditFingering() - new map state :\n";
        m_chordMap.debugDump();
        setChord(newChord);
    }
    
    delete chordEditorDialog;

    refresh();    
}

void
GuitarChordSelectorDialog::slotOk()
{
    if (m_chordMap.needSave()) {
        saveUserChordMap();
        m_chordMap.clearNeedSave();
    }
    
    KDialogBase::slotOk();
}

void
GuitarChordSelectorDialog::setChord(const Guitar::Chord& chord)
{
    m_chord = chord;
    
    m_chordExtList->clear();
    QStringList extList = m_chordMap.getExtList(chord.getRoot());
    m_chordExtList->insertStringList(extList);
        
    Guitar::ChordMap::chordarray similarChords = m_chordMap.getChords(chord.getRoot(), extList.first());
    populateFingerings(similarChords);
}

void
GuitarChordSelectorDialog::populateFingerings(const Guitar::ChordMap::chordarray& chords)
{
    m_fingeringsList->clear();
    
    for(Guitar::ChordMap::chordarray::const_iterator i = chords.begin(); i != chords.end(); ++i) {
        const Guitar::Chord& chord = *i; 
        QString fingeringString = chord.getFingering().toString();
        NOTATION_DEBUG << "GuitarChordSelectorDialog::populateFingerings " << chord << " - fingering : " << fingeringString << endl;
        QPixmap fingeringPixmap = getFingeringPixmap(chord.getFingering());            
        new FingeringListBoxItem(chord, m_fingeringsList, fingeringPixmap, fingeringString);
    }

}


QPixmap
GuitarChordSelectorDialog::getFingeringPixmap(const Guitar::Fingering& fingering) const
{
    QPixmap pixmap(FINGERING_PIXMAP_WIDTH, FINGERING_PIXMAP_HEIGHT);
    pixmap.fill();
    
    QPainter pp(&pixmap);    
    QPainter *p = &pp;
    
    p->setViewport(FINGERING_PIXMAP_H_MARGIN, FINGERING_PIXMAP_W_MARGIN,
                   FINGERING_PIXMAP_WIDTH  - FINGERING_PIXMAP_W_MARGIN,
                   FINGERING_PIXMAP_HEIGHT - FINGERING_PIXMAP_H_MARGIN);

    Guitar::NoteSymbols::drawFingeringPixmap(fingering, m_fingeringBox->getNoteSymbols(), p);
    
    return pixmap;
}

void
GuitarChordSelectorDialog::populateExtensions(const QStringList& extList)
{
    m_chordExtList->clear();
    m_chordExtList->insertStringList(extList);
    m_chordExtList->sort();
} 

void
GuitarChordSelectorDialog::parseChordFiles(const std::vector<QString>& chordFiles)
{
    for(std::vector<QString>::const_iterator i = chordFiles.begin(); i != chordFiles.end(); ++i) {
        parseChordFile(*i);
    }
}

void
GuitarChordSelectorDialog::parseChordFile(const QString& chordFileName)
{
    ChordXmlHandler handler(m_chordMap);
    QFile chordFile(chordFileName);
    bool ok = chordFile.open(IO_ReadOnly);    
    if (!ok)
        KMessageBox::error(0, i18n("couldn't open file '%1'").arg(handler.errorString()));

    QXmlInputSource source(chordFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    NOTATION_DEBUG << "GuitarChordSelectorDialog::parseChordFile() parsing " << chordFileName << endl;
    reader.parse(source);
    if (!ok)
        KMessageBox::error(0, i18n("couldn't parse chord dictionnary : %1").arg(handler.errorString()));
    
}

std::vector<QString>
GuitarChordSelectorDialog::getAvailableChordFiles()
{
    std::vector<QString> names;

    // Read config for default directory
    QStringList chordDictFiles = KGlobal::dirs()->findAllResources("appdata", "chords/*.xml");

    for(QStringList::iterator i = chordDictFiles.begin(); i != chordDictFiles.end(); ++i) {
        NOTATION_DEBUG << "GuitarChordSelectorDialog::getAvailableChordFiles : adding file " << *i << endl;
        names.push_back(*i);
    }
    
    return names;
}

bool
GuitarChordSelectorDialog::saveUserChordMap()
{
    // Read config for user directory
    QString userDir = KGlobal::dirs()->saveLocation("appdata", "chords/");

    QString userChordDictPath = userDir + "/chords.xml";
    
    NOTATION_DEBUG << "GuitarChordSelectorDialog::saveUserChordMap() : saving user chord map to " << userChordDictPath << endl;
    QString errMsg;
    
    m_chordMap.saveDocument(userChordDictPath, errMsg);
    
    return errMsg.isEmpty();    
}


}


#include "GuitarChordSelectorDialog.moc"
