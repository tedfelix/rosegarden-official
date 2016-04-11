/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MAGIC_LAYER_COMMAND_H
#define RG_MAGIC_LAYER_COMMAND_H

#include "document/Command.h"

#include <QCoreApplication>


namespace Rosegarden
{

class Segment;
class Composition;

class MagicLayerCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::MagicLayerCommand)

public:
    /** Takes an input segment as a template and creates a "layer" based on that
     * template, possessing the same characteristics but for the color, which is
     * mechanically altered in order to promote contrast with the base segment.
     *
     * Removes selected events from the input segment.  Magics them to this newly
     * created segment.  This allows the user to select some notes and presto
     * magnifico shift them into a new layer without a lot of manual faffing
     * about.  (Feature Request #
     */
    MagicLayerCommand(Segment *segment, Composition &composition);

    virtual ~MagicLayerCommand();

    /** Returns a pointer to the newly created layer segment.  Not valid until
     * after invokation
     */
    Segment *getSegment() const;

    virtual void execute();
    virtual void unexecute();
    
private:
    Segment     *m_segment;
    Composition &m_composition;
    bool         m_detached;
};


}

#endif
