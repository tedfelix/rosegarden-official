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
#ifndef GUITAR_STRING_H_
#define GUITAR_STRING_H_

#include <qstring.h>

namespace Guitar
{
class GuitarString
{
public:

    //! Enumeration of all position guitar string actions
    enum Action {
        MUTED = 1,
        OPEN,
        PRESSED
    };

    //! Constructor
    GuitarString ()
            : m_state ( GuitarString::MUTED ),
            m_tune ( 0 )
    {}

    GuitarString ( GuitarString const& rhs )
            : m_state ( rhs.m_state ),
            m_tune ( rhs.m_tune )
    {}

    bool operator< ( GuitarString const& rhs ) const
    {
        bool result = true;
        result = ( m_state < rhs.m_state );
        return result;
    }

    bool operator== ( GuitarString const& rhs ) const
    {
        bool result = true;
        result = ( m_state == rhs.m_state );
        return result;
    }

    //! Return the string representation of present action
    static QString actionStringName ( GuitarString::Action val )
    {
        switch ( val )
        {
        case MUTED:
            {
                return "muted";
            }
        case OPEN:
            {
                return "open";
            }
        case PRESSED:
        default:
            {
                return "pressed";
            }
        }
    }

    //! Return the string representation of present action
    static GuitarString::Action
    actionValue ( QString act )
    {
        QString lower = act.lower();
        if ( lower == "muted" )
        {
            return MUTED;
        }
        else if ( lower == "open" )
        {
            return OPEN;
        }
        else
        {
            return PRESSED;
        }
    }

    //! Return the string representation of present action
    static GuitarString::Action
    actionValue ( unsigned int& act )
    {
        if ( act == MUTED )
        {
            return MUTED;
        }
        else if ( act == OPEN )
        {
            return OPEN;
        }
        else
        {
            return PRESSED;
        }
    }

    //! Present action of string
    Action m_state;

    //! FUTURE USE: Base tuning of string
    unsigned int m_tune;
};
}

#endif /* GUITAR_STRING_H_ */
