//
// C++ Interface: DuplicateException
//
// Description:
//
//
// Author: Chris Cannam <cannam@all-day-breakfast.com>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef DUPLICATE_EXCEPTION_H_
#define DUPLICATE_EXCEPTION_H_

#include <exception>

namespace guitar
{

class Chord;

class DuplicateException : public std::exception
{
public:
    DuplicateException ( Chord* dup_ptr, Chord* orig_ptr );
    Chord* getDuplicate () const;
    Chord* getOriginal () const;
private:
    Chord* m_dup_ptr;
    Chord* m_orig_ptr;
};

}

#endif
