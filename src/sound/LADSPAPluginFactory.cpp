/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
    See the AUTHORS file for more details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[LADSPAPluginFactory]"

#include "LADSPAPluginFactory.h"

#include <cstdlib>
#include "misc/Strings.h"
#include "misc/Debug.h"

#include <dlfcn.h>
#include <QDir>
#include <cmath>

#include "base/AudioPluginInstance.h"
#include "LADSPAPluginInstance.h"
#include "MappedStudio.h"
#include "PluginIdentifier.h"

#include <lrdf.h>


namespace Rosegarden
{

LADSPAPluginFactory::LADSPAPluginFactory()
{}

LADSPAPluginFactory::~LADSPAPluginFactory()
{
    for (std::set
                <RunnablePluginInstance *>::iterator i = m_instances.begin();
                i != m_instances.end(); ++i) {
            (*i)->setFactory(0);
            delete *i;
        }
    m_instances.clear();
    unloadUnusedLibraries();
}

const std::vector<QString> &
LADSPAPluginFactory::getPluginIdentifiers() const
{
    return m_identifiers;
}

void
LADSPAPluginFactory::enumeratePlugins(MappedObjectPropertyList &list)
{
    for (std::vector<QString>::iterator i = m_identifiers.begin();
            i != m_identifiers.end(); ++i) {

        const LADSPA_Descriptor *descriptor = getLADSPADescriptor(*i);

        if (!descriptor) {
            RG_WARNING << "enumeratePlugins() WARNING: couldn't get descriptor for identifier: " << *i;
            continue;
        }

//	std::cerr << "Enumerating plugin identifier " << *i << std::endl;

        list.push_back(*i);
        list.push_back(descriptor->Name);
        list.push_back(QString("%1").arg(descriptor->UniqueID));
        list.push_back(descriptor->Label);
        list.push_back(descriptor->Maker);
        list.push_back(descriptor->Copyright);
        list.push_back("false"); // is synth
        list.push_back("false"); // is grouped

        if (m_taxonomy.find(descriptor->UniqueID) != m_taxonomy.end() &&
                m_taxonomy[descriptor->UniqueID] != "") {
//            		std::cerr << "LADSPAPluginFactory: cat for " << *i<< " found in taxonomy as " << m_taxonomy[descriptor->UniqueID] << std::endl;
            list.push_back(m_taxonomy[descriptor->UniqueID]);

        } else if (m_fallbackCategories.find(*i) !=
                   m_fallbackCategories.end()) {
            list.push_back(m_fallbackCategories[*i]);
//            		std::cerr << "LADSPAPluginFactory: cat for " << *i  <<" found in fallbacks as " << m_fallbackCategories[*i] << std::endl;

        } else {
            list.push_back("");
//            		std::cerr << "LADSPAPluginFactory: cat for " << *i << " not found (despite having " << m_fallbackCategories.size() << " fallbacks)" << std::endl;

        }

        list.push_back(QString("%1").arg(descriptor->PortCount));

        for (unsigned long p = 0; p < descriptor->PortCount; ++p) {

            int type = 0;
            if (LADSPA_IS_PORT_CONTROL(descriptor->PortDescriptors[p])) {
                type |= PluginPort::Control;
            } else {
                type |= PluginPort::Audio;
            }
            if (LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[p])) {
                type |= PluginPort::Input;
            } else {
                type |= PluginPort::Output;
            }

            list.push_back(QString("%1").arg(p));
            list.push_back(descriptor->PortNames[p]);
            list.push_back(QString("%1").arg(type));
            list.push_back(QString("%1").arg(getPortDisplayHint(descriptor, p)));
            list.push_back(QString("%1").arg(getPortMinimum(descriptor, p)));
            list.push_back(QString("%1").arg(getPortMaximum(descriptor, p)));
            list.push_back(QString("%1").arg(getPortDefault(descriptor, p)));
        }
    }

    unloadUnusedLibraries();
}


