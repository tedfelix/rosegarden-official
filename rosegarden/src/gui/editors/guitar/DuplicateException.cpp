#include "DuplicateException.h"
#include "Chord.h"

namespace Rosegarden
{

namespace Guitar
{

DuplicateException ::DuplicateException ( Chord* dup_ptr, Chord* orig_ptr )
        : m_dup_ptr ( dup_ptr ),
        m_orig_ptr ( orig_ptr )
{}

Chord* DuplicateException::getDuplicate () const
{
    return m_dup_ptr;
}

Chord* DuplicateException::getOriginal () const
{
    return m_orig_ptr;
}

}

}
