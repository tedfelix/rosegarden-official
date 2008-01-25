/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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

#ifdef HAVE_LIBLO

#include "AudioPluginOSCGUI.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "base/Exception.h"
#include "sound/PluginIdentifier.h"
#include <kprocess.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qstring.h>
#include <algorithm>


namespace Rosegarden
{

AudioPluginOSCGUI::AudioPluginOSCGUI(AudioPluginInstance *instance,
                                     QString serverURL, QString friendlyName) :
        m_gui(0),
        m_address(0),
        m_basePath(""),
        m_serverUrl(serverURL)
{
    QString identifier = strtoqstr(instance->getIdentifier());

    QString filePath = getGUIFilePath(identifier);
    if (!filePath) {
        throw Exception("No GUI found");
    }

    QString type, soName, label;
    PluginIdentifier::parseIdentifier(identifier, type, soName, label);
    QFileInfo soInfo(soName);

    // arguments: osc url, dll name, label, instance tag

    m_gui = new KProcess();

    *m_gui << filePath
    << m_serverUrl
    << soInfo.fileName()
    << label
    << friendlyName;

    RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: Starting process "
    << filePath << " " << m_serverUrl << " "
    << soInfo.fileName() << " " << label << " " << friendlyName << endl;

    if (!m_gui->start(KProcess::NotifyOnExit, KProcess::NoCommunication)) {
        RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: Couldn't start process " << filePath << endl;
        delete m_gui;
        m_gui = 0;
        throw Exception("Failed to start GUI");
    }
}

AudioPluginOSCGUI::~AudioPluginOSCGUI()
{
    quit();
}

QString
AudioPluginOSCGUI::getGUIFilePath(QString identifier)
{
    QString type, soName, label;
    PluginIdentifier::parseIdentifier(identifier, type, soName, label);

    RG_DEBUG << "AudioPluginOSCGUI::getGUIFilePath(" << identifier << ")" << endl;

    QFileInfo soInfo(soName);
    if (soInfo.isRelative()) {
        //!!!
        RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: Unable to deal with relative .so path \"" << soName << "\" in identifier \"" << identifier << "\" yet" << endl;
        throw Exception("Can't deal with relative .soname");
    }

    QDir dir(soInfo.dir());
    QString fileBase(soInfo.baseName(TRUE));

    if (!dir.cd(fileBase)) {
        RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: No GUI subdir for plugin .so " << soName << endl;
        throw Exception("No GUI subdir available");
    }

    const QFileInfoList *list = dir.entryInfoList();

    // in order of preference:
    const char *suffixes[] = { "_rg", "_kde", "_qt", "_gtk2", "_gtk", "_x11", "_gui"
                             };
    int nsuffixes = sizeof(suffixes) / sizeof(suffixes[0]);

    for (int k = 0; k <= nsuffixes; ++k) {

        for (int fuzzy = 0; fuzzy <= 1; ++fuzzy) {

            QFileInfoListIterator i(*list);
            QFileInfo *info;

            while ((info = i.current()) != 0) {

                RG_DEBUG << "Looking at " << info->fileName() << " in path "
                << info->filePath() << " for suffix " << (k == nsuffixes ? "(none)" : suffixes[k]) << ", fuzzy " << fuzzy << endl;

                ++i;

                if (!(info->isFile() || info->isSymLink())
                        || !info->isExecutable()) {
                    RG_DEBUG << "(not executable)" << endl;
                    continue;
                }

                if (fuzzy) {
                    if (info->fileName().left(fileBase.length()) != fileBase)
                        continue;
                    RG_DEBUG << "(is file base)" << endl;
                } else {
                    if (info->fileName().left(label.length()) != label)
                        continue;
                    RG_DEBUG << "(is label)" << endl;
                }

                if (k == nsuffixes || info->fileName().lower().endsWith(suffixes[k])) {
                    RG_DEBUG << "(ends with suffix " << (k == nsuffixes ? "(none)" : suffixes[k]) << " or out of suffixes)" << endl;
                    return info->filePath();
                }
                RG_DEBUG << "(doesn't end with suffix " << (k == nsuffixes ? "(none)" : suffixes[k]) << ")" << endl;
            }
        }
    }

    return QString();
}

void
AudioPluginOSCGUI::setGUIUrl(QString url)
{
    if (m_address)
        lo_address_free(m_address);

    char *host = lo_url_get_hostname(url);
    char *port = lo_url_get_port(url);
    m_address = lo_address_new(host, port);
    free(host);
    free(port);

    m_basePath = lo_url_get_path(url);
}

void
AudioPluginOSCGUI::show()
{
    RG_DEBUG << "AudioPluginOSCGUI::show" << endl;

    if (!m_address)
        return ;
    QString path = m_basePath + "/show";
    lo_send(m_address, path, "");
}

void
AudioPluginOSCGUI::hide()
{
    if (!m_address)
        return ;
    QString path = m_basePath + "/hide";
    lo_send(m_address, path, "");
}

void
AudioPluginOSCGUI::quit()
{
    if (!m_address)
        return ;
    QString path = m_basePath + "/quit";
    lo_send(m_address, path, "");
}

void
AudioPluginOSCGUI::sendProgram(int bank, int program)
{
    if (!m_address)
        return ;
    QString path = m_basePath + "/program";
    lo_send(m_address, path, "ii", bank, program);
}

void
AudioPluginOSCGUI::sendPortValue(int port, float value)
{
    if (!m_address)
        return ;
    QString path = m_basePath + "/control";
    lo_send(m_address, path, "if", port, value);
}

void
AudioPluginOSCGUI::sendConfiguration(QString key, QString value)
{
    if (!m_address)
        return ;
    QString path = m_basePath + "/configure";
    lo_send(m_address, path, "ss", key.data(), value.data());
}

}

#endif
