/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "base/Exception.h"

#include <iostream>

namespace Rosegarden {

Exception::Exception(const char *message) :
    m_message(message)
{
#ifndef NDEBUG
    std::cerr << "WARNING: Rosegarden::Exception: \""
	      << message << "\"" << std::endl;
#endif
}

Exception::Exception(const char *message, const char *file, int line) :
    m_message(message)
{
#ifndef NDEBUG
    std::cerr << "WARNING: Rosegarden::Exception: \""
	      << message << "\" at " << file << ":" << line << std::endl;
#endif
}

Exception::Exception(std::string message) :
    m_message(message)
{
#ifndef NDEBUG
    std::cerr << "WARNING: Rosegarden::Exception: \""
	      << message << "\"" << std::endl;
#endif
}

Exception::Exception(std::string message, std::string file, int line) :
    m_message(message)
{
#ifndef NDEBUG
    std::cerr << "WARNING: Rosegarden::Exception: \""
	      << message << "\" at " << file << ":" << line << std::endl;
#endif
}

Exception::Exception(QString message) :
    m_message(message.toUtf8().data())
{
#ifndef NDEBUG
    std::cerr << "WARNING: Rosegarden::Exception: \""
	      << m_message << "\"" << std::endl;
#endif
}

Exception::Exception(QString message, QString file, int line) :
    m_message(message.toUtf8().data())
{
#ifndef NDEBUG
    std::cerr << "WARNING: Rosegarden::Exception: \""
	      << m_message << "\" at " << file.toLocal8Bit().data() << ":" << line << std::endl;
#endif
}

}
