/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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


#include "CountdownDialog.h"
#include <qlayout.h>

#include <klocale.h>
#include "CountdownBar.h"
#include <qaccel.h>
#include <qdialog.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qwidget.h>
#include "misc/Debug.h"


namespace Rosegarden
{

CountdownDialog::CountdownDialog(QWidget *parent, int seconds):
        QDialog(parent, "", false, WStyle_StaysOnTop | WStyle_DialogBorder),
        m_pastEndMode(false),
        m_totalTime(seconds),
        m_progressBarWidth(150),
        m_progressBarHeight(15)
{
    QBoxLayout *layout = new QBoxLayout(this, QBoxLayout::TopToBottom, 10, 14);
    setCaption(i18n("Recording..."));

    QHBox *hBox = new QHBox(this);
    m_label = new QLabel(hBox);
    m_time = new QLabel(hBox);

    layout->addWidget(hBox, 0, AlignCenter);

    m_label->setText(i18n("Recording time remaining:  "));
    m_progressBar =
        new CountdownBar(this, m_progressBarWidth, m_progressBarHeight);

    m_progressBar->setFixedSize(m_progressBarWidth, m_progressBarHeight);

    // Simply re-emit from Stop button
    //
    m_stopButton = new QPushButton(i18n("Stop"), this);
    m_stopButton->setFixedWidth(60);

    layout->addWidget(m_progressBar, 0, AlignCenter);
    layout->addWidget(m_stopButton, 0, AlignRight);

    connect (m_stopButton, SIGNAL(released()), this, SIGNAL(stopped()));

    // Set the total time to show the bar in initial position
    //
    setElapsedTime(0);

    m_accelerators = new QAccel(this);

}

void
CountdownDialog::setLabel(const QString &label)
{
    m_label->setText(label);
}

void
CountdownDialog::setTotalTime(int seconds)
{
    m_totalTime = seconds;
    setElapsedTime(0); // clear
}

void
CountdownDialog::setElapsedTime(int elapsedSeconds)
{
    int seconds = m_totalTime - elapsedSeconds;

    if (seconds < 0) {
        seconds = - seconds;
        if (!m_pastEndMode)
            setPastEndMode();
    }

    QString h, m, s;
    h.sprintf("%02d", seconds / 3600);
    m.sprintf("%02d", seconds / 60);
    s.sprintf("%02d", seconds % 60);

    if (seconds < 3600) // less than an hour
    {
        m_time->setText(QString("%1:%2").arg(m).arg(s));
    } else if (seconds < 86400) // less than a day
    {
        m_time->setText(QString("%1:%2:%3").arg(h).arg(m).arg(s));
    } else {
        m_time->setText(i18n("Just how big is your hard disk?"));
    }

    // Draw the progress bar
    //
    if (m_pastEndMode) {
        m_progressBar->setPosition(m_progressBarWidth);
    } else {
        // Attempt a simplistic fix for #1838190.  In the context of an isolated
	// test example, I'm fairly sure m_totalTime was 0, causing a divide by
	// zero error, though the trace just listed it as an "Arithmetic
	// exception."
        if (m_totalTime == 0) {
	    RG_DEBUG << "CountdownDialog::setElapsedTime: FAILSAFE CODE FIRED, see bug #1838190 for details" << endl;
	    m_totalTime = 1;
	}
        int barPosition = m_progressBarWidth -
                          (elapsedSeconds * m_progressBarWidth) / m_totalTime;
        m_progressBar->setPosition(barPosition);
    }

    // Dialog complete if the display time is zero
    if (seconds == 0)
        emit completed();

}

void
CountdownDialog::setPastEndMode()
{
    if (m_pastEndMode) // already called
        return ;

    m_pastEndMode = true;
    m_label->setText(i18n("Recording beyond end of composition:  "));

}

}
#include "CountdownDialog.moc"