void
LADSPAPluginFactory::populatePluginSlot(QString identifier, MappedPluginSlot &slot)
{
    const LADSPA_Descriptor *descriptor = getLADSPADescriptor(identifier);

    if (descriptor) {

        slot.setStringProperty(MappedPluginSlot::Label, descriptor->Label);
        slot.setStringProperty(MappedPluginSlot::PluginName, descriptor->Name);
        slot.setStringProperty(MappedPluginSlot::Author, descriptor->Maker);
        slot.setStringProperty(MappedPluginSlot::Copyright, descriptor->Copyright);
        slot.setProperty(MappedPluginSlot::PortCount, descriptor->PortCount);

        if (m_taxonomy.find(descriptor->UniqueID) != m_taxonomy.end() &&
                m_taxonomy[descriptor->UniqueID] != "") {
            //		std::cerr << "LADSPAPluginFactory: cat for " << identifier<< " found in taxonomy as " << m_taxonomy[descriptor->UniqueID] << std::endl;
            slot.setStringProperty(MappedPluginSlot::Category,
				   m_taxonomy[descriptor->UniqueID]);

        } else if (m_fallbackCategories.find(identifier) !=
                   m_fallbackCategories.end()) {
            //		std::cerr << "LADSPAPluginFactory: cat for " << identifier  <<" found in fallbacks as " << m_fallbackCategories[identifier] << std::endl;
            slot.setStringProperty(MappedPluginSlot::Category,
				   m_fallbackCategories[identifier]);

        } else {
            //		std::cerr << "LADSPAPluginFactory: cat for " << identifier << " not found (despite having " << m_fallbackCategories.size() << " fallbacks)" << std::endl;
            slot.setStringProperty(MappedPluginSlot::Category, "");
        }

        slot.destroyChildren();

        for (unsigned long i = 0; i < descriptor->PortCount; i++) {

            if (LADSPA_IS_PORT_CONTROL(descriptor->PortDescriptors[i]) &&
                    LADSPA_IS_PORT_INPUT(descriptor->PortDescriptors[i])) {

                MappedStudio *studio = dynamic_cast<MappedStudio *>(slot.getParent());
                if (!studio) {
                    RG_WARNING << "WARNING: LADSPAPluginFactory::populatePluginSlot: can't find studio";
                    return ;
                }

                MappedPluginPort *port =
                    dynamic_cast<MappedPluginPort *>
                    (studio->createObject(MappedObject::PluginPort));

                slot.addChild(port);
                port->setParent(&slot);

                port->setProperty(MappedPluginPort::PortNumber, i);
                port->setStringProperty(MappedPluginPort::Name,
					descriptor->PortNames[i]);
                port->setProperty(MappedPluginPort::Maximum,
                                  getPortMaximum(descriptor, i));
                port->setProperty(MappedPluginPort::Minimum,
                                  getPortMinimum(descriptor, i));
                port->setProperty(MappedPluginPort::Default,
                                  getPortDefault(descriptor, i));
                port->setProperty(MappedPluginPort::DisplayHint,
                                  getPortDisplayHint(descriptor, i));
            }
        }
    }

    //!!! leak here if the plugin is not instantiated too...?
}

MappedObjectValue
LADSPAPluginFactory::getPortMinimum(const LADSPA_Descriptor *descriptor, int port)
{
    LADSPA_PortRangeHintDescriptor d =
        descriptor->PortRangeHints[port].HintDescriptor;

    MappedObjectValue minimum = 0.0;

    if (LADSPA_IS_HINT_BOUNDED_BELOW(d)) {
        MappedObjectValue lb = descriptor->PortRangeHints[port].LowerBound;
        minimum = lb;
    } else if (LADSPA_IS_HINT_BOUNDED_ABOVE(d)) {
        MappedObjectValue ub = descriptor->PortRangeHints[port].UpperBound;
        minimum = std::min(0.f, ub - 1.f);
    }

    if (LADSPA_IS_HINT_SAMPLE_RATE(d)) {
        minimum *= m_sampleRate;
    }

    if (LADSPA_IS_HINT_LOGARITHMIC(d)) {
        if (minimum == 0.f) minimum = 1.f;
    }

    return minimum;
}

