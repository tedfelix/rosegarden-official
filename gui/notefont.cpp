// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "notefont.h"

#include <qfileinfo.h>
#include <klocale.h>
#include <iostream>

using std::string;
using std::map;
using std::set;
using std::cout;
using std::cerr;
using std::endl;

NoteFontMap::NoteFontMap(string name) :
    m_name(name),
    m_characterDestination(0),
    m_hotspotCharName("")
{
    m_fontDirectory = "./pixmaps";
//        KGlobal::dirs()->findResource("appdata", "pixmaps");

    QString mapFileName = QString("%1/%2/mapping.xml")
        .arg(m_fontDirectory.c_str())
        .arg(name.c_str());

    QFileInfo mapFileInfo(mapFileName);

    if (!mapFileInfo.isReadable()) {
        throw MappingFileReadFailed(string("Can't open mapping file ") +
                                    mapFileName.latin1());
    }

    QFile mapFile(mapFileName);

    QXmlInputSource source(mapFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    bool ok = reader.parse(source);
    mapFile.close();

    if (!ok) {
        throw MappingFileReadFailed(m_errorString.latin1());
    }
}

NoteFontMap::~NoteFontMap()
{
    // empty
}

bool
NoteFontMap::characters(QString &chars)
{
    if (!m_characterDestination) return true;
    *m_characterDestination += chars.latin1();
    return true;
}


bool
NoteFontMap::startElement(const QString &, const QString &,
                          const QString &qName,
                          const QXmlAttributes &attributes)
{
    QString lcName = qName.lower();
    m_characterDestination = 0;

    // The element names are actually unique within the whole file; 
    // we don't bother checking we're in the right context.  Leave that
    // to the DTD, when we have one.

    if (lcName == "rosegarden-font-encoding") {
        
    } else if (lcName == "font-information") { 

        m_origin = attributes.value("origin").latin1();
        m_copyright = attributes.value("copyright").latin1();
        m_mappedBy = attributes.value("mapped-by").latin1();

    } else if (lcName == "font-sizes") {

    } else if (lcName == "font-size") {

        QString s = attributes.value("note-height");
        if (!s) {
            m_errorString = i18n("note-height not found");
            return false;
        }
        int noteHeight = s.toInt();

        SizeData sizeData;

        s = attributes.value("staff-line-thickness");
        if (s) sizeData.setStaffLineThickness(s.toInt());

        s = attributes.value("stem-thickness");
        if (s) sizeData.setStemThickness(s.toInt());

        m_sizes[noteHeight] = sizeData;

    } else if (lcName == "font-symbol-map") {

    } else if (lcName == "symbol") {

        QString symbolName = attributes.value("name");
        if (!symbolName) {
            m_errorString = i18n("name is a required attribute of symbol");
            return false;
        }

        QString src = attributes.value("src");
        if (!src) {
            m_errorString = i18n("src is a required attribute of symbol (until real font support is implemented)");
            return false;
        }

        SymbolData symbolData;
        symbolData.setSrc(src.latin1());

        QString inversionSrc = attributes.value("inversion-src");
        if (inversionSrc) symbolData.setInversion(inversionSrc.latin1());

        m_data[symbolName.upper().latin1()] = symbolData;

    } else if (lcName == "font-hotspots") {

    } else if (lcName == "hotspot") {

        QString s = attributes.value("name");
        if (!s) {
            m_errorString = i18n("name is a required attribute of hotspot");
            return false;
        }
        m_hotspotCharName = s.upper().latin1();

    } else if (lcName == "when") {

        if (m_hotspotCharName == "") {
            m_errorString = i18n("when-element must be in hotspot-element");
            return false;
        }

        QString s = attributes.value("note-height");
        if (!s) {
            m_errorString = i18n("note-height is a required attribute of when");
            return false;
        }
        int noteHeight = s.toInt();

        s = attributes.value("x");
        if (!s) {
            m_errorString = i18n("x is a required attribute of when");
            return false;
        }
        int x = s.toInt();

        s = attributes.value("y");
        if (!s) {
            m_errorString = i18n("y is a required attribute of when");
            return false;
        }
        int y = s.toInt();

        HotspotDataMap::iterator i = m_hotspots.find(m_hotspotCharName);
        if (i == m_hotspots.end()) {
            m_hotspots[m_hotspotCharName] = HotspotData();
            i = m_hotspots.find(m_hotspotCharName);
        }

        i->second.addHotspot(noteHeight, x, y);

    } else {

    }

    if (m_characterDestination) *m_characterDestination = "";
    return true;
}

set<int>
NoteFontMap::getSizes() const
{
    set<int> sizes;

    for (SizeDataMap::const_iterator i = m_sizes.begin();
         i != m_sizes.end(); ++i) {
        sizes.insert(i->first);
    }

    return sizes;
}

set<string>
NoteFontMap::getCharNames() const
{
    set<string> names;

    for (SymbolDataMap::const_iterator i = m_data.begin();
         i != m_data.end(); ++i) {
        names.insert(i->first);
    }

    return names;
}

bool
NoteFontMap::checkFile(int size, string &src) const
{
    QString pixmapFileName = QString("%1/%2/%3/%4.xpm")
        .arg(m_fontDirectory.c_str())
        .arg(m_name.c_str())
        .arg(size)
        .arg(src.c_str());
    src = pixmapFileName.latin1();

    QFileInfo pixmapFileInfo(pixmapFileName);

    if (!pixmapFileInfo.isReadable()) {
        cerr << "Warning: Unable to open pixmap file " << pixmapFileName
             << endl;
        return false;
    }

    return true;
}


bool
NoteFontMap::getSrc(int size, string charName, string &src) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    src = i->second.getSrc();
    return checkFile(size, src);
}

