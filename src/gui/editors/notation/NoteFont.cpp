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

#define RG_MODULE_STRING "[NoteFont]"

#include "NoteFont.h"
#include "misc/Debug.h"

#include "misc/Strings.h"
#include "base/Exception.h"
#include "base/Profiler.h"
#include "gui/general/PixmapFunctions.h"
#include "NoteCharacter.h"
#include "NoteFontMap.h"
#include "SystemFont.h"
#include <QBitmap>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <iostream>

namespace Rosegarden
{

NoteFont::FontPixmapMap *NoteFont::m_fontPixmapMap = nullptr;

NoteFont::DrawRepMap *NoteFont::m_drawRepMap = nullptr;
QPixmap *NoteFont::m_blankPixmap = nullptr;


NoteFont::NoteFont(QString fontName, int size) :
    m_fontMap(fontName)
{
    // Do the size checks first, to avoid doing the extra work if they fail

    std::set<int> sizes = m_fontMap.getSizes();

    if (!sizes.empty()) {
        m_size = *sizes.begin();
    } else {
        throw BadNoteFont(QObject::tr("No sizes listed for font \"%1\"").arg(fontName));
    }

    if (size > 0) {
        if (sizes.find(size) == sizes.end()) {
            throw BadNoteFont(QObject::tr("Font \"%1\" not available in size %2").arg(fontName).arg(size));
        } else {
            m_size = size;
        }
    }

    // Create the global font map and blank pixmap if necessary

    if (m_fontPixmapMap == nullptr) {
        m_fontPixmapMap = new FontPixmapMap();
    }

    if (m_blankPixmap == nullptr) {
        m_blankPixmap = new QPixmap(10, 10);
        m_blankPixmap->fill(Qt::transparent);
    }

    // Locate our font's pixmap map in the font map, create if necessary

    QString fontKey = QString("__%1__%2__")
        .arg(m_fontMap.getName())
        .arg(m_size);

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
    thickness = m_size / 9 + 1;
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
    getStaffLineThickness(length);
    length = (m_size + length) * 7 / 2;
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
    thickness = (m_size < 7 ? 1 : m_size / 7);
    return m_fontMap.getStaffLineThickness(m_size, thickness);
}

bool
NoteFont::getLegerLineThickness(unsigned int &thickness) const
{
    thickness = (m_size < 6 ? 1 : m_size / 6);
    return m_fontMap.getLegerLineThickness(m_size, thickness);
}

bool
NoteFont::lookup(CharName charName, bool inverted, QPixmap *&pixmap) const
{
    PixmapMap::iterator i = m_map->find(charName);
    if (i != m_map->end()) {
        if (inverted) {
            pixmap = i->second.second;
            if (!pixmap && i->second.first)
                return false;
        } else {
            pixmap = i->second.first;
            if (!pixmap && i->second.second)
                return false;
        }
        return true;
    }
    pixmap = nullptr;
    return false;
}

void
NoteFont::add
(CharName charName, bool inverted, QPixmap *pixmap) const
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
            (*m_map)[charName] = PixmapPair(pixmap, nullptr);
        }
    }
}
    
