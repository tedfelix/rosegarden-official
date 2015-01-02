/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_DSSI_PLUGIN_FACTORY_H
#define RG_DSSI_PLUGIN_FACTORY_H

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

    virtual std::vector<QString> getLRDFPath(QString &baseUri);

    virtual void discoverPlugins(QString soName);

    virtual const LADSPA_Descriptor *getLADSPADescriptor(QString identifier);
    virtual const DSSI_Descriptor *getDSSIDescriptor(QString identifier);
};

}

#endif

