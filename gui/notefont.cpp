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

#include "notefont.h"

#include <qfileinfo.h>
#include <qimage.h>
#include <qbitmap.h>
#include <qdir.h>
#include <qpainter.h>
#include <qfontdatabase.h>

#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <iostream>

#include "rosestrings.h"
#include "rosedebug.h"
#include "pixmapfunctions.h"

using std::string;
using std::map;
using std::set;
using std::cout;
using std::cerr;
using std::endl;

#ifdef HAVE_XFT
#include <X11/Xft/Xft.h>
#endif


NoteFontMap::NoteFontMap(string name) :
    m_name(name),
    m_autocrop(false),
    m_smooth(false),
    m_characterDestination(0),
    m_hotspotCharName(""),
    m_ok(true)
{
    m_fontDirectory = KGlobal::dirs()->findResource("appdata", "fonts/");

    QString mapFileName;

    QString mapFileMixedName = QString("%1/mappings/%2.xml")
        .arg(m_fontDirectory)
        .arg(strtoqstr(name));

    QFileInfo mapFileMixedInfo(mapFileMixedName);

    if (!mapFileMixedInfo.isReadable()) {

	QString mapFileLowerName = QString("%1/mappings/%2.xml")
	    .arg(m_fontDirectory)
	    .arg(strtoqstr(name).lower());

	QFileInfo mapFileLowerInfo(mapFileLowerName);

	if (!mapFileLowerInfo.isReadable()) {
	    if (mapFileLowerName != mapFileMixedName) {
		throw MappingFileReadFailed
		    (qstrtostr(i18n("Can't open font mapping file %1 or %2").
			       arg(mapFileMixedName).arg(mapFileLowerName)));
	    } else {
		throw MappingFileReadFailed
		    (qstrtostr(i18n("Can't open font mapping file %1").
			       arg(mapFileMixedName)));
	    }
	} else {
	    mapFileName = mapFileLowerName;
	}
    } else {
	mapFileName = mapFileMixedName;
    }

    QFile mapFile(mapFileName);

    QXmlInputSource source(mapFile);
    QXmlSimpleReader reader;
    reader.setContentHandler(this);
    reader.setErrorHandler(this);
    bool ok = reader.parse(source);
    mapFile.close();

    if (!ok) {
        throw MappingFileReadFailed(qstrtostr(m_errorString));
    }
}

NoteFontMap::~NoteFontMap()
{
    for (SystemFontMap::iterator i = m_systemFontCache.begin();
	 i != m_systemFontCache.end(); ++i) {
	delete i->second;
    }
}

bool
NoteFontMap::characters(QString &chars)
{
    if (!m_characterDestination) return true;
    *m_characterDestination += qstrtostr(chars);
    return true;
}