MappedObjectValue
LADSPAPluginFactory::getPortMaximum(const LADSPA_Descriptor *descriptor, int port)
{
    LADSPA_PortRangeHintDescriptor d =
        descriptor->PortRangeHints[port].HintDescriptor;

    MappedObjectValue maximum = 1.0;

//    std::cerr << "LADSPAPluginFactory::getPortMaximum(" << port << ")" << std::endl;
//    std::cerr << "bounded above: " << LADSPA_IS_HINT_BOUNDED_ABOVE(d) << std::endl;

    if (LADSPA_IS_HINT_BOUNDED_ABOVE(d)) {
        MappedObjectValue ub = descriptor->PortRangeHints[port].UpperBound;
        maximum = ub;
    } else {
        MappedObjectValue lb = descriptor->PortRangeHints[port].LowerBound;
	if (LADSPA_IS_HINT_LOGARITHMIC(d)) {
	    if (lb == 0.f) lb = 1.f;
	    maximum = lb * 100.f;
	} else {
	    if (lb == 1.f) maximum = 10.f;
	    else maximum = lb + 10;
	}
    }

    if (LADSPA_IS_HINT_SAMPLE_RATE(d)) {
//	std::cerr << "note: port has sample rate hint" << std::endl;
        maximum *= m_sampleRate;
    }

//    std::cerr << "maximum: " << maximum << std::endl;
//    if (LADSPA_IS_HINT_LOGARITHMIC(d)) {
//	std::cerr << "note: port is logarithmic" << std::endl;
//    }
//    std::cerr << "note: minimum is reported as " << getPortMinimum(descriptor, port) << " (from bounded = " << LADSPA_IS_HINT_BOUNDED_BELOW(d) << ", bound = " << descriptor->PortRangeHints[port].LowerBound << ")" << std::endl;

    return maximum;
}

MappedObjectValue
LADSPAPluginFactory::getPortDefault(const LADSPA_Descriptor *descriptor, int port)
{
    MappedObjectValue minimum = getPortMinimum(descriptor, port);
    MappedObjectValue maximum = getPortMaximum(descriptor, port);
    MappedObjectValue deft;

    if (m_portDefaults.find(descriptor->UniqueID) !=
            m_portDefaults.end()) {
        if (m_portDefaults[descriptor->UniqueID].find(port) !=
	    m_portDefaults[descriptor->UniqueID].end()) {

            deft = m_portDefaults[descriptor->UniqueID][port];
            if (deft < minimum) deft = minimum;
            if (deft > maximum) deft = maximum;
//	    std::cerr << "port " << port << ": default " << deft << " from defaults" << std::endl;
            return deft;
        }
    }

    LADSPA_PortRangeHintDescriptor d =
        descriptor->PortRangeHints[port].HintDescriptor;

    bool logarithmic = LADSPA_IS_HINT_LOGARITHMIC(d);

    float logmin = 0, logmax = 0;
    if (logarithmic) {
        float thresh = powf(10, -10);
        if (minimum < thresh) logmin = -10;
        else logmin = log10f(minimum);
        if (maximum < thresh) logmax = -10;
        else logmax = log10f(maximum);
    }

    if (!LADSPA_IS_HINT_HAS_DEFAULT(d)) {

        deft = minimum;

    } else if (LADSPA_IS_HINT_DEFAULT_MINIMUM(d)) {

	// See comment for DEFAULT_MAXIMUM below
	if (!LADSPA_IS_HINT_BOUNDED_BELOW(d)) {
	    deft = descriptor->PortRangeHints[port].LowerBound;
	    if (LADSPA_IS_HINT_SAMPLE_RATE(d)) {
		deft *= m_sampleRate;
	    }
//	    std::cerr << "default-minimum: " << deft << std::endl;
	    if (deft < minimum || deft > maximum) deft = minimum;
//	    std::cerr << "default-minimum: " << deft << std::endl;
	} else {
	    deft = minimum;
	}

    } else if (LADSPA_IS_HINT_DEFAULT_LOW(d)) {
	
	if (logarithmic) {
	    deft = powf(10, logmin * 0.75 + logmax * 0.25);
	} else {
	    deft = minimum * 0.75 + maximum * 0.25;
	}
	
    } else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(d)) {
	
	if (logarithmic) {
	    deft = powf(10, logmin * 0.5 + logmax * 0.5);
	} else {
	    deft = minimum * 0.5 + maximum * 0.5;
	}
	
    } else if (LADSPA_IS_HINT_DEFAULT_HIGH(d)) {
	
	if (logarithmic) {
	    deft = powf(10, logmin * 0.25 + logmax * 0.75);
	} else {
	    deft = minimum * 0.25 + maximum * 0.75;
	}

    } else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(d)) {

	// CMT plugins employ this grossness (setting DEFAULT_MAXIMUM
	// without BOUNDED_ABOVE and then using the UPPER_BOUND as the
	// port default)
	if (!LADSPA_IS_HINT_BOUNDED_ABOVE(d)) {
	    deft = descriptor->PortRangeHints[port].UpperBound;
	    if (LADSPA_IS_HINT_SAMPLE_RATE(d)) {
		deft *= m_sampleRate;
	    }
//	    std::cerr << "default-maximum: " << deft << std::endl;
	    if (deft < minimum || deft > maximum) deft = maximum;
//	    std::cerr << "default-maximum: " << deft << std::endl;
	} else {
	    deft = maximum;
	}

    } else if (LADSPA_IS_HINT_DEFAULT_0(d)) {

        deft = 0.0;

    } else if (LADSPA_IS_HINT_DEFAULT_1(d)) {

        deft = 1.0;

    } else if (LADSPA_IS_HINT_DEFAULT_100(d)) {

        deft = 100.0;

    } else if (LADSPA_IS_HINT_DEFAULT_440(d)) {

        deft = 440.0;

    } else {

        deft = minimum;
    }

