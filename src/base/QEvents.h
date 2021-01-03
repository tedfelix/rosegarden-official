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

#pragma once

#include <QEvent>

namespace Rosegarden
{
    // QEvents from AlsaDriver to RosegardenMainWindow.
    // See RosegardenSequencer::getNextTransportRequest() and
    // SequencerDataBlock which also enable communication between the
    // two threads.
    constexpr QEvent::Type PreviousTrack = QEvent::User;
    constexpr QEvent::Type NextTrack = QEvent::Type(QEvent::User + 1);
    constexpr QEvent::Type Loop = QEvent::Type(QEvent::User + 2);
    constexpr QEvent::Type Rewind = QEvent::Type(QEvent::User + 3);
    constexpr QEvent::Type FastForward = QEvent::Type(QEvent::User + 4);
    constexpr QEvent::Type Stop = QEvent::Type(QEvent::User + 5);
    constexpr QEvent::Type AddMarker = QEvent::Type(QEvent::User + 6);
    constexpr QEvent::Type PreviousMarker = QEvent::Type(QEvent::User + 7);
    constexpr QEvent::Type NextMarker = QEvent::Type(QEvent::User + 8);

    class ButtonEvent : public QEvent
    {
    public:
        explicit ButtonEvent(QEvent::Type type, bool i_pressed) :
            QEvent(type),
            pressed(i_pressed)
        {
        }

        bool pressed;
    };
}