int 
NoteFontMap::toSize(int baseSize, double factor, bool limitAtOne)
{
    double dsize = factor * baseSize;
    dsize += 0.5;
    if (limitAtOne && dsize < 1.0) dsize = 1.0;
    return int(dsize);
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

	QString s;
	
	s = attributes.value("name");
	if (s) m_name = qstrtostr(s);
        
    } else if (lcName == "font-information") { 

	QString s;

        s = attributes.value("origin");
        if (s) m_origin = qstrtostr(s);

        s = attributes.value("copyright");
        if (s) m_copyright = qstrtostr(s);

        s = attributes.value("mapped-by");
        if (s) m_mappedBy = qstrtostr(s);

        s = attributes.value("type");
        if (s) m_type = qstrtostr(s);

	s = attributes.value("autocrop");
	if (s) m_autocrop = (s.lower() == "yes" || s.lower() == "true");

        s = attributes.value("smooth");
        if (s) m_smooth = (s.lower() == "true");

    } else if (lcName == "font-sizes") {

    } else if (lcName == "font-size") {

        QString s = attributes.value("note-height");
        if (!s) {
            m_errorString = "note-height is a required attribute of font-size";
            return false;
        }
        int noteHeight = s.toInt();

        SizeData &sizeData = m_sizes[noteHeight];

        s = attributes.value("staff-line-thickness");
        if (s) sizeData.setStaffLineThickness(s.toInt());

        s = attributes.value("leger-line-thickness");
        if (s) sizeData.setLegerLineThickness(s.toInt());

        s = attributes.value("stem-thickness");
        if (s) sizeData.setStemThickness(s.toInt());

        s = attributes.value("beam-thickness");
        if (s) sizeData.setBeamThickness(s.toInt());

        s = attributes.value("stem-length");
        if (s) sizeData.setStemLength(s.toInt());

        s = attributes.value("flag-spacing");
        if (s) sizeData.setFlagSpacing(s.toInt());

        s = attributes.value("border-x");
        if (s) sizeData.setBorderX(s.toInt());

        s = attributes.value("border-y");
        if (s) sizeData.setBorderY(s.toInt());

        s = attributes.value("font-height");
        if (s) sizeData.setFontHeight(s.toInt());

    } else if (lcName == "font-scale") {
	
	double fontHeight = -1.0;
	double beamThickness = -1.0;
	double stemLength = -1.0;
	double flagSpacing = -1.0;
	double staffLineThickness = -1.0;
	double legerLineThickness = -1.0;
	double stemThickness = -1.0;
	double borderX = -1.0;
	double borderY = -1.0;

	QString s;

        s = attributes.value("font-height");
	if (s) fontHeight = s.toDouble();
	else {
	    m_errorString = "font-height is a required attribute of font-scale";
	    return false;
	}

        s = attributes.value("staff-line-thickness");
	if (s) staffLineThickness = s.toDouble();

        s = attributes.value("leger-line-thickness");
	if (s) legerLineThickness = s.toDouble();

        s = attributes.value("stem-thickness");
	if (s) stemThickness = s.toDouble();

        s = attributes.value("beam-thickness");
	if (s) beamThickness = s.toDouble();

        s = attributes.value("stem-length");
	if (s) stemLength = s.toDouble();

        s = attributes.value("flag-spacing");
	if (s) flagSpacing = s.toDouble();

        s = attributes.value("border-x");
	if (s) borderX = s.toDouble();

        s = attributes.value("border-y");
	if (s) borderY = s.toDouble();

	//!!! need to be able to calculate max size -- checkFont needs
	//to take a size argument; unfortunately Qt doesn't seem to be
	//able to report to us when a scalable font was loaded in the
	//wrong size, so large sizes might be significantly inaccurate
	//as it just stops scaling up any further at somewhere around
	//120px.  We could test whether the metric for the black
	//notehead is noticeably smaller than the notehead should be,
	//and reject if so?  [update -- no, that doesn't work either,
	//Qt just returns the correct metric even if drawing the
	//incorrect size]
	
	for (int sz = 1; sz <= 30; sz += (sz == 1 ? 1 : 2)) {

	    SizeData &sizeData = m_sizes[sz];
	    unsigned int temp, temp1;

	    if (sizeData.getStaffLineThickness(temp) == false &&
		staffLineThickness >= 0.0)
		sizeData.setStaffLineThickness(toSize(sz, staffLineThickness, true));

	    if (sizeData.getLegerLineThickness(temp) == false &&
		legerLineThickness >= 0.0)
		sizeData.setLegerLineThickness(toSize(sz, legerLineThickness, true));

	    if (sizeData.getStemThickness(temp) == false &&
		stemThickness >= 0.0)
		sizeData.setStemThickness(toSize(sz, stemThickness, true));

	    if (sizeData.getBeamThickness(temp) == false &&
		beamThickness >= 0.0)
		sizeData.setBeamThickness(toSize(sz, beamThickness, true));

	    if (sizeData.getStemLength(temp) == false &&
		stemLength >= 0.0)
		sizeData.setStemLength(toSize(sz, stemLength, true));

	    if (sizeData.getFlagSpacing(temp) == false &&
		flagSpacing >= 0.0)
		sizeData.setFlagSpacing(toSize(sz, flagSpacing, true));

	    if (sizeData.getBorderThickness(temp, temp1) == false) {
		if (borderX >= 0.0)
		    sizeData.setBorderX(toSize(sz, borderX, false));
		if (borderY >= 0.0)
		    sizeData.setBorderY(toSize(sz, borderY, false));
	    }

	    if (sizeData.getFontHeight(temp) == false)
		sizeData.setFontHeight(toSize(sz, fontHeight, true));
	}

    } else if (lcName == "font-symbol-map") {

    } else if (lcName == "codebase") {

	int bn = 0, fn = 0;
	bool ok;
	QString base = attributes.value("base");
	if (!base) {
	    m_errorString = "base is a required attribute of codebase";
	    return false;
	}
	bn = base.toInt(&ok);
	if (!ok || bn < 0) {
	    m_errorString =
		QString("invalid base attribute \"%1\" (must be integer >= 0)").
		arg(base);
	    return false;
	}
	
	QString fontId = attributes.value("font-id");
	if (!fontId) {
	    m_errorString = "font-id is a required attribute of codebase";
	    return false;
	}
	fn = fontId.stripWhiteSpace().toInt(&ok);
	if (!ok || fn < 0) {
	    m_errorString =
		QString("invalid font-id attribute \"%1\" (must be integer >= 0)").
		arg(fontId);
	    return false;
	}

	m_bases[fn] = bn;

    } else if (lcName == "symbol") {

        QString symbolName = attributes.value("name");
        if (!symbolName) {
            m_errorString = "name is a required attribute of symbol";
            return false;
        }
        SymbolData symbolData;

        QString src = attributes.value("src");
	QString code = attributes.value("code");

	int n = -1;
	bool ok = false;
	if (code) {
	    n = code.stripWhiteSpace().toInt(&ok);
	    if (!ok || n < 0) {
		m_errorString =
		    QString("invalid code attribute \"%1\" (must be integer >= 0)").
		    arg(code);
		return false;
	    }
	    symbolData.setCode(n);
	}

        if (!src && n < 0) {
            m_errorString = "symbol must have either src or code attribute";
            return false;
        }
        if (src) symbolData.setSrc(qstrtostr(src));

        QString inversionSrc = attributes.value("inversion-src");
        if (inversionSrc) symbolData.setInversionSrc(qstrtostr(inversionSrc));

        QString inversionCode = attributes.value("inversion-code");
        if (inversionCode) {
	    n = inversionCode.stripWhiteSpace().toInt(&ok);
	    if (!ok || n < 0) {
		m_errorString =
		    QString("invalid inversion code attribute \"%1\" (must be integer >= 0)").
		    arg(inversionCode);
		return false;
	    }
	    symbolData.setInversionCode(n);
	}

	QString fontId = attributes.value("font-id");
	if (fontId) {
	    n = fontId.stripWhiteSpace().toInt(&ok);
	    if (!ok || n < 0) {
		m_errorString =
		    QString("invalid font-id attribute \"%1\" (must be integer >= 0)").
		    arg(fontId);
		return false;
	    }
	    symbolData.setFontId(n);
	}

        m_data[qstrtostr(symbolName.upper())] = symbolData;

    } else if (lcName == "font-hotspots") {

    } else if (lcName == "hotspot") {

        QString s = attributes.value("name");
        if (!s) {
            m_errorString = "name is a required attribute of hotspot";
            return false;
        }
        m_hotspotCharName = qstrtostr(s.upper());

    } else if (lcName == "scaled") {

        if (m_hotspotCharName == "") {
            m_errorString = "scaled-element must be in hotspot-element";
            return false;
        }

	QString s = attributes.value("x");
	double x = 0.0;
	if (s) x = s.toDouble();

	s = attributes.value("y");
	if (!s) {
	    m_errorString = "y is a required attribute of scaled";
	    return false;
	}
	double y = s.toDouble();
	
        HotspotDataMap::iterator i = m_hotspots.find(m_hotspotCharName);
        if (i == m_hotspots.end()) {
            m_hotspots[m_hotspotCharName] = HotspotData();
            i = m_hotspots.find(m_hotspotCharName);
        }

	i->second.setScaledHotspot(x, y);

    } else if (lcName == "when") {

        if (m_hotspotCharName == "") {
            m_errorString = "when-element must be in hotspot-element";
            return false;
        }

        QString s = attributes.value("note-height");
        if (!s) {
            m_errorString = "note-height is a required attribute of when";
            return false;
        }
        int noteHeight = s.toInt();

        s = attributes.value("x");
        int x = 0;
        if (s) x = s.toInt();

        s = attributes.value("y");
        if (!s) {
            m_errorString = "y is a required attribute of when";
            return false;
        }
        int y = s.toInt();

        HotspotDataMap::iterator i = m_hotspots.find(m_hotspotCharName);
        if (i == m_hotspots.end()) {
            m_hotspots[m_hotspotCharName] = HotspotData();
            i = m_hotspots.find(m_hotspotCharName);
        }

        i->second.addHotspot(noteHeight, x, y);

    } else if (lcName == "font-requirements") {

    } else if (lcName == "font-requirement") {

	QString id = attributes.value("font-id");
	int n = -1;
	bool ok = false;
	if (id) {
	    n = id.stripWhiteSpace().toInt(&ok);
	    if (!ok) {
		m_errorString =
		    QString("invalid font-id attribute \"%1\" (must be integer >= 0)").
		    arg(id);
		return false;
	    }
	} else {
	    m_errorString = "font-id is a required attribute of font-requirement";
	    return false;
	}

	QString name = attributes.value("name");
	QString names = attributes.value("names");

	if (name) {
	    if (names) {
		m_errorString = "font-requirement may have name or names attribute, but not both";
		return false;
	    }

	    SystemFont *font = SystemFont::loadSystemFont
		(SystemFontSpec(name, 12));

	    if (font) {
		m_systemFontNames[n] = name;
		delete font;
	    } else {
		cerr << QString("Warning: Unable to load font \"%1\"").arg(name) << endl;
		m_ok = false;
	    }

	} else if (names) {

	    bool have = false;
	    QStringList list = QStringList::split(",", names, false);
	    for (QStringList::Iterator i = list.begin(); i != list.end(); ++i) {
		SystemFont *font = SystemFont::loadSystemFont
		    (SystemFontSpec(*i, 12));
		if (font) {
		    m_systemFontNames[n] = *i;
		    have = true;
		    delete font;
		    break;
		}
	    }
	    if (!have) {
		cerr << QString("Warning: Unable to load any of the fonts in \"%1\"").
		    arg(names) << endl;
		m_ok = false;
	    }

	} else {
	    m_errorString = "font-requirement must have either name or names attribute";
	    return false;
	}	    

    } else {

    }

    if (m_characterDestination) *m_characterDestination = "";
    return true;
}

