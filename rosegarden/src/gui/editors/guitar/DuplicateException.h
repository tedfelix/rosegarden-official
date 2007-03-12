
#ifndef DUPLICATE_EXCEPTION_H_
#define DUPLICATE_EXCEPTION_H_

#include <exception>

namespace Rosegarden
{

namespace Guitar
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

}

#endif
