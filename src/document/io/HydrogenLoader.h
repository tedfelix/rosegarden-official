
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_HYDROGENLOADER_H
#define RG_HYDROGENLOADER_H

#include "base/PropertyName.h"
#include <string>
#include <vector>


class QString;
class QObject;


namespace Rosegarden
{

class Studio;
class Segment;
class Composition;


/**
 * Hydrogen drum machine file importer - should work for 0.8.1 and above
 * assuming they don't change the file spec without telling us.
 *
 */

class HydrogenLoader
{
public:
    HydrogenLoader(Studio *);

    /**
      * Load and parse the Hydrogen file \a fileName, and write it into the
      * given Composition (clearing the existing segment data first).
      * Return true for success.
      */
    bool load(const QString& fileName, Composition &);

protected:
    Composition *m_composition;
    Studio      *m_studio;
    std::string              m_fileName;

private:
    static const int MAX_DOTS = 4;
    static const PropertyName SKIP_PROPERTY;
};

typedef std::vector<std::pair<std::string, Segment*> > SegmentMap;
typedef std::vector<std::pair<std::string, Segment*> >::iterator SegmentMapIterator;
typedef std::vector<std::pair<std::string, Segment*> >::const_iterator SegmentMapConstIterator;


}

#endif
