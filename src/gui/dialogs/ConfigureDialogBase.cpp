/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ConfigureDialogBase.h"

#include "gui/configuration/ConfigurationPage.h"
#include "misc/Debug.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <QWidget>
#include <QTabWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace Rosegarden
{
/*
ConfigureDialogBase::ConfigureDialogBase(QWidget *parent,
        QString label,
        const char *name):
        KDialogBase(IconList, !label.isEmpty() ? label : tr("Configure"), Help | Apply | Ok | Cancel,
                    Ok, parent, name, true) // modal
*/
ConfigureDialogBase::ConfigureDialogBase( QWidget *parent, QString label, const char *name  ):QDialog(parent)
//        , QMessageBox::StandardButtons buttons ) :QDialog(parent)
{
    
    this->setAttribute( Qt::WA_DeleteOnClose );
    
    this->setWindowTitle( tr("Configure Rosegarden") );
    this->setObjectName( (name) );
    
    QVBoxLayout *dlgLay = new QVBoxLayout( this );
    this->setLayout( dlgLay );
    
    //QTabWidget *
    m_tabWidget = new QTabWidget( this );
    // tab-widget, that conains the misc config option pages
    dlgLay->addWidget( m_tabWidget );
    
    
    // setup dialog buttons
    //
    QDialogButtonBox::StandardButtons sbuttons = \
            QDialogButtonBox::Ok |
            QDialogButtonBox::Cancel | 
            QDialogButtonBox::Apply |
            QDialogButtonBox::RestoreDefaults |
            QDialogButtonBox::Help;
    //QDialogButtonBox *
    m_dialogButtonBox = new QDialogButtonBox( sbuttons, Qt::Horizontal, this );
    m_dialogButtonBox->setObjectName( "dialog_base_button_box" );
    dlgLay->addWidget( m_dialogButtonBox );
    
    // fist disable the Apply button
    QPushButton * btApply;
    btApply = m_dialogButtonBox->button( QDialogButtonBox::Apply );
    btApply->setEnabled( false );
    
    
    // qt4 connctions for QDialogButtonBox:
    //
     connect( m_dialogButtonBox, SIGNAL(clicked(QAbstractButton *)),
             this, SLOT(slotButtonBoxButtonClicked(QAbstractButton *)) );
//    connect(m_dialogButtonBox, SIGNAL(accepted()), this, SLOT(slotOk()));
//    connect(m_dialogButtonBox, SIGNAL(rejected()), this, SLOT(slotCancelOrClose()));
        
    /*
    // setup dialog buttons OLD CODE:
    QWidget *buttWidget = new QWidget( this );
    dlgLay->addWidget( buttWidget );
    
    QPushButton *applyButt = new QPushButton("Apply");
    QPushButton *okButt = new QPushButton("Ok");
    QPushButton *cancelButt = new QPushButton("Cancel");
    
    QHBoxLayout *buttLay = new QHBoxLayout( buttWidget );
    buttLay->addWidget( applyButt );
    buttLay->addWidget( okButt );
    buttLay->addWidget( cancelButt );
    */
    
}

QWidget* ConfigureDialogBase::addPage( const QString& iconLabel, const QString& label, const QIcon& icon ){
    /**     add a configuration options tab to the tabWidget ; return the tab-page <QWidget*>  */
    QWidget *page = new QWidget();
    if( ! m_tabWidget ){
        std::cerr << "ERROR: m_tabWidget is NULL in ConfigureDialogBase::addPage " << std::endl;
        return 0;
    }
    m_tabWidget->addTab( page, icon, label );
    return page;
}


ConfigureDialogBase::~ConfigureDialogBase()
{}


void ConfigureDialogBase::slotButtonBoxButtonClicked(QAbstractButton * button){
    
    QDialogButtonBox::ButtonRole bRole = m_dialogButtonBox->buttonRole( button );
    
    if( bRole == QDialogButtonBox::ApplyRole ){
        slotApply();
//        close();
    }
    else if( bRole == QDialogButtonBox::AcceptRole ){
        slotOk();
    }
    else if( bRole == QDialogButtonBox::HelpRole ){
//         slotHelp();
    }
    else if( bRole == QDialogButtonBox::ResetRole ){
//         slotRestoreDefaults();
    }else{
        // cancel
        //### do we need to reset/restore anything here, before closing the conf dialog ?
        slotCancelOrClose();
        //close();
    }
}


void
ConfigureDialogBase::slotApply()
{
    RG_DEBUG << "CONFIGUREDIALOGBASE SLOTAPPLY()" << endl;
    for (configurationpages::iterator i = m_configurationPages.begin();
            i != m_configurationPages.end(); ++i)
        (*i)->apply();
}

void
ConfigureDialogBase::slotActivateApply()
{
    
    //QDialogButtonBox *dbb = findChild<QDialogButtonBox *>( "dialog_base_button_box" );
    if( ! m_dialogButtonBox ){
        std::cerr << "ERROR: QDialogButtonBox is NULL in ConfigureDialogBase::slotActivateApply() " << std::endl;
        return;
    }
    
    QPushButton * btApply;
    btApply = m_dialogButtonBox->button( QDialogButtonBox::Apply );
    btApply->setEnabled( true );
}

void
ConfigureDialogBase::slotOk()
{
    slotApply();
    accept();
}

void
ConfigureDialogBase::slotCancelOrClose()
{
    close();
}

}
#include "ConfigureDialogBase.moc"
