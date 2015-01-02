/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_OSCMESSAGE_H
#define RG_OSCMESSAGE_H

#include <lo/lo.h>

#include <string>
#include <utility>
#include <vector>

namespace Rosegarden
{



class OSCMessage
{
public:
    OSCMessage() { }
    ~OSCMessage();

    void setTarget(const int &target) { m_target = target; }
    int getTarget() const { return m_target; }

    void setTargetData(const int &targetData) { m_targetData = targetData; }
    int getTargetData() const { return m_targetData; }

    void setMethod(const std::string &method) { m_method = method; }
    std::string getMethod() const { return m_method; }

    void clearArgs();
    void addArg(char type, lo_arg *arg);

    size_t getArgCount() const;
    const lo_arg *getArg(size_t i, char &type) const;

private:
    int m_target;
    int m_targetData;
    std::string m_method;
    typedef std::pair<char, lo_arg *> OSCArg;
    std::vector<OSCArg> m_args;
};


class TimerCallbackAssistant;


}

#endif
