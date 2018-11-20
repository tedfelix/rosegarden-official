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

#ifndef RG_MIXERWINDOW_H
#define RG_MIXERWINDOW_H

#include "base/Instrument.h"
#include "base/MidiProgram.h"

#include <QMainWindow>


class QWidget;
class QCloseEvent;
class QShortcut;


namespace Rosegarden
{

class Studio;
class RosegardenDocument;


class MixerWindow: public QMainWindow
{
    Q_OBJECT

public:
    MixerWindow(QWidget *parent, RosegardenDocument *document);
    QShortcut* getShortcuts() { return m_shortcuts; }

signals:
    void closing();
    void windowActivated();

protected slots:
    void slotClose();

protected:
    void closeEvent(QCloseEvent *) override;
    virtual void windowActivationChange(bool);

    /// Send MIDI volume and pan messages to the "external controller" port.
    /**
     * This is called when a Mixer window (MIDI or Audio) is activated by
     * the user.  It allows the device connected to the "external controller"
     * port to stay in sync with whichever Mixer window is active.
     */
    virtual void sendControllerRefresh() = 0;

    RosegardenDocument *m_document;
    Studio *m_studio;
    InstrumentId m_currentId;

    QShortcut *m_shortcuts;

};


}

#endif