bool
NoteFontMap::error(const QXmlParseException& exception)
{
    m_errorString = QString("%1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    return QXmlDefaultHandler::error(exception);
}

bool
NoteFontMap::fatalError(const QXmlParseException& exception)
{
    m_errorString = QString("%1 at line %2, column %3")
	.arg(exception.message())
	.arg(exception.lineNumber())
	.arg(exception.columnNumber());
    return QXmlDefaultHandler::fatalError(exception);
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

set<CharName>
NoteFontMap::getCharNames() const
{
    set<CharName> names;

    for (SymbolDataMap::const_iterator i = m_data.begin();
         i != m_data.end(); ++i) {
        names.insert(i->first);
    }

    return names;
}

bool
NoteFontMap::checkFile(int size, string &src) const
{
    QString pixmapFileMixedName = QString("%1/%2/%3/%4.xpm")
        .arg(m_fontDirectory)
        .arg(strtoqstr(m_name))
        .arg(size)
        .arg(strtoqstr(src));

    QFileInfo pixmapFileMixedInfo(pixmapFileMixedName);

    if (!pixmapFileMixedInfo.isReadable()) {
	
	QString pixmapFileLowerName = QString("%1/%2/%3/%4.xpm")
	    .arg(m_fontDirectory)
	    .arg(strtoqstr(m_name).lower())
	    .arg(size)
	    .arg(strtoqstr(src));

	QFileInfo pixmapFileLowerInfo(pixmapFileLowerName);

	if (!pixmapFileLowerInfo.isReadable()) {
	    if (pixmapFileMixedName != pixmapFileLowerName) {
		cerr << "Warning: Unable to open pixmap file "
		     << pixmapFileMixedName << " or " << pixmapFileLowerName
		     << endl;
	    } else {
		cerr << "Warning: Unable to open pixmap file "
		     << pixmapFileMixedName << endl;
	    }
	    return false;
	} else {
	    src = qstrtostr(pixmapFileLowerName);
	}
    } else {
	src = qstrtostr(pixmapFileMixedName);
    }

    return true;
}

bool
NoteFontMap::hasInversion(int, CharName charName) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;
    return i->second.hasInversion();
}

bool
NoteFontMap::getSrc(int size, CharName charName, string &src) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    src = i->second.getSrc();
    if (src == "") return false;
    return checkFile(size, src);
}

