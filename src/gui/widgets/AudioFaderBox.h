
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _RG_AUDIOFADERBOX_H_
#define _RG_AUDIOFADERBOX_H_

#include <qframe.h>
#include <qpixmap.h>
#include <qstring.h>
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


class AudioFaderBox : public QFrame
{
    Q_OBJECT

public:
    AudioFaderBox(QWidget *parent,
		  QString id = "",
		  bool haveInOut = true,
		  const char *name = 0);

    void setAudioChannels(int);

    void setIsSynth(bool);

    bool owns(const QObject *object);

    QPushButton               *m_synthButton;
    std::vector<QPushButton*>  m_plugins;

    AudioVUMeter              *m_vuMeter;

    Fader           *m_fader;
    Fader           *m_recordFader;
    Rotary          *m_pan;

    QPixmap                    m_monoPixmap;
    QPixmap                    m_stereoPixmap;

    QSignalMapper             *m_signalMapper;

    QLabel                    *m_inputLabel;
    QLabel                    *m_outputLabel;

    AudioRouteMenu            *m_audioInput; 
    AudioRouteMenu            *m_audioOutput; 

    QPushButton               *m_synthGUIButton;

    QString                    m_id;

    bool                       isStereo() const { return m_isStereo; }

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