bool
NoteFontMap::getInversionSrc(int size, string charName, string &src) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    if (!i->second.hasInversion()) return false;
    src = i->second.getInversion();
    return checkFile(size, src);
}

bool
NoteFontMap::getStaffLineThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getStaffLineThickness(thickness);
}

bool
NoteFontMap::getStemThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getStemThickness(thickness);
}

bool
NoteFontMap::getHotspot(int size, string charName, int &x, int &y) const
{
    HotspotDataMap::const_iterator i = m_hotspots.find(charName);
    if (i == m_hotspots.end()) return false;

    return i->second.getHotspot(size, x, y);
}

void
NoteFontMap::dump() const
{
    // debug code

    cout << "Font data:\nName: " << getName() << "\nOrigin: " << getOrigin()
         << "\nCopyright: " << getCopyright() << "\nMapped by: "
         << getMappedBy() << endl;

    set<int> sizes = getSizes();
    set<string> names = getCharNames();

    for (set<int>::iterator sizei = sizes.begin(); sizei != sizes.end();
         ++sizei) {

        cout << "\nSize: " << *sizei << "\n" << endl;

        unsigned int t = 0;

        if (getStaffLineThickness(*sizei, t)) {
            cout << "Staff line thickness: " << t << endl;
        }

        if (getStemThickness(*sizei, t)) {
            cout << "Stem thickness: " << t << endl;
        }

        for (set<string>::iterator namei = names.begin();
             namei != names.end(); ++namei) {

            cout << "\nCharacter: " << *namei << endl;

            string s;
            int x, y;

            if (getSrc(*sizei, *namei, s)) {
                cout << "Src: " << s << endl;
            }

            if (getInversionSrc(*sizei, *namei, s)) {
                cout << "Inversion src: " << s << endl;
            }
            
            if (getHotspot(*sizei, *namei, x, y)) {
                cout << "Hot spot: (" << x << "," << y << ")" << endl;
            }
        }
    }
}


NoteFont::PixmapMap *NoteFont::m_map = 0;

NoteFont::NoteFont(string fontName, int size) :
    m_fontMap(fontName)
{
    std::set<int> sizes = m_fontMap.getSizes();

    if (sizes.size() > 0) {
        m_currentSize = *sizes.begin();
    } else {
        throw BadFont("No sizes available");
    }

    if (size > 0) {
        if (sizes.find(size) == sizes.end()) {
            throw BadFont("Font not available in this size");
        } else {
            m_currentSize = size;
        }
    }

    if (m_map == 0) {
        m_map = new PixmapMap();
    }
}

NoteFont::~NoteFont()
{ 
    // empty
}

bool
NoteFont::getStemThickness(unsigned int &thickness) const
{
    thickness = 1;
    return m_fontMap.getStemThickness(m_currentSize, thickness);
}

bool
NoteFont::getStaffLineThickness(unsigned int &thickness) const
{
    thickness = 1;
    return m_fontMap.getStaffLineThickness(m_currentSize, thickness);
}

string
NoteFont::getKey(string charName) const
{
    QString s = QString("__%1__%2__%3__")
        .arg(m_fontMap.getName().c_str())
        .arg(m_currentSize)
        .arg(charName.c_str());
    return s.latin1();
}

