/****************************************************************************
** Form implementation generated from reading ui file 'guitarchordeditor.ui'
**
** Created: Sun Nov 13 13:08:11 2005
**      by: The User Interface Compiler ($Id$)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "guitarchordeditor.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qvbox.h>

#include <kcombobox.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kmessagebox.h>
#include <klocale.h>

#include "guitarchordeditor.moc"

#include <vector>
#include <sstream>

class ChordInfo
{
public:

    QString chord_type;
    KComboBox* scaleComboBox;
    KComboBox* modifierComboBox;
    KLineEdit* suffixLineEdit;
};

/*
 *  Constructs a GuitarChordEditor as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
GuitarChordEditor::GuitarChordEditor( Guitar::GuitarNeck* g_ptr,
                                      Guitar::ChordMap * cMap,
                                      QWidget* parent )
        : KDialogBase( parent,
                       "guitarchordeditordialog",
                       true,
                       "Guitar Chord Editor",
                       0,
                       KDialogBase::NoDefault,
                       true ),
        m_map ( cMap ),
        m_old_chord_ptr ( 0 )
{
    setName( "GuitarChordEditor" );
    setCaption( tr( "Guitar Chord Editor" ) );

    QVBox* mainWidget = makeVBoxMainWidget();

    // Note selection
    QFrame* chordFrame = new QFrame ( mainWidget );
    QGridLayout* chordFrameLayout = new QGridLayout( chordFrame, 1, 1, 20, 6 );

    // Chord Name tabs
    tabWidget = new QTabWidget ( chordFrame );
    chordFrameLayout->addMultiCellWidget ( tabWidget, 0, 3, 2, 3 );
    createTab ( "Chord" );

    // Buttons
    KPushButton* clearPushButton = new KPushButton( i18n ( "&Clear" ), chordFrame );
    clearPushButton->setAccel( QKeySequence( tr( "Alt+C" ) ) );
    chordFrameLayout->addWidget( clearPushButton, 4, 0 );
    connect( clearPushButton, SIGNAL( pressed() ), this, SLOT( clearDisplay() ) );

    KPushButton* addAliasPushButton = new KPushButton( i18n ( "A&dd Alias" ), chordFrame );
    addAliasPushButton->setAccel( QKeySequence( tr( "Alt+D" ) ) );
    chordFrameLayout->addWidget( addAliasPushButton, 4, 1 );
    connect( addAliasPushButton, SIGNAL( pressed() ), this, SLOT( newAliasTab() ) );

    KPushButton* cancelPushButton = new KPushButton( i18n ( "C&ancel" ), chordFrame );
    chordFrameLayout->addWidget( cancelPushButton, 4, 2 );
    cancelPushButton->setAccel( QKeySequence( tr( "Alt+A" ) ) );
    connect( cancelPushButton, SIGNAL( pressed() ), this, SLOT( reject() ) );

    KPushButton* okPushButton = new KPushButton( i18n ( "&Ok" ), chordFrame );
    okPushButton->setAccel( QKeySequence( tr( "Alt+O" ) ) );
    chordFrameLayout->addWidget( okPushButton, 4, 3 );
    connect( okPushButton, SIGNAL( pressed() ), this, SLOT( saveChord() ) );

    KPushButton* modePushButton = new KPushButton( i18n ( "Toggle &Mode" ), chordFrame );
    modePushButton->setAccel( QKeySequence( tr( "Alt+M" ) ) );
    chordFrameLayout->addWidget( modePushButton, 5, 0 );
    connect( modePushButton, SIGNAL( pressed() ), this, SLOT( toggleMode() ) );

    fingerConstPtr =
        new Guitar::FingeringConstructor ( g_ptr,
                                           chordFrame,
                                           Guitar::FingeringConstructor::EDITABLE,
                                           "fingerConstPtr" );

    chordFrameLayout->addMultiCellWidget( fingerConstPtr, 0, 3, 0, 1 );

    m_status = new KStatusBar ( mainWidget );
    m_status->setSizeGripEnabled ( false );
    m_status->message ( "MODE: Insert" );
}

void GuitarChordEditor::saveChord()
{
    Guitar::ChordName * name_ptr = this->getChordName();
    Guitar::Fingering* arrangement = fingerConstPtr->getFingering();

    int answer = KMessageBox::questionYesNo ( this,
                 "Do you really want to save this chord?" );
    if ( answer == KMessageBox::No )
    {
        this->reject();
        return ;
    }

    try
    {
        // Remove old chord if set
        if ( m_old_chord_ptr != 0 )
        {
            m_map->erase( m_old_chord_ptr );
            delete m_old_chord_ptr;
            m_old_chord_ptr = 0;
        }

        Guitar::Chord* newChord = new Guitar::Chord ( name_ptr, arrangement );
        m_map->insert( newChord );
        this->accept();
    }
    catch ( Guitar::DuplicateException & de )
    {
        Guitar::Chord * dup_ptr = de.getDuplicate();
        Guitar::ChordName* dup_name_ptr = dup_ptr->getName();
        std::stringstream error_msg;

        error_msg << "Duplicate chord found. " << std::endl
        << "Add new alias to "
        << dup_name_ptr->toString();

        KMessageBox::sorry ( this,
                             error_msg.str(),
                             "Duplicate Chord",
                             KMessageBox::Notify );
        this->reject();
    }
}

void GuitarChordEditor::newAliasTab()
{
    createTab ( "Alias" );
}

void GuitarChordEditor::clearDisplay()
{
    fingerConstPtr->clear();
    for ( InfoMap::iterator pos = m_infoMap.begin();
            pos != m_infoMap.end();
            ++pos )
    {
        InfoPair map_pair = ( *pos );
        tabWidget->removePage( map_pair.first );
        delete map_pair.second;
    }
    m_infoMap.clear();

    createTab ( "Chord" );
}

void GuitarChordEditor::createTab ( QString const& name )
{
    QFrame * nameFrame = new QFrame ( tabWidget );
    ChordInfo* cInfo = new ChordInfo();

    // Setup the scale line of the tab
    QGridLayout* nameLayout = new QGridLayout ( nameFrame, 1, 1, 20, 6 );

    // - Add scale label
    QLabel* scaleLabel = new QLabel( i18n( "Scale" ), nameFrame );
    nameLayout->addWidget( scaleLabel, 0, 0 );

    // - Add combo box
    cInfo->scaleComboBox = new KComboBox( FALSE, nameFrame );
    nameLayout->addWidget( cInfo->scaleComboBox, 0, 1 );
    cInfo->scaleComboBox->insertItem( tr( "A_Flat" ) );
    cInfo->scaleComboBox->insertItem( tr( "A" ) );
    cInfo->scaleComboBox->insertItem( tr( "A_Sharp" ) );
    cInfo->scaleComboBox->insertItem( tr( "B_Flat" ) );
    cInfo->scaleComboBox->insertItem( tr( "B" ) );
    cInfo->scaleComboBox->insertItem( tr( "B_Sharp" ) );
    cInfo->scaleComboBox->insertItem( tr( "C" ) );
    cInfo->scaleComboBox->insertItem( tr( "C_Sharp" ) );
    cInfo->scaleComboBox->insertItem( tr( "D_Flat" ) );
    cInfo->scaleComboBox->insertItem( tr( "D" ) );
    cInfo->scaleComboBox->insertItem( tr( "D_Sharp" ) );
    cInfo->scaleComboBox->insertItem( tr( "E_Flat" ) );
    cInfo->scaleComboBox->insertItem( tr( "E" ) );
    cInfo->scaleComboBox->insertItem( tr( "E_Sharp" ) );
    cInfo->scaleComboBox->insertItem( tr( "F" ) );
    cInfo->scaleComboBox->insertItem( tr( "F_Sharp" ) );
    cInfo->scaleComboBox->insertItem( tr( "G_Flat" ) );
    cInfo->scaleComboBox->insertItem( tr( "G" ) );
    cInfo->scaleComboBox->insertItem( tr( "G_Sharp" ) );

    // - Add modifier label
    QLabel* modifierLabel = new QLabel( i18n ( "Modifier" ), nameFrame );
    nameLayout->addWidget( modifierLabel, 1, 0 );

    // - Add combo box
    cInfo->modifierComboBox = new KComboBox( FALSE, nameFrame );
    nameLayout->addWidget( cInfo->modifierComboBox, 1, 1 );
    cInfo->modifierComboBox->insertItem( tr( "Major" ) );
    cInfo->modifierComboBox->insertItem( tr( "Minor" ) );
    cInfo->modifierComboBox->insertItem( tr( "Perfect" ) );
    cInfo->modifierComboBox->insertItem( tr( "Aug" ) );
    cInfo->modifierComboBox->insertItem( tr( "Plus" ) );
    cInfo->modifierComboBox->insertItem( tr( "Dim" ) );
    cInfo->modifierComboBox->insertItem( tr( "Sus" ) );

    // - Add suffix label
    QLabel* suffixLabel = new QLabel( i18n( "Suffix" ), nameFrame );
    nameLayout->addWidget( suffixLabel, 2, 0 );

    // - Add suffix line edit
    cInfo->suffixLineEdit = new KLineEdit( nameFrame );
    nameLayout->addWidget( cInfo->suffixLineEdit, 2, 1 );

    tabWidget->insertTab( nameFrame, tr( name ) );
    cInfo->chord_type = name;
    m_infoMap.push_back ( std::make_pair ( nameFrame, cInfo ) );
}

void GuitarChordEditor::setChord ( Guitar::Chord* c_ptr )
{
    m_old_chord_ptr = new Guitar::Chord ( *c_ptr );

    // Setup FingeringConstructor with new arrangement
    fingerConstPtr->setFingering( c_ptr->getArrangement() );

    /**
    Setup Chord Tab for main name

    We are expecting that when a Guitar Chord Editor is created that only
        one tab exists in the tab widget and therefore only one ChordInfo object.
    */
    Guitar::ChordName* name_ptr = c_ptr->getName();
    InfoMap::reverse_iterator m_pos = m_infoMap.rbegin();
    InfoPair map_pair = ( *m_pos );
    ChordInfo* info_ptr = map_pair.second;
    info_ptr->scaleComboBox->setCurrentItem ( name_ptr->getScale() );
    info_ptr->modifierComboBox->setCurrentItem ( name_ptr->getModifier() );
    info_ptr->suffixLineEdit->setText ( name_ptr->getSuffix() );

    // For all aliases in name
    std::vector<Guitar::ChordName*> aliases = name_ptr->getAliasList();
    for ( std::vector<Guitar::ChordName*>::const_iterator aPos = aliases.begin();
            aPos != aliases.end();
            ++aPos )
    {
        this->newAliasTab();
        name_ptr = ( *aPos );
        m_pos = m_infoMap.rbegin();
        map_pair = ( *m_pos );
        info_ptr = map_pair.second;
        info_ptr->scaleComboBox->setCurrentItem ( name_ptr->getScale() );
        info_ptr->modifierComboBox->setCurrentItem ( name_ptr->getModifier() );
        info_ptr->suffixLineEdit->setText ( name_ptr->getSuffix() );
    }
}