NoteCharacterDrawRep *
NoteFont::lookupDrawRep(QPixmap *pixmap) const
{
    if (!m_drawRepMap)
        m_drawRepMap = new DrawRepMap();

    if (m_drawRepMap->find(pixmap) != m_drawRepMap->end()) {

        return (*m_drawRepMap)[pixmap];

    } else {

        QImage image = pixmap->toImage();
        if (image.isNull())
            return nullptr;

#if 0
// ??? Removing since this does nothing other than trigger a compiler warning.
        if (image.depth() > 1) {
//            image = image.convertDepth(1, Qt::MonoOnly | Qt::ThresholdDither);
              // ??? Since the return is ignored, this does nothing other
              //     than trigger a compiler warning.
              image.convertToFormat(QImage::Format_Mono, Qt::ThresholdDither);
        }
#endif

        NoteCharacterDrawRep *a = new NoteCharacterDrawRep();

        for (int yi = 0; yi < image.height(); ++yi) {

            unsigned char *line = image.scanLine(yi);

            int startx = 0;

            for (int xi = 0; xi <= image.width(); ++xi) {

                bool pixel = false;

                if (xi < image.width()) {
                    if (image.format() == QImage::Format_Mono) { //!!! mono == LittleEndian or do I have it backwards?
                        if (*(line + (xi >> 3)) & 1 << (xi & 7))
                            pixel = true;
                    } else {
                        if (*(line + (xi >> 3)) & 1 << (7 - (xi & 7)))
                            pixel = true;
                    }
                }

                if (!pixel) {
                    if (startx < xi) {
//                         a->resize(a->size() + 2, QGArray::SpeedOptim );	//&&& whats QGArray
                        a->setPoint(a->size() - 2, startx, yi);
                        a->setPoint(a->size() - 1, xi - 1, yi);
                    }
                    startx = xi + 1;
                }
            }
        }

        (*m_drawRepMap)[pixmap] = a;
        return a;
    }
}

bool
NoteFont::getPixmap(CharName charName, QPixmap &pixmap, bool inverted) const
{
    QPixmap *found = nullptr;
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

    Profiler profiler("NoteFont::getPixmap: cache miss");

    QString src;
    ok = false;

    if (!inverted) ok = m_fontMap.getSrc(m_size, charName, src);
    else ok = m_fontMap.getInversionSrc(m_size, charName, src);

    if (ok) {
        NOTATION_DEBUG << "NoteFont::getPixmap: Loading \"" << src << "\"";

        found = new QPixmap(src);

        if (!found->isNull()) {
            add(charName, inverted, found);
            pixmap = *found;
            return true;
        }

        std::cerr << "NoteFont::getPixmap: Warning: Unable to read pixmap file " << src << std::endl;
    } else {

        int code = -1;
        if (!inverted) ok = m_fontMap.getCode(m_size, charName, code);
        else ok = m_fontMap.getInversionCode(m_size, charName, code);

        int glyph = -1;
        if (!inverted) ok = m_fontMap.getGlyph(m_size, charName, glyph);
        else ok = m_fontMap.getInversionGlyph(m_size, charName, glyph);

        if (code < 0 && glyph < 0) {
            std::cerr << "NoteFont::getPixmap: Warning: No pixmap, code, or glyph for character \""
                      << charName << "\"" << (inverted ? " (inverted)" : "")
                      << " in font \"" << m_fontMap.getName() << "\"" << std::endl;
            add(charName, inverted, nullptr);
            pixmap = *m_blankPixmap;
            return false;
        }

        int charBase = 0;
        SystemFont *systemFont =
            m_fontMap.getSystemFont(m_size, charName, charBase);

        if (!systemFont) {
            if (!inverted && m_fontMap.hasInversion(m_size, charName)) {
                if (!getPixmap(charName, pixmap, !inverted))
                    return false;
                found = new QPixmap(PixmapFunctions::flipVertical(pixmap));
                add(charName, inverted, found);
                pixmap = *found;
                return true;
            }

            std::cerr << "NoteFont::getPixmap: Warning: No system font for character \""
                      << charName << "\"" << (inverted ? " (inverted)" : "")
                      << " in font \"" << m_fontMap.getName() << "\"" << std::endl;

            add(charName, inverted, nullptr);
            pixmap = *m_blankPixmap;
            return false;
        }

        SystemFont::Strategy strategy =
            m_fontMap.getStrategy(m_size, charName);

        bool success;
        found = new QPixmap(systemFont->renderChar(charName,
                                                   glyph,
                                                   code + charBase,
                                                   strategy,
                                                   success));

        if (success) {
            add(charName, inverted, found);
            pixmap = *found;
            return true;
        } else {
            add(charName, inverted, nullptr);
            pixmap = *m_blankPixmap;
            return false;
        }
    }

    add(charName, inverted, nullptr);
    pixmap = *m_blankPixmap;
    return false;
}

