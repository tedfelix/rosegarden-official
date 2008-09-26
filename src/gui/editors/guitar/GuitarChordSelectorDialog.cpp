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

#include "GuitarChordSelectorDialog.h"
#include "GuitarChordEditorDialog.h"
#include "ChordXmlHandler.h"
#include "FingeringBox.h"
#include "FingeringListBoxItem.h"
#include "misc/Debug.h"
#include "gui/general/IconLoader.h"
#include "misc/Strings.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

#include <klocale.h>
//#include <kstandarddirs.h>

namespace Rosegarden
{

GuitarChordSelectorDialog::GuitarChordSelectorDialog(QWidget *parent)
    : QDialog(parent)
{
    setModal(true);
    setWindowTitle(i18n("Guitar Chord Selector"));

#warning Dialog needs QDialogButtonBox(Ok|QDialogButtonBox::Cancel)

    QWidget *page = new QWidget(this);
	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addWidget(page);
//    setMainWidget(page);
    
	QGridLayout *topLayout = new QGridLayout(page, 3, 4); //, spacingHint());
    
    topLayout->addWidget(new QLabel(i18n("Root"), page), 0, 0);
    m_rootNotesList = new QListWidget(page);
    topLayout->addWidget(m_rootNotesList, 1, 0);
    
    topLayout->addWidget(new QLabel(i18n("Extension"), page), 0, 1);
    m_chordExtList = new QListWidget(page);
    topLayout->addWidget(m_chordExtList, 1, 1);
    
    m_newFingeringButton = new QPushButton(i18n("New"), page);
    m_deleteFingeringButton = new QPushButton(i18n("Delete"), page);
    m_editFingeringButton = new QPushButton(i18n("Edit"), page);
    
    m_chordComplexityCombo = new QComboBox(page);
    m_chordComplexityCombo->addItem(i18n("beginner"));
    m_chordComplexityCombo->addItem(i18n("common"));
    m_chordComplexityCombo->addItem(i18n("all"));
    
    connect(m_chordComplexityCombo, SIGNAL(activated(int)),
            this, SLOT(slotComplexityChanged(int)));
    
    QVBoxLayout* vboxLayout = new QVBoxLayout(page, 5);
    topLayout->addMultiCellLayout(vboxLayout, 1, 3, 2, 2);
    vboxLayout->addWidget(m_chordComplexityCombo);    
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
    m_fingeringsList = new QListWidget(page);
    topLayout->addWidget(m_fingeringsList, 1, 3, 2, 1);
    
    m_fingeringBox = new FingeringBox(false, page);
    topLayout->addWidget(m_fingeringBox, 2, 0, 0+1, 1- 1);
    
    connect(m_rootNotesList, SIGNAL(highlighted(int)),
            this, SLOT(slotRootHighlighted(int)));
    connect(m_chordExtList, SIGNAL(highlighted(int)),
            this, SLOT(slotChordExtHighlighted(int)));
    connect(m_fingeringsList, SIGNAL(highlighted(QListWidgetItem*)),
            this, SLOT(slotFingeringHighlighted(QListWidgetItem*)));
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
        m_rootNotesList->addItems(rootList);

        QStringList extList = m_chordMap.getExtList(rootList.first());
        populateExtensions(extList);
        
        Guitar::ChordMap::chordarray chords = m_chordMap.getChords(rootList.first(), extList.first());
        populateFingerings(chords);

        m_chord.setRoot(rootList.first());
        m_chord.setExt(extList.first());
    }
    
	m_rootNotesList->sortItems( Qt::AscendingOrder );
    
//    m_rootNotesList->setCurrentIndex(0);
    m_rootNotesList->setCurrentRow(0);
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

    m_chord.setRoot(m_rootNotesList->item(i)->text() );

    QStringList extList = m_chordMap.getExtList(m_chord.getRoot());
    populateExtensions(extList);
    if (m_chordExtList->count() > 0)
        //m_chordExtList->setCurrentIndex(0);
		m_chordExtList->setCurrentRow(0);
    else
        m_fingeringsList->clear(); // clear any previous fingerings    
}

void
GuitarChordSelectorDialog::slotChordExtHighlighted(int i)
{
    NOTATION_DEBUG << "GuitarChordSelectorDialog::slotChordExtHighlighted " << i << endl;

    Guitar::ChordMap::chordarray chords = m_chordMap.getChords( m_chord.getRoot(), m_chordExtList->item(i)->text() );
    populateFingerings(chords);
    
	//m_fingeringsList->setCurrentIndex(0);
	m_fingeringsList->setCurrentRow(0);
	        
}

