/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */
/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[AudioPluginInstance]"
#define RG_NO_DEBUG_PRINT 1

#include "base/AudioPluginInstance.h"

#include "Instrument.h"
#include "sound/PluginIdentifier.h"
#include "misc/Strings.h"
#include "misc/Debug.h"

#include <iostream>
#include <cstring>

#include <sstream>

namespace Rosegarden
{

// ------------------ PluginPort ---------------------
//

PluginPort::PluginPort(int number,
                       const std::string& name,
                       PluginPort::PortType type,
                       PluginPort::PortDisplayHint displayHint,
                       PortData lowerBound,
                       PortData upperBound,
                       PortData defaultValue):
    m_number(number),
    m_name(name),
    m_type(type),
    m_displayHint(displayHint),
    m_lowerBound(lowerBound),
    m_upperBound(upperBound),
    m_default(defaultValue)
{
}

AudioPluginInstance::AudioPluginInstance(unsigned int position):
    m_mappedId(-1),
    m_identifier(""),
    m_arch(DSSI),
    m_position(position),
    m_assigned(false),
    m_bypass(false),
    m_program("")
{
}

#if 0
AudioPluginInstance::AudioPluginInstance(const std::string& identifier,
                                         unsigned int position):
                m_mappedId(-1),
                m_identifier(identifier),
                m_position(position),
                m_assigned(true),
                m_bypass(false)
{
}
#endif

AudioPluginInstance::~AudioPluginInstance()
{
    clearPorts();
}

std::string
AudioPluginInstance::toXmlString() const
{
    // ??? rename: stream
    std::stringstream plugin;

    if (!m_assigned) {
        // ??? Isn't this just ""?
        return plugin.str();
    }

    if (m_position == Instrument::SYNTH_PLUGIN_POSITION) {
        plugin << "            <synth";
    } else {
        plugin << "            <plugin"
               << " position=\""
               << m_position
               << "\"";
    }

    if (m_arch == PluginArch::LV2) {
        // Format something 23.12 will interpret into a helpful error message.
        plugin << " identifier=\"" << "lv2:lv2_not_supported:" << encode(m_label) << "\"";
    } else {
        plugin << " identifier=\"" << encode(m_identifier) << "\"";
    }

    // New for 24.06+
    if (m_arch == PluginArch::LV2)
        plugin << " lv2uri=\"" << encode(m_identifier) << "\"";

    plugin << " label=\"" << encode(m_label) << "\"";

    plugin << " bypassed=\"";
    if (m_bypass)
        plugin << "true\" ";
    else
        plugin << "false\" ";

    if (m_program != "") {
        plugin << "program=\"" << encode(m_program) << "\"";
    }

    plugin << ">" << std::endl;

    for (size_t i = 0; i < m_ports.size(); i++)
    {
        plugin << "                <port id=\""
               << m_ports[i]->number
               << "\" value=\""
               << m_ports[i]->value
               << "\" changed=\""
               << (m_ports[i]->changedSinceProgramChange ? "true" : "false")
               << "\"/>" << std::endl;
    }

    for (ConfigMap::const_iterator i = m_config.begin(); i != m_config.end(); ++i) {
        plugin << "                <configure key=\""
               << encode(i->first) << "\" value=\""
               << encode(i->second) << "\"/>" << std::endl;
    }

    if (m_position == Instrument::SYNTH_PLUGIN_POSITION) {
        plugin << "            </synth>";
    } else {
        plugin << "            </plugin>";
    }

    plugin << std::endl;

    return plugin.str();
}


void
AudioPluginInstance::addPort(int number, PortData value)
{
    m_ports.push_back(new PluginPortInstance(number, value));
}

/* unused
bool
AudioPluginInstance::removePort(int number)
{
    PortInstanceIterator it = m_ports.begin();

    for (; it != m_ports.end(); ++it)
    {
        if ((*it)->number == number)
        {
            delete (*it);
            m_ports.erase(it);
            return true;
        }
    }

    return false;
}
*/

PluginPortInstance*
AudioPluginInstance::getPort(int number)
{
    PortInstanceIterator it = m_ports.begin();

    for (; it != m_ports.end(); ++it)
    {
        if ((*it)->number == number)
            return *it;
    }

    return nullptr;
}

void
AudioPluginInstance::clearPorts()
{
    PortInstanceIterator it = m_ports.begin();
    for (; it != m_ports.end(); ++it)
        delete (*it);
    m_ports.erase(m_ports.begin(), m_ports.end());
}

std::string
AudioPluginInstance::getConfigurationValue(std::string k) const
{
    ConfigMap::const_iterator i = m_config.find(k);
    if (i != m_config.end()) return i->second;
    return "";
}

void
AudioPluginInstance::setProgram(const std::string& program)
{
    m_program = program;

    PortInstanceIterator it = m_ports.begin();
    for (; it != m_ports.end(); ++it) {
        (*it)->changedSinceProgramChange = false;
    }
}

void
AudioPluginInstance::setConfigurationValue(const std::string& k,
                                           const std::string& v)
{
    m_config[k] = v;
}

std::string
AudioPluginInstance::getDistinctiveConfigurationText() const
{
    std::string base = getConfigurationValue("load");

    if (base == "") {
        for (ConfigMap::const_iterator i = m_config.begin();
             i != m_config.end(); ++i) {

            if (!strncmp(i->first.c_str(),
                         "__ROSEGARDEN__",
                         strlen("__ROSEGARDEN__"))) continue;

            if (i->second != "" && i->second[0] == '/') {
                base = i->second;
                break;
            } else if (base != "") {
                base = i->second;
            }
        }
    }

    if (base == "") return "";

    std::string::size_type s = base.rfind('/');
    if (s < base.length() - 1) base = base.substr(s + 1);

    std::string::size_type d = base.rfind('.');
    if (d < base.length() - 1 && d > 0) base = base.substr(0, d);

    return base;
}

std::string
AudioPluginInstance::getDisplayName() const
{
    QString displayName = strtoqstr(getProgram());

    // the comment below applies to dssi plugins but not to LV2
    // plugins. The label is now set externally. For dssi plugins this
    // is exactly the string from the identifier so no change - but it
    // now works for LV2!

    // The identifier contains the name of the soft synth in the "label"
    // part.
    //QString identifier = strtoqstr(getIdentifier());

    if (displayName == "")
        displayName = strtoqstr(getDistinctiveConfigurationText());

    if (displayName != "")
        displayName = strtoqstr(m_label) + ": " + displayName;
    else
        displayName = strtoqstr(m_label);

    return qstrtostr(displayName);
}

void AudioPluginInstance::setLabel(const std::string& label)
{
    m_label = label;
}

}
