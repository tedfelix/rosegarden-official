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
        m_minimumTimeHasExpired(false),
        m_minimumDuration(500)
{
    RG_DEBUG << "ProgressDialog::ProgressDialog - " << labelText << " - modal : " << modal << endl;
    setObjectName("My name is Bootney Lee, and I am a ProgressDialog with magnificent hair!");

    setWindowTitle(tr("Rosegarden"));
    setModal(modal);
    setAttribute(Qt::WA_DeleteOnClose);

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

    connect(m_progressBar, SIGNAL(valueChanged(int)),
            this, SLOT(slotCheckShow(int)));

    QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Cancel);
    layout->addWidget(bb);
    bb->setCenterButtons(true);

    connect(bb, SIGNAL(rejected()), this, SLOT(cancel()));

    setMinimumDuration(1000);
    m_timer = new QTimer;
    m_timer->start(minimumDuration());
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotMinimumTimeElapsed()));
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

void
ProgressDialog::hideEvent(QHideEvent* e)
{
    if (m_minimumTimeHasExpired) {
        RG_DEBUG << "ProgressDialog::hideEvent() - minimum time has elapsed.  Hiding..." << endl;
        QDialog::hideEvent(e);
        m_modalVisible = false;
    } else {
        RG_DEBUG << "ProgressDialog::hideEvent() - minimum time has not elapsed.  Ignoring hide event..." << endl;
        m_modalVisible = true;
    }
}

void
ProgressDialog::slotSetOperationName(QString name)
{
    RG_DEBUG << "ProgressDialog::slotSetOperationName(" << name << ") visible : " << isVisible() << endl;

    m_label->setText(name);
    // increase resize only, never shrink
    int w = QMAX( isVisible() ? width() : 0, sizeHint().width() );
    int h = QMAX( isVisible() ? height() : 0, sizeHint().height() );
    resize( w, h );
}

void
ProgressDialog::cancel()
{
    RG_DEBUG << "ProgressDialog::slotCancel()\n";
    emit canceled();
    slotFreeze();
    reject();
}

void
ProgressDialog::slotCheckShow(int)
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

void
ProgressDialog::slotMinimumTimeElapsed()
{
    RG_DEBUG << "ProgressDialog::slotMinimumTimeElapsed() - the QTimer has reached the minimum duration set in the ctor." << endl;
    m_minimumTimeHasExpired = true;
}

void
ProgressDialog::slotFreeze()
{
    RG_DEBUG << "ProgressDialog::slotFreeze()\n";

    m_wasVisible = isVisible();
    if (isVisible()) {
        m_modalVisible = false;
        hide();
    }

    m_chrono.restart();
    m_timer->stop();
    m_frozen = true;
}

void
ProgressDialog::slotThaw()
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

void
ProgressDialog::processEvents()
{

    RG_DEBUG << "ProgressDialog::processEvents: modalVisible is "
             << m_modalVisible << endl;
    if (m_modalVisible) {
        qApp->processEvents(QEventLoop::AllEvents, 50);
    } else {
        rosegardenApplication->refreshGUI(50);
    }
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
    std::cout << "ProgressDialog::setValue(" << value << ")" << std::endl;
    m_progressBar->hide();
    m_progressBar->setValue(value);
    m_progressBar->show();
}

void
ProgressDialog::setProgress(int value)
{
    std::cout << "ProgressDialog::setProgress(" << value << ") calling setValue()" << std::endl;
}

void
ProgressDialog::incrementProgress(int value)
{
    std::cout << "ProgressDialog::incrementProgress(" << value << ") calling setValue()" << std::endl;
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