bool
NoteFont::getColouredPixmap(CharName baseCharName, QPixmap &pixmap,
                            int hue, int minimum, bool inverted, int saturation) const
{
    CharName charName(getNameWithColour(baseCharName, hue));

    QPixmap *found = nullptr;
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
        add(charName, inverted, nullptr);
        pixmap = *m_blankPixmap;
        return false;
    }

    found = new QPixmap
        (PixmapFunctions::colourPixmap(basePixmap, hue, minimum, saturation));
    add(charName, inverted, found);
    pixmap = *found;
    return ok;
}

bool
NoteFont::getShadedPixmap(CharName baseCharName, QPixmap &pixmap,
                          bool inverted) const
{
    CharName charName(getNameShaded(baseCharName));

    QPixmap *found = nullptr;
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
        add(charName, inverted, nullptr);
        pixmap = *m_blankPixmap;
        return false;
    }

    found = new QPixmap(PixmapFunctions::shadePixmap(basePixmap));
    add(charName, inverted, found);
    pixmap = *found;
    return ok;
}

CharName
NoteFont::getNameWithColour(CharName base, int hue) const
{
    return QString("%1__%2").arg(hue).arg(base);
}

CharName
NoteFont::getNameShaded(CharName base) const
{
    return QString("shaded__%1").arg(base);
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
    int w, h;
    getDimensions(charName, w, h, inverted);
    bool ok = m_fontMap.getHotspot(m_size, charName, w, h, x, y);

    if (!ok) {
        x = 0;
        y = h / 2;
    }

    if (inverted) {
        y = h - y;
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

bool
NoteFont::getCharacter(CharName charName,
                       NoteCharacter &character,
                       CharacterType type,
                       bool inverted)
{
    Profiler profiler("NoteFont::getCharacter");

    QPixmap pixmap;
    if (!getPixmap(charName, pixmap, inverted))
        return false;

    if (type == Screen) {
        character = NoteCharacter(pixmap,
                                  getHotspot(charName, inverted),
                                  nullptr);
    } else {

        // Get the pointer direct from cache (depends on earlier call
        // to getPixmap to put it in the cache if available)

        QPixmap *pmapptr = nullptr;
        bool found = lookup(charName, inverted, pmapptr);

        NoteCharacterDrawRep *rep = nullptr;
        if (found && pmapptr)
            rep = lookupDrawRep(pmapptr);

        character = NoteCharacter(pixmap,
                                  getHotspot(charName, inverted),
                                  rep);
    }

    return true;
}

NoteCharacter
NoteFont::getCharacter(CharName charName,
                       CharacterType type,
                       bool inverted)
{
    NoteCharacter character;
    getCharacter(charName, character, type, inverted);
    return character;
}

bool
NoteFont::getCharacterColoured(CharName charName,
                               int hue, int minimum,
                               NoteCharacter &character,
                               CharacterType type,
                               bool inverted,
                               int saturation)
{
    QPixmap pixmap;
    if (!getColouredPixmap(charName, pixmap, hue, minimum, inverted, saturation)) {
        return false;
    }

    if (type == Screen) {

        character = NoteCharacter(pixmap,
                                  getHotspot(charName, inverted),
                                  nullptr);

    } else {

        // Get the pointer direct from cache (depends on earlier call
        // to getPixmap to put it in the cache if available)

        QPixmap *pmapptr = nullptr;
        CharName cCharName(getNameWithColour(charName, hue));
        bool found = lookup(cCharName, inverted, pmapptr);

        NoteCharacterDrawRep *rep = nullptr;
        if (found && pmapptr)
            rep = lookupDrawRep(pmapptr);

        character = NoteCharacter(pixmap,
                                  getHotspot(charName, inverted),
                                  rep);
    }

    return true;
}

NoteCharacter
NoteFont::getCharacterColoured(CharName charName,
                               int hue, int minimum,
                               CharacterType type,
                               bool inverted)
{
    NoteCharacter character;
    getCharacterColoured(charName, hue, minimum, character, type, inverted);
    return character;
}


}
