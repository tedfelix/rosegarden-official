
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

#ifndef RG_AUDIOPLUGINOSCGUI_H
#define RG_AUDIOPLUGINOSCGUI_H

#include <lo/lo.h>

#include <QString>


class QProcess;


namespace Rosegarden
{

class AudioPluginInstance;


class AudioPluginOSCGUI
{
public:
    AudioPluginOSCGUI(AudioPluginInstance *instance,
                      QString serverURL, QString friendlyName);
    virtual ~AudioPluginOSCGUI();

    void setGUIUrl(QString url);

    void show();
    void hide();
    void quit();
    void sendProgram(int bank, int program);
    void sendPortValue(int port, float value);
    void sendConfiguration(QString key, QString value);

    static QString getGUIFilePath(QString identifier);

protected:
    QProcess *m_gui;
    lo_address m_address;
    QString m_basePath;
    QString m_serverUrl;
};
    

}


#endif

