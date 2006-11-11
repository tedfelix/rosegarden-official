// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
 
    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>
 
    The moral right of the authors to claim authorship of this work
    has been asserted.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "GuitarTabSelectorDialog.h"
#include <kapplication.h>
#include "GuitarChordEditor.h"
#include "FingeringConstructor.h"
#include "GuitarXmlHandler.h"
#include "base/Exception.h"
#include "DuplicateException.h"

#include <iostream>

#include <qdir.h>
#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qtextedit.h>

#include <kaction.h>
#include <kapp.h>
#include <kconfig.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstddirs.h>
#include <kpushbutton.h>

namespace Rosegarden
{

/*---------------------------------------------------------------
              GuitarTabSelectorDialog
  ---------------------------------------------------------------*/

GuitarTabSelectorDialog::GuitarTabSelectorDialog( QWidget *parent )
        : KDialogBase( parent,
                       "guitartabselectordialog",
                       true,
                       "Guitar Chord Selector",
                       0,
                       KDialogBase::NoDefault,
                       true ),
        m_chordMap ( new Guitar::ChordMap () ),
        m_guitar ( new Guitar::GuitarNeck() ),
        m_arrangement ( new Guitar::Fingering( m_guitar ) )
{}

GuitarTabSelectorDialog::~ GuitarTabSelectorDialog()
{}

void GuitarTabSelectorDialog::init ( void )
{
    populate();
    setupChordMap();
    setupNameList();
}

void GuitarTabSelectorDialog::setupChordMap ( void )
{
    std::vector<QString> chordFiles = this->getAvailableChordFiles();
    Guitar::GuitarXmlHandler handler;

    // For each chord file
    //   read chord files and populate Chord map
    for ( std::vector<QString>::iterator pos = chordFiles.begin();
            pos != chordFiles.end();
            ++pos ) {
        try {
            std::cout << "GuitarTabSelectorDialog::setupChordList - reading "
            << *pos << std::endl;
            Guitar::ChordMap const* map_ptr = handler.parse ( *pos );

            /*
                       std::cout << " Found " << map_ptr->size() << " chords" << std::endl;

                       std::cout << " Found map:" << std::endl << std::endl
                       << map_ptr->toString() << std::endl;
            */
            bool notDone = true;
            // Set beginning iterator to start of ChordMap
            Guitar::ChordMap::const_iterator pos = map_ptr->begin();
            while ( notDone ) {
                try {
                    m_chordMap->append( pos, map_ptr->end() );
                    // We had a successful append so we can go to the next file
                    notDone = false;
                } catch ( Guitar::DuplicateException & de ) {
                    // If a duplicate is found
                    // We report it to standard out
                    std::cout << "Duplicate chord found in " << *pos << std::endl
                    << std::endl << "Chord found is:" << std::endl
                    << ( de.getDuplicate() ) ->toString() << std::endl;

                    // Increment iterator to move past duplicate
                    ++pos;

                    // If iterator == map_ptr->end()
                    //   set notDone to FALSE
                    if ( pos == map_ptr->end() ) {
                        notDone = false;
                    }

                }
            }
        } catch ( Exception & re ) {
            std::cerr << "GuitarTabSelectorDialog::setupChordList - failure to parse "
            << ( *pos ) << std::endl
            << "  Skipping to next file" << std::endl;
        }
    }
}

std::vector<QString>
GuitarTabSelectorDialog::getAvailableChordFiles ( void )
{
    std::vector<QString> names;

    // Read config for default directory
    QString chordDir = KGlobal::dirs() ->findResource( "appdata", "default_chords/" );

    // Read config for user directory
    QString userDir = KGlobal::dirs() ->findResource ( "appdata", "user_chords/" );

    if ( !chordDir.isEmpty() ) {
        std::vector<QString> default_files = this->readDirectory ( chordDir );
        names.insert ( names.end(), default_files.begin(), default_files.end() );
    }

    if ( !userDir.isEmpty() ) {
        std::vector<QString> user_files = this->readDirectory ( userDir );
        names.insert ( names.end(), user_files.begin(), user_files.end() );
    }

    return names;
}

std::vector<QString>
GuitarTabSelectorDialog::readDirectory ( QString chordDir )
{
    std::vector<QString> names;

    QDir dir( chordDir );

    dir.setFilter( QDir::Files | QDir::Readable );
    QStringList files = dir.entryList();

    for ( QStringList::Iterator i = files.begin();
            i != files.end();
            ++i ) {
        if ( ( *i ).length() > 4 && ( *i ).right( 4 ) == ".xml" ) {
            QFileInfo fileInfo( QString( "%1/%2" ).arg( chordDir ).arg( *i ) );
            if ( fileInfo.exists() && fileInfo.isReadable() ) {
                names.push_back( QString( "%1%2" ).arg( chordDir ).arg( *i ) );
            }
        }
    }

    return names;
}

void GuitarTabSelectorDialog::setupNameList ( void )
{
    m_scaleList->clear();
    m_modifierList->clear();
    m_suffixList->clear();
    m_versionList->clear();
    m_aliases->clear();

    std::list<QString> nameList = m_chordMap->getNameList();

    for ( std::list<QString>::const_iterator pos = nameList.begin();
            pos != nameList.end();
            ++pos ) {
        m_scaleList->insertItem ( *pos );
    }
}

void GuitarTabSelectorDialog::slotDisplayChord ()
{
    Guitar::ChordName * chord_id = new Guitar::ChordName();
    chord_id->setName ( m_scaleList->currentText(),
                        m_modifierList->currentText(),
                        m_suffixList->currentText(),
                        ( m_versionList->currentText() ).toInt() );


    Guitar::ChordMap::pair c_pos = m_chordMap->find( chord_id );
    if ( c_pos.first ) {
        Guitar::Chord * chord_ptr = c_pos.second;
        m_arrangement = chord_ptr->getArrangement();
        m_aliases->clear();

        // Print aliases
        Guitar::ChordName* cName = chord_ptr->getName();
        std::vector<Guitar::ChordName*> aliases = cName->getAliasList();
        for ( std::vector<Guitar::ChordName*>::const_iterator pos = aliases.begin();
                pos != aliases.end();
                ++pos ) {
            m_aliases->insert ( ( *pos ) ->toString() );
        }
    }

    emit displayChord( m_arrangement );
}

void GuitarTabSelectorDialog::populate ()
{
    setName( "GuitarTabSelectorDialog" );
    setCaption( tr( "Guitar Chord Selector" ) );

    QVBox* mainWidget = makeVBoxMainWidget();

    // Note selection
    QFrame* chordFrame = new QFrame (mainWidget);
    QGridLayout* chordFrameLayout = new QGridLayout(chordFrame, 1, 1, 20, 6);

    //---------------------
    // Chord list boxes
    //---------------------

    // Scale list
    QLabel* scaleTextLabel = new QLabel( i18n ("Scale"), chordFrame );
    chordFrameLayout->addWidget ( scaleTextLabel, 0, 0 );
    m_scaleList = new QListBox( chordFrame );
    chordFrameLayout->addWidget( m_scaleList, 1, 0 );
    connect ( m_scaleList, SIGNAL( highlighted( int ) ),
              this, SLOT( slotSetupModifierList( int ) ) );

    // Modifier list
    QLabel* modifierTextLabel = new QLabel( i18n("Modifier"), chordFrame );
    chordFrameLayout->addWidget( modifierTextLabel, 0, 1 );
    m_modifierList = new QListBox( chordFrame );
    chordFrameLayout->addWidget( m_modifierList, 1, 1 );
    connect ( m_modifierList, SIGNAL( highlighted( int ) ), this,
              SLOT( slotSetupSuffixList( int ) ) );

    // Suffix list
    QLabel* suffixTextLabel = new QLabel( i18n( "Suffix" ), chordFrame );
    chordFrameLayout->addWidget( suffixTextLabel, 0, 2 );
    m_suffixList = new QListBox( chordFrame );
    chordFrameLayout->addWidget( m_suffixList, 1, 2 );
    connect ( m_suffixList, SIGNAL( highlighted( int ) ),
              this, SLOT( slotSetupVersionList( int ) ) );

    // Version list
    QLabel* versionTextLabel = new QLabel( i18n ( "Version" ), chordFrame );
    chordFrameLayout->addWidget( versionTextLabel, 0, 3 );
    m_versionList = new QListBox( chordFrame );
    chordFrameLayout->addWidget( m_versionList, 1, 3 );
    connect ( m_versionList, SIGNAL( highlighted( int ) ),
              this, SLOT( slotDisplayChord() ) );

    // Buttons
    KPushButton* modifyPushButton = new KPushButton( i18n( "&Modify" ), chordFrame );
    modifyPushButton->setAccel( QKeySequence( tr( "Alt+M" ) ) );
    chordFrameLayout->addWidget( modifyPushButton, 2, 3 );
    connect( modifyPushButton, SIGNAL( released() ), this, SLOT( modifyChord() ) );

    KPushButton* createPushButton = new KPushButton( i18n ( "&New" ), chordFrame );
    createPushButton->setAccel( QKeySequence( tr( "Alt+R" ) ) );
    chordFrameLayout->addWidget( createPushButton, 3, 3 );
    connect( createPushButton, SIGNAL( released() ), this, SLOT( createChord() ) );

    KPushButton* clearPushButton = new KPushButton( i18n ( "&Clear" ), chordFrame );
    clearPushButton->setAccel( QKeySequence( tr( "Alt+C" ) ) );
    chordFrameLayout->addWidget( clearPushButton, 4, 3 );
    connect( clearPushButton, SIGNAL( released() ), this, SLOT( clearChord() ) );

    KPushButton* cancelPushButton = new KPushButton( i18n ( "C&ancel" ), chordFrame );
    cancelPushButton->setAccel( QKeySequence( tr( "Alt+A" ) ) );
    chordFrameLayout->addWidget( cancelPushButton, 5, 3 );
    connect( cancelPushButton, SIGNAL( pressed() ), this, SLOT( reject() ) );

    KPushButton* okPushButton = new KPushButton( i18n ( "&Ok" ), chordFrame );
    okPushButton->setAccel( QKeySequence( tr( "Alt+O" ) ) );
    chordFrameLayout->addWidget( okPushButton, 6, 3 );
    connect( okPushButton, SIGNAL( pressed() ), this, SLOT( slotSaveChords() ) );

    // Alias box
    QLabel* aliasLabel = new QLabel( i18n ( "Aliases" ), chordFrame );
    chordFrameLayout->addWidget( aliasLabel, 2, 1, Qt::AlignBottom | Qt::AlignLeft );
    m_aliases = new QTextEdit( chordFrame );
    m_aliases->setReadOnly(true);
    chordFrameLayout->addMultiCellWidget( m_aliases, 3, 6, 1, 2 );

    // Fingering constructor
    m_fingering =
        new Guitar::FingeringConstructor( m_guitar,
                                          chordFrame,
                                          Guitar::FingeringConstructor::DISPLAY_ONLY );

    chordFrameLayout->addMultiCellWidget( m_fingering, 2, 6, 0, 0 );
    connect( this, SIGNAL( displayChord( Guitar::Fingering* ) ),
             m_fingering, SLOT( setFingering( Guitar::Fingering* ) ) );
}

void GuitarTabSelectorDialog::slotSetupModifierList ( int index )
{
    QString name = m_scaleList->text ( index );

    m_modifierList->clear();
    m_suffixList->clear();
    m_versionList->clear();
    m_aliases->clear();

    std::list <QString> modifierList = m_chordMap->getModifierList ( name );

    for ( std::list<QString>::const_iterator pos = modifierList.begin();
            pos != modifierList.end();
            ++pos ) {
        m_modifierList->insertItem ( *pos );
    }
}

void GuitarTabSelectorDialog::slotSetupSuffixList ( int index )
{
    QString name = m_scaleList->currentText ();
    QString modifier = m_modifierList->text ( index );

    m_suffixList->clear();
    m_versionList->clear();
    m_aliases->clear();

    std::list <QString> suffixList = m_chordMap->getSuffixList ( name, modifier );

    for ( std::list<QString>::const_iterator pos = suffixList.begin();
            pos != suffixList.end();
            ++pos ) {
        m_suffixList->insertItem ( *pos );
    }
}

void GuitarTabSelectorDialog::slotSetupVersionList ( int index )
{
    QString name = m_scaleList->currentText ();
    QString modifier = m_modifierList->currentText ();
    QString suffix = m_suffixList->text ( index );

    m_versionList->clear();
    m_aliases->clear();

    // Grab list of versions for this name
    std::list <QString> versionList =
        m_chordMap->getVersionList ( name, modifier, suffix );

    for ( std::list<QString>::const_iterator pos = versionList.begin();
            pos != versionList.end();
            ++pos ) {
        m_versionList->insertItem ( *pos );
    }

}

void GuitarTabSelectorDialog::slotSaveChords ()
{
    // Find directory where we save chords. If its missing it will
    // be created for us.
    QString saveDir = locateLocal ( "appdata", "user_chords/", true );
    m_chordMap->save ( saveDir );

    this->accept();
}

void GuitarTabSelectorDialog::clearChord()
{
    setupNameList();
    if ( m_arrangement != 0 ) {
        delete m_arrangement;
    }
    m_arrangement = new Guitar::Fingering ( m_guitar );
    m_guitar->clear();
    m_aliases->clear();
    emit displayChord ( m_arrangement );
}

Guitar::Fingering
GuitarTabSelectorDialog::getArrangement ( void ) const
{
    return Guitar::Fingering( *m_arrangement );
}

/**
	@todo: Fix error that does not display existing chord in guitar tab selector
	       when clicked on in sheet music
	@todo: Fix error that does not display barre part of a chord in chord
	       diagram
*/
void GuitarTabSelectorDialog::setArrangement ( Guitar::Fingering* chord )
{
    m_arrangement = new Guitar::Fingering (*chord);
    emit displayChord ( m_arrangement );
}

void GuitarTabSelectorDialog::modifyChord()
{
    GuitarChordEditor m_edit ( m_guitar, m_chordMap, this );

    Guitar::ChordName * chord_id = new Guitar::ChordName();
    chord_id->setName ( m_scaleList->currentText(),
                        m_modifierList->currentText(),
                        m_suffixList->currentText(),
                        ( m_versionList->currentText() ).toInt() );


    Guitar::ChordMap::pair c_pos = m_chordMap->find( chord_id );
    if ( c_pos.first ) {
        Guitar::Chord * chord_ptr = c_pos.second;
        m_edit.setChord ( chord_ptr );
        m_edit.disableResize();

        if ( m_edit.exec() ) {
            setupNameList();
        }
    }
}

void GuitarTabSelectorDialog::createChord()
{
    GuitarChordEditor m_edit ( m_guitar, m_chordMap, this );
    m_edit.disableResize();

    if ( m_edit.exec() ) {
        setupNameList();
    }
}

}

#include "GuitarTabSelectorDialog.moc"

