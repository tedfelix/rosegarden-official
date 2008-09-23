
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

#ifndef _RG_CONFIGUREDIALOGBASE_H_
#define _RG_CONFIGUREDIALOGBASE_H_

#include <QMessageBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QString>
#include <vector>


class QWidget;
class QTabWidget;

namespace Rosegarden
{

class ConfigurationPage;


class ConfigureDialogBase : public QDialog
{
    Q_OBJECT
public:
//	ConfigureDialogBase(QDialogButtonBox::QWidget *parent=0,
//						QString label = 0,
//	  const char *name=0);
	ConfigureDialogBase( QWidget *parent=0, QString label = 0, const char *name=0 );
//	  , QMessageBox::StandardButtons = QMessageBox::Apply|QMessageBox::Ok|QMessageBox::Cancel );
	
    //virtual 
	~ConfigureDialogBase();

    typedef std::vector<ConfigurationPage*> configurationpages;

	QWidget* addPage( const QString& iconLabel, const QString& title, const QIcon& icon );
	
	
protected slots:
    virtual void slotOk();
    virtual void slotApply();
    virtual void slotCancelOrClose();

    virtual void slotActivateApply();

protected:

    configurationpages m_configurationPages;
	
	QTabWidget* m_tabWidget;
};


}

#endif
