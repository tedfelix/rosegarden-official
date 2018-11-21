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
 * In the past, this class also dumped the logging to RG_DEBUG.  This
 * way one only needed to log to Audit and the logging would also end
 * up in the debug output.  The problem with this is that Audit delays
 * logging to RG_DEBUG until the Audit object is destroyed.  This means
 * that when there is a crash, logging it lost.
 *
 * Now, if you want logging to RG_DEBUG/RG_WARNING and Audit, you have
 * to explicitly use both.  E.g.:
 *
 *   audit << "AlsaDriver::initialiseMidi(): initialised MIDI subsystem\n";
 *   RG_DEBUG << "initialiseMidi(): initialised MIDI subsystem";
 *
 * It's redundant, but there's no easy way around this.
 */
class Audit : public std::stringstream
{
public:
    Audit() { }

    ~Audit() override;

    static std::string getAudit();

protected:
    static std::string m_audit;
};


}

#endif
