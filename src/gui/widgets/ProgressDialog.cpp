/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
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
#include "ProgressBar.h"

#include "misc/Debug.h"
#include "gui/application/RosegardenApplication.h"

#include <QCursor>
#include <QDialog>
#include <QString>
#include <QTimer>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>

#include <iostream>

namespace Rosegarden
{


bool ProgressDialog::m_modalVisible = false;

ProgressDialog::ProgressDialog(const QString &labelText,
                               int totalSteps,
                               QWidget *parent,
                               bool modal) :
        QDialog(parent),
        m_wasVisible(false),
        m_frozen(false),
        m_modal(modal),
        m_minimumDuration(500)
{
    RG_DEBUG << "ProgressDialog::ProgressDialog - " << labelText << " - modal : " << modal << endl;

    setWindowTitle(tr("Rosegarden"));
    setModal(modal);

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *info = new QLabel(tr("<qt><h3>Processing...</h3></qt>"));
    layout->addWidget(info);

    QGroupBox *box = new QGroupBox;
    layout->addWidget(box);

    QVBoxLayout *boxLayout = new QVBoxLayout;
    box->setLayout(boxLayout);

    m_label = new QLabel(labelText);
    boxLayout->addWidget(m_label);

    m_progressBar = new ProgressBar(totalSteps);
    boxLayout->addWidget(m_progressBar);
    m_progressBar->setValue(50); // debug

    connect(m_progressBar, SIGNAL(valueChanged (int)),
            this, SLOT(slotCheckShow(int)));

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Cancel);
    layout->addWidget(bb);
    bb->setCenterButtons(true);

    connect(bb, SIGNAL(rejected()), this, SLOT(cancel()));

    setMinimumDuration(1000);
    m_timer = new QTimer;
    m_timer->start(minimumDuration());
    connect(m_timer, SIGNAL(timeout()), this, SLOT(accept()));
    m_chrono.start();

    CurrentProgressDialog::set(this);
    show();
}

ProgressDialog::~ProgressDialog()
{
    RG_DEBUG << "~ProgressDialog()" << endl;
    m_modalVisible = false;
    delete m_timer;
}

void ProgressDialog::hideEvent(QHideEvent* e)
{
    std::cerr << "ProgressDialog::hideEvent() from " << std::endl;
    show();
/*    
    if (!allowCancel())
        QApplication::restoreOverrideCursor();

    KProgressDialog::hideEvent(e);
    m_modalVisible = false;
*/
}

ProgressBar*
ProgressDialog::progressBar()
{
    return m_progressBar;
}

void
ProgressDialog::slotSetOperationName(QString name)
{
    RG_DEBUG << "ProgressDialog::slotSetOperationName("
             << name << ") visible : " << isVisible() << endl;

    m_label->setText(name);
    // Little trick stolen from QProgressDialog
    // increase resize only, never shrink
    int w = QMAX( isVisible() ? width() : 0, sizeHint().width() );
    int h = QMAX( isVisible() ? height() : 0, sizeHint().height() );
    resize( w, h );
}

void ProgressDialog::cancel()
{
    RG_DEBUG << "ProgressDialog::slotCancel()\n";
    emit cancel();
    slotFreeze();
    reject();
}

void ProgressDialog::slotCheckShow(int)
{
    RG_DEBUG << "ProgressDialog::slotCheckShow() : "
             << m_chrono.elapsed() << " - " << minimumDuration()
             << endl;

    if (!isVisible() &&
        !m_frozen &&
        m_chrono.elapsed() > minimumDuration()) {

        RG_DEBUG << "ProgressDialog::slotCheckShow() : showing dialog\n";
        show();
        if (m_modal) m_modalVisible = true;
        processEvents();
    }
}

void ProgressDialog::slotFreeze()
{
    RG_DEBUG << "ProgressDialog::slotFreeze()\n";

    /*
    m_wasVisible = isVisible();
    if (isVisible()) {
        m_modalVisible = false;
        hide();
    }*/

    // This is also a convenient place to ensure the wait cursor (if
    // currently shown) returns to the original cursor to ensure that
    // the user can respond to whatever's freezing the progress dialog
    QApplication::restoreOverrideCursor();

    m_chrono.restart();
    m_timer->stop();
    m_frozen = true;
}

void ProgressDialog::slotThaw()
{
    RG_DEBUG << "ProgressDialog::slotThaw()\n";

    if (m_wasVisible) {
        if (m_modal) m_modalVisible = true;
        show();
    }

    // Restart timer
    m_timer->start(minimumDuration());
    m_frozen = false;
    m_chrono.restart();
}

void ProgressDialog::processEvents()
{
/*
    //    RG_DEBUG << "ProgressDialog::processEvents: modalVisible is "
    //	     << m_modalVisible << endl;
    if (m_modalVisible) {
        kapp->processEvents(50);
    } else {
        rgapp->refreshGUI(50);
    }
*/
}

void
ProgressDialog::setAutoClose(bool state)
{
    if (state) RG_DEBUG << "Something set ProgressDialog::setAutoClose(true)" << endl;
}

void
ProgressDialog::setAutoReset(bool state)
{
    if (state) RG_DEBUG << "Something set ProgressDialog::setAutoReset(true)" << endl;
}

void
ProgressDialog::setValue(int value)
{
    m_progressBar->setValue(value);
}

void
ProgressDialog::setMinimumDuration(int duration)
{
    m_minimumDuration = duration;
}

int
ProgressDialog::minimumDuration()
{
    return m_minimumDuration;
}

void
ProgressDialog::setLabelText(QString text)
{
    slotSetOperationName(text);
}

}
#include "ProgressDialog.moc"
