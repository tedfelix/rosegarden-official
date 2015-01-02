/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EXCEPTION_H
#define RG_EXCEPTION_H

#include <string>
#include <QString>
#include <exception>

namespace Rosegarden {

class Exception : public virtual std::exception
{
public:
    Exception(const char *message);
    Exception(const char *message, const char *file, int line);

    Exception(std::string message);
    Exception(std::string message, std::string file, int line);

    Exception(QString message);
    Exception(QString message, QString file, int line);

    virtual ~Exception() throw () {}

    std::string getMessage() const { return m_message; }

private:
    std::string m_message;
};


}

#endif