bool
NoteFontMap::getInversionSrc(int size, CharName charName, string &src) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    if (!i->second.hasInversion()) return false;
    src = i->second.getInversionSrc();
    return checkFile(size, src);
}

SystemFont *
NoteFontMap::getSystemFont(int size, CharName charName, int &charBase)
    const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    SizeDataMap::const_iterator si = m_sizes.find(size);
    if (si == m_sizes.end()) return false;

    unsigned int fontHeight = 0;
    if (!si->second.getFontHeight(fontHeight)) {
	fontHeight = size;
    }

    int fontId = i->second.getFontId();
    SystemFontNameMap::const_iterator fni = m_systemFontNames.find(fontId);
    if (fontId < 0 || fni == m_systemFontNames.end()) return false;
    QString fontName = fni->second;

    CharBaseMap::const_iterator bi = m_bases.find(fontId);
    if (bi == m_bases.end()) charBase = 0;
    else charBase = bi->second;
    
    SystemFontSpec spec(fontName, fontHeight);
    SystemFontMap::const_iterator fi = m_systemFontCache.find(spec);
    if (fi != m_systemFontCache.end()) {
	return fi->second;
    }

    SystemFont *font = SystemFont::loadSystemFont(spec);
    if (!font) return 0;
    m_systemFontCache[spec] = font;

    NOTATION_DEBUG << "NoteFontMap::getFont: loaded font " << fontName
		   << " at pixel size " << fontHeight << endl;

    return font;
}

bool
NoteFontMap::getCode(int, CharName charName, int &code) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    code = i->second.getCode();
    return (code >= 0);
}

bool
NoteFontMap::getInversionCode(int, CharName charName, int &code) const
{
    SymbolDataMap::const_iterator i = m_data.find(charName);
    if (i == m_data.end()) return false;

    code = i->second.getInversionCode();
    return (code >= 0);
}

bool
NoteFontMap::getStaffLineThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getStaffLineThickness(thickness);
}

bool
NoteFontMap::getLegerLineThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getLegerLineThickness(thickness);
}

bool
NoteFontMap::getStemThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getStemThickness(thickness);
}

bool
NoteFontMap::getBeamThickness(int size, unsigned int &thickness) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getBeamThickness(thickness);
}

bool
NoteFontMap::getStemLength(int size, unsigned int &length) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getStemLength(length);
}

bool
NoteFontMap::getFlagSpacing(int size, unsigned int &spacing) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getFlagSpacing(spacing);
}

bool
NoteFontMap::getBorderThickness(int size,
                                unsigned int &x, unsigned int &y) const
{
    SizeDataMap::const_iterator i = m_sizes.find(size);
    if (i == m_sizes.end()) return false;

    return i->second.getBorderThickness(x, y);
}

bool
NoteFontMap::getHotspot(int size, CharName charName, int &x, int &y) const
{
    HotspotDataMap::const_iterator i = m_hotspots.find(charName);
    if (i == m_hotspots.end()) return false;
    return i->second.getHotspot(size, x, y);
}

bool
NoteFontMap::HotspotData::getHotspot(int size, int &x, int &y) const
{
    DataMap::const_iterator i = m_data.find(size);
    if (i == m_data.end()) {
	x = 0;
	if (m_scaled.first  >= 0) x = toSize(size, m_scaled.first,  false);
	if (m_scaled.second >= 0) {
	    y = toSize(size, m_scaled.second, false);
	    return true;
	}
	else return false;
    }
    x = i->second.first;
    y = i->second.second;
    return true;
}

void
NoteFontMap::dump() const
{
    // debug code

    cout << "Font data:\nName: " << getName() << "\nOrigin: " << getOrigin()
         << "\nCopyright: " << getCopyright() << "\nMapped by: "
         << getMappedBy() << "\nType: " << getType() << "\nAutocrop: "
	 << shouldAutocrop() << "\nSmooth: " << isSmooth() << endl;

    set<int> sizes = getSizes();
    set<CharName> names = getCharNames();

    for (set<int>::iterator sizei = sizes.begin(); sizei != sizes.end();
         ++sizei) {

        cout << "\nSize: " << *sizei << "\n" << endl;

        unsigned int t = 0;
	unsigned int bx = 0, by = 0;

        if (getStaffLineThickness(*sizei, t)) {
            cout << "Staff line thickness: " << t << endl;
        }

        if (getLegerLineThickness(*sizei, t)) {
            cout << "Leger line thickness: " << t << endl;
        }

        if (getStemThickness(*sizei, t)) {
            cout << "Stem thickness: " << t << endl;
        }

        if (getBeamThickness(*sizei, t)) {
            cout << "Beam thickness: " << t << endl;
        }

        if (getStemLength(*sizei, t)) {
            cout << "Stem length: " << t << endl;
        }

        if (getFlagSpacing(*sizei, t)) {
            cout << "Flag spacing: " << t << endl;
        }

        if (getBorderThickness(*sizei, bx, by)) {
            cout << "Border thickness: " << bx << "x" << by << endl;
        }

        for (set<CharName>::iterator namei = names.begin();
             namei != names.end(); ++namei) {

            cout << "\nCharacter: " << namei->c_str() << endl;

            string s;
            int x, y, c;

            if (getSrc(*sizei, *namei, s)) {
                cout << "Src: " << s << endl;
            }

            if (getInversionSrc(*sizei, *namei, s)) {
                cout << "Inversion src: " << s << endl;
            }
            
            if (getCode(*sizei, *namei, c)) {
                cout << "Code: " << c << endl;
            }

            if (getInversionCode(*sizei, *namei, c)) {
                cout << "Inversion code: " << c << endl;
            }
            
            if (getHotspot(*sizei, *namei, x, y)) {
                cout << "Hot spot: (" << x << "," << y << ")" << endl;
            }
        }
    }
}


