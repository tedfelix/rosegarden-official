/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOPLUGINCONNECTIONDIALOG_H
#define RG_AUDIOPLUGINCONNECTIONDIALOG_H

#include "base/AudioPluginInstance.h"
#include "base/Studio.h"


#include <QDialog>
class QComboBox;

#include <vector>


namespace Rosegarden
{


/// The "Audio Plugin Connections" dialog.
class AudioPluginConnectionDialog : public QDialog
{
    Q_OBJECT

 public:
    AudioPluginConnectionDialog
        (QWidget *parent,
         const PluginPort::ConnectionList& connections);

    void getConnections
        (PluginPort::ConnectionList& connections) const;

 public slots:
    void slotInstrumentChanged(int index);

 private:
    void setupChannelCB(int connectionIndex,
                        int instrumentIndex);

    InstrumentList m_iListAudio;
    InstrumentList m_iListAudioSynth;
    std::vector<QString> m_pluginPorts;
    std::vector<QComboBox*> m_instrumentCB;
    std::vector<QComboBox*> m_channelCB;
    PluginPort::ConnectionList m_connections;
    std::vector<bool> m_fixedInstrument;
};


}

#endif
