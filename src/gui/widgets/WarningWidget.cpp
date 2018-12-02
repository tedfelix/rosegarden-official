/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[WarningWidget]"

#include "WarningWidget.h"

#include "gui/general/IconLoader.h"
#include "misc/Strings.h"
#include "misc/Debug.h"

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMessageBox>

#include <iostream>
namespace Rosegarden
{

WarningWidget::WarningWidget(QWidget *parent) :
    QWidget(parent),
    m_text(""),
    m_informativeText(""),
    m_warningDialog(new WarningDialog(parent))
{
    setContentsMargins(0, 0, 0, 0);
    
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    QHBoxLayout *layout = new QHBoxLayout();
    setLayout(layout);
    layout->setMargin(0);
    layout->setSpacing(2);

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
            &QAbstractButton::clicked,
            this,
            &WarningWidget::displayMessageQueue);
    m_warningButton->setToolTip(tr("<qt><p>Performance problems detected!</p><p>Click to display details</p></qt>"));
    m_warningButton->hide();

    m_graphicsButton = new QToolButton();
    layout->addWidget(m_graphicsButton);
    m_graphicsButton->setIconSize(QSize(16, 16));
    m_graphicsButton->setIcon(IconLoader().loadPixmap("safe-graphics"));
    connect(m_graphicsButton,
            &QAbstractButton::clicked,
            this,
            &WarningWidget::displayGraphicsAdvisory);
    m_graphicsButton->hide();

    m_infoButton = new QToolButton();
    layout->addWidget(m_infoButton);
    m_infoButton->setIconSize(QSize(16, 16));
    m_infoButton->setIcon(IconLoader().loadPixmap("messagebox-information"));
    connect(m_infoButton,
            &QAbstractButton::clicked,
            this,
            &WarningWidget::displayMessageQueue);
    m_infoButton->setToolTip(tr("<qt><p>Information available.</p><p>Click to display details</p></qt>"));
    m_infoButton->hide();

    // Set these to false initially, assuming an all clear state.  When some
    // problem crops up, these will be set true as appropriate by
    // RosegardenMainWindow, which manages this widget
    setMidiWarning(false);
    setAudioWarning(false);
    setTimerWarning(false);
    setGraphicsAdvisory(false);
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
WarningWidget::setGraphicsAdvisory(const bool status)
{
    if (status) {
        m_graphicsButton->show();
        m_graphicsButton->setToolTip(tr("<qt>Safe graphics mode<br>Click for more information</qt>"));
    } else {
        m_graphicsButton->hide();
    }
}

void
WarningWidget::queueMessage(const int type, const QString text, const QString informativeText)
{
    RG_DEBUG << "WarningWidget::queueMessage(" << text
             << ", " << informativeText << ")" << endl;

    // we'll go ahead and splay this out in a big'ol switch in case there are
    // ever other warning types that have special icons
    switch (type) {
    case Info:
        m_infoButton->show();
        break;
    case Midi:
    case Audio:
    case Timer:
    case Other:
    default:
        m_warningButton->show();
    }

    // this is all a bit awkard, but there's no std::triplet and I don't want to
    // convert this all over to a vector or something, so I just nested a
    // std::pair in a std::pair, and I can't be bothered to typedef the
    // sub-component bit here
    std::pair<QString, QString> m;
    m.first = text;
    m.second = informativeText;
    
    Message message(m, type);

    m_queue.enqueue(message);
}

void
WarningWidget::displayMessageQueue()
{
    while (!m_queue.isEmpty()) {
        std::cerr << " - emptying queue..." << std::endl;
        m_warningDialog->addWarning(m_queue.dequeue());
    }
    m_warningDialog->show();
}

void
WarningWidget::displayGraphicsAdvisory()
{
    QMessageBox::information(this, 
                             tr("Rosegarden"),
                             tr("<qt><p>Rosegarden is using safe graphics mode.  This provides the greatest stability, but graphics performance is very slow.</p><p>You may wish to visit <b>Edit -> Preferences -> Behavior -> Graphics performance</b> and try \"Normal\" or \"Fast\" for better performance.</p></qt>"));
}

// NOTES:
// Potential future warnings:
//
// * Chris's newly refactored autoconnect logic couldn't find a plausible looking
// thing to talk to, so you probably need to run QSynth now &c.
//
}
