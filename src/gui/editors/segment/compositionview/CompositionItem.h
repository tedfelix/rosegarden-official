
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

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

//#include <qguardedptr.h>
#include <QObject>
#include <QRect>
#include <QPointer>


namespace Rosegarden
{

/// The interface for a composition item.
/**
 * This is primarily an interface (abstract base) class that defines the
 * interface for a composition item.  It also has a saved rectangle which
 * is a common property of composition items.  CompositionItem is a
 * QPointer to this class.
 *
 * See the deriver (CompositionItemImpl) for details.
 *
 * Given that there is only one deriver from this interface and
 * probably has been for quite some time, this class can probably be removed.
 */
class _CompositionItem : public QObject {	
public:
    virtual bool isRepeating() const = 0;
    virtual QRect rect() const = 0;
    virtual void translate(int x, int y) = 0;
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


typedef QPointer<_CompositionItem> CompositionItem;  

bool operator<(const CompositionItem& , const CompositionItem& );


}

#endif