//    std::cerr << "port " << port << " default = "<< deft << std::endl;

    return deft;
}

int
LADSPAPluginFactory::getPortDisplayHint(const LADSPA_Descriptor *descriptor, int port)
{
    LADSPA_PortRangeHintDescriptor d =
        descriptor->PortRangeHints[port].HintDescriptor;
    int hint = PluginPort::NoHint;

    if (LADSPA_IS_HINT_TOGGLED(d))
        hint |= PluginPort::Toggled;
    if (LADSPA_IS_HINT_INTEGER(d))
        hint |= PluginPort::Integer;
    if (LADSPA_IS_HINT_LOGARITHMIC(d))
        hint |= PluginPort::Logarithmic;

    return hint;
}


RunnablePluginInstance *
LADSPAPluginFactory::instantiatePlugin(QString identifier,
                                       int instrument,
                                       int position,
                                       unsigned int sampleRate,
                                       unsigned int blockSize,
                                       unsigned int channels)
{
    const LADSPA_Descriptor *descriptor = getLADSPADescriptor(identifier);

    if (descriptor) {

        LADSPAPluginInstance *instance =
            new LADSPAPluginInstance
            (this, instrument, identifier, position, sampleRate, blockSize, channels,
             descriptor);

        m_instances.insert(instance);

        return instance;
    }

    return 0;
}

void
LADSPAPluginFactory::releasePlugin(RunnablePluginInstance *instance,
                                   QString identifier)
{
    if (m_instances.find(instance) == m_instances.end()) {
        RG_WARNING << "WARNING: LADSPAPluginFactory::releasePlugin: Not one of mine!";
        return ;
    }

    QString type, soname, label;
    PluginIdentifier::parseIdentifier(identifier, type, soname, label);

    m_instances.erase(m_instances.find(instance));

    bool stillInUse = false;

    for (std::set
                <RunnablePluginInstance *>::iterator ii = m_instances.begin();
                ii != m_instances.end(); ++ii) {
            QString itype, isoname, ilabel;
            PluginIdentifier::parseIdentifier((*ii)->getIdentifier(), itype, isoname, ilabel);
            if (isoname == soname) {
                //	    std::cerr << "LADSPAPluginFactory::releasePlugin: dll " << soname << " is still in use for plugin " << ilabel << std::endl;
                stillInUse = true;
                break;
            }
        }

    if (!stillInUse) {
        //	std::cerr << "LADSPAPluginFactory::releasePlugin: dll " << soname << " no longer in use, unloading" << std::endl;
        unloadLibrary(soname);
    }
}

