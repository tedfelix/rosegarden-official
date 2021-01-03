
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

#ifndef RG_NOTESTYLEFACTORY_H
#define RG_NOTESTYLEFACTORY_H


#include "NoteStyle.h"
#include "base/Exception.h"

#include <QSharedPointer>

#include <map>
#include <vector>

namespace Rosegarden
{

class NoteStyle;
class Event;

class NoteStyleFactory
{
public:
    static std::vector<NoteStyleName> getAvailableStyleNames();
    static const NoteStyleName DefaultStyle;

    static QSharedPointer<NoteStyle> getStyle(NoteStyleName name);
    static QSharedPointer<NoteStyle> getStyleForEvent(Event *event);

    typedef Exception StyleUnavailable;

private:
    typedef std::map<QString, QSharedPointer<NoteStyle> > StyleMap;
    static StyleMap m_styles;
};



}

#endif
