// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#include "notecharname.h"
#include "NotationTypes.h"
#include "Exception.h"


// Helper class for looking up information about a font

class NoteFontMap : public QXmlDefaultHandler
{
public:
    typedef Rosegarden::Exception MappingFileReadFailed;

    NoteFontMap(std::string name); // load and parse the XML mapping file
    ~NoteFontMap();

    /**
     * ok() returns false if the file read succeeded but the font
     * relies on system fonts that are not available.  (If the file
     * read fails, the constructor throws MappingFileReadFailed.)
     */
    bool ok() const { return m_ok; }

    std::string getName() const { return m_name; }
    std::string getOrigin() const { return m_origin; }
    std::string getCopyright() const { return m_copyright; }
    std::string getMappedBy() const { return m_mappedBy; }
    std::string getType() const { return m_type; }
    bool shouldAutocrop() const { return m_autocrop; }
    bool isSmooth() const { return m_smooth; }

    std::set<int> getSizes() const;
    std::set<CharName> getCharNames() const;

    bool getStaffLineThickness(int size, unsigned int &thickness) const;
    bool getStemThickness(int size, unsigned int &thickness) const;
    bool getBeamThickness(int size, unsigned int &thickness) const;
    bool getStemLength(int size, unsigned int &length) const;
    bool getFlagSpacing(int size, unsigned int &spacing) const;
    bool getBorderThickness(int size, unsigned int &X, unsigned int &y) const;
    
    bool hasInversion(int size, CharName charName) const;

    bool getSrc(int size, CharName charName, std::string &src) const;
    bool getInversionSrc(int size, CharName charName, std::string &src) const;

    bool getFont(int size, CharName charName, QFont &font, int &charBase) const;
    bool getCode(int size, CharName charName, int &code) const;
    bool getInversionCode(int size, CharName charName, int &code) const;

    bool getHotspot(int size, CharName charName, int &x, int &y) const;

    // Xml handler methods:

    virtual bool startElement
    (const QString& namespaceURI, const QString& localName,
     const QString& qName, const QXmlAttributes& atts);

    virtual bool characters(QString &);

    bool error(const QXmlParseException& exception);
    bool fatalError(const QXmlParseException& exception);

    void dump() const;

    // want this to be private, but need access from HotspotData
    static int toSize(int baseSize, double factor, bool limitAtOne);

private:
    class SymbolData
    {
    public:
        SymbolData() : m_fontId(0),
		       m_src(""), m_inversionSrc(""),
		       m_code(-1), m_inversionCode(-1) { }
        ~SymbolData() { }

	void setFontId(int id) { m_fontId = id; }
	int  getFontId() const { return m_fontId; }

        void setSrc(std::string src) { m_src = src; }
        std::string getSrc() const { return m_src; }

	void setCode(int code) { m_code = code; }
	int  getCode() const { return m_code; }

        void setInversionSrc(std::string inversion) { m_inversionSrc = inversion; }
        std::string getInversionSrc() const { return m_inversionSrc; }

        void setInversionCode(int code) { m_inversionCode = code; }
        int  getInversionCode() const { return m_inversionCode; }

        bool hasInversion() const {
	    return m_inversionCode >= 0 || m_inversionSrc != "";
	}

    private:
	int m_fontId;
        std::string m_src;
        std::string m_inversionSrc;
	int m_code;
	int m_inversionCode;
    };

    class HotspotData
    {
    private:
        typedef std::pair<int, int> Point;
        typedef std::pair<double, double> ScaledPoint;
        typedef std::map<int, Point> DataMap;

    public:
        HotspotData() : m_scaled(-1.0, -1.0) { }
        ~HotspotData() { }
        
        void addHotspot(int size, int x, int y) {
            m_data[size] = Point(x, y);
        }

	void setScaledHotspot(double x, double y) {
	    m_scaled = ScaledPoint(x, y);
	}

        bool getHotspot(int size, int &x, int &y) const;

    private:
        DataMap m_data;
	ScaledPoint m_scaled;
    };

    class SizeData
    {
    public:
        SizeData() : m_stemThickness(-1),
                     m_beamThickness(-1),
                     m_stemLength(-1),
                     m_flagSpacing(-1),
                     m_staffLineThickness(-1),
                     m_borderX(-1), m_borderY(-1),
		     m_fontHeight(-1) { }
        ~SizeData() { }