const LADSPA_Descriptor *
LADSPAPluginFactory::getLADSPADescriptor(QString identifier)
{
    QString type, soname, label;
    PluginIdentifier::parseIdentifier(identifier, type, soname, label);

    if (m_libraryHandles.find(soname) == m_libraryHandles.end()) {
        loadLibrary(soname);
        if (m_libraryHandles.find(soname) == m_libraryHandles.end()) {
            RG_WARNING << "getLADSPADescriptor() WARNING: loadLibrary failed for " << soname;
            return 0;
        }
    }

    void *libraryHandle = m_libraryHandles[soname];

    LADSPA_Descriptor_Function fn = (LADSPA_Descriptor_Function)
                                    dlsym(libraryHandle, "ladspa_descriptor");

    if (!fn) {
        RG_WARNING << "getLADSPADescriptor() WARNING: No descriptor function in library " << soname;
        return 0;
    }

    const LADSPA_Descriptor *descriptor = 0;

    int index = 0;
    while ((descriptor = fn(index))) {
        if (descriptor->Label == label)
            return descriptor;
        ++index;
    }

    RG_WARNING << "getLADSPADescriptor() WARNING: No such plugin as " << label << " in library " << soname;

    return 0;
}

void
LADSPAPluginFactory::loadLibrary(QString soName)
{
    RG_DEBUG << "loadLibrary(" << soName << ")";

    QByteArray bso = soName.toLocal8Bit();
    void *libraryHandle = dlopen(bso.data(), RTLD_NOW);
    if (!libraryHandle) {
        RG_WARNING << "loadLibrary() failed for" << soName << "-" << dlerror();
        return;
    }

    m_libraryHandles[soName] = libraryHandle;
}

void
LADSPAPluginFactory::unloadLibrary(QString soName)
{
    LibraryHandleMap::iterator li = m_libraryHandles.find(soName);
    if (li != m_libraryHandles.end()) {
        //	std::cerr << "unloading " << soName << std::endl;
        dlclose(m_libraryHandles[soName]);
        m_libraryHandles.erase(li);
    }
}

void
LADSPAPluginFactory::unloadUnusedLibraries()
{
    std::vector<QString> toUnload;

    for (LibraryHandleMap::iterator i = m_libraryHandles.begin();
            i != m_libraryHandles.end(); ++i) {

        bool stillInUse = false;

        for (std::set
                    <RunnablePluginInstance *>::iterator ii = m_instances.begin();
                    ii != m_instances.end(); ++ii) {

                QString itype, isoname, ilabel;
                PluginIdentifier::parseIdentifier((*ii)->getIdentifier(), itype, isoname, ilabel);
                if (isoname == i->first) {
                    stillInUse = true;
                    break;
                }
            }

        if (!stillInUse)
            toUnload.push_back(i->first);
    }

    for (std::vector<QString>::iterator i = toUnload.begin();
            i != toUnload.end(); ++i) {
        unloadLibrary(*i);
    }
}


// It is only later, after they've gone,
// I realize they have delivered a letter.
// It's a letter from my wife.  "What are you doing
// there?" my wife asks.  "Are you drinking?"
// I study the postmark for hours.  Then it, too, begins to fade.
// I hope someday to forget all this.


std::vector<QString>
LADSPAPluginFactory::getPluginPath()
{
    std::vector<QString> pathList;
    std::string path;

    char *cpath = getenv("LADSPA_PATH");
    if (cpath)
        path = cpath;

    if (path == "") {
//      path = "/usr/local/lib/ladspa:/usr/lib/ladspa";
        path = "/usr/local/lib/ladspa:/usr/lib/ladspa:/usr/local/lib64/ladspa:/usr/lib64/ladspa";
        char *home = getenv("HOME");
        if (home)
            path = std::string(home) + "/.ladspa:" + path;
    }

    std::string::size_type index = 0, newindex = 0;

    while ((newindex = path.find(':', index)) < path.size()) {
        pathList.push_back(path.substr(index, newindex - index).c_str());
        index = newindex + 1;
    }

    pathList.push_back(path.substr(index).c_str());

    return pathList;
}

