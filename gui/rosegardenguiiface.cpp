// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

/*
 * Code borrowed from KDE Konqueror : KonqMainWindowIface.cc
 * Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2000 David Faure <faure@kde.org>
 */

#include "rosegardenguiiface.h"

#include <dcopclient.h>
#include <kdcopactionproxy.h>
#include <kapp.h>
#include <kmainwindow.h>
#include <kaction.h>

RosegardenIface::RosegardenIface(KMainWindow* mainWindow)
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