        void setStemThickness(unsigned int i) {
            m_stemThickness = (int)i;
        }
        void setBeamThickness(unsigned int i) {
            m_beamThickness = (int)i;
        }
        void setStemLength(unsigned int i) {
            m_stemLength = (int)i;
        }
        void setFlagSpacing(unsigned int i) {
            m_flagSpacing = (int)i;
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
	void setFontHeight(unsigned int h) {
	    m_fontHeight = (int)h;
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

        bool getStemLength(unsigned int &i) const {
            if (m_stemLength >= 0) {
                i = (unsigned int)m_stemLength;
                return true;
            } else return false;
        }

        bool getFlagSpacing(unsigned int &i) const {
            if (m_flagSpacing >= 0) {
                i = (unsigned int)m_flagSpacing;
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

	bool getFontHeight(unsigned int &h) const {
	    if (m_fontHeight >= 0) {
		h = (unsigned int)m_fontHeight;
		return true;
	    } else return false;
	}	
       
    private:
        int m_stemThickness;
        int m_beamThickness;
        int m_stemLength;
        int m_flagSpacing;
        int m_staffLineThickness;
        int m_borderX;
        int m_borderY;
	int m_fontHeight;
    };

    //--------------- Data members ---------------------------------

    std::string m_name;
    std::string m_origin;
    std::string m_copyright;
    std::string m_mappedBy;
    std::string m_type;
    bool m_autocrop;
    bool m_smooth;

    typedef __HASH_NS::hash_map<CharName, SymbolData,
                          CharNameHash, CharNamesEqual> SymbolDataMap;
    SymbolDataMap m_data;

    typedef __HASH_NS::hash_map<CharName, HotspotData,
                          CharNameHash, CharNamesEqual> HotspotDataMap;
    HotspotDataMap m_hotspots;

    typedef __HASH_NS::hash_map<int, SizeData> SizeDataMap;
    SizeDataMap m_sizes;

    typedef __HASH_NS::hash_map<int, QFont> SystemFontMap;
    SystemFontMap m_fonts;

    typedef __HASH_NS::hash_map<int, int> CharBaseMap;
    CharBaseMap m_bases;

    // For use when reading the XML file:
    bool m_expectingCharacters;
    std::string *m_characterDestination;
    std::string m_hotspotCharName;
    QString m_errorString;

    bool checkFont(QString name, int size, QFont &font) const;
    bool checkFile(int size, std::string &src) const;
    QString m_fontDirectory;

    bool m_ok;
};


// Encapsulates NoteFontMap, and loads pixmaps etc on demand

class NoteFont
{
public:
    typedef Rosegarden::Exception BadFont;
    ~NoteFont();

    std::string getName() const { return m_fontMap.getName(); }
    int getSize() const { return m_size; }
    bool isSmooth() const { return m_fontMap.isSmooth(); }
    const NoteFontMap &getNoteFontMap() const { return m_fontMap; }

    /// Returns false + thickness=1 if not specified
    bool getStemThickness(unsigned int &thickness) const;

    /// Returns false + a guess at suitable thickness if not specified
    bool getBeamThickness(unsigned int &thickness) const;

    /// Returns false + a guess at suitable length if not specified
    bool getStemLength(unsigned int &length) const;

    /// Returns false + a guess at suitable spacing if not specified
    bool getFlagSpacing(unsigned int &spacing) const;

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
    QCanvasPixmap* getCanvasPixmap
    (CharName charName, bool inverted = false) const;



    /// Returns false + blank pixmap if it can't find the right one
    bool getColouredPixmap(CharName charName, QPixmap &pixmap,
                           int hue, int minValue,
			   bool inverted = false) const;

    /// Ignores problems, returning blank pixmap if necessary
    QPixmap getColouredPixmap(CharName charName, int hue, int minValue,
                              bool inverted = false) const;

    /// Ignores problems, returning blank canvas pixmap if necessary
    QCanvasPixmap* getColouredCanvasPixmap
    (CharName charName, int hue, int minValue,
     bool inverted = false) const;



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
    friend class NoteFontFactory;
    NoteFont(std::string fontName, int size = 0);
    std::set<int> getSizes() const { return m_fontMap.getSizes(); }

    bool lookup(CharName charName, bool inverted, QPixmap *&pixmap) const;
    void add(CharName charName, bool inverted, QPixmap *pixmap) const;

    CharName getNameWithColour(CharName origName, int hue) const;
    QPixmap *recolour(QPixmap in, int hue, int minValue) const;

    typedef std::pair<QPixmap *, QPixmap *>
            PixmapPair;

    typedef __HASH_NS::hash_map<CharName, PixmapPair, CharNameHash, CharNamesEqual>
            PixmapMap;

    typedef std::map<std::string, PixmapMap *>
            FontPixmapMap;

    //--------------- Data members ---------------------------------

    int m_size;
    NoteFontMap m_fontMap;

    mutable PixmapMap *m_map; // pointer at a member of m_fontPixmapMap
    static FontPixmapMap *m_fontPixmapMap;
    static QPixmap *m_blankPixmap;
};


class NoteFontFactory
{
public:
    typedef Rosegarden::Exception NoFontsAvailable;

    // Any method passed a fontName argument may throw BadFont or
    // MappingFileReadFailed; any other method may throw NoFontsAvailable

    static NoteFont *getFont(std::string fontName, int size);

    static std::set<std::string> getFontNames();
    static std::vector<int> getAllSizes(std::string fontName); // sorted
    static std::vector<int> getScreenSizes(std::string fontName); // sorted

    static std::string getDefaultFontName();
    static int getDefaultSize(std::string fontName);

private:
    static std::set<std::string> m_fontNames;
    static std::map<std::pair<std::string, int>, NoteFont *> m_fonts;
};


#endif

