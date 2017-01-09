/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "Chord.h"
#include "base/Event.h"
#include "misc/Strings.h"

#include <QString>
#include <QRegExp>

namespace Rosegarden
{

namespace Guitar
{
const std::string Chord::EventType              = "guitarchord";
const short Chord::EventSubOrdering             = -60;

static const PropertyName RootPropertyName = "root";
static const PropertyName ExtPropertyName = "ext";
static const PropertyName FingeringPropertyName = "fingering";

Chord::Chord()
    : m_isUserChord(false)
{
}

Chord::Chord(const QString& root, const QString& ext)
    : m_root(root),
      m_ext(ext),
      m_isUserChord(false)
{
    if (m_ext.isEmpty())
        m_ext = QString::null;
}

Chord::Chord(const Event& e)
    : m_isUserChord(false)
{
    std::string f;
    bool ok;

    ok = e.get<String>(RootPropertyName, f);
    if (ok)
        m_root = strtoqstr(f);

    ok = e.get<String>(ExtPropertyName, f);
    if (ok) {
        if (f.length() == 0)
            m_ext = QString::null;
        else
            m_ext = strtoqstr(f);
    }
    
    ok = e.get<String>(FingeringPropertyName, f);
    if (ok) {
        QString qf(strtoqstr(f));
        QString errString;
    
        Fingering fingering = Fingering::parseFingering(qf, errString);    
        setFingering(fingering);
    }
}

Event* Chord::getAsEvent(timeT absoluteTime) const
{
    Event *e = new Event(EventType, absoluteTime, 0, EventSubOrdering);
    e->set<String>(RootPropertyName, qstrtostr(m_root));
    e->set<String>(ExtPropertyName, qstrtostr(m_ext));
    e->set<String>(FingeringPropertyName, getFingering().toString());
    return e;
}

bool Chord::hasAltBass() const
{
    static const QRegExp ALT_BASS_REGEXP("/[A-G]");
    return m_ext.contains(ALT_BASS_REGEXP);
}

bool operator<(const Chord& a, const Chord& b)
{
    if (a.m_root != b.m_root) {
        return a.m_root < b.m_root;
    } else if (a.m_ext != b.m_ext) {
        if (a.m_ext.isEmpty()) // chords with no ext need to be stored first
            return true;
        if (b.m_ext.isEmpty())
            return false;
        return a.m_ext < b.m_ext;
    } else {
        return a.m_fingering < b.m_fingering;
    }

}

}

}
