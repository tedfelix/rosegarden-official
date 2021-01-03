
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTEFONTMAP_H
#define RG_NOTEFONTMAP_H

#include "base/Exception.h"
#include <map>
#include <set>
#include <string>
#include "SystemFont.h"
#include <QString>
#include <QStringList>
#include <utility>
#include <qxml.h>
#include "gui/editors/notation/NoteCharacterNames.h"

class QXmlParseException;
class QXmlAttributes;


namespace Rosegarden
{



class NoteFontMap : public QXmlDefaultHandler
{
public:
    typedef Exception MappingFileReadFailed;

    NoteFontMap(QString name); // load and parse the XML mapping file
    ~NoteFontMap() override;

    /**
     * ok() returns false if the file read succeeded but the font
     * relies on system fonts that are not available.  (If the file
     * read fails, the constructor throws MappingFileReadFailed.)
     */
    bool ok() const { return m_ok; }

    QString getName() const { return m_name; }
    QString getOrigin() const { return m_origin; }
    QString getCopyright() const { return m_copyright; }
    QString getMappedBy() const { return m_mappedBy; }
    QString getType() const { return m_type; }
    bool isSmooth() const { return m_smooth; }

    std::set<int> getSizes() const;
    std::set<CharName> getCharNames() const;

    bool getStaffLineThickness(int size, unsigned int &thickness) const;
    bool getLegerLineThickness(int size, unsigned int &thickness) const;
    bool getStemThickness(int size, unsigned int &thickness) const;
    bool getBeamThickness(int size, unsigned int &thickness) const;
    bool getStemLength(int size, unsigned int &length) const;
    bool getFlagSpacing(int size, unsigned int &spacing) const;
    
    bool hasInversion(int size, CharName charName) const;

    bool getSrc(int size, CharName charName, QString &src) const;
    bool getInversionSrc(int size, CharName charName, QString &src) const;

    SystemFont *getSystemFont(int size, CharName charName, int &charBase) const;
    SystemFont::Strategy getStrategy(int size, CharName charName) const;
    bool getCode(int size, CharName charName, int &code) const;
    bool getInversionCode(int size, CharName charName, int &code) const;
    bool getGlyph(int size, CharName charName, int &glyph) const;
    bool getInversionGlyph(int size, CharName charName, int &glyph) const;

    bool getHotspot(int size, CharName charName, int width, int height,
                    int &x, int &y) const;

    // Xml handler methods:

    bool startElement
    (const QString& namespaceURI, const QString& localName,
     const QString& qName, const QXmlAttributes& atts) override;

    bool characters(const QString &) override;

    bool error(const QXmlParseException& exception) override;
    bool fatalError(const QXmlParseException& exception) override;

    void dump() const;

    // Not for general use, but very handy for diagnostic display
    QStringList getSystemFontNames() const;

    // want this to be private, but need access from HotspotData
    static int toSize(int baseSize, double factor, bool limitAtOne);

private:
    class SymbolData
    {
    public:
        SymbolData() : m_fontId(0),
                       m_src(""), m_inversionSrc(""),
                       m_code(-1), m_inversionCode(-1),
                       m_glyph(-1), m_inversionGlyph(-1) { }
        ~SymbolData() { }

        void setFontId(int id) { m_fontId = id; }
        int  getFontId() const { return m_fontId; }

        void setSrc(QString src) { m_src = src; }
        QString getSrc() const { return m_src; }

        void setCode(int code) { m_code = code; }
        int  getCode() const { return m_code; }

        void setGlyph(int glyph) { m_glyph = glyph; }
        int  getGlyph() const { return m_glyph; }

        void setInversionSrc(QString inversion) { m_inversionSrc = inversion; }
        QString getInversionSrc() const { return m_inversionSrc; }

        void setInversionCode(int code) { m_inversionCode = code; }
        int  getInversionCode() const { return m_inversionCode; }

        void setInversionGlyph(int glyph) { m_inversionGlyph = glyph; }
        int  getInversionGlyph() const { return m_inversionGlyph; }

        bool hasInversion() const {
            return m_inversionGlyph >= 0 ||
                   m_inversionCode  >= 0 ||
                   m_inversionSrc   != "";
        }

    private:
        int m_fontId;
        QString m_src;
        QString m_inversionSrc;
        int m_code;
        int m_inversionCode;
        int m_glyph;
        int m_inversionGlyph;
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

        bool getHotspot(int size, int width, int height, int &x, int &y) const;

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
                     m_legerLineThickness(-1) { }
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
        void setLegerLineThickness(unsigned int i) {
            m_legerLineThickness = (int)i;
        }
        void setFontHeight(int fontId, unsigned int h) {
            m_fontHeights[fontId] = (int)h;
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

        bool getLegerLineThickness(unsigned int &i) const {
            if (m_legerLineThickness >= 0) {
                i = (unsigned int)m_legerLineThickness;
                return true;
            } else return false;
        }

        bool getFontHeight(int fontId, unsigned int &h) const {
            std::map<int, int>::const_iterator fhi = m_fontHeights.find(fontId);
            if (fhi != m_fontHeights.end()) {
                h = (unsigned int)fhi->second;
                return true;
            }
            return false;
        }       
       
    private:
        int m_stemThickness;
        int m_beamThickness;
        int m_stemLength;
        int m_flagSpacing;
        int m_staffLineThickness;
        int m_legerLineThickness;
        std::map<int, int> m_fontHeights; // per-font-id
    };

    //--------------- Data members ---------------------------------

    QString m_name;
    QString m_origin;
    QString m_copyright;
    QString m_mappedBy;
    QString m_type;
    bool m_smooth;

    QString m_srcDirectory;

    typedef std::map<CharName, SymbolData> SymbolDataMap;
    SymbolDataMap m_data;

    typedef std::map<CharName, HotspotData> HotspotDataMap;
    HotspotDataMap m_hotspots;

    typedef std::map<int, SizeData> SizeDataMap;
    SizeDataMap m_sizes;

    typedef std::map<int, QString> SystemFontNameMap;
    SystemFontNameMap m_systemFontNames;

    typedef std::map<int, SystemFont::Strategy> SystemFontStrategyMap;
    SystemFontStrategyMap m_systemFontStrategies;

    typedef std::map<SystemFontSpec, SystemFont *> SystemFontMap;
    mutable SystemFontMap m_systemFontCache;

    typedef std::map<int, int> CharBaseMap;
    CharBaseMap m_bases;

    // For use when reading the XML file:
    bool m_expectingCharacters;
    QString *m_characterDestination;
    QString m_hotspotCharName;
    QString m_errorString;

    bool checkFile(int size, QString &src) const;

    bool m_ok;
};


class NoteCharacterDrawRep;


}

#endif
