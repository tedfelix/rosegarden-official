// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include <klocale.h>
#include <qstringlist.h>
#include <qdir.h>

#include "diskspace.h"

namespace Rosegarden
{

DiskSpace::DiskSpace(const QString &path):
    m_path(path),
    m_device(""),
    m_diskSpaceKBytes(0)
{
    // Check for path existence
    QDir dir(path);
    if (!dir.exists()) throw i18n("Directory doesn't exist.");

    m_proc = new KProcess;

    *m_proc << "/bin/df";
    *m_proc << "-k" << m_path;

    connect(m_proc, SIGNAL(receivedStdout (KProcess*, char *, int)),
            this, SLOT(slotProcessStdout(KProcess*, char *, int)));

    // Start the process as blocking and capture All output
    //
    if(m_proc->start(KProcess::Block, KProcess::All) == false)
    {
        //KMessageBox::error(this, i18n("Couldn't determine free disk space"));
        throw i18n("Can't determine free disk space.");
    }

}

DiskSpace::~DiskSpace()
{
}

int
DiskSpace::getFreeKBytes() const
{
    while(m_proc->isRunning()); // wait for completion
    return m_diskSpaceKBytes;
}


void
DiskSpace::slotProcessStdout(KProcess*, char *buf, int no)
{
   QString outputStr;
    for (int i = 0; i < no; ++i) outputStr +=buf[i];

    // split by forward slash to begin with
    QStringList list(QStringList::split("/", outputStr));

    if (list.size() < 4)
    {
        m_diskSpaceKBytes = 0;
        throw i18n("Couldn't extract disk space information.");
    }

    QStringList valueList(QStringList::split(" ", list[2]));
    m_diskSpaceKBytes = valueList[3].toInt();

    // Device upon which the record path is mounted - in case we ever
    // need it.
    //
    m_device = QString("/%1/%2").arg(list[1]).arg(valueList[0]);

    /*
    cout << "DEVICE \"" << m_device << "\" = remaining = " 
         << m_diskSpaceKBytes << endl;
         */

}

}

