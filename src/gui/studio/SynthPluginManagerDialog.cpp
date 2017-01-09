/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SynthPluginManagerDialog]"

#include "SynthPluginManagerDialog.h"


#include "misc/Debug.h"
#include "AudioPlugin.h"
#include "AudioPluginManager.h"
#include "AudioPluginOSCGUIManager.h"
#include "base/AudioPluginInstance.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "base/Studio.h"
#include "document/RosegardenDocument.h"
#include "misc/ConfigGroups.h"
#include "misc/Strings.h"


#include <QAction>
#include <QLayout>
#include <QComboBox>
#include <QMainWindow>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QScrollArea>
#include <QDesktopServices>
#include <QUrl>

namespace Rosegarden
{

SynthPluginManagerDialog::SynthPluginManagerDialog(QWidget *parent,
                                                   RosegardenDocument *doc,
                                                   AudioPluginOSCGUIManager *guiManager) :
    QMainWindow ( parent ),
    m_document ( doc ),
    m_studio ( &doc->getStudio() ),
    m_pluginManager ( doc->getPluginManager() ),
    m_guiManager ( guiManager )
{
    // start constructor
    //
    setWindowTitle ( tr ( "Manage Synth Plugins" ) );
    resize ( 760, 520 );
    move ( 100, 80 );
    
    setupGuiMain();
    setupGuiCreatePluginList();
    
//    createGUI ( "synthpluginmanager.rc" );

    setAttribute(Qt::WA_DeleteOnClose);
    setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));
}

void
SynthPluginManagerDialog:: setupGuiMain()
{
    m_centralWidget = new QWidget ( this );
    m_centralWidget->setObjectName ( QString::fromUtf8 ( "m_centralWidget" ) );

    m_mainLayout = new QVBoxLayout ( m_centralWidget );
    m_mainLayout->setObjectName ( QString::fromUtf8 ( "mainLayout" ) );

    m_groupBoxPluginList = new QGroupBox ( m_centralWidget );
    m_groupBoxPluginList->setObjectName ( QString::fromUtf8 ( "m_groupBoxPluginList" ) );

    m_verticalLayout_2 = new QVBoxLayout ( m_groupBoxPluginList );
    m_verticalLayout_2->setObjectName ( QString::fromUtf8 ( "verticalLayout_2" ) );

    m_scrollArea = new QScrollArea ( m_groupBoxPluginList );
    m_scrollArea->setObjectName ( QString::fromUtf8 ( "m_scrollArea" ) );
    m_scrollArea->setVerticalScrollBarPolicy ( Qt::ScrollBarAlwaysOn );
    m_scrollArea->setWidgetResizable ( true );

    m_scrollWidget = new QWidget();
    m_scrollWidget->setObjectName ( QString::fromUtf8 ( "m_scrollWidget" ) );
    m_scrollWidget->setGeometry ( QRect ( 0, 0, 740, 489 ) );
    m_scrollArea->setWidget ( m_scrollWidget );
    m_verticalLayout_2->addWidget ( m_scrollArea );

    m_scrollWidgetLayout = new QGridLayout ( m_scrollWidget );
    m_scrollWidgetLayout->setObjectName ( QString::fromUtf8 ( "m_scrollWidgetLayout" ) );

    m_mainLayout->addWidget ( m_groupBoxPluginList );

    setCentralWidget ( m_centralWidget );
        
        
        
    //
    // start dialog button-box setup
    // ------------------------------------------------------------------
    //
    QDialogButtonBox::StandardButtons sbuttons = \
        QDialogButtonBox::Close |
//                 QDialogButtonBox::Ok |
//                 QDialogButtonBox::Cancel |
//                 QDialogButtonBox::Apply |
//                 QDialogButtonBox::RestoreDefaults |
        QDialogButtonBox::Help;

    m_dialogButtonBox = new QDialogButtonBox;
    m_dialogButtonBox->setObjectName("dialog_base_button_box");
    m_dialogButtonBox->setStandardButtons(sbuttons);
    m_dialogButtonBox->setOrientation(Qt::Horizontal);
    m_mainLayout->addWidget(m_dialogButtonBox);

    // fist disable the Apply button
    QPushButton * btApply;
    btApply = m_dialogButtonBox->button ( QDialogButtonBox::Apply );
    if ( btApply ){
        btApply->setEnabled ( false );
    }

//         connect(m_dialogButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect ( m_dialogButtonBox, SIGNAL ( rejected() ), this, SLOT ( slotClose() ) );
    connect ( m_dialogButtonBox, SIGNAL ( helpRequested() ), this, SLOT ( slotHelpRequested() ) );

    //
    // end dialog button-box setup
    // ------------------------------------------------------------------
        
        
}// end setupGuiMain()




