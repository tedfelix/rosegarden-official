
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_LINEDSTAFFMANAGER_H_
#define _RG_LINEDSTAFFMANAGER_H_





namespace Rosegarden
{

class LinedStaff;


/**
 * LinedStaffManager is a trivial abstract base for classes that own
 * and position sets of LinedStaffs, as a convenient API to permit
 * clients (such as canvas implementations) to discover which staff
 * lies where.
 * 
 * LinedStaffManager is not used by LinedStaff.
 */

class LinedStaff;

class LinedStaffManager
{
public:
    virtual ~LinedStaffManager() {}
    virtual LinedStaff *getStaffForCanvasCoords(int x, int y) const = 0;
};



}

#endif
