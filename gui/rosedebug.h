/***************************************************************************
                          rosedebug.h  -  description
                             -------------------
    begin                : Fri Aug 18 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ROSEDEBUG_H
#define ROSEDEBUG_H

#include <string>
#include <iostream>
#include <kdebug.h>

class Event;

#define KDEBUG_AREA 1010

kdbgstream&
operator<<(kdbgstream&, const string&);

kdbgstream&
operator<<(kdbgstream&, const Event&);

// This doesn't work - keeping it just in case I somehow get it
// working someday

class kdbgostreamAdapter : public ostream
{
public:
    kdbgostreamAdapter(kdbgstream &e) : m_kdbgStream(e) {}

    ostream& operator<<(bool i);
    ostream& operator<<(short i);
    ostream& operator<<(unsigned short i);
    ostream& operator<<(char i);
    ostream& operator<<(unsigned char i);
    ostream& operator<<(int i);
    ostream& operator<<(unsigned int i);
    ostream& operator<<(long i);
    ostream& operator<<(unsigned long i);
    ostream& operator<<(const QString& string);
    ostream& operator<<(const char *string);
    ostream& operator<<(const QCString& string);
    ostream& operator<<(void * p);
    ostream& operator<<(KDBGFUNC f);
    ostream& operator<<(double d);

    kdbgstream& dbgStream() { return m_kdbgStream; }

protected:
    kdbgstream &m_kdbgStream;
};

// ostream& endl(ostream& s);

void DBCheckThrow();


#endif
