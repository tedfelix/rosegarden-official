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

#define RG_MODULE_STRING "[AudioPluginOSCGUI]"

#include "AudioPluginOSCGUI.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "base/Exception.h"
#include "sound/PluginIdentifier.h"
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QString>
#include <algorithm>


namespace Rosegarden
{

AudioPluginOSCGUI::AudioPluginOSCGUI(AudioPluginInstance *instance,
                                     QString serverURL, QString friendlyName) :
        m_gui(nullptr),
        m_address(nullptr),
        m_basePath(""),
        m_serverUrl(serverURL)
{
    QString identifier = strtoqstr(instance->getIdentifier());

    QString filePath = getGUIFilePath(identifier);
    if ( filePath.isEmpty() ) {
        throw Exception("No GUI found");
    }

    QString type, soName, label;
    PluginIdentifier::parseIdentifier(identifier, type, soName, label);
    QFileInfo soInfo(soName);

    //setup osc process
    // arguments: osc url, dll name, label, instance tag

    m_gui = new QProcess();
    QStringList guiArgs;
   
    guiArgs << m_serverUrl
    << soInfo.fileName()
    << label
    << friendlyName;

    RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: Starting process "
    << filePath << " " << m_serverUrl << " "
    << soInfo.fileName() << " " << label << " " << friendlyName << endl;

    m_gui->start(filePath, guiArgs);
    if (!m_gui->waitForStarted()) {  //@@@ JAS Check here first for errors
        RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: Couldn't start process " << filePath;
        delete m_gui;
        m_gui = nullptr;
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

    RG_DEBUG << "AudioPluginOSCGUI::getGUIFilePath(" << identifier << ")";

    QFileInfo soInfo(soName);
    if (soInfo.isRelative()) {
        //!!!
        RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: Unable to deal with relative .so path \"" << soName << "\" in identifier \"" << identifier << "\" yet";
        throw Exception("Can't deal with relative .soname");
    }

    QDir dir(soInfo.dir());
    QString fileBase(soInfo.completeBaseName());

    if (!dir.cd(fileBase)) {
        RG_DEBUG << "AudioPluginOSCGUI::AudioPluginOSCGUI: No GUI subdir for plugin .so " << soName;
        throw Exception("No GUI subdir available");
    }

    QFileInfoList list = dir.entryInfoList();

    // in order of preference:
    const char *suffixes[] = { "_rg", "_kde", "_qt", "_gtk2", "_gtk", "_x11", "_gui"
                             };
    int nsuffixes = int(sizeof(suffixes) / sizeof(suffixes[0]));

    for (int k = 0; k <= nsuffixes; ++k) {

        for (int fuzzy = 0; fuzzy <= 1; ++fuzzy) {

            QFileInfoList::iterator info;

            for (info = list.begin(); info != list.end(); ++info) { //### JAS Check for errors
                RG_DEBUG << "Looking at " << info->fileName() << " in path "
                << info->filePath() << " for suffix " << (k == nsuffixes ? "(none)" : suffixes[k]) << ", fuzzy " << fuzzy << endl;

                if (!(info->isFile() || info->isSymLink())
                        || !info->isExecutable()) {
                    RG_DEBUG << "(not executable)";
                    continue;
                }

                if (fuzzy) {
                    if (info->fileName().left(fileBase.length()) != fileBase)
                        continue;
                    RG_DEBUG << "(is file base)";
                } else {
                    if (info->fileName().left(label.length()) != label)
                        continue;
                    RG_DEBUG << "(is label)";
                }

                if (k == nsuffixes || info->fileName().toLower().endsWith(suffixes[k])) {
                    RG_DEBUG << "(ends with suffix " << (k == nsuffixes ? "(none)" : suffixes[k]) << " or out of suffixes)";
                    return info->filePath();
                }
                RG_DEBUG << "(doesn't end with suffix " << (k == nsuffixes ? "(none)" : suffixes[k]) << ")";
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

    QByteArray burl = url.toUtf8();

    char *host = lo_url_get_hostname(burl.data());
    char *port = lo_url_get_port(burl.data());
    m_address = lo_address_new(host, port);
    free(host);
    free(port);
    m_basePath = lo_url_get_path(burl.data());
}

void
AudioPluginOSCGUI::show()
{
    RG_DEBUG << "AudioPluginOSCGUI::show";

    if (!m_address) return;
    QString path = m_basePath + "/show";
    QByteArray ba = path.toUtf8();
    lo_send(m_address, ba.data(), "");
}

void
AudioPluginOSCGUI::hide()
{
    if (!m_address) return;
    QString path = m_basePath + "/hide";
    QByteArray ba = path.toUtf8();
    lo_send(m_address, ba.data(), "");
}

void
AudioPluginOSCGUI::quit()
{
    if (!m_address) return;
    QString path = m_basePath + "/quit";
    QByteArray ba = path.toUtf8();
    lo_send(m_address, ba.data(), "");
}

void
AudioPluginOSCGUI::sendProgram(int bank, int program)
{
    if (!m_address) return;
    QString path = m_basePath + "/program";
    QByteArray ba = path.toUtf8();
    lo_send(m_address, ba.data(), "ii", bank, program);
}

void
AudioPluginOSCGUI::sendPortValue(int port, float value)
{
    if (!m_address) return;
    QString path = m_basePath + "/control";
    QByteArray ba = path.toUtf8();
    lo_send(m_address, ba.data(), "if", port, value);
}

void
AudioPluginOSCGUI::sendConfiguration(QString key, QString value)
{
    if (!m_address) return;
    QString path = m_basePath + "/configure";
    QByteArray ba = path.toUtf8();
    QByteArray bk = key.toUtf8();
    QByteArray bv = value.toUtf8();
    lo_send(m_address, ba.data(), "ss", bk.data(), bv.data());
}

}

