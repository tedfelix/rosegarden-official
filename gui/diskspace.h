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

#ifndef _DISKSPACE_H_
#define _DISKSPACE_H_

#include <kprocess.h>

#include <qstring.h>
#include <qobject.h>

namespace Rosegarden
{

class DiskSpace : public QObject
{
    Q_OBJECT
public:
    DiskSpace(const QString &path);
    virtual ~DiskSpace();

    QString getPath() const { return m_path; }
    int getFreeKBytes() const;

protected slots:
    void slotProcessStdout(KProcess *proc, char *buf, int no);

protected:

    QString   m_path;            // initial path
    QString   m_device;          // device on which this path is mounted
    int       m_diskSpaceKBytes; // KB available

    KProcess *m_proc;
};

}

#endif // _DISKSPACE_H_
