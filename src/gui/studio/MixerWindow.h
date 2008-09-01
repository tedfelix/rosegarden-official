
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_MIXERWINDOW_H_
#define _RG_MIXERWINDOW_H_

#include "base/MidiProgram.h"
#include <kmainwindow.h>


class QWidget;
class QCloseEvent;
class QShortcut;


namespace Rosegarden
{

class Studio;
class RosegardenGUIDoc;


class MixerWindow: public KMainWindow
{
    Q_OBJECT

public:
    MixerWindow(QWidget *parent, RosegardenGUIDoc *document);
    QShortcut* getShortcuterators() { return m_shortcuterators; }

signals:
    void closing();
    void windowActivated();

protected slots:
    void slotClose();

protected:
    virtual void closeEvent(QCloseEvent *);
    virtual void windowActivationChange(bool);

    virtual void sendControllerRefresh() = 0;

    RosegardenGUIDoc *m_document;
    Studio *m_studio;
    InstrumentId m_currentId;

    QShortcut *m_shortcuterators;

};


}

#endif
