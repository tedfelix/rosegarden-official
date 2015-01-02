/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

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
#include "gui/general/ResourceFinder.h"
#include "gui/general/IconLoader.h"
#include "misc/Strings.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QGroupBox>

//#include <QDir>

namespace Rosegarden
{

GuitarChordSelectorDialog::GuitarChordSelectorDialog(QWidget *parent)
    : QDialog(parent)
{
    QString localStyle = "QListView {background-color: #FFFFFF; alternate-background-color: #EEEEFF; color: #000000; selection-background-color: #80AFFF; selection-color: #FFFFFF;}";
    // we'll use "localStyle" as a future search point, but switch over to a
    // more meaningful variable name for the actual style assignment
    //
    // Note that I'm just slapping another local stylesheet on these damn
    // QListView objects, because they're stubbornly refusing to be touched from
    // the external stylesheet, and I'm beyond losing patience with dicking
    // around to solve problems like this.  Our stylesheet and style code are a
    // complete mess, and I will probably have to rewrite all of this one day,
    // but not before Thorn/Abraham Darby releases.
    QString listStyle = localStyle;

    setModal(true);
    setWindowTitle(tr("Guitar Chord Selector"));
    setWindowIcon(IconLoader().loadPixmap("window-guitar"));

    QGridLayout *metagrid = new QGridLayout;
    setLayout(metagrid);
    QGroupBox *page = new QGroupBox(this);
    QGridLayout *topLayout = new QGridLayout(page);
    metagrid->addWidget(page, 0, 0);
    
    topLayout->addWidget(new QLabel(tr("Root"), page), 0, 0);
    m_rootNotesList = new QListWidget(page);
    m_rootNotesList->setStyleSheet(listStyle);
    topLayout->addWidget(m_rootNotesList, 1, 0);
    
    topLayout->addWidget(new QLabel(tr("Extension"), page), 0, 1);
    m_chordExtList = new QListWidget(page);
    m_chordExtList->setStyleSheet(listStyle);
    topLayout->addWidget(m_chordExtList, 1, 1);
    
    m_newFingeringButton = new QPushButton(tr("New"), page);
    m_deleteFingeringButton = new QPushButton(tr("Delete"), page);
    m_editFingeringButton = new QPushButton(tr("Edit"), page);
    
    m_chordComplexityCombo = new QComboBox(page);
    m_chordComplexityCombo->addItem(tr("beginner"));
    m_chordComplexityCombo->addItem(tr("common"));
    m_chordComplexityCombo->addItem(tr("all"));
    //m_chordComplexityCombo->setMinimumContentsLength(20);
    
    connect(m_chordComplexityCombo, SIGNAL(activated(int)),
            this, SLOT(slotComplexityChanged(int)));

    page->setContentsMargins(5, 5, 5, 5);
    QVBoxLayout* vboxLayout = new QVBoxLayout();
    //topLayout->addLayout(vboxLayout, 1, 2, 3, 1);
    topLayout->addLayout(vboxLayout, 2, 1);
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
    
    topLayout->addWidget(new QLabel(tr("Fingerings"), page), 0, 3);
    m_fingeringsList = new QListWidget(page);
    m_fingeringsList->setStyleSheet(listStyle);

    // try setting size to something 200 can be divided into evenly, in the hope
    // of avoiding fuzzy half pixel scaling problems (50 was no good, but 100
    // works well for grid lines here; dots still look awful, but who cares)
    m_fingeringsList->setIconSize(QSize(100, 100));

    topLayout->addWidget(m_fingeringsList, 1, 3, 2, 1);
    
    m_fingeringBox = new FingeringBox(false, page, true);
    topLayout->addWidget(m_fingeringBox, 2, 0, 1, 2);
    
    connect(m_rootNotesList, SIGNAL(currentRowChanged(int)),
            this, SLOT(slotRootHighlighted(int)));
    connect(m_chordExtList, SIGNAL(currentRowChanged(int)),
            this, SLOT(slotChordExtHighlighted(int)));

    // connect itemClicked() so it will fire if a user clicks directly on the
    // fingering list doodad thingummy (and comments like "fingering list doodad
    // thingummy" are what you get when you abandon half-finished code the core
    // developers don't really understand, and expect them to keep it alive for
    // you in perpetuity)
    //
    connect(m_fingeringsList, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(slotFingeringHighlighted(QListWidgetItem*)));

    // connect currentRowChanged() so this can be fired when other widgets are
    // manipulated, which will cause this one to pop back up to the top.  This
    // slot triggers other updates in a leg bone connected to the thigh bone
    // fashion, so we have to wire it to some input to get those updates to
    // happen, and overloading this to fire two different ways was quick and
    // cheap
    //
    connect(m_fingeringsList, SIGNAL(currentRowChanged(int)),
            this, SLOT(slotFingeringHighlighted(int)));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void
GuitarChordSelectorDialog::init()
{
    // populate the listboxes
    //
    QString chordFile = getChordFile();
    
    parseChordFile(chordFile);

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
    
    m_rootNotesList->sortItems(Qt::AscendingOrder );
    
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
// RG_DEBUG << "GuitarChordSelectorDialog::slotRootHighlighted " << i;

    if (i < 0) return;

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
// RG_DEBUG << "GuitarChordSelectorDialog::slotChordExtHighlighted " << i;

    if (i < 0) return;

    Guitar::ChordMap::chordarray chords = m_chordMap.getChords(m_chord.getRoot(), m_chordExtList->item(i)->text() );
    populateFingerings(chords);
    
    //m_fingeringsList->setCurrentIndex(0);
    m_fingeringsList->setCurrentRow(0);
            
}

void
GuitarChordSelectorDialog::slotFingeringHighlighted(int i)
{
// RG_DEBUG << "GuitarChordSelectorDialog::slotFingeringHighlighted(int)";

    QListWidgetItem* it = m_fingeringsList->item(i);
    if (it) slotFingeringHighlighted(it);
}

void
GuitarChordSelectorDialog::slotFingeringHighlighted(QListWidgetItem* listBoxItem)
{
// RG_DEBUG << "GuitarChordSelectorDialog::slotFingeringHighlighted("
//   "QListWidgetItem*)";
    
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
    //QListWidgetItem *tmpItem = 0; (unused)
    QList<QListWidgetItem*> tmpItemList;
    
    if (chordEditorDialog->exec() == QDialog::Accepted) {
        m_chordMap.insert(newChord);
        // populate lists
        //
        tmpItemList = m_rootNotesList->findItems(newChord.getRoot(), Qt::MatchExactly);
        if (tmpItemList.isEmpty() ) {
            m_rootNotesList->addItem(newChord.getRoot());
            m_rootNotesList->sortItems(Qt::AscendingOrder );
        }
        
        tmpItemList = m_rootNotesList->findItems(newChord.getExt(), Qt::MatchExactly);
        if (tmpItemList.isEmpty() ) {
            m_chordExtList->addItem(newChord.getExt());
            m_chordExtList->sortItems(Qt::AscendingOrder );
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

// RG_DEBUG << "GuitarChordSelectorDialog::slotEditFingering() - "
//   "current map state :";
// m_chordMap.debugDump();

        m_chordMap.substitute(m_chord, newChord);

// RG_DEBUG << "GuitarChordSelectorDialog::slotEditFingering() - "
//   "new map state :";
// m_chordMap.debugDump();

        setChord(newChord);
    }
    
    delete chordEditorDialog;

    refresh();    
}

void
GuitarChordSelectorDialog::accept()
{
    if (m_chordMap.needSave()) {
        saveUserChordMap();
        m_chordMap.clearNeedSave();
    }
    
    QDialog::accept();
}

void
GuitarChordSelectorDialog::setChord(const Guitar::Chord& chord)
{
// RG_DEBUG << "GuitarChordSelectorDialog::setChord() " /*<< chord*/;
    
    m_chord = chord;

    // select the chord's root
    //
    m_rootNotesList->setCurrentRow(0);
    //QListWidgetItem* correspondingRoot = m_rootNotesList->findItem(chord.getRoot(), Qt::ExactMatch);
    QList<QListWidgetItem*> correspondingRoot = m_rootNotesList->findItems(chord.getRoot(), Qt::MatchExactly);
    if (! correspondingRoot.isEmpty() )
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
        if (! correspondingExt.isEmpty() )
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
        QString fingeringString = strtoqstr(chord.getFingering().toString() );

// RG_DEBUG << "GuitarChordSelectorDialog::populateFingerings " << chord;

        QIcon fingeringPixmap = getFingeringPixmap(chord.getFingering());
        
        FingeringListBoxItem *item = new FingeringListBoxItem(chord, m_fingeringsList, fingeringPixmap, fingeringString);

// RG_DEBUG << "GuitarChordSelectorDialog::populateFingerings(): " <<
//   fingeringString;

        if (refFingering == chord.getFingering()) {

// RG_DEBUG << "GuitarChordSelectorDialog::populateFingerings - "
//   "fingering found " << fingeringString;

            //m_fingeringsList->setSelected(item, true);
            m_fingeringsList->setCurrentItem(item);
        }
    }

}


QPixmap
GuitarChordSelectorDialog::getFingeringPixmap(const Guitar::Fingering& fingering) const
{
// RG_DEBUG << "GuitarChordSelectorDialog::getFingeringPixmap()";

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

// RG_DEBUG << "GuitarChordSelectorDialog::populateExtensions - "
//   "adding '" << *i << "'";

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
GuitarChordSelectorDialog::parseChordFile(const QString& chordFileName)
{
    ChordXmlHandler handler(m_chordMap);
    QFile chordFile(chordFileName);
    bool ok = chordFile.open(QIODevice::ReadOnly);    
    if (!ok)
        QMessageBox::critical(0, tr("Rosegarden"), tr("couldn't open file '%1'").arg(handler.errorString()));

    QXmlInputSource source(&chordFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);

// RG_DEBUG << "GuitarChordSelectorDialog::parseChordFile() parsing " << 
//   chordFileName;

    reader.parse(source);

// RG_DEBUG << "  parsed OK, without crashing!  W00t!";

    if (!ok)
        QMessageBox::critical(0, tr("Rosegarden"), tr("couldn't parse chord dictionary : %1").arg(handler.errorString()));
    
}

void
GuitarChordSelectorDialog::setEditionEnabled(bool enabled)
{
    m_deleteFingeringButton->setEnabled(enabled);
    m_editFingeringButton->setEnabled(enabled);
}

QString
GuitarChordSelectorDialog::getChordFile()
{
    QString name = "";

    // unbundle the factory chords.xml to ~/.local; user edits will be saved to
    // this file, rather than a separate file as was previously the case in
    // Rosegarden Classic
    if (!ResourceFinder().unbundleResource("chords", "chords.xml")) return name;

    name = ResourceFinder().getResourcePath("chords", "chords.xml");

// RG_DEBUG << "GuitarChordSelectorDialog::getChordFile : adding file \" " << 
//   name << "\"";
// RG_DEBUG << "  (if file on the preceding line was \"\" then this is a BUG)";

    return name;
}

bool
GuitarChordSelectorDialog::saveUserChordMap()
{
    // Read config for user directory
    
    ResourceFinder rf;
    QString userChordDictPath = rf.getResourceSaveDir("chords");
    userChordDictPath += "/chords.xml";
    
// RG_DEBUG << "GuitarChordSelectorDialog::saveUserChordMap() : "
//   "saving user chord map to " << userChordDictPath;
    
    QString errMsg;

    // we're just saving back to chords.xml now, so we'll set this to be false,
    // and we'll declare a named variable so it's easier for the next guy to
    // figure out WTF the middle parameter in saveDocument() is for.  ARGH!
    bool userChordsOnly = false;

    m_chordMap.saveDocument(userChordDictPath, userChordsOnly, errMsg);
    
    return errMsg.isEmpty();
}


}


#include "GuitarChordSelectorDialog.moc"