void
GuitarChordSelectorDialog::slotFingeringHighlighted(QListWidgetItem* listBoxItem)
{
    NOTATION_DEBUG << "GuitarChordSelectorDialog::slotFingeringHighlighted\n";
    
    FingeringListBoxItem* fingeringItem = dynamic_cast<FingeringListBoxItem*>(listBoxItem);
    if (fingeringItem) {
        m_chord = fingeringItem->getChord();
        m_fingeringBox->setFingering(m_chord.getFingering());
        setEditionEnabled(m_chord.isUserChord());
    }
}

void
GuitarChordSelectorDialog::slotComplexityChanged(int)
{
    // simply repopulate the extension list box
    // 
    QStringList extList = m_chordMap.getExtList(m_chord.getRoot());
    populateExtensions(extList);
    if (m_chordExtList->count() > 0)
        //m_chordExtList->setCurrentIndex(0);
		m_chordExtList->setCurrentRow(0);
    else
        m_fingeringsList->clear(); // clear any previous fingerings    
}

void
GuitarChordSelectorDialog::slotNewFingering()
{
    Guitar::Chord newChord;
    newChord.setRoot(m_chord.getRoot());
    newChord.setExt(m_chord.getExt());
    
    GuitarChordEditorDialog* chordEditorDialog = new GuitarChordEditorDialog(newChord, m_chordMap, this);
    QListWidgetItem *tmpItem = 0;
	QList<QListWidgetItem*> tmpItemList;
	
    if (chordEditorDialog->exec() == QDialog::Accepted) {
        m_chordMap.insert(newChord);
        // populate lists
        //
		tmpItemList = m_rootNotesList->findItems( newChord.getRoot(), Qt::MatchExactly);
        if ( tmpItemList.isEmpty() ) {
            m_rootNotesList->addItem(newChord.getRoot());
			m_rootNotesList->sortItems( Qt::AscendingOrder );
        }
        
		tmpItemList = m_rootNotesList->findItems( newChord.getExt(), Qt::MatchExactly);
		if ( tmpItemList.isEmpty() ) {
            m_chordExtList->addItem(newChord.getExt());
			m_chordExtList->sortItems( Qt::AscendingOrder );
		}
    }    

    delete chordEditorDialog;
    
    refresh();
}

void
GuitarChordSelectorDialog::slotDeleteFingering()
{
    if (m_chord.isUserChord()) {
        m_chordMap.remove(m_chord);
		//delete m_fingeringsList->selectedItem();
		delete m_fingeringsList->currentItem();
	}
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
    
    //KDialogBase::slotOk(); 
	accept();
}

void
GuitarChordSelectorDialog::setChord(const Guitar::Chord& chord)
{
    NOTATION_DEBUG << "GuitarChordSelectorDialog::setChord " << chord << endl;
    
    m_chord = chord;

    // select the chord's root
    //
    m_rootNotesList->setCurrentRow(0);
	//QListWidgetItem* correspondingRoot = m_rootNotesList->findItem(chord.getRoot(), Qt::ExactMatch);
	QList<QListWidgetItem*> correspondingRoot = m_rootNotesList->findItems(chord.getRoot(), Qt::MatchExactly);
	if ( ! correspondingRoot.isEmpty() )
		//m_rootNotesList->setSelected(correspondingRoot[0], true);
		m_rootNotesList->setCurrentItem(correspondingRoot[0]);
    
    // update the dialog's complexity setting if needed, then populate the extension list
    //
    QString chordExt = chord.getExt();
    int complexityLevel = m_chordComplexityCombo->currentIndex();
    int chordComplexity = evaluateChordComplexity(chordExt);
    
    if (chordComplexity > complexityLevel) {
        m_chordComplexityCombo->setCurrentIndex(chordComplexity);
    }

    QStringList extList = m_chordMap.getExtList(chord.getRoot());
    populateExtensions(extList);
    
    // select the chord's extension
    //
    if (chordExt.isEmpty()) {
        chordExt = "";
		//m_chordExtList->setSelected(0, true);
		m_chordExtList->setCurrentItem(0);
	} else {                
		//QListWidgetItem* correspondingExt = m_chordExtList->findItem(chordExt, Qt::ExactMatch);
		QList<QListWidgetItem*> correspondingExt = m_chordExtList->findItems(chordExt, Qt::MatchExactly);
		if ( ! correspondingExt.isEmpty() )
			m_chordExtList->setCurrentItem(correspondingExt[0]);
			//m_chordExtList->setSelected(correspondingExt, true);
	}
    
    // populate fingerings and pass the current chord's fingering so it is selected
    //
    Guitar::ChordMap::chordarray similarChords = m_chordMap.getChords(chord.getRoot(), chord.getExt());
    populateFingerings(similarChords, chord.getFingering());
}

