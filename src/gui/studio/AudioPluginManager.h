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

#ifndef RG_AUDIOPLUGINMANAGER_H
#define RG_AUDIOPLUGINMANAGER_H

#include "AudioPluginClipboard.h"

#include <QMutex>
#include <QSharedPointer>
#include <QString>
#include <QThread>

#include <vector>

#include "AudioPlugin.h"

namespace Rosegarden
{

class AudioPlugin;


class AudioPluginManager
{
public:
    explicit AudioPluginManager(bool enableSound);

    // Get a straight list of names
    //
    // unused std::vector<QString> getPluginNames();

    // Some useful members
    //
    QSharedPointer<AudioPlugin> getPlugin(int number);

    QSharedPointer<AudioPlugin> getPluginByIdentifier(QString identifier);
    int getPositionByIdentifier(QString identifier);

    // Deprecated -- the GUI shouldn't be using unique ID because it's
    // bound to a particular plugin type (and not necessarily unique
    // anyway).  It should use the identifier instead, which is a
    // structured string managed by the sequencer.  Keep this in only
    // for compatibility with old .rg files.
    //
    QSharedPointer<AudioPlugin> getPluginByUniqueId(unsigned long uniqueId);

    typedef std::vector<QSharedPointer<AudioPlugin> >::iterator iterator;
    //typedef std::vector<QSharedPointer<AudioPlugin> >::iterator const_iterator;

    iterator begin();
    iterator end();

    // Sample rate
    //
    unsigned int getSampleRate() const;

    AudioPluginClipboard* getPluginClipboard() { return &m_pluginClipboard; }

protected:
    QSharedPointer<AudioPlugin> addPlugin(
                           const QString &identifier,
                           PluginArch arch,
                           const QString &name,
                           unsigned long uniqueId,
                           const QString &label,
                           const QString &author,
                           const QString &copyright,
                           bool isSynth,
                           bool isGrouped,
                           const QString &category);

    bool removePlugin(const QString &identifier);

    class Enumerator : public QThread
    {
    public:
        explicit Enumerator(AudioPluginManager *);
        void run() override;
        bool isDone() const { return m_done; }

    protected:
        AudioPluginManager *m_manager;
        bool m_done;
    };

    void awaitEnumeration() const;
    void fetchSampleRate();

    std::vector<QSharedPointer<AudioPlugin> > m_plugins;
    mutable unsigned int      m_sampleRate;
    AudioPluginClipboard      m_pluginClipboard;
    Enumerator                m_enumerator;
    QMutex                    m_mutex;
    bool                      m_enableSound;
};



}

#endif
