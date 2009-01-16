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


#include "ProgressDialog.h"
#include "CurrentProgressDialog.h"
#include "misc/Debug.h"
#include "gui/application/RosegardenApplication.h"
#include <QCursor>
#include <QProgressDialog>
#include <QProgressBar>
#include <QString>
#include <QTimer>
#include <QWidget>


namespace Rosegarden
{

bool ProgressDialog::m_modalVisible = false;


ProgressDialog::ProgressDialog(QWidget *creator,
                               const char *name,
                               bool modal
		):
// 		QProgressDialog(creator, name,
// 						tr("Processing..."), QString::null, modal),
		
// 		QProgressDialog( tr("Processing..."), QString("Cancel"), creator ),	//modal),
		QProgressDialog( creator, Qt::Dialog ),		// creator = parent ??
        m_wasVisible(false),
        m_frozen(false),
        m_modal(modal)
{
	this->setObjectName( name );
	if (modal){
		setWindowModality( Qt::WindowModal );
	}else{
		setWindowModality( Qt::NonModal );
	}
// 	setMinimum();
// 	setMaximum();
// 	setValue();
// 	setLabelText( tr("Processing...") );
	setWindowTitle( tr("Processing...") );
	setCancelButtonText( QString("Cancel") );
	
	
    setCaption(tr("Processing..."));
    RG_DEBUG << "ProgressDialog::ProgressDialog type 1 - "
    << labelText() << " - modal : " << modal << endl;

    connect(progressBar(), SIGNAL(percentageChanged (int)),
            this, SLOT(slotCheckShow(int)));

    m_chrono.start();

    CurrentProgressDialog::set
        (this);

    setMinimumDuration(500); // set a default value for this
}


QProgressBar* ProgressDialog::progressBar()
{	/* return the progress bar created in constructor */
	return m_progressBar;
}



ProgressDialog::ProgressDialog(
    const QString &labelText,
    int totalSteps,
    QWidget *creator,
    const char *name,
    bool modal) :
//         QProgressDialog(creator,
//                         name,
//                         tr("Processing..."),
//                         labelText,
//                         modal),
		
// 		QProgressDialog( tr("Processing..."), QString("Cancel"), creator ),	//modal),
		QProgressDialog( creator, Qt::Dialog ),	//modal),
		
		m_wasVisible(false),
        m_frozen(false),
        m_modal(modal)
{
	
	this->setObjectName( name );
	if (modal){
		setWindowModality( Qt::WindowModal );
	}else{
		setWindowModality( Qt::NonModal );
	}
// 	setMinimum();
// 	setMaximum();
// 	setValue();
// 	setLabelText( tr("Processing...") );
	setWindowTitle( tr("Processing...") );
	setCancelButtonText( QString("Cancel") );
	
	
	// qt4 note: progressBar() doesn't exist anymore. 
	// one can call setBar(QProgressBar*) but not retrieve it 
	m_progressBar = new QProgressBar();
	setBar( m_progressBar );
	
// 	progressBar()->setTotalSteps(totalSteps);
	progressBar()->setMaximum(totalSteps);
	

    RG_DEBUG << "ProgressDialog::ProgressDialog type 2 - "
    << labelText << " - modal : " << modal << endl;

    connect(progressBar(), SIGNAL(percentageChanged (int)),
            this, SLOT(slotCheckShow(int)));

    m_chrono.start();

    CurrentProgressDialog::set
        (this);

    setMinimumDuration(500); // set a default value for this
}

ProgressDialog::~ProgressDialog()
{
    m_modalVisible = false;
}

void
ProgressDialog::polish()
{
    QProgressDialog::polish();

//@@@ JAS I don't think this is necassary now deactivating
//@@@ might want to remove ProgressDialog::polish(), later.
//&&&    if (allowCancel())
//&&&        setCursor(Qt::ArrowCursor);
//&&&    else
//&&&        QApplication::setOverrideCursor(QCursor(Qt::waitCursor));
}

void ProgressDialog::hideEvent(QHideEvent* e)
{
//@@@ JAS I don't think this is necassary now deactivating
//&&&    if (!allowCancel())
//&&&        QApplication::restoreOverrideCursor();

    QProgressDialog::hideEvent(e);
    m_modalVisible = false;
}

void
ProgressDialog::slotSetOperationName(QString name)
{
    //     RG_DEBUG << "ProgressDialog::slotSetOperationName("
    //              << name << ") visible : " << isVisible() << endl;

    setLabelText(name);
    // Little trick stolen from QProgressDialog
    // increase resize only, never shrink
    int w = QMAX( isVisible() ? width() : 0, sizeHint().width() );
    int h = QMAX( isVisible() ? height() : 0, sizeHint().height() );
    resize( w, h );
}

void ProgressDialog::slotCancel()
{
    RG_DEBUG << "ProgressDialog::slotCancel()\n";
    QProgressDialog::cancel();
    slotFreeze();
}

void ProgressDialog::slotCheckShow(int)
{
    //     RG_DEBUG << "ProgressDialog::slotCheckShow() : "
    //              << m_chrono.elapsed() << " - " << minimumDuration()
    //              << endl;

    if (!isVisible() &&
            !m_frozen &&
            m_chrono.elapsed() > minimumDuration()) {
        RG_DEBUG << "ProgressDialog::slotCheckShow() : showing dialog\n";
        show();
        if (m_modal)
            m_modalVisible = true;
        processEvents();
    }
}

void ProgressDialog::slotFreeze()
{
    RG_DEBUG << "ProgressDialog::slotFreeze()\n";

    m_wasVisible = isVisible();
    if (isVisible()) {
        m_modalVisible = false;
        hide();
    }

    // This is also a convenient place to ensure the wait cursor (if
    // currently shown) returns to the original cursor to ensure that
    // the user can respond to whatever's freezing the progress dialog
    QApplication::restoreOverrideCursor();

    //### JAS Is mShowTimer a KDE thing.  I can't find this member.
    //### JAS Disabling this code, probably not needed.
    //&&&    mShowTimer->stop();
    m_frozen = true;
}

void ProgressDialog::slotThaw()
{
    RG_DEBUG << "ProgressDialog::slotThaw()\n";

    if (m_wasVisible) {
        if (m_modal)
            m_modalVisible = true;
        show();
    }

    // Restart timer
    //### JAS Is mShowTimer a KDE thing.  I can't find this member.
    //### JAS Disabling this code, probably not needed.
    //&&& mShowTimer->start(minimumDuration());
    m_frozen = false;
    m_chrono.restart();
}

void ProgressDialog::processEvents()
{
    //    RG_DEBUG << "ProgressDialog::processEvents: modalVisible is "
    //	     << m_modalVisible << endl;
    if (m_modalVisible) {
        qApp->processEvents(QEventLoop::AllEvents, 50); //@@@ JAS added AllEvents
    } else {
        rgapp->refreshGUI(50);
    }
}

}
#include "ProgressDialog.moc"
