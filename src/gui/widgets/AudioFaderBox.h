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

#ifndef RG_AUDIOFADERBOX_H
#define RG_AUDIOFADERBOX_H

#include "gui/widgets/PluginPushButton.h"

#include <QFrame>
#include <QPixmap>
#include <QString>

#include <vector>


class QWidget;
class QSignalMapper;
class QPushButton;
class QObject;
class QLabel;


namespace Rosegarden
{

class Studio;
class Rotary;
class Instrument;
class Fader;
class AudioVUMeter;
class AudioRouteMenu;
class PluginPushButton;


class AudioFaderBox : public QFrame
{
    Q_OBJECT

public:
    AudioFaderBox(QWidget *parent,
                  QString id = "",
                  bool haveInOut = true,
                  const char *name = nullptr);

    void setAudioChannels(int);

    void setIsSynth(bool);

    bool owns(const QObject *object);

    void setFont(QFont);

    PluginPushButton        *m_synthButton;
    std::vector<PluginPushButton*>  m_plugins;

    AudioVUMeter            *m_vuMeter;

    Fader                   *m_fader;
    Fader                   *m_recordFader;
    Rotary                  *m_pan;

    QPixmap                  m_monoPixmap;
    QPixmap                  m_stereoPixmap;

    QSignalMapper           *m_signalMapper;

    QLabel                  *m_inputLabel;
    QLabel                  *m_outputLabel;

    AudioRouteMenu          *m_audioInput; 
    AudioRouteMenu          *m_audioOutput; 

    QPushButton             *m_synthGUIButton;

    QString                  m_id;

    bool                     isStereo() const { return m_isStereo; }

signals:
    void audioChannelsChanged(int);

public slots:
    void slotSetInstrument(Studio *studio,
                           Instrument *instrument);

protected slots:
    void slotChannelStateChanged();

protected:
    QPushButton               *m_stereoButton;
    bool                       m_isStereo;
};



}

#endif