NoteFont::FontPixmapMap *NoteFont::m_fontPixmapMap = 0;
QPixmap *NoteFont::m_blankPixmap = 0;

NoteFont::NoteFont(string fontName, int size) :
    m_fontMap(fontName)
{
    // Do the size checks first, to avoid doing the extra work if they fail

    std::set<int> sizes = m_fontMap.getSizes();

    if (sizes.size() > 0) {
        m_size = *sizes.begin();
    } else {
        throw BadNoteFont(std::string("No sizes listed for font ") + fontName);
    }

    if (size > 0) {
        if (sizes.find(size) == sizes.end()) {
            throw BadNoteFont(qstrtostr(QString("Font \"%1\" not available in size %2").arg(strtoqstr(fontName)).arg(size)));
        } else {
            m_size = size;
        }
    }

    // Create the global font map and blank pixmap if necessary

    if (m_fontPixmapMap == 0) {
        m_fontPixmapMap = new FontPixmapMap();
    }

    if (m_blankPixmap == 0) {
        m_blankPixmap = new QPixmap(10, 10);
        m_blankPixmap->setMask(QBitmap(10, 10, TRUE));
    }

    // Locate our font's pixmap map in the font map, create if necessary

    string fontKey = qstrtostr(QString("__%1__%2__")
        .arg(strtoqstr(m_fontMap.getName()))
        .arg(m_size));

    FontPixmapMap::iterator i = m_fontPixmapMap->find(fontKey);
    if (i == m_fontPixmapMap->end()) {
        (*m_fontPixmapMap)[fontKey] = new PixmapMap();
    }

    m_map = (*m_fontPixmapMap)[fontKey];
}

NoteFont::~NoteFont()
{ 
    // empty
}


bool
NoteFont::getStemThickness(unsigned int &thickness) const
{
    thickness = m_size/9 + 1;
    return m_fontMap.getStemThickness(m_size, thickness);
}

bool
NoteFont::getBeamThickness(unsigned int &thickness) const
{
    thickness = m_size / 2;
    return m_fontMap.getBeamThickness(m_size, thickness);
}

bool
NoteFont::getStemLength(unsigned int &length) const
{
    length = m_size * 13 / 4;
    return m_fontMap.getStemLength(m_size, length);
}

bool
NoteFont::getFlagSpacing(unsigned int &spacing) const
{
    spacing = m_size;
    return m_fontMap.getFlagSpacing(m_size, spacing);
}

bool
NoteFont::getStaffLineThickness(unsigned int &thickness) const
{
    thickness = (m_size < 16 ? 1 : m_size / 16);
    return m_fontMap.getStaffLineThickness(m_size, thickness);
}

bool
NoteFont::getLegerLineThickness(unsigned int &thickness) const
{
    thickness = (m_size < 12 ? 1 : m_size / 12);
    return m_fontMap.getStaffLineThickness(m_size, thickness);
}

bool
NoteFont::getBorderThickness(unsigned int &x, unsigned int &y) const
{
    x = 0;
    y = 0;
    return m_fontMap.getBorderThickness(m_size, x, y);
}

bool
NoteFont::lookup(CharName charName, bool inverted, QPixmap *&pixmap) const
{
    PixmapMap::iterator i = m_map->find(charName);
    if (i != m_map->end()) {
        if (inverted) {
	    pixmap = i->second.second;
	    if (!pixmap && i->second.first) return false;
	} else {
	    pixmap = i->second.first;
	    if (!pixmap && i->second.second) return false;
	}
	return true;
    }
    pixmap = 0;
    return false;
}

void
NoteFont::add(CharName charName, bool inverted, QPixmap *pixmap) const
{
    PixmapMap::iterator i = m_map->find(charName);
    if (i != m_map->end()) {
        if (inverted) {
            delete i->second.second;
            i->second.second = pixmap;
        } else {
            delete i->second.first;
            i->second.first = pixmap;
        }
    } else {
        if (inverted) {
            (*m_map)[charName] = PixmapPair(0, pixmap);
        } else {
            (*m_map)[charName] = PixmapPair(pixmap, 0);
        }
    }
}

