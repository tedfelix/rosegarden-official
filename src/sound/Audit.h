/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIT_H
#define RG_AUDIT_H

#include <string>
#include <sstream>

namespace Rosegarden {


/// Logging class that makes it easier for users to get debug info.
/**
 * This is used at startup to log information that could be helpful for
 * diagnosing certain problems related to audio and MIDI.  The contents
 * of this log can be examined by the user via the preferences dialog.
 * Edit > Preferences... > General tab > Details... button.
 *
 * There are some issues with this class as it stands:
 *
 *   1. It batches the logging and sends it to RG_DEBUG on destruction
 *      of an Audit instance.  This means that if there is a crash,
 *      important debug output will be lost.
 *
 *   2. It uses std::stringstream which means it cannot take advantage
 *      of the insertion operators that have been coded for QDebug.
 *
 * Recommendation: Remove the RG_DEBUG from the dtor.
 *   Then use Audit in parallel with RG_DEBUG/RG_WARNING.  It's duplicated
 *   code, but it solves the key issue which is the loss of logging in the
 *   case of a crash.
 */
class Audit : public std::stringstream
{
public:
    Audit() { }

    virtual ~Audit();

    static std::string getAudit();

protected:
    static std::string m_audit;
};


}

#endif
