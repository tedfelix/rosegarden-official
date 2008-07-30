/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Parts of this file are from KDE Konqueror : KonqMainWindowIface
    Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 2000 David Faure <faure@kde.org>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "RosegardenIface.h"

#include "sound/MappedComposition.h"
#include <dcopobject.h>
#include <dcopref.h>
#include <kaction.h>
#include <kdcopactionproxy.h>
#include <kmainwindow.h>
#include <qcstring.h>
#include <qstring.h>
#include <qvaluelist.h>
#include <kapplication.h>
#include <dcopclient.h>


namespace Rosegarden
{

RosegardenIface::RosegardenIface(KMainWindow* mainWindow)
        : DCOPObject(mainWindow->name()),
        m_dcopActionProxy(0)
{}

void RosegardenIface::iFaceDelayedInit(KMainWindow* mainWindow)
{
    m_dcopActionProxy = new KDCOPActionProxy(mainWindow->actionCollection(),
                        this);
}

DCOPRef RosegardenIface::action(const QCString &name)
{
    return DCOPRef(kapp->dcopClient()->appId(),
                   m_dcopActionProxy->actionObjectId(name));
}

QCStringList RosegardenIface::actions()
{
    QCStringList res;
    QValueList<KAction *> lst = m_dcopActionProxy->actions();
    QValueList<KAction *>::ConstIterator it = lst.begin();
    QValueList<KAction *>::ConstIterator end = lst.end();
    for (; it != end; ++it )
        res.append( (*it)->name() );

    return res;
}

QMap<QCString,DCOPRef> RosegardenIface::actionMap()
{
  return m_dcopActionProxy->actionMap();
}

}
