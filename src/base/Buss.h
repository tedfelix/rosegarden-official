/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_BUSS_H
#define RG_BUSS_H

#include "MidiProgram.h"  // for MidiByte
#include "PluginContainer.h"
#include "XmlExportable.h"

#include <string>

namespace Rosegarden
{


typedef unsigned int BussId;

class Buss : public XmlExportable, public PluginContainer
{
public:
    Buss(BussId id);

    void setId(BussId id) { m_id = id; }
    BussId getId() const override { return m_id; }

    void setLevel(float dB) { m_level = dB; }
    float getLevel() const { return m_level; }

    void setPan(MidiByte pan) { m_pan = pan; }
    MidiByte getPan() const { return m_pan; }

    int getMappedId() const { return m_mappedId; }
    void setMappedId(int id) { m_mappedId = id; }

    std::string toXmlString() const override;
    std::string getName() const override;
    std::string getPresentationName() const override;
    std::string getAlias() const override;

private:
    BussId m_id;
    float m_level;
    MidiByte m_pan;
    int m_mappedId;
};


}

#endif
