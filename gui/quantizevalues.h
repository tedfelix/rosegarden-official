// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <string>
#include <vector>

#include "Event.h"


// Convenient way of sharing possible quantization values across clients
//

#ifndef _QUANTIZEVALUES_H_
#define _QUANTIZEVALUES_H_

typedef std::vector<std::pair<Rosegarden::timeT, std::string> > QuantizeList;
typedef std::vector<std::pair<Rosegarden::timeT, std::string> >::iterator QuantizeListIterator;

class QuantizeValues : public QuantizeList
{
public:

    QuantizeValues();
    virtual ~QuantizeValues() {;}

private:
};

#endif // _QUANTIZEVALUES_H_


