// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef _CONTROLPARAMETER_H_
#define _CONTROLPARAMETER_H_

#include <string>

#include "XmlExportable.h"
#include "MidiProgram.h"

namespace Rosegarden
{

class ControlParameter : public XmlExportable
{
public:
    ControlParameter();
    ControlParameter(const std::string &name,
                     const std::string &type,
                     const std::string &description,
                     int min = 0,
                     int max = 127,
                     int def = 0,
                     MidiByte controllerValue = 0,
                     unsigned int colour = 0,
                     int ipbPositon = -1);

    ControlParameter(const ControlParameter &control);
    ControlParameter& operator=(const ControlParameter &control);

    // ControlParameter comparison on IPB position
    //
    struct ControlPositionCmp
    {
        bool operator()(Rosegarden::ControlParameter *c1,
                        Rosegarden::ControlParameter *c2)
        {
            return (c1->getIPBPosition() < c2->getIPBPosition());
        }

        bool operator()(const Rosegarden::ControlParameter &c1,
                        const Rosegarden::ControlParameter &c2)
        {
            return (c1.getIPBPosition() < c2.getIPBPosition());
        }
    };

    std::string getName() const { return m_name; }
    std::string getType() const { return m_type; }
    std::string getDescription() const { return m_description; }

    int getMin() const { return m_min; }
    int getMax() const { return m_max; }
    int getDefault() const { return m_default; }

    MidiByte getControllerValue() const { return m_controllerValue; }

    unsigned int getColourIndex() const { return m_colourIndex; }

    int getIPBPosition() const { return m_ipbPosition; }

    void setName(const std::string &name) { m_name = name; }
    void setType(const std::string &type) { m_type = type; }
    void setDescription(const std::string &des) { m_description = des; }

    void setMin(int min) { m_min = min; }
    void setMax(int max) { m_max = max; }
    void setDefault(int def) { m_default = def; }

    void setControllerValue(MidiByte con) { m_controllerValue = con; }

    void setColourIndex(unsigned int colour) { m_colourIndex = colour; }

    void setIPBPosition(int position) { m_ipbPosition = position; }

    virtual std::string toXmlString();

protected:

    // ControlParameter name as it's displayed ("Velocity", "Controller")
    std::string    m_name;

    // use event types in here ("controller", "pitchbend");
    std::string    m_type;

    std::string    m_description;

    int            m_min;
    int            m_max;
    int            m_default;

    MidiByte       m_controllerValue;

    unsigned int   m_colourIndex;

    int            m_ipbPosition; // position on Instrument Parameter Box


};

}

#endif // _CONTROLPARAMETER_H_
