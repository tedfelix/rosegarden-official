/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[CountdownDialog]"

#include "CountdownDialog.h"
#include <QLayout>

#include "CountdownBar.h"
#include <qshortcut.h>
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QHBoxLayout>
#include "misc/Debug.h"


namespace Rosegarden
{

CountdownDialog::CountdownDialog(QWidget *parent, int seconds):
		QDialog(parent, Qt::WindowStaysOnTopHint | Qt::Dialog ),
        m_pastEndMode(false),
        m_totalTime(seconds),
        m_progressBarWidth(150),
        m_progressBarHeight(15)

{
    setContentsMargins(10, 10, 10, 10);
    QBoxLayout *layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    layout->setSpacing(14);

    setWindowTitle(tr("Recording..."));

    QWidget *hBox = new QWidget(this);
    QHBoxLayout *hBoxLayout = new QHBoxLayout;
    m_label = new QLabel( hBox );
    hBoxLayout->addWidget(m_label);
    m_time = new QLabel( hBox );
    hBoxLayout->addWidget(m_time);
    hBox->setLayout(hBoxLayout);

    layout->addWidget(hBox, 0, Qt::AlignCenter);

    m_label->setText(tr("Recording time remaining:  "));
    m_progressBar =
        new CountdownBar(this, m_progressBarWidth, m_progressBarHeight);

    m_progressBar->setFixedSize(m_progressBarWidth, m_progressBarHeight);

    // Simply re-emit from Stop button
    //
    m_stopButton = new QPushButton(tr("Stop"), this);
    m_stopButton->setFixedWidth(60);

    layout->addWidget(m_progressBar, 0, Qt::AlignCenter);
    layout->addWidget(m_stopButton, 0, Qt::AlignRight);
    setLayout(layout);

    connect (m_stopButton, &QAbstractButton::released, this, &CountdownDialog::stopped);

    // Set the total time to show the bar in initial position
    //
    setElapsedTime(0);

    m_shortcuts = new QShortcut(this);

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
        m_time->setText(tr("Just how big is your hard disk?"));
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
	    RG_DEBUG << "CountdownDialog::setElapsedTime: FAILSAFE CODE FIRED, see bug #1838190 for details";
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
    m_label->setText(tr("Recording beyond end of composition:  "));

}

}
