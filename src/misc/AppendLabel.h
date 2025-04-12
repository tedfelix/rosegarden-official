/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_APPENDLABEL_H
#define RG_APPENDLABEL_H

#include <string>
namespace Rosegarden 
{

/// Append suffix to label, but only if the config says to.
/*
 * This is used to append a suffix (e.g. "(recorded)") when a Segment is
 * transformed in some way.
 *
 * ??? Use QString instead of std::string.
 */
std::string appendLabel(const std::string &label, const std::string &suffix);

}
#endif