bool
NoteFont::getPixmap(string charName, QPixmap &pixmap, bool inverted) const
{
    string key(getKey(charName));

    PixmapMap::iterator i = m_map->find(key);
    if (i != m_map->end()) {
        pixmap = i->second.second;
        return i->second.first;
    }

    string src;
    bool ok = false;

    if (!inverted) ok = m_fontMap.getSrc(m_currentSize, charName, src);
    else  ok = m_fontMap.getInversionSrc(m_currentSize, charName, src);
    
    if (ok) {
        pixmap = QPixmap(src.c_str());
        if (!pixmap.isNull()) {
            (*m_map)[key] = PixmapPair(true, pixmap);
            return true;
        }
        cerr << "Warning: Unable to read pixmap file " << src << endl;
    }

    pixmap = getBlankPixmap();
    (*m_map)[key] = PixmapPair(false, pixmap);
    return false;
}

QPixmap
NoteFont::getBlankPixmap() const
{
    return QPixmap(10, 10);
}

bool
NoteFont::getDimensions(string charName, int &x, int &y, bool inverted) const
{
    QPixmap pixmap;
    bool ok = getPixmap(charName, pixmap, inverted);
    x = pixmap.width();
    y = pixmap.height();
    return ok;
}

int
NoteFont::getWidth(string charName) const
{
    int x, y;
    getDimensions(charName, x, y);
    return x;
}

int
NoteFont::getHeight(string charName) const
{
    int x, y;
    getDimensions(charName, x, y);
    return y;
}

bool
NoteFont::getHotspot(string charName, int &x, int &y, bool inverted) const
{
    bool ok = m_fontMap.getHotspot(m_currentSize, charName, x, y);

    if (!ok) {
        int w, h;
        getDimensions(charName, w, h, inverted);
        x = w/2;
        y = h/2;
    }

    if (inverted) {
        y = getHeight(charName) - y;
    }
    
    return ok;
}

const std::string NoteCharNames::SHARP = "MUSIC SHARP SIGN"; 
const std::string NoteCharNames::FLAT = "MUSIC FLAT SIGN";
const std::string NoteCharNames::NATURAL = "MUSIC NATURAL SIGN";
const std::string NoteCharNames::DOUBLE_SHARP = "MUSICAL SYMBOL DOUBLE SHARP";
const std::string NoteCharNames::DOUBLE_FLAT = "MUSICAL SYMBOL DOUBLE FLAT";

const std::string NoteCharNames::BREVE = "MUSICAL SYMBOL BREVE";
const std::string NoteCharNames::WHOLE_NOTE = "MUSICAL SYMBOL WHOLE NOTE";
const std::string NoteCharNames::VOID_NOTEHEAD = "MUSICAL SYMBOL VOID NOTEHEAD";
const std::string NoteCharNames::NOTEHEAD_BLACK = "MUSICAL SYMBOL NOTEHEAD BLACK";

const std::string NoteCharNames::FLAG_1 = "MUSICAL SYMBOL COMBINING FLAG-1";
const std::string NoteCharNames::FLAG_2 = "MUSICAL SYMBOL COMBINING FLAG-2";
const std::string NoteCharNames::FLAG_3 = "MUSICAL SYMBOL COMBINING FLAG-3";
const std::string NoteCharNames::FLAG_4 = "MUSICAL SYMBOL COMBINING FLAG-4";

const std::string NoteCharNames::MULTI_REST = "MUSICAL SYMBOL MULTI REST";
const std::string NoteCharNames::WHOLE_REST = "MUSICAL SYMBOL WHOLE REST";
const std::string NoteCharNames::HALF_REST = "MUSICAL SYMBOL HALF REST";
const std::string NoteCharNames::QUARTER_REST = "MUSICAL SYMBOL QUARTER REST";
const std::string NoteCharNames::EIGHTH_REST = "MUSICAL SYMBOL EIGHTH REST";
const std::string NoteCharNames::SIXTEENTH_REST = "MUSICAL SYMBOL SIXTEENTH REST";
const std::string NoteCharNames::THIRTY_SECOND_REST = "MUSICAL SYMBOL THIRTY-SECOND REST";
const std::string NoteCharNames::SIXTY_FOURTH_REST = "MUSICAL SYMBOL SIXTY-FOURTH REST";
const std::string NoteCharNames::DOT = "MUSICAL SYMBOL COMBINING AUGMENTATION DOT";
const std::string NoteCharNames::C_CLEF = "MUSICAL SYMBOL C CLEF";
const std::string NoteCharNames::G_CLEF = "MUSICAL SYMBOL G CLEF";
const std::string NoteCharNames::F_CLEF = "MUSICAL SYMBOL F CLEF";

