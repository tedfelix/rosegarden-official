/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[Exception]"

#include "base/Exception.h"

#include "misc/Debug.h"


namespace Rosegarden {


Exception::Exception(const char *message) :
    m_message(message)
{
    RG_DEBUG << "Creating:" << message;
}

Exception::Exception(const char *message, const char *file, int line) :
    m_message(message)
{
    RG_DEBUG << "Creating:" << message << "at" << file << ":" << line;
}

Exception::Exception(const std::string &message) :
    m_message(message)
{
    RG_DEBUG << "Creating:" << message;
}

Exception::Exception(const std::string &message,
                     const std::string &file,
                     int line) :
    m_message(message)
{
    RG_DEBUG << "Creating:" << message << "at" << file << ":" << line;
}

Exception::Exception(const QString &message) :
    m_message(message.toUtf8().data())
{
    RG_DEBUG << "Creating:" << message;
}

Exception::Exception(const QString &message, const QString &file, int line) :
    m_message(message.toUtf8().data())
{
    RG_DEBUG << "Creating:" << message << "at" << file << ":" << line;
}


}
