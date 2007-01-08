// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
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

#ifndef GUITAR_BARRE_LIST_H_
#define GUITAR_BARRE_LIST_H_

#include "Barre.h"
#include <vector>

namespace Rosegarden
{

namespace Guitar
{
    class BarreList
    {
    public:
        typedef std::vector<Barre*>::iterator iterator;
        typedef std::vector<Barre*>::const_iterator const_iterator;

        //! Add Barre object to list
        void push_back (Barre* obj);

        //! Return iterator to start of Barre list
        iterator begin (void);

        //! Return iterator to end of Barre list
        iterator end (void);

        //! Return TRUE if Barre list is empty
        bool empty (void) const;

        //! Return Barre list size
        unsigned int const size (void) const;

        //! Remove the contents of the Barre list iterator from Barre lsit
        void erase (BarreList::iterator& pos);

        //! Remove the Barre object from Barre list
        void erase (Barre* bar_ptr);

    private:

        //! List of contained Barre objects
        std::vector<Barre*> m_data;
    };

}

}

#endif /* GUITAR_BARLIST_H_ */
