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

#ifndef RG_SF2_PATCH_EXTRACTOR_H
#define RG_SF2_PATCH_EXTRACTOR_H

#include <string>
#include <map>

namespace Rosegarden {

/**
 * Trivial class to suck the patch map out of a .sf2 SoundFont file.
 * Inspired by (but not based on) sftovkb by Takashi Iwai.
 * 
 * SoundFont is a straightforward RIFF format so there's some
 * redundancy between this and RIFFAudioFile -- we don't take any
 * advantage of that, and this class is completely self-contained.
 *
 * Tolerates garbled files; will just suck all it can rather than
 * throw an error, except if the file is not a SoundFont at all.
 */

class SF2PatchExtractor
{
public:
    typedef std::map<int, std::string> Bank;
    typedef std::map<int, Bank> Device;

    struct FileNotFoundException { };
    struct WrongFileFormatException { };

    static bool isSF2File(std::string fileName);
    static Device read(std::string fileName);
};

}

#endif

