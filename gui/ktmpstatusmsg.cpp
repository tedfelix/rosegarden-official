// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include <qapplication.h>

#include <kstatusbar.h>
#include <klocale.h>

#include "ktmpstatusmsg.h"

KTmpStatusMsg::KTmpStatusMsg(const QString& msg, KStatusBar* bar, int id)
    : m_statusBar(bar),
      m_id(id)
{
    m_statusBar->changeItem(msg, m_id);
    qApp->processEvents(500);
}

KTmpStatusMsg::~KTmpStatusMsg()
{
    m_statusBar->clear();
    m_statusBar->changeItem(m_defaultMsg, m_id);
    qApp->processEvents(500);
}


void KTmpStatusMsg::setDefaultMsg(const QString& m)
{
    m_defaultMsg = m;
}

const QString& KTmpStatusMsg::getDefaultMsg()
{
    return m_defaultMsg;
}

void  KTmpStatusMsg::setDefaultId(int id)
{
    m_defaultId = id;
}

int  KTmpStatusMsg::getDefaultId()
{
    return m_defaultId;
}


int KTmpStatusMsg::m_defaultId = 1;
QString KTmpStatusMsg::m_defaultMsg = i18n("  Ready.");