void SynthPluginManagerDialog:: setupGuiCreatePluginList(){
    //
    m_synthPlugins.clear();
    m_synthPlugins.push_back ( -1 );

    int count = 0;

    for ( PluginIterator itr = m_pluginManager->begin();
          itr != m_pluginManager->end(); ++itr ){

        if ( ( *itr )->isSynth() ){
            m_synthPlugins.push_back ( count );
        }

        ++count;
    }

    for ( unsigned int i = 0; i < SoftSynthInstrumentCount; ++i ){

        InstrumentId id = SoftSynthInstrumentBase + i;
        Instrument *instrument = m_studio->getInstrumentById ( id );
        if ( !instrument )
            continue;

        //  pluginLayout->addWidget(new QLabel(instrument->getPresentationName().c_str(),
        //                     pluginFrame), i, 0);
        m_scrollWidgetLayout->addWidget ( new QLabel ( QString ( "%1" ).arg ( i + 1 ),
                                                       m_scrollWidget ), i, 0 );

        AudioPluginInstance *plugin = instrument->getPlugin
            ( Instrument::SYNTH_PLUGIN_POSITION );

        std::string identifier;
        if ( plugin )
            identifier = plugin->getIdentifier();

        // int currentIndex = 0;

        QComboBox *pluginCombo = new QComboBox ( m_scrollWidget );
        pluginCombo->addItem ( tr ( "<none>" ) );

        for ( size_t j = 0; j < m_synthPlugins.size(); ++j ){

            if ( m_synthPlugins[j] == -1 )
                continue;

            AudioPlugin *plugin =
                m_pluginManager->getPlugin ( m_synthPlugins[j] );

            pluginCombo->addItem ( plugin->getName() );

            if ( plugin->getIdentifier() == identifier.c_str() ){
                pluginCombo->setCurrentIndex ( pluginCombo->count() - 1 );
            }
        }

        connect ( pluginCombo, SIGNAL ( activated ( int ) ),
                  this, SLOT ( slotPluginChanged ( int ) ) );

        m_scrollWidgetLayout->addWidget ( pluginCombo, i, 1 );

        m_synthCombos.push_back ( pluginCombo );

        QPushButton *controlsButton = new QPushButton ( tr ( "Controls" ), m_scrollWidget );
        m_scrollWidgetLayout->addWidget ( controlsButton, i, 2 );
        connect ( controlsButton, SIGNAL ( clicked() ), this, SLOT ( slotControlsButtonClicked() ) );
        m_controlsButtons.push_back ( controlsButton );

        QPushButton *guiButton = new QPushButton ( tr ( "Editor >>" ), m_scrollWidget );
        m_scrollWidgetLayout->addWidget ( guiButton, i, 3 );
        guiButton->setEnabled ( m_guiManager->hasGUI
                                ( id, Instrument::SYNTH_PLUGIN_POSITION ) );
        connect ( guiButton, SIGNAL ( clicked() ), this, SLOT ( slotGUIButtonClicked() ) );
        m_guiButtons.push_back ( guiButton );

    }// end for i
        
        
}// end setupGuiCreatePluginList()



SynthPluginManagerDialog::~SynthPluginManagerDialog(){
    RG_DEBUG << "\n*** SynthPluginManagerDialog::~SynthPluginManagerDialog()"
             << endl;
}


void SynthPluginManagerDialog:: slotHelpRequested() {
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:manual-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:synth-plugin-manager-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}


void
SynthPluginManagerDialog::updatePlugin ( InstrumentId id, int plugin ){
    if ( id < SoftSynthInstrumentBase )
        return ;
    size_t row = size_t(id - SoftSynthInstrumentBase);
    if ( row >= m_synthCombos.size() )
        return ;

    QComboBox *comboBox = m_synthCombos[row];

    for ( size_t i = 0; i < m_synthPlugins.size(); ++i ){
        if ( m_synthPlugins[i] == plugin ){
            blockSignals ( true );
            comboBox->setCurrentIndex ( i );
            blockSignals ( false );
            return ;
        }
    }

    blockSignals ( true );
    comboBox->setCurrentIndex ( 0 );
    blockSignals ( false );
    return ;
}