void
GuitarChordSelectorDialog::populateFingerings(const Guitar::ChordMap::chordarray& chords, const Guitar::Fingering& refFingering)
{
    m_fingeringsList->clear();
    
    for(Guitar::ChordMap::chordarray::const_iterator i = chords.begin(); i != chords.end(); ++i) {
        const Guitar::Chord& chord = *i; 
        QString fingeringString = strtoqstr( chord.getFingering().toString() );
        NOTATION_DEBUG << "GuitarChordSelectorDialog::populateFingerings " << chord << endl;
        
		QIcon fingeringPixmap = getFingeringPixmap(chord.getFingering());
		
        FingeringListBoxItem *item = new FingeringListBoxItem(chord, m_fingeringsList, fingeringPixmap, fingeringString);
        if (refFingering == chord.getFingering()) {
            NOTATION_DEBUG << "GuitarChordSelectorDialog::populateFingerings - fingering found " << fingeringString << endl;
			//m_fingeringsList->setSelected(item, true);
			m_fingeringsList->setCurrentItem(item);
		}
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

    if (m_chordComplexityCombo->currentIndex() != COMPLEXITY_ALL) {
        // some filtering needs to be done
        int complexityLevel = m_chordComplexityCombo->currentIndex();
        
        QStringList filteredList;
        for(QStringList::const_iterator i = extList.constBegin(); i != extList.constEnd(); ++i) {
            if (evaluateChordComplexity((*i).toLower().trimmed()) <= complexityLevel) {
                NOTATION_DEBUG << "GuitarChordSelectorDialog::populateExtensions - adding '" << *i << "'\n";
                filteredList.append(*i); 
            }
        }
        
        m_chordExtList->addItems(filteredList);
        
    } else {
        m_chordExtList->addItems(extList);
    }
}

int
GuitarChordSelectorDialog::evaluateChordComplexity(const QString& ext)
{
    if (ext.isEmpty() ||
        ext == "7" ||
        ext == "m" ||
        ext == "5")
        return COMPLEXITY_BEGINNER;
    
    if (ext == "dim" ||
        ext == "dim7" ||
        ext == "aug" ||
        ext == "sus2" ||
        ext == "sus4" ||
        ext == "maj7" ||
        ext == "m7" ||
        ext == "mmaj7" ||
        ext == "m7b5" ||
        ext == "7sus4")
        
        return COMPLEXITY_COMMON;
        
     return COMPLEXITY_ALL; 
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
    bool ok = chordFile.open(QIODevice::ReadOnly);    
    if (!ok)
        QMessageBox::critical(0, "", i18n("couldn't open file '%1'", handler.errorString()));

    QXmlInputSource source(chordFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    NOTATION_DEBUG << "GuitarChordSelectorDialog::parseChordFile() parsing " << chordFileName << endl;
    reader.parse(source);
    if (!ok)
        QMessageBox::critical(0, "", i18n("couldn't parse chord dictionary : %1", handler.errorString()));
    
}

void
GuitarChordSelectorDialog::setEditionEnabled(bool enabled)
{
    m_deleteFingeringButton->setEnabled(enabled);
    m_editFingeringButton->setEnabled(enabled);
}

std::vector<QString>
GuitarChordSelectorDialog::getAvailableChordFiles()
{
    std::vector<QString> names;

    // Read config for default directory
//    QStringList chordDictFiles = KGlobal::dirs()->findAllResources("appdata", "chords/*.xml");
	
	QString dicFile;
	IconLoader il;
	QStringList chordDictFiles = il.getResourceFiles( "chords", "xml" );
	
// 	for(QStringList::iterator i = chordDictFiles.begin(); i != chordDictFiles.end(); ++i) {
	for( int i=0; i< chordDictFiles.count(); i++ ){
		dicFile = chordDictFiles.at(i);
		
		NOTATION_DEBUG << "GuitarChordSelectorDialog::getAvailableChordFiles : adding file " << dicFile << endl;
		names.push_back(dicFile);
    }
    
    return names;
}

bool
GuitarChordSelectorDialog::saveUserChordMap()
{
    // Read config for user directory
//     QString userDir = KGlobal::dirs()->saveLocation("appdata", "chords/");
//     QString userChordDictPath = userDir + "/user_chords.xml";
    
	IconLoader il;
    
	QString userChordDictPath = il.getResourcePath("chords/user_chords.xml");
	
	
    NOTATION_DEBUG << "GuitarChordSelectorDialog::saveUserChordMap() : saving user chord map to " << userChordDictPath << endl;
    QString errMsg;
    
    m_chordMap.saveDocument(userChordDictPath, true, errMsg);
    
    return errMsg.isEmpty();    
}


}


#include "GuitarChordSelectorDialog.moc"
