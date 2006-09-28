// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _DSSI_PLUGIN_FACTORY_H_
#define _DSSI_PLUGIN_FACTORY_H_

#ifdef HAVE_DSSI

#include "LADSPAPluginFactory.h"
#include <dssi.h>

namespace Rosegarden
{

class DSSIPluginInstance;

class DSSIPluginFactory : public LADSPAPluginFactory
{
public:
    virtual ~DSSIPluginFactory();

    virtual void enumeratePlugins(MappedObjectPropertyList &list);

    virtual void populatePluginSlot(QString identifier, MappedPluginSlot &slot);

    virtual RunnablePluginInstance *instantiatePlugin(QString identifier,
						      int instrumentId,
						      int position,
						      unsigned int sampleRate,
						      unsigned int blockSize,
						      unsigned int channels);

protected:
    DSSIPluginFactory();
    friend class PluginFactory;

    virtual std::vector<QString> getPluginPath();

#ifdef HAVE_LIBLRDF
    virtual std::vector<QString> getLRDFPath(QString &baseUri);
#endif

    virtual void discoverPlugins(QString soName);

    virtual const LADSPA_Descriptor *getLADSPADescriptor(QString identifier);
    virtual const DSSI_Descriptor *getDSSIDescriptor(QString identifier);
};

}

#endif

#endif

