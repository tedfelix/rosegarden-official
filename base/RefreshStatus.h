// -*- c-basic-offset: 4 -*-


/*
    Rosegarden-4
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

#ifndef _REFRESH_STATUS_H_
#define _REFRESH_STATUS_H_

namespace Rosegarden 
{

class RefreshStatus
{
public:
    RefreshStatus() : m_needsRefresh(true) {}

    bool needsRefresh() { return m_needsRefresh; }
    void setNeedsRefresh(bool s) { m_needsRefresh = s; }

protected:
    bool m_needsRefresh;
};

template<class RS>
class RefreshStatusArray
{
public:
    unsigned int getNewRefreshStatusId();
    size_t size() { return m_refreshStatuses.size(); }
    RS& getRefreshStatus(unsigned int id) { return m_refreshStatuses[id]; }
    void updateRefreshStatuses();

protected:
    std::vector<RS> m_refreshStatuses;
};

template<class RS>
unsigned int RefreshStatusArray<RS>::getNewRefreshStatusId()
{
    m_refreshStatuses.push_back(RS());
    unsigned int res = m_refreshStatuses.size() - 1;
    return res;
}

void breakpoint();

template<class RS>
void RefreshStatusArray<RS>::updateRefreshStatuses()
{
    breakpoint(); // for debug purposes, so one can set a breakpoint
    // in this template code (set it in breakpoint() itself).
    for(unsigned int i = 0; i < m_refreshStatuses.size(); ++i)
        m_refreshStatuses[i].setNeedsRefresh(true);
}


}

#endif
