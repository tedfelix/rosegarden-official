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

#ifndef _NOTE_FONT_H_
#define _NOTE_FONT_H_

#include <string>
#include <set>
#include <map>

#include <qstring.h>
#include <qpixmap.h>
#include <qcanvas.h>
#include <qxml.h>

#include "PropertyName.h"
#include "NotationTypes.h"

#if (__GNUC__ < 3)
#include <hash_map>
#else
#include <ext/hash_map>
#endif

typedef Rosegarden::PropertyName CharName;
typedef Rosegarden::PropertyNameHash CharNameHash;
typedef Rosegarden::PropertyNamesEqual CharNamesEqual;


// Helper class for looking up information about a font

class NoteFontMap : public QXmlDefaultHandler
{
public:
    struct MappingFileReadFailed {
        MappingFileReadFailed(std::string r) : reason(r) { }
        std::string reason;
    };

    NoteFontMap(std::string name); // load and parse the XML mapping file
    ~NoteFontMap();

    std::string getName() const { return m_name; }
    std::string getOrigin() const { return m_origin; }
    std::string getCopyright() const { return m_copyright; }
    std::string getMappedBy() const { return m_mappedBy; }

    std::set<int> getSizes() const;
    std::set<CharName> getCharNames() const;

    bool getStaffLineThickness(int size, unsigned int &thickness) const;
    bool getStemThickness(int size, unsigned int &thickness) const;
    bool getBeamThickness(int size, unsigned int &thickness) const;
    bool getBorderThickness(int size, unsigned int &X, unsigned int &y) const;

    bool getSrc(int size, CharName charName, std::string &src) const;
    bool getInversionSrc(int size, CharName charName, std::string &src) const;

    bool getHotspot(int size, CharName charName, int &x, int &y) const;

    // Xml handler methods:

    virtual bool startElement
    (const QString& namespaceURI, const QString& localName,
     const QString& qName, const QXmlAttributes& atts);

    virtual bool characters(QString &);

    void dump() const;

private:
    class SymbolData
    {
    public:
        SymbolData() : m_src(""), m_inversion("") { }
        ~SymbolData() { }

        void setSrc(std::string src) { m_src = src; }
        std::string getSrc() const { return m_src; }

        void setInversion(std::string inversion) { m_inversion = inversion; }
        bool hasInversion() const { return m_inversion != ""; }
        std::string getInversion() const { return m_inversion; }

    private:
        std::string m_src;
        std::string m_inversion;
    };

    class HotspotData
    {
    private:
        typedef std::pair<int, int> Point;
        typedef std::map<int, Point> DataMap;

    public:
        HotspotData() { }
        ~HotspotData() { }
        
        void addHotspot(int size, int x, int y) {
            m_data[size] = Point(x, y);
        }

        bool getHotspot(int size, int &x, int &y) const {
            DataMap::const_iterator i = m_data.find(size);
            if (i == m_data.end()) return false;
            x = i->second.first;
            y = i->second.second;
            return true;
        }

    private:
        DataMap m_data;
    };

    class SizeData
    {
    public:
        SizeData() : m_stemThickness(-1),
                     m_beamThickness(-1),
                     m_staffLineThickness(-1),
                     m_borderX(-1), m_borderY(-1) { }
        ~SizeData() { }

        void setStemThickness(unsigned int i) {
            m_stemThickness = (int)i;
        }
        void setBeamThickness(unsigned int i) {
            m_beamThickness = (int)i;
        }
        void setStaffLineThickness(unsigned int i) {
            m_staffLineThickness = (int)i;
        }
        void setBorderX(unsigned int x) {
            m_borderX = (int)x;
        }
        void setBorderY(unsigned int y) {
            m_borderY = (int)y;
        }

        bool getStemThickness(unsigned int &i) const {
            if (m_stemThickness >= 0) {
                i = (unsigned int)m_stemThickness;
                return true;
            } else return false;
        }

        bool getBeamThickness(unsigned int &i) const {
            if (m_beamThickness >= 0) {
                i = (unsigned int)m_beamThickness;
                return true;
            } else return false;
        }

        bool getStaffLineThickness(unsigned int &i) const {
            if (m_staffLineThickness >= 0) {
                i = (unsigned int)m_staffLineThickness;
                return true;
            } else return false;
        }

        bool getBorderThickness(unsigned int &x, unsigned int &y) const {
            if (m_borderX >= 0) x = m_borderX;
            else x = 0;
            if (m_borderY >= 0) y = m_borderY;
            else y = 0;
            return (m_borderX >= 0 || m_borderY >= 0);
        }
       
    private:
        int m_stemThickness;
        int m_beamThickness;
        int m_staffLineThickness;
        int m_borderX;
        int m_borderY;
    };

    std::string m_name;
    std::string m_origin;
    std::string m_copyright;
    std::string m_mappedBy;

    typedef std::hash_map<CharName, SymbolData,
                          CharNameHash, CharNamesEqual> SymbolDataMap;
    SymbolDataMap m_data;

