// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef ROSEDEBUG_H
#define ROSEDEBUG_H

#include <string>
#include <iostream>
#include <kdebug.h>

namespace Rosegarden { class Event; class Segment; }

#define KDEBUG_AREA 1010

#ifndef NDEBUG

kdbgstream&
operator<<(kdbgstream&, const std::string&);

kdbgstream&
operator<<(kdbgstream&, const Rosegarden::Event&);

kdbgstream&
operator<<(kdbgstream&, const Rosegarden::Segment&);

#else

inline kndbgstream&
operator<<(kndbgstream &s, const std::string&) { return s; }

inline kndbgstream&
operator<<(kndbgstream &s, const Rosegarden::Event&) { return s; }

inline kndbgstream&
operator<<(kndbgstream &s, const Rosegarden::Segment&) { return s; }

#endif

#ifndef NO_TIMING

#include <iostream>
#include <time.h>

#define START_TIMING \
  clock_t dbgStart = clock();
#define ELAPSED_TIME \
  ((clock() - dbgStart) * 1000 / CLOCKS_PER_SEC)
#define PRINT_ELAPSED(n) \
  std::cout << n << ": " << ELAPSED_TIME << "ms elapsed" << std::endl;

#else

#define START_TIMING
#define ELAPSED_TIME  0
#define PRINT_ELAPSED(n)

#endif




// This doesn't work - keeping it just in case I somehow get it
// working someday

#ifdef NOT_DEFINED

// can't be bothered to even get this to compile with gcc-3.0 at the
// moment

class kdbgostreamAdapter : public std::ostream
{
public:
    kdbgostreamAdapter(kdbgstream &e) : m_kdbgStream(e) {}

    std::ostream& operator<<(bool i);
    std::ostream& operator<<(short i);
    std::ostream& operator<<(unsigned short i);
    std::ostream& operator<<(char i);
    std::ostream& operator<<(unsigned char i);
    std::ostream& operator<<(int i);
    std::ostream& operator<<(unsigned int i);
    std::ostream& operator<<(long i);
    std::ostream& operator<<(unsigned long i);
    std::ostream& operator<<(const QString& str);
    std::ostream& operator<<(const char *str);
    std::ostream& operator<<(const QCString& str);
    std::ostream& operator<<(void * p);
    std::ostream& operator<<(KDBGFUNC f);
    std::ostream& operator<<(double d);

    kdbgstream& dbgStream() { return m_kdbgStream; }

protected:
    kdbgstream &m_kdbgStream;
};

#endif

// std::ostream& endl(std::ostream& s);

void DBCheckThrow();


#endif