bool
NoteFont::getPixmap(CharName charName, QPixmap &pixmap, bool inverted) const
{
    QPixmap *found = 0;
    bool ok = lookup(charName, inverted, found);
    if (ok) {
	if (found) {
	    pixmap = *found;
	    return true;
	} else {
	    pixmap = *m_blankPixmap;
	    return false;
	}
    }

    if (inverted && !m_fontMap.hasInversion(m_size, charName)) {
	if (!getPixmap(charName, pixmap, !inverted)) return false;
	found = new QPixmap(PixmapFunctions::flipVertical(pixmap));
	add(charName, inverted, found);
	pixmap = *found;
	return true;
    }

    string src;
    ok = false;

    if (!inverted) ok = m_fontMap.getSrc(m_size, charName, src);
    else  ok = m_fontMap.getInversionSrc(m_size, charName, src);
    
    if (ok) {
	NOTATION_DEBUG
	    << "NoteFont::getPixmap: Loading \"" << src << "\"" << endl;

        found = new QPixmap(strtoqstr(src));

        if (!found->isNull()) {

            if (found->mask() == 0) {
                cerr << "NoteFont::getPixmap: Warning: No automatic mask "
                     << "for character \"" << charName << "\"" 
                     << (inverted ? " (inverted)" : "") << " in font \""
                     << m_fontMap.getName() << "-" << m_size
                     << "\"; consider making xpm background transparent"
                     << endl;
		found->setMask(PixmapFunctions::generateMask(*found));
            }

            add(charName, inverted, found);
            pixmap = *found;
            return true;
        }

        cerr << "NoteFont::getPixmap: Warning: Unable to read pixmap file " << src << endl;
    } else {

	int code = -1;
	int charBase = 0;
	if (!inverted) ok = m_fontMap.getCode(m_size, charName, code);
	else  ok = m_fontMap.getInversionCode(m_size, charName, code);

	if (!ok) {
	    cerr << "NoteFont::getPixmap: Warning: No pixmap or code for character \""
		 << charName << "\"" << (inverted ? " (inverted)" : "")
		 << " in font \"" << m_fontMap.getName() << "\"" << endl;
	    add(charName, inverted, 0);
	    pixmap = *m_blankPixmap;
	    return false;
	}

	SystemFont *systemFont =
	    m_fontMap.getSystemFont(m_size, charName, charBase);

	if (!systemFont) {
	    if (!inverted && m_fontMap.hasInversion(m_size, charName)) {
		if (!getPixmap(charName, pixmap, !inverted)) return false;
		found = new QPixmap(PixmapFunctions::flipVertical(pixmap));
		add(charName, inverted, found);
		pixmap = *found;
		return true;
	    }

	    cerr << "NoteFont::getPixmap: Warning: No system font for character \""
		 << charName << "\"" << (inverted ? " (inverted)" : "")
		 << " in font \"" << m_fontMap.getName() << "\"" << endl;
	    add(charName, inverted, 0);
	    pixmap = *m_blankPixmap;
	    return false;
	}
	
	found = new QPixmap(systemFont->renderChar(code + charBase,
						   m_fontMap.shouldAutocrop()));
	add(charName, inverted, found);
	pixmap = *found;
	return true;
    }

    add(charName, inverted, 0);
    pixmap = *m_blankPixmap;
    return false;
}

QPixmap
NoteFont::getPixmap(CharName charName, bool inverted) const
{
    QPixmap p;
    (void)getPixmap(charName, p, inverted);
    return p;
}

QCanvasPixmap*
NoteFont::getCanvasPixmap(CharName charName, bool inverted) const
{
    QPixmap p;
    (void)getPixmap(charName, p, inverted);

    int x, y;
    (void)getHotspot(charName, x, y, inverted);

    return new QCanvasPixmap(p, QPoint(x, y));
}

bool
NoteFont::getColouredPixmap(CharName baseCharName, QPixmap &pixmap,
                            int hue, int minValue, bool inverted) const
{
    CharName charName(getNameWithColour(baseCharName, hue));

    QPixmap *found = 0;
    bool ok = lookup(charName, inverted, found);
    if (ok) {
	if (found) {
	    pixmap = *found;
	    return true;
	} else {
	    pixmap = *m_blankPixmap;
	    return false;
	}
    }

    QPixmap basePixmap;
    ok = getPixmap(baseCharName, basePixmap, inverted);

    if (!ok) {
	add(charName, inverted, 0);
	pixmap = *m_blankPixmap;
	return false;
    }

    found = new QPixmap
	(PixmapFunctions::colourPixmap(basePixmap, hue, minValue));
    add(charName, inverted, found);
    pixmap = *found;
    return ok;
}

QPixmap
NoteFont::getColouredPixmap(CharName charName, int hue, int minValue, bool inverted) const
{
    QPixmap p;
    (void)getColouredPixmap(charName, p, hue, minValue, inverted);
    return p;
}

QCanvasPixmap*
NoteFont::getColouredCanvasPixmap(CharName charName, int hue, int minValue,
                                  bool inverted) const
{
    QPixmap p;
    (void)getColouredPixmap(charName, p, hue, minValue, inverted);

    int x, y;
    (void)getHotspot(charName, x, y, inverted);

    return new QCanvasPixmap(p, QPoint(x, y));
}

CharName
NoteFont::getNameWithColour(CharName base, int hue) const
{
    return qstrtostr(QString("%1__%2").arg(hue).arg(strtoqstr(base)));
}

bool
NoteFont::getDimensions(CharName charName, int &x, int &y, bool inverted) const
{
    QPixmap pixmap;
    bool ok = getPixmap(charName, pixmap, inverted);
    x = pixmap.width();
    y = pixmap.height();
    return ok;
}

int
NoteFont::getWidth(CharName charName) const
{
    int x, y;
    getDimensions(charName, x, y);
    return x;
}

int
NoteFont::getHeight(CharName charName) const
{
    int x, y;
    getDimensions(charName, x, y);
    return y;
}

bool
NoteFont::getHotspot(CharName charName, int &x, int &y, bool inverted) const
{
    bool ok = m_fontMap.getHotspot(m_size, charName, x, y);

    if (!ok) {
        int w, h;
        getDimensions(charName, w, h, inverted);
        x = 0;
        y = h/2;
    }

    if (inverted) {
        y = getHeight(charName) - y;
    }
    
    return ok;
}

