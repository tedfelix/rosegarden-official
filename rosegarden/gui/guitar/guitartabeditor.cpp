#include "guitartabeditor.h"
#include "guitarxmlhandler.h"
#include "base/Exception.h"
#include "guitartabeditor.moc"

#include <iostream>

#include <qdir.h>
#include <qgroupbox.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qcombobox.h>

#include <kaction.h>
#include <kapp.h>
#include <kconfig.h>
#include <kglobal.h>
#include <klocale.h>
#include <kstddirs.h>

/*---------------------------------------------------------------
              GuitarTabEditorWindow
  ---------------------------------------------------------------*/

GuitarTabEditorWindow::GuitarTabEditorWindow( QWidget *parent )
        : KMainWindow( parent, "guitarchordeditor" ),
        m_data ( new GuitarTabBase() )
{}

GuitarTabEditorWindow::~ GuitarTabEditorWindow()
{}

void GuitarTabEditorWindow::init ( void )
{

    m_data->init();
    populate();

    KStdAction::save ( this, SLOT( slotSaveChords() ), actionCollection() );
    KStdAction::open ( this, SLOT( slotLoadChords() ), actionCollection() );
    KStdAction::quit ( this, SLOT( close() ), actionCollection() );

    createGUI( "guitar.rc" );
}

void GuitarTabEditorWindow::slotDisplayChord ()
{
    m_data->setPresentArrangement( m_nameList->currentText(),
                                   m_modifierList->currentText(),
                                   m_suffixList->currentText(),
                                   m_versionList->currentText() );
    emit displayChord( m_data->getPresentArrangement() );
}

void GuitarTabEditorWindow::populate ()
{
    // Setup GUI
    m_chordBox = new QVBox( this );
    setCentralWidget( m_chordBox );

    m_status = new KStatusBar( this );

    QGroupBox *selectChord = new QGroupBox ( 4, Horizontal,
                             i18n( "Chord Selector" ), m_chordBox );

    QGroupBox *nameBox = new QGroupBox( 1, Horizontal, i18n( "Chord" ), selectChord );
    m_nameList = new QListBox ( nameBox );
    m_nameList->insertStringList ( m_data->getNameList() );
    m_nameList->setCurrentItem( -1 );

    QGroupBox *modifierBox = new QGroupBox( 1, Horizontal, i18n( "Modifier" ), selectChord );
    m_modifierList = new QListBox ( modifierBox );

    QGroupBox *suffixBox = new QGroupBox( 1, Horizontal, i18n( "Additivies" ), selectChord );
    m_suffixList = new QListBox ( suffixBox );

    QGroupBox* versionBox = new QGroupBox ( 1, Horizontal, i18n( "Version" ), selectChord );
    m_versionList = new QListBox ( versionBox );

    /*-----------------------*/
    /* Chord Editing segment */
    /*-----------------------*/
    QGroupBox *fingeringBox = new QGroupBox( 3, Vertical,
                              i18n( "Fingering" ), m_chordBox );

    Guitar::Guitar* guitar = new guitar::Guitar();
    m_fingering = new Guitar::FingeringConstructor ( guitar, fingeringBox );
    fingeringBox->addSpace( 0 );
    fingeringBox->addSpace( 0 );

    m_chord_scales = new QComboBox ( fingeringBox );
    m_chord_mods = new QComboBox ( fingeringBox );
    m_chord_additive = new QLineEdit ( fingeringBox );

    this->setupChordEditLists();

    // Connect signals
    connect( this, SIGNAL( displayChord( Guitar::Fingering* ) ),
             m_fingering, SLOT( setFingering( Guitar::Fingering* ) ) );

    connect ( m_nameList, SIGNAL( highlighted( int ) ),
              this, SLOT( slotSetupModifierList( int ) ) );

    connect ( m_modifierList, SIGNAL( highlighted( int ) ), this,
              SLOT( slotSetupSuffixList( int ) ) );

    connect ( m_suffixList, SIGNAL( highlighted( int ) ),
              this, SLOT( slotSetupVersionList( int ) ) );

    connect ( m_versionList, SIGNAL( highlighted( int ) ),
              this, SLOT( slotDisplayChord() ) );

    connect( this, SIGNAL( statusInfo( QString const& ) ),
             m_status, SLOT( message( const QString& ) ) );
}

