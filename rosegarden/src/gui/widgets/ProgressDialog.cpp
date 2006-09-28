/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ProgressDialog.h"

#include <klocale.h>
#include "misc/Debug.h"
#include "CurrentProgressDialog.h"
#include <qapplication.h>
#include <qcursor.h>
#include <qprogressdialog.h>
#include <qstring.h>
#include <qwidget.h>


namespace Rosegarden
{

bool ProgressDialog::m_modalVisible = false;


ProgressDialog::ProgressDialog(QWidget *creator,
                               const char *name,
                               bool modal):
        KProgressDialog(creator, name,
                        i18n("Processing..."), QString::null, modal),
        m_wasVisible(false),
        m_frozen(false),
        m_modal(modal)
{
    setCaption(i18n("Processing..."));
    RG_DEBUG << "ProgressDialog::ProgressDialog type 1 - "
    << labelText() << " - modal : " << modal << endl;

    connect(progressBar(), SIGNAL(percentageChanged (int)),
            this, SLOT(slotCheckShow(int)));

    m_chrono.start();

    CurrentProgressDialog::set
        (this);

    setMinimumDuration(500); // set a default value for this
}

ProgressDialog::ProgressDialog(
    const QString &labelText,
    int totalSteps,
    QWidget *creator,
    const char *name,
    bool modal) :
        KProgressDialog(creator,
                        name,
                        i18n("Processing..."),
                        labelText,
                        modal),
        m_wasVisible(false),
        m_frozen(false),
        m_modal(modal)
{
    progressBar()->setTotalSteps(totalSteps);

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
    KProgressDialog::polish();

    if (allowCancel())
        setCursor(Qt::ArrowCursor);
    else
        QApplication::setOverrideCursor(QCursor(Qt::waitCursor));
}

void ProgressDialog::hideEvent(QHideEvent* e)
{
    if (!allowCancel())
        QApplication::restoreOverrideCursor();

    KProgressDialog::hideEvent(e);
    m_modalVisible = false;
}

void
ProgressDialog::slotSetOperationName(QString name)
{
    //     RG_DEBUG << "ProgressDialog::slotSetOperationName("
    //              << name << ") visible : " << isVisible() << endl;

    setLabel(name);
    // Little trick stolen from QProgressDialog
    // increase resize only, never shrink
    int w = QMAX( isVisible() ? width() : 0, sizeHint().width() );
    int h = QMAX( isVisible() ? height() : 0, sizeHint().height() );
    resize( w, h );
}

void ProgressDialog::slotCancel()
{
    RG_DEBUG << "ProgressDialog::slotCancel()\n";
    KProgressDialog::slotCancel();
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

    mShowTimer->stop();
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
    mShowTimer->start(minimumDuration());
    m_frozen = false;
    m_chrono.restart();
}

void ProgressDialog::processEvents()
{
    //    RG_DEBUG << "ProgressDialog::processEvents: modalVisible is "
    //	     << m_modalVisible << endl;
    if (m_modalVisible) {
        kapp->processEvents(50);
    } else {
        rgapp->refreshGUI(50);
    }
}

}
