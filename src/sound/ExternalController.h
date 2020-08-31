/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
  Rosegarden
  A sequencer and musical notation editor.
  Copyright 2020 the Rosegarden development team.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#pragma once

#include <QObject>
#include <QSharedPointer>


namespace Rosegarden
{


class MappedEvent;


/// Support for the "external controller" port.
class ExternalController : public QObject
{
    Q_OBJECT

public:
    // The global instance.
    static QSharedPointer<ExternalController> self();

    /// The three windows that currently handle external controller events.
    enum Window { Main, AudioMixer, MidiMixer };
    /// The currently active window for external controller events.
    /**
     * External controller events are forwarded to the currently active
     * window.
     *
     * Set by the three windows that can handle external controller events.
     */
    Window m_activeWindow;

    /// Handle MappedEvent's from the external controller port.
    /**
     * This routine handles remote control events received from a
     * device connected to the "external controller" port.
     *
     * This routine handles controller 81 which opens or
     * brings to the top various windows based on the value.
     *
     * All other events are forwarded to the active window.
     * See m_activeWindow.
     */
    void processEvent(const MappedEvent *event);

signals:
    /// External controller port events for the windows that handle them.
    /**
     * ??? Split into three signals, one for each window.
     */
    void externalController(const MappedEvent *event,
                            Window m_activeWindow);

private:
    // Access through self() only.
    ExternalController();

};


}