std::vector<QString>
LADSPAPluginFactory::getLRDFPath(QString &baseUri)
{
    std::vector<QString> pathList = getPluginPath();
    std::vector<QString> lrdfPaths;

    lrdfPaths.push_back("/usr/local/share/ladspa/rdf");
    lrdfPaths.push_back("/usr/share/ladspa/rdf");

    for (std::vector<QString>::iterator i = pathList.begin();
            i != pathList.end(); ++i) {
        lrdfPaths.push_back(*i + "/rdf");
    }

    baseUri = LADSPA_BASE;
    return lrdfPaths;
}

void
LADSPAPluginFactory::discoverPlugins()
{
    RG_DEBUG << "discoverPlugins() begin...";

    std::vector<QString> pathList = getPluginPath();

    RG_DEBUG << "discoverPlugins() Paths:";
    for (std::vector<QString>::iterator i = pathList.begin();
         i != pathList.end();
         ++i) {
        RG_DEBUG << "  " << *i;
    }

    // Initialise liblrdf and read the description files
    //
    lrdf_init();

    QString baseUri;
    std::vector<QString> lrdfPaths = getLRDFPath(baseUri);

    bool haveSomething = false;

    for (size_t i = 0; i < lrdfPaths.size(); ++i) {
        QDir dir(lrdfPaths[i], "*.rdf;*.rdfs");
        for (unsigned int j = 0; j < dir.count(); ++j) {
	    QByteArray ba = QString("file:" + lrdfPaths[i] + "/" + dir[j]).toLocal8Bit();
	    if (!lrdf_read_file(ba.data())) {
                //RG_DEBUG << "discoverPlugins(): read RDF file " << (lrdfPaths[i] + "/" + dir[j]);
                haveSomething = true;
            }
        }
    }

    if (haveSomething) {
        generateTaxonomy(baseUri + "Plugin", "");
    }

    generateFallbackCategories();

    for (std::vector<QString>::iterator i = pathList.begin();
            i != pathList.end(); ++i) {

        QDir pluginDir(*i, "*.so");

        for (unsigned int j = 0; j < pluginDir.count(); ++j) {
            discoverPlugin(QString("%1/%2").arg(*i).arg(pluginDir[j]));
        }
    }

    // Cleanup after the RDF library
    //
    lrdf_cleanup();

    RG_DEBUG << "discoverPlugins() end...";
}

void
LADSPAPluginFactory::discoverPlugin(const QString &soName)
{
    QByteArray bso = soName.toLocal8Bit();
    void *libraryHandle = dlopen(bso.data(), RTLD_LAZY);

    if (!libraryHandle) {
        RG_WARNING << "discoverPlugin() WARNING: couldn't dlopen " << soName << " - " << dlerror();
        return ;
    }

    LADSPA_Descriptor_Function fn = (LADSPA_Descriptor_Function)
                                    dlsym(libraryHandle, "ladspa_descriptor");

    if (!fn) {
        RG_WARNING << "discoverPlugin() WARNING: No descriptor function in " << soName;
        return ;
    }

    const LADSPA_Descriptor *descriptor = 0;

    int index = 0;
    while ((descriptor = fn(index))) {

        char * def_uri = 0;
        lrdf_defaults *defs = 0;

        QString category = m_taxonomy[descriptor->UniqueID];

        if (category == "" && descriptor->Name != 0) {
            std::string name = descriptor->Name;
            if (name.length() > 4 &&
                    name.substr(name.length() - 4) == " VST") {
                category = "VST effects";
                m_taxonomy[descriptor->UniqueID] = category;
            }
        }

        //RG_DEBUG << "discoverPlugin(): Plugin id is " << descriptor->UniqueID
        //         << ", category is \"" << (category ? category : QString("(none)"))
        //         << "\", name is " << descriptor->Name
        //         << ", label is " << descriptor->Label;

        def_uri = lrdf_get_default_uri(descriptor->UniqueID);
        if (def_uri) {
            defs = lrdf_get_setting_values(def_uri);
        }

        int controlPortNumber = 1;

        for (unsigned long i = 0; i < descriptor->PortCount; i++) {

            if (LADSPA_IS_PORT_CONTROL(descriptor->PortDescriptors[i])) {

                if (def_uri && defs) {

                    for (unsigned int j = 0; j < defs->count; j++) {
                        if (defs->items[j].pid == (unsigned long)controlPortNumber) {
                            //RG_DEBUG << "discoverPlugin(): Default for this port (" << defs->items[j].pid << ", " << defs->items[j].label << ") is " << defs->items[j].value << "; applying this to port number " << i << " with name " << descriptor->PortNames[i];
                            m_portDefaults[descriptor->UniqueID][i] =
                                defs->items[j].value;
                        }
                    }
                }

                ++controlPortNumber;
            }
        }

        QString identifier = PluginIdentifier::createIdentifier
                             ("ladspa", soName, descriptor->Label);
        //RG_DEBUG << "discoverPlugin(): Added plugin identifier " << identifier;
        m_identifiers.push_back(identifier);

        ++index;
    }

    if (dlclose(libraryHandle) != 0) {
        RG_WARNING << "discoverPlugin() WARNING: can't unload " << libraryHandle;
        return ;
    }
}

