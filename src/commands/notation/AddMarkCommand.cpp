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


#include "AddMarkCommand.h"

#include <klocale.h>
#include "misc/Strings.h"
#include "base/Selection.h"
#include "document/BasicSelectionCommand.h"
#include "base/BaseProperties.h"
#include <qstring.h>


namespace Rosegarden
{

using namespace BaseProperties;

QString
AddMarkCommand::getGlobalName(Mark markType)
{
    QString m = strtoqstr(markType);

    // Gosh, lots of collisions
    if (markType == Marks::Sforzando)
        m = i18n("S&forzando");
    else if (markType == Marks::Staccato)
        m = i18n("Sta&ccato");
    else if (markType == Marks::Rinforzando)
        m = i18n("R&inforzando");
    else if (markType == Marks::Tenuto)
        m = i18n("T&enuto");
    else if (markType == Marks::Trill)
        m = i18n("Tri&ll");
    else if (markType == Marks::LongTrill)
        m = i18n("Trill &with Line");
    else if (markType == Marks::TrillLine)
        m = i18n("Trill Line");
    else if (markType == Marks::Turn)
        m = i18n("&Turn");
    else if (markType == Marks::Accent)
        m = i18n("&Accent");
    else if (markType == Marks::Staccatissimo)
        m = i18n("&Staccatissimo");
    else if (markType == Marks::Marcato)
        m = i18n("&Marcato");
    else if (markType == Marks::Pause)
        m = i18n("&Pause");
    else if (markType == Marks::UpBow)
        m = i18n("&Up-Bow");
    else if (markType == Marks::DownBow)
        m = i18n("&Down-Bow");
    else if (markType == Marks::Mordent)
        m = i18n("Mo&rdent");
    else if (markType == Marks::MordentInverted)
        m = i18n("Inverted Mordent");
    else if (markType == Marks::MordentLong)
        m = i18n("Long Mordent");
    else if (markType == Marks::MordentLongInverted)
        m = i18n("Lon&g Inverted Mordent");
    else
        m = i18n("&%1%2").arg(m[0].upper()).arg(m.right(m.length() - 1));
    // FIXME: That last i18n has very little chance of working, unless
    // by some miracle the exact same string was translated elsewhere already
    // but we'll leave it as a warning

    m = i18n("Add %1").arg(m);
    return m;
}

void
AddMarkCommand::modifySegment()
{
    EventSelection::eventcontainer::iterator i;

    for (i = m_selection->getSegmentEvents().begin();
            i != m_selection->getSegmentEvents().end(); ++i) {

        long n = 0;
        (*i)->get
        <Int>(MARK_COUNT, n);
        (*i)->set
        <Int>(MARK_COUNT, n + 1);
        (*i)->set
        <String>(getMarkPropertyName(n),
                 m_mark);
    }
}

}