void
SynthPluginManagerDialog::slotClose()
{
    emit closing();
    close();
}

void
SynthPluginManagerDialog::closeEvent ( QCloseEvent *e )
{
    emit closing();
    QMainWindow::closeEvent ( e );
}

void
SynthPluginManagerDialog::slotGUIButtonClicked(){
    const QObject *s = sender();

    int instrumentNo = -1;

    for ( size_t i = 0; i < m_guiButtons.size(); ++i ){
        if ( s == m_guiButtons[i] )
            instrumentNo = int(i);
    }

    if ( instrumentNo == -1 ){
        RG_DEBUG << "WARNING: SynthPluginManagerDialog::slotGUIButtonClicked: unknown sender";
        return ;
    }

    InstrumentId id = SoftSynthInstrumentBase + instrumentNo;

    emit showPluginGUI ( id, Instrument::SYNTH_PLUGIN_POSITION );
}



void SynthPluginManagerDialog:: slotControlsButtonClicked(){
    const QObject *s = sender();

    int instrumentNo = -1;

    for ( size_t i = 0; i < m_controlsButtons.size(); ++i ){
        if ( s == m_controlsButtons[i] )
            instrumentNo = int(i);
    }

    if ( instrumentNo == -1 ){
        RG_DEBUG << "WARNING: SynthPluginManagerDialog::slotControlsButtonClicked: unknown sender";
        return ;
    }

    InstrumentId id = SoftSynthInstrumentBase + instrumentNo;

    emit showPluginDialog ( this, id, Instrument::SYNTH_PLUGIN_POSITION );
    // note: slot is in RosegardenMainWindow.cpp
}



void SynthPluginManagerDialog::slotPluginChanged ( int index ){
    const QObject *s = sender();

    RG_DEBUG << "SynthPluginManagerDialog::slotPluginChanged(" << index
             << ")" << endl;

    int instrumentNo = -1;

    for ( size_t i = 0; i < m_synthCombos.size(); ++i ){
        if ( s == m_synthCombos[i] )
            instrumentNo = int(i);
    }

    if ( instrumentNo == -1 ){
        RG_DEBUG << "WARNING: SynthPluginManagerDialog::slotValueChanged: unknown sender";
        return ;
    }

    InstrumentId id = SoftSynthInstrumentBase + instrumentNo;

    if ( index >= int ( m_synthPlugins.size() ) ){
        RG_DEBUG << "WARNING: SynthPluginManagerDialog::slotValueChanged: synth "
                 << index << " out of range" << endl;
        return ;
    }

    // NB m_synthPlugins[0] is -1 to represent the <none> item

    AudioPlugin *plugin = m_pluginManager->getPlugin ( m_synthPlugins[index] );
    Instrument *instrument = m_studio->getInstrumentById ( id );

    if ( instrument ){

        AudioPluginInstance *pluginInstance = instrument->getPlugin
            ( Instrument::SYNTH_PLUGIN_POSITION );

        if ( pluginInstance ){

            if ( plugin ){
                RG_DEBUG << "plugin is " << plugin->getIdentifier();
                pluginInstance->setIdentifier ( qstrtostr ( plugin->getIdentifier() ) );

                // set ports to defaults

                AudioPlugin::PortIterator it = plugin->begin();
                int count = 0;

                for ( ; it != plugin->end(); ++it ){

                    if ( ( ( *it )->getType() & PluginPort::Control ) &&
                         ( ( *it )->getType() & PluginPort::Input ) ){

                        if ( pluginInstance->getPort ( count ) == 0 ){
                            pluginInstance->addPort ( count, ( float ) ( *it )->getDefaultValue() );
                        }
                        else{
                            pluginInstance->getPort ( count )->value = ( *it )->getDefaultValue();
                        }
                    }

                    ++count;
                }

            }
            else{
                pluginInstance->setIdentifier ( "" );
            }
        }
    }

    if ( instrumentNo < int(m_guiButtons.size()) ){
        m_guiButtons[instrumentNo]->setEnabled
            ( m_guiManager->hasGUI
              ( id, Instrument::SYNTH_PLUGIN_POSITION ) );
    }

    emit pluginSelected ( id, Instrument::SYNTH_PLUGIN_POSITION,
                          m_synthPlugins[index] );

    adjustSize();
}

}