QPoint
NoteFont::getHotspot(CharName charName, bool inverted) const
{
    int x, y;
    (void)getHotspot(charName, x, y, inverted);
    return QPoint(x, y);
}


std::set<std::string>
NoteFontFactory::getFontNames()
{
    if (m_fontNames.empty()) {

	QString mappingDir = 
	    KGlobal::dirs()->findResource("appdata", "fonts/mappings/");
	QDir dir(mappingDir);
	if (!dir.exists()) {
	    cerr << "NoteFontFactory::getFontNames: mapping directory \""
		 << mappingDir << "\" not found" << endl;
	    return m_fontNames;
	}

	dir.setFilter(QDir::Files | QDir::Readable);
	QStringList files = dir.entryList();
	for (QStringList::Iterator i = files.begin(); i != files.end(); ++i) {

	    if ((*i).length() > 4 && (*i).right(4).lower() == ".xml") {

		string name(qstrtostr((*i).left((*i).length() - 4)));

		try {
		    NoteFontMap map(name);
		    if (map.ok()) m_fontNames.insert(map.getName());
		} catch (Rosegarden::Exception e) {
		    KMessageBox::error(0, strtoqstr(e.getMessage()));
		    throw;
		}
	    }
	}
    } 

    return m_fontNames;
}

std::vector<int>
NoteFontFactory::getAllSizes(std::string fontName)
{
    NoteFont *font = getFont(fontName, 0);
    if (!font) return std::vector<int>();

    std::set<int> s(font->getSizes());
    std::vector<int> v;
    for (std::set<int>::iterator i = s.begin(); i != s.end(); ++i) {
	v.push_back(*i);
    }

    std::sort(v.begin(), v.end());
    return v;
}

std::vector<int>
NoteFontFactory::getScreenSizes(std::string fontName)
{
    NoteFont *font = getFont(fontName, 0);
    if (!font) return std::vector<int>();

    std::set<int> s(font->getSizes());
    std::vector<int> v;
    for (std::set<int>::iterator i = s.begin(); i != s.end(); ++i) {
	if (*i <= 16) v.push_back(*i);
    }
    std::sort(v.begin(), v.end());
    return v;
}

NoteFont *
NoteFontFactory::getFont(std::string fontName, int size)
{
    std::map<std::pair<std::string, int>, NoteFont *>::iterator i =
	m_fonts.find(std::pair<std::string, int>(fontName, size));

    if (i == m_fonts.end()) {
	try {
	    NoteFont *font = new NoteFont(fontName, size);
	    m_fonts[std::pair<std::string, int>(fontName, size)] = font;
	    return font;
	} catch (Rosegarden::Exception e) {
	    KMessageBox::error(0, strtoqstr(e.getMessage()));
	    throw;
	}
    } else {
	return i->second;
    }
}

std::string
NoteFontFactory::getDefaultFontName()
{
    std::set<std::string> fontNames = getFontNames();
    if (fontNames.find("Feta") != fontNames.end()) return "Feta";
    else if (fontNames.size() == 0) {
	QString message = i18n("Can't obtain a default font -- no fonts found");
	KMessageBox::error(0, message);
	throw NoFontsAvailable(qstrtostr(message));
    }
    else return *fontNames.begin();
}

int
NoteFontFactory::getDefaultSize(std::string fontName)
{
    // always return 8 if it's supported!
    std::vector<int> sizes(getScreenSizes(fontName));
    for (unsigned int i = 0; i < sizes.size(); ++i) {
	if (sizes[i] == 8) return sizes[i];
    }
    return sizes[sizes.size()/2];
}

bool
NoteFontFactory::isAvailableInSize(std::string fontName, int size)
{
    std::vector<int> sizes(getAllSizes(fontName));
    for (unsigned int i = 0; i < sizes.size(); ++i) {
	if (sizes[i] == size) return true;
    }
    return false;
}
    

std::set<std::string> NoteFontFactory::m_fontNames;
std::map<std::pair<std::string, int>, NoteFont *> NoteFontFactory::m_fonts;


class SystemFontQt : public SystemFont
{
public:
    SystemFontQt(QFont &font) : m_font(font) { }
    virtual ~SystemFontQt() { }

    virtual QPixmap renderChar(unsigned int code, bool autocrop);

private:
    QFont m_font;
};

QPixmap
SystemFontQt::renderChar(unsigned int code, bool autocrop)
{
    QFontMetrics metrics(m_font);
    QChar qc(code);
    QRect bounding = metrics.boundingRect(qc);

    QPixmap map;

    if (autocrop) {
	map = QPixmap(bounding.width(), bounding.height() + 1);
    } else {
	map = QPixmap(metrics.width(qc), metrics.height());
    }
    
    map.fill();
    QPainter painter;
    painter.begin(&map);
    painter.setFont(m_font);
    painter.setPen(Qt::black);
    
    NOTATION_DEBUG << "SystemFontQt::renderChar: Drawing character code "
		   << code << " (string " << QString(qc) << ")" << endl;
    
    if (autocrop) {
	painter.drawText(-bounding.left(), -bounding.top(), qc);
    } else {
	painter.drawText(0, metrics.ascent(), qc);
    }
    
    painter.end();
    map.setMask(PixmapFunctions::generateMask(map, Qt::white.rgb()));
    return map;
}


#ifdef HAVE_XFT