void GuitarTabEditorWindow::slotSetupModifierList ( int index )
{
    QString name = m_nameList->text ( index );
    m_data->setupModifierList( name );
    m_modifierList->clear();
    m_suffixList->clear();
    m_versionList->clear();

    m_modifierList->insertStringList( m_data->getModifierList() );
}

void GuitarTabEditorWindow::setupChordEditLists ( void )
{
    m_chord_scales->insertItem( "A_Flat" );
    m_chord_scales->insertItem( "A" );
    m_chord_scales->insertItem( "A_Sharp" );
    m_chord_scales->insertItem( "B_Flat" );
    m_chord_scales->insertItem( "B" );
    m_chord_scales->insertItem( "B_Sharp" );
    m_chord_scales->insertItem( "C" );
    m_chord_scales->insertItem( "C_Sharp" );
    m_chord_scales->insertItem( "D_Flat" );
    m_chord_scales->insertItem( "D" );
    m_chord_scales->insertItem( "E_Flat" );
    m_chord_scales->insertItem( "E_Flat" );
    m_chord_scales->insertItem( "E" );
    m_chord_scales->insertItem( "E_Sharp" );
    m_chord_scales->insertItem( "F" );
    m_chord_scales->insertItem( "F_Sharp" );
    m_chord_scales->insertItem( "G_Flat" );
    m_chord_scales->insertItem( "G" );
    m_chord_scales->insertItem( "G_Sharp" );

    m_chord_mods->insertItem( "Major" );
    m_chord_mods->insertItem( "Minor" );
    m_chord_mods->insertItem( "Perfect" );
    m_chord_mods->insertItem( "Aug" );
    m_chord_mods->insertItem( "Plus" );
    m_chord_mods->insertItem( "Dim" );
    m_chord_mods->insertItem( "Sus" );
}

void GuitarTabEditorWindow::slotSetupSuffixList ( int index )
{
    QString name = m_nameList->currentText ();
    QString modifier = m_modifierList->text ( index );
    m_data->setupSuffixList( name, modifier );

    m_suffixList->clear();
    m_versionList->clear();

    m_suffixList->insertStringList( m_data->getSuffixList() );
}

void GuitarTabEditorWindow::slotSetupVersionList ( int index )
{
    QString name = m_nameList->currentText ();
    QString modifier = m_modifierList->currentText ();
    QString suffix = m_suffixList->text ( index );
    m_versionList->clear();

    m_data->setupVersionList( name, modifier, suffix );
    m_versionList->insertStringList( m_data->getVersionList() );
}

void GuitarTabEditorWindow::slotSaveChords ()
{
    // Read config for user directory
    KConfig * config = kapp->config();
    config->setGroup( "Guitar Chord Options" );
    QString userDir = config->readEntry ( "Chord Directory" );

    //m_chordMap.save ( userDir );
}


void GuitarTabEditorWindow::slotLoadChords ()
{
    // Read config for user directory
    KConfig * config = kapp->config();
    config->setGroup( "Guitar Chord Options" );
    QString userDir = config->readEntry ( "Chord Directory" );

    //m_chordMap.save ( userDir );
}

Guitar::Fingering
GuitarTabEditorWindow::getArrangement ( void ) const
{
    return Guitar::Fingering( *( m_data->getPresentArrangement() ) );
}

void GuitarTabEditorWindow::setArrangement ( Guitar::Fingering& chord )
{
    m_data->setArrangement ( chord );
    emit displayChord ( m_data->getPresentArrangement() );
}

void
GuitarTabEditorWindow::closeEvent( QCloseEvent *e )
{
    emit closing();
    KMainWindow::closeEvent( e );
}
