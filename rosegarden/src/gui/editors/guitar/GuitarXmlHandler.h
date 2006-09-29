// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.
 
    This program is Copyright 2000-2005
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

#ifndef GUITAR_XML_HANDLER_H_
#define GUITAR_XML_HANDLER_H_

#include "ChordMap.h"
#include <qdom.h>

namespace Rosegarden
{

namespace Guitar
{

    class GuitarXmlHandler
    {
    public:
        //! Constructor
        GuitarXmlHandler ();

        //! Return a chord map parsed from the given file
        Guitar::ChordMap const* parse (QString const& file);

    private:

        /**
         * \brief Begins process parsed xml file starting with the Chord_Group
         *
         * \param obj Chord_Group to be parsed
         */
        void handleChordGroup (Guitar::ChordMap* chordMap, QDomNode const& obj);

    };

} /* namespace Guitar */

}


#endif /* GUITAR_XML_HANDLER_H_ */