void
LADSPAPluginFactory::generateFallbackCategories()
{
    std::vector<QString> pluginPath = getPluginPath();
    std::vector<QString> path;

    for (size_t i = 0; i < pluginPath.size(); ++i) {
//      if (pluginPath[i].contains("/lib/")) {
        if (pluginPath[i].contains("/lib/") || pluginPath[i].contains("/lib64/")) {
            QString p(pluginPath[i]);
            p.replace("/lib/", "/share/");
            p.replace("/lib64/", "/share/");
            path.push_back(p);
            //	    std::cerr << "LADSPAPluginFactory::generateFallbackCategories: path element " << p << std::endl;
        }
        path.push_back(pluginPath[i]);
        //	std::cerr << "LADSPAPluginFactory::generateFallbackCategories: path element " << pluginPath[i] << std::endl;
    }

    for (size_t i = 0; i < path.size(); ++i) {

        QDir dir(path[i], "*.cat");

//        	std::cerr << "LADSPAPluginFactory::generateFallbackCategories: directory " << path[i] << " has " << dir.count() << " .cat files" << std::endl;
        for (unsigned int j = 0; j < dir.count(); ++j) {

            QFile file(path[i] + "/" + dir[j]);

            //	    std::cerr << "LADSPAPluginFactory::generateFallbackCategories: about to open " << (path[i] + "/" + dir[j]) << std::endl;

            if (file.open(QIODevice::ReadOnly)) {
                //		    std::cerr << "...opened" << std::endl;
                QTextStream stream(&file);
                QString line;

				line = stream.readLine();
				while ( !(stream.atEnd() || line.isNull()) ) {	// note: atEnd() does not work with stdin, use line.isNull() instead
                    //		    std::cerr << "line is: \"" << line << "\"" << std::endl;
                    QString id = line.section("::", 0, 0);
                    QString cat = line.section("::", 1, 1);
                    m_fallbackCategories[id] = cat;
//                    		    std::cerr << "set id \"" << id << "\" to cat \"" << cat << "\"" << std::endl;
					line = stream.readLine();
				}
            }
        }
    }
}

void
LADSPAPluginFactory::generateTaxonomy(QString uri, QString base)
{
    QByteArray ba = uri.toLocal8Bit();
    lrdf_uris *uris = lrdf_get_instances(ba.data());

    if (uris != NULL) {
        for (unsigned int i = 0; i < uris->count; ++i) {
            m_taxonomy[lrdf_get_uid(uris->items[i])] = base;
        }
        lrdf_free_uris(uris);
    }

    uris = lrdf_get_subclasses(ba.data());

    if (uris != NULL) {
        for (unsigned int i = 0; i < uris->count; ++i) {
            char *label = lrdf_get_label(uris->items[i]);
            generateTaxonomy(uris->items[i],
                             base + QString(base.length() > 0 ? " > " : "")
			     + QString(label));
        }
        lrdf_free_uris(uris);
    }
}

}
