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
#include <qxml.h>


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
    std::set<std::string> getCharNames() const;

    bool getStaffLineThickness(int size, unsigned int &thickness) const;
    bool getStemThickness(int size, unsigned int &thickness) const;

    bool getSrc(int size, std::string charName, std::string &src) const;
    bool getInversionSrc(int size, std::string charName, std::string &src) const;

    bool getHotspot(int size, std::string charName, int &x, int &y) const;

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
        SizeData() : m_stemThickness(-1), m_staffLineThickness(-1) { }
        ~SizeData() { }

        void setStemThickness(unsigned int i) {
            m_stemThickness = (int)i;
        }
        void setStaffLineThickness(unsigned int i) {
            m_staffLineThickness = (int)i;
        }

        bool getStemThickness(unsigned int &i) const {
            if (m_stemThickness >= 0) {
                i = (unsigned int)m_stemThickness;
                return true;
            } else return false;
        }

        bool getStaffLineThickness(unsigned int &i) const {
            if (m_staffLineThickness >= 0) {
                i = (unsigned int)m_staffLineThickness;
                return true;
            } else return false;
        }
       
    private:
        int m_stemThickness;
        int m_staffLineThickness;
    };

    std::string m_name;
    std::string m_origin;
    std::string m_copyright;
    std::string m_mappedBy;

    typedef std::map<std::string, SymbolData> SymbolDataMap;
    SymbolDataMap m_data;

    typedef std::map<std::string, HotspotData> HotspotDataMap;
    HotspotDataMap m_hotspots;

    typedef std::map<int, SizeData> SizeDataMap;
    SizeDataMap m_sizes;

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

    /// Returns false + thickness=1 if not specified
    bool getStaffLineThickness(unsigned int &thickness) const;

    /// Returns false + blank pixmap if it can't find the right one
    bool getPixmap(std::string charName, QPixmap &pixmap,
                   bool inverted = false) const;

    /// Returns false + dimensions of blank pixmap if none found
    bool getDimensions(std::string charName, int &x, int &y,
                       bool inverted = false) const;

    /// Ignores problems, returning dimension of blank pixmap if necessary
    int getWidth(std::string charName) const;

    /// Ignores problems, returning dimension of blank pixmap if necessary
    int getHeight(std::string charName) const;

    /// Returns false + centre of pixmap if no hotspot explicitly specified
    bool getHotspot(std::string charName, int &x, int &y,
                    bool inverted = false) const;

private:
    int m_currentSize;
    NoteFontMap m_fontMap;

    QPixmap getBlankPixmap() const;
    std::string getKey(std::string charName) const;

    typedef std::pair<bool, QPixmap> PixmapPair;
    typedef std::map<std::string, PixmapPair> PixmapMap;
    static PixmapMap *m_map;
};


#endif

