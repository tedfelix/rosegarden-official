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


#include "ConfigureDialogBase.h"

#include <klocale.h>
#include "gui/configuration/ConfigurationPage.h"
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
ConfigureDialogBase::ConfigureDialogBase(QDialogButtonBox::QWidget *parent,
        QString label,
        const char *name):
        KDialogBase(IconList, !label.isEmpty() ? label : i18n("Configure"), Help | Apply | Ok | Cancel,
                    Ok, parent, name, true) // modal
*/
ConfigureDialogBase::	ConfigureDialogBase( QWidget *parent, QString label, const char *name  ):QDialog(parent)
//		, QMessageBox::StandardButtons buttons ) :QDialog(parent)
{
//    setWFlags(WDestructiveClose);
	this->setAttribute( Qt::WA_DeleteOnClose );
	
	this->setWindowTitle( i18n("Configure Rosegarden") );
	this->setObjectName( (name) );
	
	QVBoxLayout *dlgLay = new QVBoxLayout( this );
	QTabWidget *m_tabWidget = new QTabWidget( this );
    // tab-widget, that conains the misc config option pages
	dlgLay->addWidget( m_tabWidget );
	
	QWidget *buttWidget = new QWidget( this );
	dlgLay->addWidget( buttWidget );
	
	QPushButton *applyButt = new QPushButton();
	QPushButton *okButt = new QPushButton();
	QPushButton *cancelButt = new QPushButton();
	
	QHBoxLayout *buttLay = new QHBoxLayout( buttWidget );
	buttLay->addWidget( applyButt );
	buttLay->addWidget( okButt );
	buttLay->addWidget( cancelButt );
	

}

QWidget* ConfigureDialogBase::addPage( const QString& iconLabel, const QString& label, const QIcon& icon ){
	/**	 add a configuration options tab to the tabWidget ; return the tab-page <QWidget*>  */
	QWidget *page = new QWidget();
	this->m_tabWidget->addTab( page, icon, label );
	return page;
}


ConfigureDialogBase::~ConfigureDialogBase()
{}

void
ConfigureDialogBase::slotApply()
{
    for (configurationpages::iterator i = m_configurationPages.begin();
            i != m_configurationPages.end(); ++i)
        (*i)->apply();
}

void
ConfigureDialogBase::slotActivateApply()
{
    //     ApplyButton->setDisabled(false);
}

void
ConfigureDialogBase::slotOk()
{
    slotApply();
    accept();
}

void
ConfigureDialogBase::slotCancelOrClose()
{}

}
#include "ConfigureDialogBase.moc"
