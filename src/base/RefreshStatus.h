/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */


/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_REFRESH_STATUS_H
#define RG_REFRESH_STATUS_H

#include <QtGlobal>
#include <vector>

namespace Rosegarden 
{

/// Flag indicating that a refresh is needed.
/**
 * This is a flag indicating that a refresh may be required for
 * some part of the user interface.  The implementation is
 * a simple wrapper around a bool.
 *
 * See SegmentRefreshStatus which derives from this and adds a
 * time range.
 *
 * See RefreshStatusArray which supports multiple observers.
 */
class RefreshStatus
{
public:
    RefreshStatus() : m_needsRefresh(false) {}

    bool needsRefresh() { return m_needsRefresh; }
    void setNeedsRefresh(bool s) { m_needsRefresh = s; }

protected:
    bool m_needsRefresh;
};

/// Polled notification mechanism.
/**
 * This supports providing refresh notification flags (e.g. RefreshStatus)
 * for a number of observers.  An observer can check the flag
 * periodically and perform a refresh when the flag is set.  Then they
 * can clear the flag.
 *
 * Polling mechanisms such as this are useful when changes to an underlying
 * data object are fast and frequent but the observers would like to update
 * at a slower pace (usually to save CPU time).
 *
 * See Composition which instantiates this with RefreshStatus and
 * Segment which instantiates this with SegmentRefreshStatus (which adds
 * a time range to the refresh flag in RefreshStatus).
 */
template<class RS>
class RefreshStatusArray
{
public:
    /// Creates a new refresh status object for an observer.
    /**
     * Use the returned ID when calling getRefreshStatus() to check whether
     * a refresh is needed.  This ID identifies a specific observer.  That
     * observer can clear these refresh flags without affecting other
     * observers.
     */
    unsigned int getNewRefreshStatusId();

    /// Returns the number of observers.
    size_t size() { return m_refreshStatuses.size(); }

    /// Returns the refresh status object for a particular observer.
    /**
     * Observers can set or clear the flags via the reference that is
     * returned.
     */
    RS& getRefreshStatus(unsigned int id)
    {
        Q_ASSERT_X(id < m_refreshStatuses.size(),
                   "RefreshStatusArray::getRefreshStatus()",
                   "ID out of bounds");

        return m_refreshStatuses[id];
    }

    /// Sets all the refresh flags to true.
    /**
     * rename: needsRefresh()
     */
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

// Defined in Composition.cpp.
void breakpoint();

template<class RS>
void RefreshStatusArray<RS>::updateRefreshStatuses()
{
    // breakpoint(); // for debug purposes, so one can set a breakpoint
    // in this template code (set it in breakpoint() itself which is in
    // Composition.cpp).

    // Set all the refresh flags to true.
    for(unsigned int i = 0; i < m_refreshStatuses.size(); ++i)
        m_refreshStatuses[i].setNeedsRefresh(true);
}


}

#endif
