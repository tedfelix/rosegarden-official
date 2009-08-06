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

#include "WarningWidget.h"

#include "gui/general/IconLoader.h"
#include "misc/Strings.h"

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QToolButton>

#include <iostream>
namespace Rosegarden
{

WarningWidget::WarningWidget() :
        QWidget(),
        m_text(""),
        m_informativeText(""),
        m_warningDialog(new WarningDialog())
{
    std::cerr << "WarningWidget()" << std::endl;

    setContentsMargins(0, 0, 0, 0);
    
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    QHBoxLayout *layout = new QHBoxLayout();
    setLayout(layout);

    m_midiIcon = new QLabel();
    layout->addWidget(m_midiIcon);

    m_audioIcon = new QLabel();
    layout->addWidget(m_audioIcon);

    m_timerIcon = new QLabel();
    layout->addWidget(m_timerIcon);

    m_warningButton = new QToolButton();
    layout->addWidget(m_warningButton);
    m_warningButton->setIconSize(QSize(16, 16));
    m_warningButton->setIcon(IconLoader().loadPixmap("warning"));
    connect(m_warningButton,
            SIGNAL(clicked()),
            this,
            SLOT(displayMessageQueue()));
    m_warningButton->setToolTip(tr("<qt>Runtime problems detected!<br>Click to display details</qt>"));
    m_warningButton->hide();

    // Set these to false initially, assuming an all clear state.  When some
    // problem crops up, these will be set true as appropriate by
    // RosegardenMainWindow, which manages this widget
    setMidiWarning(false);
    setAudioWarning(false);
    setTimerWarning(false);
}

WarningWidget::~WarningWidget()
{
}

void
WarningWidget::setMidiWarning(const bool status)
{
    if (status) {
        m_midiIcon->hide();
    } else {
        m_midiIcon->setPixmap(IconLoader().loadPixmap("midi-ok"));
        m_midiIcon->show();
        m_midiIcon->setToolTip(tr("MIDI OK"));
    }
}

void
WarningWidget::setAudioWarning(const bool status)
{
    if (status) {
        m_audioIcon->hide();
    } else {
        m_audioIcon->setPixmap(IconLoader().loadPixmap("audio-ok"));
        m_audioIcon->show();
        m_audioIcon->setToolTip(tr("audio OK"));
    }
}

void
WarningWidget::setTimerWarning(const bool status)
{
    if (status) {
        m_timerIcon->hide();
    } else {
        m_timerIcon->setPixmap(IconLoader().loadPixmap("timer-ok"));
        m_timerIcon->show();
        m_timerIcon->setToolTip(tr("timer OK"));
    }
}

void
WarningWidget::queueMessage(const QString text, const QString informativeText)
{
    std::cerr << "WarningWidget::queuetMessage(" << qstrtostr(text)
              << ", " << qstrtostr(informativeText)
              << ")" << std::endl;
    m_warningButton->show();

    Message message(text, informativeText);

    m_queue.enqueue(message);
}

void
WarningWidget::displayMessageQueue()
{
    std::cerr << "WarningWidget::displayMessageQueue()" << std::endl;

//    m_warningDialog->exec();

    while (!m_queue.isEmpty()) {
        std::cerr << " - emptying queue..." << std::endl;
        m_warningDialog->addWarning(m_queue.dequeue());
    }    
    m_warningDialog->show();
}

// seems to be worthless, but I'll leave it in case I have new inspiration later
QSize
WarningWidget::sizeHint()
{
    return QSize(128, 16);
}

// NOTES:
// Potential future warnings:
//
// * Chris's newly refactored autoconnect logic couldn't find a plausible looking
// thing to talk to, so you probably need to run QSynth now &c.
//
}
#include "WarningWidget.moc"