    typedef std::hash_map<CharName, HotspotData,
                          CharNameHash, CharNamesEqual> HotspotDataMap;
    HotspotDataMap m_hotspots;

    typedef std::hash_map<int, SizeData> SizeDataMap;
    SizeDataMap m_sizes;

    // For use when reading the XML file:
    bool m_expectingCharacters;
    std::string *m_characterDestination;
    std::string m_hotspotCharName;
    QString m_errorString;

    bool checkFile(int size, std::string &src) const;
    std::string m_fontDirectory;
};


// Encapsulates NoteFontMap, and loads pixmaps etc on demand

class NoteFont
{
public:
    struct BadFont {
        BadFont(std::string r) : reason(r) { }
        std::string reason;
    };

    // can throw BadFont, MappingFileReadFailed:
    NoteFont(std::string fontName, int size = 0);
    ~NoteFont();

    const NoteFontMap &getNoteFontMap() const { return m_fontMap; }

    std::set<int> getSizes() const { return m_fontMap.getSizes(); }

    int getCurrentSize() const { return m_currentSize; }
    void setCurrentSize(int s) { m_currentSize = s; }



    /// Returns false + thickness=1 if not specified
    bool getStemThickness(unsigned int &thickness) const;

    /// Returns false + a guess at suitable thickness if not specified
    bool getBeamThickness(unsigned int &thickness) const;

    /// Returns false + thickness=1 if not specified
    bool getStaffLineThickness(unsigned int &thickness) const;

    /// Returns false + thickness=0,0 if not specified
    bool getBorderThickness(unsigned int &x, unsigned int &y) const;



    /// Returns false + blank pixmap if it can't find the right one
    bool getPixmap(CharName charName, QPixmap &pixmap,
                   bool inverted = false) const;

    /// Ignores problems, returning blank pixmap if necessary
    QPixmap getPixmap(CharName charName, bool inverted = false) const;

    /// Ignores problems, returning blank canvas pixmap if necessary
    QCanvasPixmap getCanvasPixmap
    (CharName charName, bool inverted = false) const;



    /// Returns false + dimensions of blank pixmap if none found
    bool getDimensions(CharName charName, int &x, int &y,
                       bool inverted = false) const;

    /// Ignores problems, returning dimension of blank pixmap if necessary
    int getWidth(CharName charName) const;

    /// Ignores problems, returning dimension of blank pixmap if necessary
    int getHeight(CharName charName) const;



    /// Returns false + centre-left of pixmap if no hotspot specified
    bool getHotspot(CharName charName, int &x, int &y,
                    bool inverted = false) const;

    /// Ignores problems, returns centre-left of pixmap if necessary
    QPoint getHotspot(CharName charName, bool inverted = false) const;


private:
    int m_currentSize;
    NoteFontMap m_fontMap;

    QPixmap *lookup(CharName charName, bool inverted) const;
    void add(CharName charName, bool inverted, QPixmap *pixmap) const;

    typedef std::pair<QPixmap *, QPixmap *>
            PixmapPair;

    typedef std::hash_map<CharName, PixmapPair, CharNameHash, CharNamesEqual>
            PixmapMap;

    typedef std::map<std::string, PixmapMap *>
            FontPixmapMap;

    mutable PixmapMap *m_map; // pointer at a member of m_fontPixmapMap
    static FontPixmapMap *m_fontPixmapMap;
    static QPixmap *m_blankPixmap;
};


/// A selection of Unicode character names for symbols in a note font

namespace NoteCharacterNames
{
extern const CharName SHARP;
extern const CharName FLAT;
extern const CharName NATURAL;
extern const CharName DOUBLE_SHARP;
extern const CharName DOUBLE_FLAT;

extern const CharName BREVE;
extern const CharName WHOLE_NOTE;
extern const CharName VOID_NOTEHEAD;
extern const CharName NOTEHEAD_BLACK;

extern const CharName FLAG_1;
extern const CharName FLAG_2;
extern const CharName FLAG_3;
extern const CharName FLAG_4;

extern const CharName MULTI_REST;
extern const CharName WHOLE_REST;
extern const CharName HALF_REST;
extern const CharName QUARTER_REST;
extern const CharName EIGHTH_REST;
extern const CharName SIXTEENTH_REST;
extern const CharName THIRTY_SECOND_REST;
extern const CharName SIXTY_FOURTH_REST;

extern const CharName DOT;

extern const CharName C_CLEF;
extern const CharName G_CLEF;
extern const CharName F_CLEF;

extern const CharName UNKNOWN;
}

class NoteCharacterNameLookup
{
public:
    static CharName getAccidentalCharName(const Rosegarden::Accidental &);
    static CharName getClefCharName(const Rosegarden::Clef &);
    static CharName getRestCharName(const Rosegarden::Note::Type &);
    static CharName getFlagCharName(int flagCount);
    static CharName getNoteHeadCharName(const Rosegarden::Note::Type &);
};


#endif

