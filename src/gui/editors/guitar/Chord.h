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

#ifndef RG_CHORD_H
#define RG_CHORD_H

#include "Fingering.h"
#include "base/Event.h"
#include "misc/Debug.h"

#include <QString>

namespace Rosegarden
{

class Event;

namespace Guitar
{
    
class Chord
{
    friend bool operator<(const Chord&, const Chord&);
    
public:
    static const std::string EventType;
    static const short EventSubOrdering;

	Chord();
    Chord(const QString& root, const QString& ext = QString::null);
    Chord(const Event&);

    Event* getAsEvent(timeT absoluteTime) const;
        
    bool isEmpty() const   { return m_root.isEmpty(); }
    //@@@ bool operator!() const { return !m_root; }
    
    bool isUserChord() const { return m_isUserChord; }
    void setUserChord(bool c) { m_isUserChord = c; }
     
    QString getRoot() const { return m_root; }
    void setRoot(QString r) { m_root = r; } 

    QString getExt() const { return m_ext; }
    void setExt(QString r) { m_ext = r.isEmpty() ? QString::null : r; } 
    
    bool hasAltBass() const;

    Fingering getFingering() const { return m_fingering; }
    void setFingering(Fingering f) { m_fingering = f; }

    struct ChordCmp
    {
        bool operator()(const Chord &e1, const Chord &e2) const {
            return e1 < e2;
        }
        bool operator()(const Chord *e1, const Chord *e2) const {
            return *e1 < *e2;
        }
    };

protected:

    QString m_root;
    QString m_ext;
    
    Fingering m_fingering;
    
    bool m_isUserChord;
};

bool operator<(const Chord&, const Chord&);    

}

}

#endif /*RG_CHORD_H*/