class SystemFontXft : public SystemFont
{
public:
    SystemFontXft(Display *dpy, XftFont *font) : m_dpy(dpy), m_font(font) { }
    virtual ~SystemFontXft() { if (m_font) XftFontClose(m_dpy, m_font); }
    
    virtual QPixmap renderChar(unsigned int code, bool autocrop);

private:
    Display *m_dpy;
    XftFont *m_font;
};

QPixmap
SystemFontXft::renderChar(unsigned int code, bool)
{
    XGlyphInfo extents;
    FcChar32 char32(code);
    XftTextExtents32(m_dpy, m_font, &char32, 1, &extents);

    QPixmap map(extents.width, extents.height);
    map.fill();

    Drawable drawable = (Drawable)map.handle();
    if (!drawable) {
	std::cerr << "ERROR: SystemFontXft::renderChar: No drawable in QPixmap!" << std::endl;
	return map;
    }

    XftDraw *draw = XftDrawCreate(m_dpy,
				  drawable,
				  (Visual *)map.x11Visual(),
				  map.x11Colormap());

    QColor pen(Qt::black);
    XftColor col;
    col.color.red = pen.red () | pen.red() << 8;
    col.color.green = pen.green () | pen.green() << 8;
    col.color.blue = pen.blue () | pen.blue() << 8;
    col.color.alpha = 0xffff;
    col.pixel = pen.pixel();

    NOTATION_DEBUG << "SystemFontXft::renderChar: rendering character code "
		   << code << endl;
    
    XftDrawString32(draw, &col, m_font, extents.x, extents.y, &char32, 1);
    XftDrawDestroy(draw);

    map.setMask(PixmapFunctions::generateMask(map, Qt::white.rgb()));
    return map;
}

#endif /* HAVE_XFT */


SystemFont *
SystemFont::loadSystemFont(const SystemFontSpec &spec)
{
    QString name = spec.first;
    int size = spec.second;

    NOTATION_DEBUG << "SystemFont::loadSystemFont: name is " << name << ", size " << size << endl;

    if (name == "DEFAULT") {
	QFont font;
	font.setPixelSize(size);
	return new SystemFontQt(font);
    }

#ifdef HAVE_XFT

    FcPattern *pattern, *match;
    FcResult result;
    FcChar8 *matchFamily;
    XftFont *xfont = 0;

    Display *dpy = QPaintDevice::x11AppDisplay();

    if (!dpy) {
	std::cerr << "SystemFont::loadSystemFont[Xft]: Xft support requested but no X11 display available!" << std::endl;
	goto qfont;
    }

    pattern = FcPatternCreate();
    FcPatternAddString(pattern, FC_FAMILY, (FcChar8 *)name.latin1());
    FcPatternAddInteger(pattern, FC_PIXEL_SIZE, size);
    FcConfigSubstitute(FcConfigGetCurrent(), pattern, FcMatchPattern);

    result = FcResultMatch;
    match = FcFontMatch(FcConfigGetCurrent(), pattern, &result);
    FcPatternDestroy(pattern);

    FcPatternGetString(match, FC_FAMILY, 0, &matchFamily);
    NOTATION_DEBUG << "SystemFont::loadSystemFont[Xft]: match family is "
		   << (char *)matchFamily << endl;
    
    if (!match || result != FcResultMatch) {
	NOTATION_DEBUG << "SystemFont::loadSystemFont[Xft]: No match for font "
		       << name << " (result is " << result
		       << "), falling back on QFont" << endl;
	if (match) FcPatternDestroy(match);
	goto qfont;
    }

    if (QString((char *)matchFamily).lower() != name.lower()) {
	NOTATION_DEBUG << "SystemFont::loadSystemFont[Xft]: Wrong family returned, falling back on QFont" << endl;
	FcPatternDestroy(match);
	goto qfont;
    }

    xfont = XftFontOpenPattern(dpy, match);
    if (!xfont) {
	FcPatternDestroy(match);
	NOTATION_DEBUG << "SystemFont::loadSystemFont[Xft]: Unable to load font "
		       << name << " via Xft, falling back on QFont" << endl;
	goto qfont;
    } 

    NOTATION_DEBUG << "SystemFont::loadSystemFont[Xft]: successfully loaded font "
		   << name << " through Xft" << endl;
    
    return new SystemFontXft(dpy, xfont);

#endif

 qfont:
    
    QFont qfont(name, size, QFont::Normal);
    qfont.setPixelSize(size);

    QFontInfo info(qfont);

    NOTATION_DEBUG << "SystemFont::loadSystemFont[Qt]: have family " << info.family() << " (exactMatch " << info.exactMatch() << ")" << endl;

//    return info.exactMatch();

    // The Qt documentation says:
    //
    //   bool QFontInfo::exactMatch() const
    //   Returns TRUE if the matched window system font is exactly the
    //   same as the one specified by the font; otherwise returns FALSE.
    //
    // My arse.  I specify "feta", I get "Verdana", and exactMatch
    // returns true.  Uh huh.
    //
    // UPDATE: in newer versions of Qt, I specify "fughetta", I get
    // "Fughetta [macromedia]", and exactMatch returns false.  Just as
    // useless, but in a different way.

    QString family = info.family().lower();

    if (family == name.lower()) return new SystemFontQt(qfont);
    else {
	int bracket = family.find(" [");
	if (bracket > 1) family = family.left(bracket);
	if (family == name.lower()) return new SystemFontQt(qfont);
    }

    NOTATION_DEBUG << "SystemFont::loadSystemFont[Qt]: Wrong family returned, failing" << endl;
    return 0;
}