Guitar::ChordName*
GuitarChordEditor::getChordName ( void )
{
    // Main chord name
    InfoMap::const_iterator cInfo_pos = m_infoMap.begin();
    InfoPair map_pair = ( *cInfo_pos );
    QString scale = ( map_pair.second ) ->scaleComboBox->currentText();
    QString mod = ( map_pair.second ) ->modifierComboBox->currentText();
    QString suffix = ( map_pair.second ) ->suffixLineEdit->text();

    if ( scale.isEmpty() || mod.isEmpty() )
    {
        this->reject();
    }

    if ( suffix.isEmpty() )
    {
        suffix = "None";
    }

    Guitar::ChordName* main = new Guitar::ChordName();

    main->setName ( scale, mod, suffix );
    ++cInfo_pos;

    // Handle Aliases
    while ( cInfo_pos != m_infoMap.end() )
    {
        Guitar::ChordName * alias = new Guitar::ChordName();
        map_pair = ( *cInfo_pos );
        scale = ( map_pair.second ) ->scaleComboBox->currentText();
        mod = ( map_pair.second ) ->modifierComboBox->currentText();
        suffix = ( map_pair.second ) ->suffixLineEdit->text();

        if ( !scale.isEmpty() || !mod.isEmpty() )
        {
            if ( suffix.isEmpty() )
            {
                suffix = "None";
            }
            alias->setName ( scale, mod, suffix );
            main->addAlias( alias );
        }
        ++cInfo_pos;
    }

    return main;
}

void GuitarChordEditor::toggleMode()
{
    if ( fingerConstPtr->toggleState() )
    {
        m_status->message( "MODE: Insert" );
    }
    else
    {
        m_status->message( "MODE: Delete" );
    }
}
