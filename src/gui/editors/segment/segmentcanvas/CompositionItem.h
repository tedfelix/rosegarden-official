
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _RG_COMPOSITIONITEM_H_
#define _RG_COMPOSITIONITEM_H_

#include <qguardedptr.h>
#include <qobject.h>
#include <qrect.h>


namespace Rosegarden
{

class _CompositionItem : public QObject {
public:
    virtual bool isRepeating() const = 0;
    virtual QRect rect() const = 0;
    virtual void moveBy(int x, int y) = 0;
    virtual void moveTo(int x, int y) = 0;
    virtual void setX(int x) = 0;
    virtual void setY(int y) = 0;
    virtual void setZ(unsigned int z) = 0;
    virtual int  x() = 0;
    virtual int  y() = 0;
    virtual unsigned int  z() = 0;
    virtual void setWidth(int w) = 0;

    // used by itemcontainer
    virtual long hashKey() = 0;

    QRect savedRect() const   { return m_savedRect; }
    void saveRect() const     { m_savedRect = rect(); }

protected:
    mutable QRect m_savedRect;
};

typedef QGuardedPtr<_CompositionItem> CompositionItem;
bool operator<(const CompositionItem&, const CompositionItem&);


}

#endif
