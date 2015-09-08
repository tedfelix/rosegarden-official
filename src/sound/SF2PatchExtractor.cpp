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

#include "SF2PatchExtractor.h"

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <stdint.h>
#include <sys/types.h>

namespace Rosegarden
{

using std::string;
using std::cerr;
using std::endl;
using std::ifstream;
using std::ios;


struct Chunk
{
    char id[4];
    uint32_t size;

    Chunk(ifstream *, bool idOnly = false);
    bool isa(std::string s);
};

Chunk::Chunk(ifstream *file, bool idOnly)
{
    file->read((char *)this->id, 4);
    size = 0;

    if (idOnly)
        return ;

    unsigned char sz[4];
    file->read((char *)sz, 4);
    for (int i = 0; i < 4; ++i)
        size += sz[i] << (i * 8);
}

bool
Chunk::isa(string s)
{
    return string(id, 4) == s;
}

bool
SF2PatchExtractor::isSF2File(string fileName)
{
    ifstream *file = new ifstream(fileName.c_str(), ios::in | ios::binary);
    if (!file)
        throw FileNotFoundException();

    Chunk riffchunk(file);
    if (!riffchunk.isa("RIFF")) {
        file->close();
        return false;
    }

    Chunk sfbkchunk(file, true);
    if (!sfbkchunk.isa("sfbk")) {
        file->close();
        return false;
    }

    file->close();
    return true;
}

#define SF2_PRESET_HEADER_SIZE 38

SF2PatchExtractor::Device
SF2PatchExtractor::read(string fileName)
{
    Device device;

    ifstream *file = new ifstream(fileName.c_str(), ios::in | ios::binary);
    if (!file)
        throw FileNotFoundException();

    Chunk riffchunk(file);
    if (!riffchunk.isa("RIFF")) {
        file->close();
        throw WrongFileFormatException();
    }

    Chunk sfbkchunk(file, true);
    if (!sfbkchunk.isa("sfbk")) {
        file->close();
        throw WrongFileFormatException();
    }

    while (!file->eof()) {

        Chunk chunk(file);

        if (!chunk.isa("LIST")) {
            // cerr << "Skipping " << string(chunk.id, 4) << endl;
            file->seekg(chunk.size, ios::cur);
            continue;
        }

        Chunk listchunk(file, true);
        if (!listchunk.isa("pdta")) {
            // cerr << "Skipping " << string(id, 4) << endl;
            file->seekg(chunk.size - 4, ios::cur);
            continue;
        }

        int size = chunk.size - 4;
        while (size > 0) {

            Chunk pdtachunk(file);
            size -= 8 + pdtachunk.size;
            if (file->eof()) {
                break;
            }

            if (!pdtachunk.isa("phdr")) { // preset header
                // cerr << "Skipping " << string(pdtachunk.id, 4) << endl;
                file->seekg(pdtachunk.size, ios::cur);
                continue;
            }

            int presets = (pdtachunk.size / SF2_PRESET_HEADER_SIZE) - 1;
            for (int i = 0; i < presets; ++i) {

                char name[21];
                uint16_t bank, program;

                file->read((char *)name, 20);
                name[20] = '\0';
                file->read((char *)&program, 2);
                file->read((char *)&bank, 2);

                // cerr << "Read name as " << name << endl;

                file->seekg(14, ios::cur);
                device[bank][program] = name;
            }

            file->seekg(SF2_PRESET_HEADER_SIZE, ios::cur);
        }
        break;
    }

    file->close();
    return device;
}

}


#ifdef TEST_SF2_PATCH_EXTRACTOR

int main(int argc, char **argv)
{
    using SF2PatchExtractor;

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " sf2filename" << std::endl;
        return 2;
    }

    try {
        SF2PatchExtractor::Device device =
            SF2PatchExtractor::read(argv[1]);

        std::cerr << "Done.  Presets are:" << std::endl;

        for (SF2PatchExtractor::Device::iterator di = device.begin();
                di != device.end(); ++di) {

            std::cerr << "Bank " << di->first << ":" << std::endl;

            for (SF2PatchExtractor::Bank::iterator bi = di->second.begin();
                    bi != di->second.end();
                    ++bi) {

                std::cerr << "Program " << bi->first << ": \"" << bi->second
                << "\"" << std::endl;
            }
        }
    } catch (SF2PatchExtractor::WrongFileFormatException) {
        std::cerr << "Wrong file format" << std::endl;
    } catch (SF2PatchExtractor::FileNotFoundException) {
        std::cerr << "File not found or couldn't be opened" << std::endl;
    }

    return 0;
}

#endif
