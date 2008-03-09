/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <cmath>
#include "NotePixmapFactory.h"
#include "misc/Debug.h"
#include "base/NotationRules.h"
#include <kapplication.h>

#include <klocale.h>
#include <kstddirs.h>
#include <kconfig.h>
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "gui/editors/guitar/Fingering.h"
#include "gui/editors/guitar/FingeringBox.h"
#include "gui/editors/guitar/NoteSymbols.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/PixmapFunctions.h"
#include "gui/general/Spline.h"
#include "gui/kdeext/KStartupLogo.h"
#include "NotationStrings.h"
#include "NotationView.h"
#include "NoteCharacter.h"
#include "NoteCharacterNames.h"
#include "NoteFontFactory.h"
#include "NoteFont.h"
#include "NotePixmapParameters.h"
#include "NotePixmapPainter.h"
#include "NoteStyleFactory.h"
#include "NoteStyle.h"
#include <kglobal.h>
#include <kmessagebox.h>
#include <qbitmap.h>
#include <qcolor.h>
#include <qfile.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qpointarray.h>
#include <qpoint.h>
#include <qrect.h>
#include <qstring.h>
#include <qwmatrix.h>


namespace Rosegarden
{

using namespace Accidentals;

static clock_t drawBeamsTime = 0;
static clock_t makeNotesTime = 0;
static int drawBeamsCount = 0;
static int drawBeamsBeamCount = 0;

class NotePixmapCache : public std::map<CharName, QCanvasPixmap*>
{
    // nothing to add -- just so we can predeclare it in the header
};

const char* const NotePixmapFactory::defaultSerifFontFamily = "Bitstream Vera Serif";
const char* const NotePixmapFactory::defaultSansSerifFontFamily = "Bitstream Vera Sans";
const char* const NotePixmapFactory::defaultTimeSigFontFamily = "Bitstream Vera Serif";

NotePixmapFactory::NotePixmapFactory(std::string fontName, int size) :
        m_selected(false),
        m_shaded(false),
        m_tupletCountFont(defaultSerifFontFamily, 8, QFont::Bold),
        m_tupletCountFontMetrics(m_tupletCountFont),
        m_textMarkFont(defaultSerifFontFamily, 8, QFont::Bold, true),
        m_textMarkFontMetrics(m_textMarkFont),
        m_fingeringFont(defaultSerifFontFamily, 8, QFont::Bold),
        m_fingeringFontMetrics(m_fingeringFont),
        m_timeSigFont(defaultTimeSigFontFamily, 8, QFont::Bold),
        m_timeSigFontMetrics(m_timeSigFont),
        m_bigTimeSigFont(defaultTimeSigFontFamily, 12, QFont::Normal),
        m_bigTimeSigFontMetrics(m_bigTimeSigFont),
        m_ottavaFont(defaultSerifFontFamily, 8, QFont::Normal, true),
        m_ottavaFontMetrics(m_ottavaFont),
        m_trackHeaderFont(defaultSansSerifFontFamily, 10, QFont::Normal),
        m_trackHeaderFontMetrics(m_trackHeaderFont),
        m_generatedPixmap(0),
        m_generatedMask(0),
        m_generatedWidth( -1),
        m_generatedHeight( -1),
        m_inPrinterMethod(false),
        m_p(new NotePixmapPainter()),
        m_dottedRestCache(new NotePixmapCache)
{
    init(fontName, size);
}

NotePixmapFactory::NotePixmapFactory(const NotePixmapFactory &npf) :
        m_selected(false),
        m_shaded(false),
        m_tupletCountFont(npf.m_tupletCountFont),
        m_tupletCountFontMetrics(m_tupletCountFont),
        m_textMarkFont(npf.m_textMarkFont),
        m_textMarkFontMetrics(m_textMarkFont),
        m_fingeringFont(npf.m_fingeringFont),
        m_fingeringFontMetrics(m_fingeringFont),
        m_timeSigFont(npf.m_timeSigFont),
        m_timeSigFontMetrics(m_timeSigFont),
        m_bigTimeSigFont(npf.m_bigTimeSigFont),
        m_bigTimeSigFontMetrics(m_bigTimeSigFont),
        m_ottavaFont(defaultSerifFontFamily, 8, QFont::Normal, true),
        m_ottavaFontMetrics(m_ottavaFont),
        m_trackHeaderFont(defaultSansSerifFontFamily, 10, QFont::Normal),
        m_trackHeaderFontMetrics(m_trackHeaderFont),
        m_generatedPixmap(0),
        m_generatedMask(0),
        m_generatedWidth( -1),
        m_generatedHeight( -1),
        m_inPrinterMethod(false),
        m_p(new NotePixmapPainter()),
        m_dottedRestCache(new NotePixmapCache)
{
    init(npf.m_font->getName(), npf.m_font->getSize());
}

NotePixmapFactory &
NotePixmapFactory::operator=(const NotePixmapFactory &npf)
{
    if (&npf != this) {
        m_selected = npf.m_selected;
        m_shaded = npf.m_shaded;
        m_timeSigFont = npf.m_timeSigFont;
        m_timeSigFontMetrics = QFontMetrics(m_timeSigFont);
        m_bigTimeSigFont = npf.m_bigTimeSigFont;
        m_bigTimeSigFontMetrics = QFontMetrics(m_bigTimeSigFont);
        m_tupletCountFont = npf.m_tupletCountFont;
        m_tupletCountFontMetrics = QFontMetrics(m_tupletCountFont);
        m_textMarkFont = npf.m_textMarkFont;
        m_textMarkFontMetrics = QFontMetrics(m_textMarkFont);
        m_fingeringFont = npf.m_fingeringFont;
        m_fingeringFontMetrics = QFontMetrics(m_fingeringFont);
        m_ottavaFont = npf.m_ottavaFont;
        m_ottavaFontMetrics = QFontMetrics(m_ottavaFontMetrics);
        init(npf.m_font->getName(), npf.m_font->getSize());
        m_dottedRestCache->clear();
        m_textFontCache.clear();
    }
    return *this;
}

void
NotePixmapFactory::init(std::string fontName, int size)
{
    try {
        m_style = NoteStyleFactory::getStyle(NoteStyleFactory::DefaultStyle);
    } catch (NoteStyleFactory::StyleUnavailable u) {
        KStartupLogo::hideIfStillThere();
        KMessageBox::error(0, i18n(strtoqstr(u.getMessage())));
        throw;
    }

    int origSize = size;

    if (fontName != "") {
        try {
            if (size < 0)
                size = NoteFontFactory::getDefaultSize(fontName);
            m_font = NoteFontFactory::getFont(fontName, size);
        } catch (Exception f) {
            fontName = "";
            // fall through
        }
    }

    if (fontName == "") { // either because it was passed in or because read failed
        try {
            fontName = NoteFontFactory::getDefaultFontName();
            size = origSize;
            if (size < 0)
                size = NoteFontFactory::getDefaultSize(fontName);
            m_font = NoteFontFactory::getFont(fontName, size);
        } catch (Exception f) { // already reported
            throw;
        }
    }

    // Resize the fonts, because the original constructor used point
    // sizes only and we want pixels
    QFont timeSigFont(defaultTimeSigFontFamily),
        textFont(defaultSerifFontFamily);
    KConfig* config = kapp->config();
    config->setGroup(NotationViewConfigGroup);

    m_timeSigFont = config->readFontEntry("timesigfont", &timeSigFont);
    m_timeSigFont.setBold(true);
    m_timeSigFont.setPixelSize(size * 5 / 2);
    m_timeSigFontMetrics = QFontMetrics(m_timeSigFont);

    m_bigTimeSigFont = config->readFontEntry("timesigfont", &timeSigFont);
    m_bigTimeSigFont.setPixelSize(size * 4 + 2);
    m_bigTimeSigFontMetrics = QFontMetrics(m_bigTimeSigFont);

    m_tupletCountFont = config->readFontEntry("textfont", &textFont);
    m_tupletCountFont.setBold(true);
    m_tupletCountFont.setPixelSize(size * 2);
    m_tupletCountFontMetrics = QFontMetrics(m_tupletCountFont);

    m_textMarkFont = config->readFontEntry("textfont", &textFont);
    m_textMarkFont.setBold(true);
    m_textMarkFont.setItalic(true);
    m_textMarkFont.setPixelSize(size * 2);
    m_textMarkFontMetrics = QFontMetrics(m_textMarkFont);

    m_fingeringFont = config->readFontEntry("textfont", &textFont);
    m_fingeringFont.setBold(true);
    m_fingeringFont.setPixelSize(size * 5 / 3);
    m_fingeringFontMetrics = QFontMetrics(m_fingeringFont);

    m_ottavaFont = config->readFontEntry("textfont", &textFont);
    m_ottavaFont.setPixelSize(size * 2);
    m_ottavaFontMetrics = QFontMetrics(m_ottavaFont);

    m_trackHeaderFont = QFont(defaultSansSerifFontFamily);
    m_trackHeaderFont = config->readFontEntry("sansfont", &m_trackHeaderFont);
    m_trackHeaderFont.setPixelSize(size * 3 / 2);
    m_trackHeaderFontMetrics = QFontMetrics(m_trackHeaderFont);
}

NotePixmapFactory::~NotePixmapFactory()
{
    delete m_p;
    delete m_dottedRestCache;
}

std::string
NotePixmapFactory::getFontName() const
{
    return m_font->getName();
}

int
NotePixmapFactory::getSize() const
{
    return m_font->getSize();
}

QPixmap
NotePixmapFactory::toQPixmap(QCanvasPixmap* cp)
{
    QPixmap p = *cp;
    delete cp;
    return p;
}

void
NotePixmapFactory::dumpStats(std::ostream &s)
{
#ifdef DUMP_STATS
    s << "NotePixmapFactory: total times since last stats dump:\n"
    << "makeNotePixmap: "
    << (makeNotesTime * 1000 / CLOCKS_PER_SEC) << "ms\n"
    << "drawBeams: "
    << (drawBeamsTime * 1000 / CLOCKS_PER_SEC) << "ms"
    << " (drew " << drawBeamsCount << " individual points in " << drawBeamsBeamCount << " beams)"
    << endl;
    makeNotesTime = 0;
    drawBeamsTime = 0;
    drawBeamsCount = 0;
    drawBeamsBeamCount = 0;
#endif

    (void)s; // avoid warnings
}

QCanvasPixmap*
NotePixmapFactory::makeNotePixmap(const NotePixmapParameters &params)
{
    Profiler profiler("NotePixmapFactory::makeNotePixmap");
    clock_t startTime = clock();

    drawNoteAux(params, 0, 0, 0);

    QPoint hotspot(m_left, m_above + m_noteBodyHeight / 2);

    //#define ROSE_DEBUG_NOTE_PIXMAP_FACTORY
#ifdef ROSE_DEBUG_NOTE_PIXMAP_FACTORY

    m_p->painter().setPen(Qt::red);
    m_p->painter().setBrush(Qt::red);

    m_p->drawLine(0, 0, 0, m_generatedHeight - 1);
    m_p->drawLine(m_generatedWidth - 1, 0,
                  m_generatedWidth - 1,
                  m_generatedHeight - 1);

    {
        int hsx = hotspot.x();
        int hsy = hotspot.y();
        m_p->drawLine(hsx - 2, hsy - 2, hsx + 2, hsy + 2);
        m_p->drawLine(hsx - 2, hsy + 2, hsx + 2, hsy - 2);
    }
#endif

    clock_t endTime = clock();
    makeNotesTime += (endTime - startTime);

    return makeCanvasPixmap(hotspot);
}

void
NotePixmapFactory::drawNote(const NotePixmapParameters &params,
                            QPainter &painter, int x, int y)
{
    Profiler profiler("NotePixmapFactory::drawNote");
    m_inPrinterMethod = true;
    drawNoteAux(params, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::drawNoteAux(const NotePixmapParameters &params,
                               QPainter *painter, int x, int y)
{
    NoteFont::CharacterType charType = m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen;

    bool drawFlag = params.m_drawFlag;

    if (params.m_beamed)
        drawFlag = false;

    // A note pixmap is formed of note head, stem, flags,
    // accidentals, dots and beams.  Assume the note head first, then
    // do the rest of the calculations left to right, ie accidentals,
    // stem, flags, dots, beams

    m_noteBodyWidth = getNoteBodyWidth(params.m_noteType);
    m_noteBodyHeight = getNoteBodyHeight(params.m_noteType);

    // Spacing surrounding the note head.  For top and bottom, we
    // adjust this according to the discrepancy between the nominal
    // and actual heights of the note head pixmap.  For left and
    // right, we use the hotspot x coordinate of the head.
    int temp;
    if (!m_font->getHotspot(m_style->getNoteHeadCharName(params.m_noteType).first,
                            m_borderX, temp))
        m_borderX = 0;

    if (params.m_noteType == Note::Minim && params.m_stemGoesUp)
        m_borderX++;
    int actualNoteBodyHeight =
        m_font->getHeight(m_style->getNoteHeadCharName(params.m_noteType).first);

    m_left = m_right = m_borderX;
    m_above = m_borderY = (actualNoteBodyHeight - m_noteBodyHeight) / 2;
    m_below = (actualNoteBodyHeight - m_noteBodyHeight) - m_above;

    //    NOTATION_DEBUG << "actualNoteBodyHeight: " << actualNoteBodyHeight
    //		   << ", noteBodyHeight: " << m_noteBodyHeight << ", borderX: "
    //		   << m_borderX << ", borderY: "
    //		   << m_borderY << endl;

    bool isStemmed = m_style->hasStem(params.m_noteType);
    int flagCount = m_style->getFlagCount(params.m_noteType);
    int slashCount = params.m_slashes;
    if (!slashCount)
        slashCount = m_style->getSlashCount(params.m_noteType);

    if (params.m_accidental != NoAccidental) {
        makeRoomForAccidental(params.m_accidental,
                              params.m_cautionary,
                              params.m_accidentalShift,
                              params.m_accidentalExtra);
    }

    NoteCharacter dot(getCharacter(NoteCharacterNames::DOT, PlainColour, charType));
    int dotWidth = dot.getWidth();
    if (dotWidth < getNoteBodyWidth() / 2)
        dotWidth = getNoteBodyWidth() / 2;

    int stemLength = getStemLength(params);

    if (params.m_marks.size() > 0) {
        makeRoomForMarks(isStemmed, params, stemLength);
    }

    if (params.m_legerLines != 0) {
        makeRoomForLegerLines(params);
    }

    if (slashCount > 0) {
        m_left = std::max(m_left, m_noteBodyWidth / 2);
        m_right = std::max(m_right, m_noteBodyWidth / 2);
    }

    if (params.m_tupletCount > 0) {
        makeRoomForTuplingLine(params);
    }

    m_right = std::max(m_right, params.m_dots * dotWidth + dotWidth / 2);
    if (params.m_dotShifted) {
        m_right += m_noteBodyWidth;
    }
    if (params.m_onLine) {
        m_above = std::max(m_above, dot.getHeight() / 2);
    }

    if (params.m_shifted) {
        if (params.m_stemGoesUp) {
            m_right += m_noteBodyWidth;
        } else {
            m_left = std::max(m_left, m_noteBodyWidth);
        }
    }

    bool tieAbove = params.m_tieAbove;
    if (!params.m_tiePositionExplicit) {
        tieAbove = !params.m_stemGoesUp;
    }

    if (params.m_tied) {
        m_right = std::max(m_right, params.m_tieLength);
        if (!tieAbove) {
            m_below = std::max(m_below, m_noteBodyHeight * 2);
        } else {
            m_above = std::max(m_above, m_noteBodyHeight * 2);
        }
    }

    QPoint startPoint, endPoint;
    if (isStemmed && params.m_drawStem) {
        makeRoomForStemAndFlags(drawFlag ? flagCount : 0, stemLength, params,
                                startPoint, endPoint);
    }

    if (isStemmed && params.m_drawStem && params.m_beamed) {
        makeRoomForBeams(params);
    }

    // for all other calculations we use the nominal note-body height
    // (same as the gap between staff lines), but here we want to know
    // if the pixmap itself is taller than that
    /*!!!
        int actualNoteBodyHeight = m_font->getHeight
    	(m_style->getNoteHeadCharName(params.m_noteType).first);
    //	- 2*m_origin.y();
        if (actualNoteBodyHeight > m_noteBodyHeight) {
    	m_below = std::max(m_below, actualNoteBodyHeight - m_noteBodyHeight);
        }
    */
    if (painter) {
        painter->save();
        m_p->beginExternal(painter);
        //	NOTATION_DEBUG << "Translate: (" << x << "," << y << ")" << endl;
        painter->translate(x - m_left, y - m_above - m_noteBodyHeight / 2);
    } else {
        createPixmapAndMask(m_noteBodyWidth + m_left + m_right,
                            m_noteBodyHeight + m_above + m_below);
    }

    if (params.m_tupletCount > 0) {
        drawTuplingLine(params);
    }

    if (isStemmed && params.m_drawStem && drawFlag) {
        drawFlags(flagCount, params, startPoint, endPoint);
    }

    if (params.m_accidental != NoAccidental) {
        drawAccidental(params.m_accidental, params.m_cautionary);
    }

    NoteStyle::CharNameRec charNameRec
    (m_style->getNoteHeadCharName(params.m_noteType));
    CharName charName = charNameRec.first;
    bool inverted = charNameRec.second;
    NoteCharacter body = getCharacter
                         (charName,
                          params.m_highlighted ? HighlightedColour :
                          params.m_quantized ? QuantizedColour :
                          params.m_trigger ? TriggerColour :
                          params.m_inRange ? PlainColour : OutRangeColour,
                          inverted);

    QPoint bodyLocation(m_left - m_borderX,
                        m_above - m_borderY + getStaffLineThickness() / 2);
    if (params.m_shifted) {
        if (params.m_stemGoesUp) {
            bodyLocation.rx() += m_noteBodyWidth;
        } else {
            bodyLocation.rx() -= m_noteBodyWidth - 1;
        }
    }

    m_p->drawNoteCharacter(bodyLocation.x(), bodyLocation.y(), body);

    if (params.m_dots > 0) {

        int x = m_left + m_noteBodyWidth + dotWidth / 2;
        int y = m_above + m_noteBodyHeight / 2 - dot.getHeight() / 2;

        if (params.m_onLine)
            y -= m_noteBodyHeight / 2;

        if (params.m_shifted)
            x += m_noteBodyWidth;
        else if (params.m_dotShifted)
            x += m_noteBodyWidth;

        for (int i = 0; i < params.m_dots; ++i) {
            m_p->drawNoteCharacter(x, y, dot);
            x += dotWidth;
        }
    }

    if (isStemmed && params.m_drawStem) {

        if (flagCount > 0 && !drawFlag && params.m_beamed) {
            drawBeams(endPoint, params, flagCount);
        }

        if (slashCount > 0) {
            drawSlashes(startPoint, params, slashCount);
        }

        if (m_selected)
            m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
        else
            m_p->painter().setPen(Qt::black);

        // If we draw stems after beams, instead of beams after stems,
        // beam anti-aliasing won't damage stems but we have to shorten the
        // stems slightly first so that the stems don't extend all the way
        // through the beam into the anti-aliased region on the
        // other side of the beam that faces away from the note-heads.
        int shortening;
        if (flagCount > 0 && !drawFlag && params.m_beamed)
            shortening = 2;
        else
            shortening = 0;
        drawStem(params, startPoint, endPoint, shortening);
    }

    if (params.m_marks.size() > 0) {
        drawMarks(isStemmed, params, stemLength);
    }

    if (params.m_legerLines != 0) {
        drawLegerLines(params);
    }

    if (params.m_tied) {
        drawTie(tieAbove, params.m_tieLength, dotWidth * params.m_dots);
    }

    if (painter) {
        painter->restore();
    }
}


QCanvasPixmap*
NotePixmapFactory::makeNoteHaloPixmap(const NotePixmapParameters &params)
{
    int nbh0 = getNoteBodyHeight();
    int nbh = getNoteBodyHeight(params.m_noteType);
    int nbw0 = getNoteBodyHeight();
    int nbw = getNoteBodyWidth(params.m_noteType);
    int hOffset = 0;

    createPixmapAndMask(nbw + nbw0, nbh + nbh0);
    drawNoteHalo(0, 0, nbw + nbw0, nbh + nbh0);

    return makeCanvasPixmap(QPoint(nbw0 / 2, nbh0));
}


void
NotePixmapFactory::drawNoteHalo(int x, int y, int w, int h) {

   m_p->painter().setPen(QPen(QColor(GUIPalette::CollisionHaloHue,
                                     GUIPalette::CollisionHaloSaturation,
                                     255, QColor::Hsv), 1));
   m_p->painter().setBrush(QColor(GUIPalette::CollisionHaloHue,
                                  GUIPalette::CollisionHaloSaturation,
                                  255, QColor::Hsv));
   m_p->drawEllipse(x, y, w, h);
}



int
NotePixmapFactory::getStemLength(const NotePixmapParameters &params) const
{
    if (params.m_beamed && params.m_stemLength >= 0) {
        return params.m_stemLength;
    }

    int stemLength = getStemLength();

    int flagCount = m_style->getFlagCount(params.m_noteType);
    int slashCount = params.m_slashes;
    bool stemUp = params.m_stemGoesUp;
    int nbh = m_noteBodyHeight;

    if (flagCount > 2) {
        stemLength += getLineSpacing() * (flagCount - 2);
    }

    int width = 0, height = 0;

    if (flagCount > 0) {

        if (!stemUp)
            stemLength += nbh / 2;

        if (m_font->getDimensions(m_style->getFlagCharName(flagCount),
                                  width, height)) {

            stemLength = std::max(stemLength, height);

        } else if (m_font->getDimensions(m_style->getPartialFlagCharName(true),
                                         width, height) ||
                   m_font->getDimensions(m_style->getPartialFlagCharName(false),
                                         width, height)) {

            unsigned int flagSpace = m_noteBodyHeight;
            (void)m_font->getFlagSpacing(flagSpace);

            stemLength = std::max(stemLength,
                                  height + (flagCount - 1) * (int)flagSpace);
        }
    }

    if (slashCount > 3 && flagCount < 3) {
        stemLength += (slashCount - 3) * (nbh / 2);
    }

    if (params.m_stemLength >= 0) {
        if (flagCount == 0)
            return params.m_stemLength;
        stemLength = std::max(stemLength, params.m_stemLength);
    }

    return stemLength;
}

void
NotePixmapFactory::makeRoomForAccidental(Accidental a,
        bool cautionary, int shift, bool extra)
{
    // General observation: where we're only using a character to
    // determine its dimensions, we should (for the moment) just
    // request it in screen mode, because it may be quicker and we
    // don't need to render it, and the dimensions are the same.
    NoteCharacter ac
    (m_font->getCharacter(m_style->getAccidentalCharName(a)));

    QPoint ah(m_font->getHotspot(m_style->getAccidentalCharName(a)));

    m_left += ac.getWidth() + (m_noteBodyWidth / 4 - m_borderX);

    if (shift > 0) {
        if (extra) {
            // The extra flag indicates that the first shift is to get
            // out of the way of a note head, thus has to move
            // possibly further, or at least a different amount.  So
            // replace the first shift with a different one.
            --shift;
            m_left += m_noteBodyWidth - m_noteBodyWidth / 5;
        }
        if (shift > 0) {
            // The amount we shift for each accidental is the greater
            // of the probable shift for that accidental and the
            // probable shift for a sharp, on the assumption (usually
            // true in classical notation) that the sharp is the
            // widest accidental and that we may have other
            // accidentals possibly including sharps on other notes in
            // this chord that we can't know about here.
            int step = ac.getWidth() - ah.x();
            if (a != Accidentals::Sharp) {
                NoteCharacter acSharp
                (m_font->getCharacter(m_style->getAccidentalCharName
                                      (Accidentals::Sharp)));
                QPoint ahSharp
                (m_font->getHotspot(m_style->getAccidentalCharName
                                    (Accidentals::Sharp)));
                step = std::max(step, acSharp.getWidth() - ahSharp.x());
            }
            m_left += shift * step;
        }
    }

    if (cautionary)
        m_left += m_noteBodyWidth;

    int above = ah.y() - m_noteBodyHeight / 2;
    int below = (ac.getHeight() - ah.y()) -
                (m_noteBodyHeight - m_noteBodyHeight / 2); // subtract in case it's odd

    if (above > 0)
        m_above = std::max(m_above, above);
    if (below > 0)
        m_below = std::max(m_below, below);
}

void
NotePixmapFactory::drawAccidental(Accidental a, bool cautionary)
{
    NoteCharacter ac = getCharacter
                       (m_style->getAccidentalCharName(a), PlainColour, false);

    QPoint ah(m_font->getHotspot(m_style->getAccidentalCharName(a)));

    int ax = 0;

    if (cautionary) {
        ax += m_noteBodyWidth / 2;
        int bl = ac.getHeight() * 2 / 3;
        int by = m_above + m_noteBodyHeight / 2 - bl / 2;
        drawBracket(bl, true, false, m_noteBodyWidth*3 / 8, by);
        drawBracket(bl, false, false, ac.getWidth() + m_noteBodyWidth*5 / 8, by);
    }

    m_p->drawNoteCharacter(ax, m_above + m_noteBodyHeight / 2 - ah.y(), ac);
}

void
NotePixmapFactory::makeRoomForMarks(bool isStemmed,
                                    const NotePixmapParameters &params,
                                    int stemLength)
{
    int height = 0, width = 0;
    int gap = m_noteBodyHeight / 5 + 1;

    std::vector<Mark> normalMarks = params.getNormalMarks();
    std::vector<Mark> aboveMarks = params.getAboveMarks();

    for (std::vector<Mark>::iterator i = normalMarks.begin();
            i != normalMarks.end(); ++i) {

        if (!Marks::isTextMark(*i)) {

            NoteCharacter character(m_font->getCharacter(m_style->getMarkCharName(*i)));
            height += character.getHeight() + gap;
            if (character.getWidth() > width)
                width = character.getWidth();

        } else {
            // Inefficient to do this here _and_ in drawMarks, but
            // text marks are not all that common
            QString text = strtoqstr(Marks::getTextFromMark(*i));
            QRect bounds = m_textMarkFontMetrics.boundingRect(text);
            height += bounds.height() + gap;
            if (bounds.width() > width)
                width = bounds.width();
        }
    }

    if (height > 0) {
        if (isStemmed && params.m_stemGoesUp) {
            m_below += height + 1;
        } else {
            m_above += height + 1;
        }
    }

    height = 0;

    if (params.m_safeVertDistance > 0 && !aboveMarks.empty()) {
        m_above = std::max(m_above, params.m_safeVertDistance);
    }

    for (std::vector<Mark>::iterator i = aboveMarks.begin();
            i != aboveMarks.end(); ++i) {

        if (!Marks::isFingeringMark(*i)) {

            Mark m(*i);

            if (m == Marks::TrillLine)
                m = Marks::LongTrill;

            if (m == Marks::LongTrill) {
                m_right = std::max(m_right, params.m_width);
            }

            NoteCharacter character(m_font->getCharacter(m_style->getMarkCharName(m)));
            height += character.getHeight() + gap;
            if (character.getWidth() > width)
                width = character.getWidth();

        } else {

            // Inefficient to do this here _and_ in drawMarks
            QString text = strtoqstr(Marks::getFingeringFromMark(*i));
            QRect bounds = m_fingeringFontMetrics.boundingRect(text);
            height += bounds.height() + gap + 3;
            if (bounds.width() > width)
                width = bounds.width();
        }
    }

    if (height > 0) {
        if (isStemmed && params.m_stemGoesUp && params.m_safeVertDistance == 0) {
            m_above += stemLength + height + 1;
        } else {
            m_above += height + 1;
        }
    }

    m_left = std::max(m_left, width / 2 - m_noteBodyWidth / 2);
    m_right = std::max(m_right, width / 2 - m_noteBodyWidth / 2);
}

void
NotePixmapFactory::drawMarks(bool isStemmed,
                             const NotePixmapParameters &params,
                             int stemLength)
{
    int gap = m_noteBodyHeight / 5 + 1;
    int dy = gap;

    std::vector<Mark> normalMarks = params.getNormalMarks();
    std::vector<Mark> aboveMarks = params.getAboveMarks();

    bool normalMarksAreAbove = !(isStemmed && params.m_stemGoesUp);

    for (std::vector<Mark>::iterator i = normalMarks.begin();
            i != normalMarks.end(); ++i) {

        if (!Marks::isTextMark(*i)) {

            NoteCharacter character = getCharacter
                                      (m_style->getMarkCharName(*i), PlainColour,
                                       !normalMarksAreAbove);

            int x = m_left + m_noteBodyWidth / 2 - character.getWidth() / 2;
            int y = (normalMarksAreAbove ?
                     (m_above - dy - character.getHeight() - 1) :
                     (m_above + m_noteBodyHeight + m_borderY * 2 + dy));

            m_p->drawNoteCharacter(x, y, character);
            dy += character.getHeight() + gap;

        } else {

            QString text = strtoqstr(Marks::getTextFromMark(*i));
            QRect bounds = m_textMarkFontMetrics.boundingRect(text);

            m_p->painter().setFont(m_textMarkFont);
            if (!m_inPrinterMethod)
                m_p->maskPainter().setFont(m_textMarkFont);

            int x = m_left + m_noteBodyWidth / 2 - bounds.width() / 2;
            int y = (normalMarksAreAbove ?
                     (m_above - dy - 3) :
                     (m_above + m_noteBodyHeight + m_borderY * 2 + dy + bounds.height() + 1));

            m_p->drawText(x, y, text);
            dy += bounds.height() + gap;
        }
    }

    if (!normalMarksAreAbove)
        dy = gap;
    if (params.m_safeVertDistance > 0) {
        if (normalMarksAreAbove) {
            dy = std::max(dy, params.m_safeVertDistance);
        } else {
            dy = params.m_safeVertDistance;
        }
    } else if (isStemmed && params.m_stemGoesUp) {
        dy += stemLength;
    }

    for (std::vector<Mark>::iterator i = aboveMarks.begin();
            i != aboveMarks.end(); ++i) {

        if (m_selected)
            m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
        else
            m_p->painter().setPen(Qt::black);
        if (!Marks::isFingeringMark(*i)) {

            int x = m_left + m_noteBodyWidth / 2;
            int y = m_above - dy - 1;

            if (*i != Marks::TrillLine) {

                NoteCharacter character
                (getCharacter
                 (m_style->getMarkCharName(*i), PlainColour,
                  false));

                x -= character.getWidth() / 2;
                y -= character.getHeight();

                m_p->drawNoteCharacter(x, y, character);

                y += character.getHeight() / 2;
                x += character.getWidth();

                dy += character.getHeight() + gap;

            } else {

                NoteCharacter character
                (getCharacter
                 (m_style->getMarkCharName(Marks::Trill), PlainColour,
                  false));
                y -= character.getHeight() / 2;
                dy += character.getHeight() + gap;
            }

            if (*i == Marks::LongTrill ||
                    *i == Marks::TrillLine) {
                NoteCharacter extension;
                if (getCharacter(NoteCharacterNames::TRILL_LINE, extension,
                                 PlainColour, false)) {
                    x += extension.getHotspot().x();
                    while (x < m_left + params.m_width - extension.getWidth()) {
                        x -= extension.getHotspot().x();
                        m_p->drawNoteCharacter(x, y, extension);
                        x += extension.getWidth();
                    }
                }
                if (*i == Marks::TrillLine)
                    dy += extension.getHeight() + gap;
            }

        } else {
            QString text = strtoqstr(Marks::getFingeringFromMark(*i));
            QRect bounds = m_fingeringFontMetrics.boundingRect(text);

            m_p->painter().setFont(m_fingeringFont);
            if (!m_inPrinterMethod)
                m_p->maskPainter().setFont(m_fingeringFont);

            int x = m_left + m_noteBodyWidth / 2 - bounds.width() / 2;
            int y = m_above - dy - 3;

            m_p->drawText(x, y, text);
            dy += bounds.height() + gap;
        }
    }
}

void
NotePixmapFactory::makeRoomForLegerLines(const NotePixmapParameters &params)
{
    if (params.m_legerLines < 0 || params.m_restOutsideStave) {
        m_above = std::max(m_above,
                           (m_noteBodyHeight + 1) *
                           ( -params.m_legerLines / 2));
    }
    if (params.m_legerLines > 0 || params.m_restOutsideStave) {
        m_below = std::max(m_below,
                           (m_noteBodyHeight + 1) *
                           (params.m_legerLines / 2));
    }
    if (params.m_legerLines != 0) {
        m_left = std::max(m_left, m_noteBodyWidth / 5 + 1);
        m_right = std::max(m_right, m_noteBodyWidth / 5 + 1);
    }
    if (params.m_restOutsideStave) {
        m_above += 1;
        m_left = std::max(m_left, m_noteBodyWidth * 3 + 1);
        m_right = std::max(m_right, m_noteBodyWidth * 3 + 1);
    }
}

void
NotePixmapFactory::drawLegerLines(const NotePixmapParameters &params)
{
    int x0, x1, y;

    if (params.m_legerLines == 0)
        return ;

    if (params.m_restOutsideStave) {
        if (m_selected)
            m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
        else
            m_p->painter().setPen(Qt::black);
    }
    x0 = m_left - m_noteBodyWidth / 5 - 1;
    x1 = m_left + m_noteBodyWidth + m_noteBodyWidth / 5 /* + 1 */;

    if (params.m_shifted) {
        if (params.m_stemGoesUp) {
            x0 += m_noteBodyWidth;
            x1 += m_noteBodyWidth;
        } else {
            x0 -= m_noteBodyWidth;
            x1 -= m_noteBodyWidth;
        }
    }

    int offset = m_noteBodyHeight + getStaffLineThickness();
    int legerLines = params.m_legerLines;
    bool below = (legerLines < 0);

    if (below) {
        legerLines = -legerLines;
        offset = -offset;
    }

    if (params.m_restOutsideStave)
        y = m_above;
    else {
        if (!below) { // note above staff
            if (legerLines % 2) { // note is between lines
                y = m_above + m_noteBodyHeight;
            } else { // note is on a line
                y = m_above + m_noteBodyHeight / 2 - getStaffLineThickness() / 2;
            }
        } else { // note below staff
            if (legerLines % 2) { // note is between lines
                y = m_above - getStaffLineThickness();
            } else { // note is on a line
                y = m_above + m_noteBodyHeight / 2;
            }
        }
    }
    if (params.m_restOutsideStave) {
        NOTATION_DEBUG << "draw leger lines: " << legerLines << " lines, below "
        << below
        << ", note body height " << m_noteBodyHeight
        << ", thickness " << getLegerLineThickness()
        << " (staff line " << getStaffLineThickness() << ")"
        << ", offset " << offset << endl;
    }

    //    NOTATION_DEBUG << "draw leger lines: " << legerLines << " lines, below "
    //		   << below
    //		   << ", note body height " << m_noteBodyHeight
    //		   << ", thickness " << getLegerLineThickness()
    //		   << " (staff line " << getStaffLineThickness() << ")"
    //		   << ", offset " << offset << endl;

    //    bool first = true;

    if (getLegerLineThickness() > getStaffLineThickness()) {
        y -= (getLegerLineThickness() - getStaffLineThickness() + 1) / 2;
    }

    for (int i = legerLines - 1; i >= 0; --i) {
        if (i % 2) {
            //	    NOTATION_DEBUG << "drawing leger line at y = " << y << endl;
            for (int j = 0; j < getLegerLineThickness(); ++j) {
                m_p->drawLine(x0, y + j, x1, y + j);
            }
            y += offset;
            //	    if (first) {
            //		x0 += getStemThickness();
            //		x1 -= getStemThickness();
            //		first = false;
            //	    }
        }
    }
}

void
NotePixmapFactory::makeRoomForStemAndFlags(int flagCount, int stemLength,
        const NotePixmapParameters &params,
        QPoint &s0, QPoint &s1)
{
    // The coordinates we set in s0 and s1 are relative to (m_above, m_left)

    if (params.m_stemGoesUp) {
        m_above = std::max
                  (m_above, stemLength - m_noteBodyHeight / 2);
    } else {
        m_below = std::max
                  (m_below, stemLength - m_noteBodyHeight / 2 + 1);
    }

    if (flagCount > 0) {
        if (params.m_stemGoesUp) {
            int width = 0, height = 0;
            if (!m_font->getDimensions
                    (m_style->getFlagCharName(flagCount), width, height)) {
                width = m_font->getWidth(m_style->getPartialFlagCharName(false));
            }
            m_right += width;
        }
    }

    unsigned int stemThickness = getStemThickness();

    NoteStyle::HFixPoint hfix;
    NoteStyle::VFixPoint vfix;
    m_style->getStemFixPoints(params.m_noteType, hfix, vfix);

    switch (hfix) {

    case NoteStyle::Normal:
    case NoteStyle::Reversed:
        if (params.m_stemGoesUp ^ (hfix == NoteStyle::Reversed)) {
            s0.setX(m_noteBodyWidth - stemThickness);
        } else {
            s0.setX(0);
        }
        break;

    case NoteStyle::Central:
        if (params.m_stemGoesUp ^ (hfix == NoteStyle::Reversed)) {
            s0.setX(m_noteBodyWidth / 2 + 1);
        } else {
            s0.setX(m_noteBodyWidth / 2);
        }
        break;
    }

    switch (vfix) {

    case NoteStyle::Near:
    case NoteStyle::Far:
        if (params.m_stemGoesUp ^ (vfix == NoteStyle::Far)) {
            s0.setY(0);
        } else {
            s0.setY(m_noteBodyHeight);
        }
        if (vfix == NoteStyle::Near) {
            stemLength -= m_noteBodyHeight / 2;
        } else {
            stemLength += m_noteBodyHeight / 2;
        }
        break;

    case NoteStyle::Middle:
        if (params.m_stemGoesUp) {
            s0.setY(m_noteBodyHeight * 3 / 8);
        } else {
            s0.setY(m_noteBodyHeight * 5 / 8);
        }
        stemLength -= m_noteBodyHeight / 8;
        break;
    }

    if (params.m_stemGoesUp) {
        s1.setY(s0.y() - stemLength + getStaffLineThickness());
    } else {
        s1.setY(s0.y() + stemLength);
    }

    s1.setX(s0.x());
}

void
NotePixmapFactory::drawFlags(int flagCount,
                             const NotePixmapParameters &params,
                             const QPoint &, const QPoint &s1)
{
    if (flagCount < 1)
        return ;

    NoteCharacter flagChar;
    bool found = getCharacter(m_style->getFlagCharName(flagCount),
                              flagChar,
                              PlainColour,
                              !params.m_stemGoesUp);

    if (!found) {

        // Handle fonts that don't have all the flags in separate characters

        found = getCharacter(m_style->getPartialFlagCharName(false),
                             flagChar,
                             PlainColour,
                             !params.m_stemGoesUp);

        if (!found) {
            std::cerr << "Warning: NotePixmapFactory::drawFlags: No way to draw note with " << flagCount << " flags in this font!?" << std::endl;
            return ;
        }

        QPoint hotspot = flagChar.getHotspot();

        NoteCharacter oneFlagChar;
        bool foundOne =
            (flagCount > 1 ?
             getCharacter(m_style->getPartialFlagCharName(true),
                          oneFlagChar,
                          PlainColour,
                          !params.m_stemGoesUp) : false);

        unsigned int flagSpace = m_noteBodyHeight;
        (void)m_font->getFlagSpacing(flagSpace);

        for (int flag = 0; flag < flagCount; ++flag) {

            // use flag_1 in preference to flag_0 for the final flag, so
            // as to end with a flourish
            if (flag == flagCount - 1 && foundOne)
                flagChar = oneFlagChar;

            int y = m_above + s1.y();
            if (params.m_stemGoesUp)
                y += flag * flagSpace;
            else
                y -= (flag * flagSpace) + flagChar.getHeight();

            if (!m_inPrinterMethod) {

                m_p->end();

                // Super-slow

                PixmapFunctions::drawPixmapMasked(*m_generatedPixmap,
                                                  *m_generatedMask,
                                                  m_left + s1.x() - hotspot.x(),
                                                  y,
                                                  *flagChar.getPixmap());

                m_p->begin(m_generatedPixmap, m_generatedMask);

            } else {

                // No problem with mask here
                m_p->drawNoteCharacter(m_left + s1.x() - hotspot.x(),
                                       y,
                                       flagChar);
            }
        }

    } else { // the normal case

        QPoint hotspot = flagChar.getHotspot();

        int y = m_above + s1.y();
        if (!params.m_stemGoesUp)
            y -= flagChar.getHeight();

        m_p->drawNoteCharacter(m_left + s1.x() - hotspot.x(), y, flagChar);
    }
}

void
NotePixmapFactory::drawStem(const NotePixmapParameters &params,
                            const QPoint &s0, const QPoint &s1,
                            int shortening)
{
    if (params.m_stemGoesUp)
        shortening = -shortening;
    for (int i = 0; i < getStemThickness(); ++i) {
        m_p->drawLine(m_left + s0.x() + i, m_above + s0.y(),
                      m_left + s1.x() + i, m_above + s1.y() - shortening);
    }
}

void
NotePixmapFactory::makeRoomForBeams(const NotePixmapParameters &params)
{
    int beamSpacing = (int)(params.m_width * params.m_gradient);

    if (params.m_stemGoesUp) {

        beamSpacing = -beamSpacing;
        if (beamSpacing < 0)
            beamSpacing = 0;
        m_above += beamSpacing + 2;

        // allow a bit extra in case the h fixpoint is non-normal
        m_right = std::max(m_right, params.m_width + m_noteBodyWidth);

    } else {

        if (beamSpacing < 0)
            beamSpacing = 0;
        m_below += beamSpacing + 2;

        m_right = std::max(m_right, params.m_width);
    }
}

void
NotePixmapFactory::drawShallowLine(int x0, int y0, int x1, int y1,
                                   int thickness, bool smooth)
{
    if (!smooth || m_inPrinterMethod || (y0 == y1)) {

        if (!m_inPrinterMethod) {
            if (m_selected)
                m_p->painter().setBrush(GUIPalette::getColour(GUIPalette::SelectedElement));
            else
                m_p->painter().setBrush(Qt::black);
        }
        if (thickness < 4) {
            for (int i = 0; i < thickness; ++i) {
                m_p->drawLine(x0, y0 + i, x1, y1 + i);
            }
        } else {
            Profiler profiler("NotePixmapFactory::drawShallowLine(polygon)");
            QPointArray qp(4);
            qp.setPoint(0, x0, y0);
            qp.setPoint(1, x0, y0 + thickness);
            qp.setPoint(2, x1, y1 + thickness);
            qp.setPoint(3, x1, y1);
            m_p->drawPolygon(qp);
        }

        return ;
    }

    Profiler profiler("NotePixmapFactory::drawShallowLine(points)");

    int dv = y1 - y0;
    int dh = x1 - x0;

    static std::vector<QColor> colours, selectedColours;
    if (colours.size() == 0) {
        int h, s, v;
        QColor c = GUIPalette::getColour(GUIPalette::SelectedElement);
        c.hsv(&h, &s, &v);
        for (int step = 0; step < 256; step += (step == 0 ? 63 : 64)) {
            colours.push_back(QColor( -1, 0, step, QColor::Hsv));
            selectedColours.push_back(QColor(h, 255 - step, v, QColor::Hsv));
        }
    }

    int cx = x0, cy = y0;

    int inc = 1;

    if (dv < 0) {
        dv = -dv;
        inc = -1;
    }

    int g = 2 * dv - dh;
    int dg1 = 2 * (dv - dh);
    int dg2 = 2 * dv;

    int segment = (dg2 - dg1) / 4;

    while (cx < x1) {

        if (g > 0) {
            g += dg1;
            cy += inc;
        } else {
            g += dg2;
        }

        int quartile = segment ? ((dg2 - g) / segment) : 0;
        if (quartile < 0)
            quartile = 0;
        if (quartile > 3)
            quartile = 3;
        if (inc > 0)
            quartile = 4 - quartile;
        /*
                NOTATION_DEBUG
                    << "x = " << cx << ", y = " << cy
                    << ", g = " << g << ", dg1 = " << dg1 << ", dg2 = " << dg2
                    << ", seg = " << segment << ", q = " << quartile << endl;
        */ 
        // I don't know enough about Qt to be sure of this, but I
        // suspect this may be some of the most inefficient code ever
        // written:

        int off = 0;

        if (m_selected) {
            m_p->painter().setPen(selectedColours[quartile]);
        } else {
            m_p->painter().setPen(colours[quartile]);
        }

        m_p->drawPoint(cx, cy);
        drawBeamsCount ++;

        if (thickness > 1) {
            if (m_selected) {
                m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
            } else {
                m_p->painter().setPen(Qt::black);
            }
        }

        while (++off < thickness) {
            m_p->drawPoint(cx, cy + off);
            drawBeamsCount ++;
        }

        if (m_selected) {
            m_p->painter().setPen(selectedColours[4 - quartile]);
        } else {
            m_p->painter().setPen(colours[4 - quartile]);
        }

        m_p->drawPoint(cx, cy + off);
        drawBeamsCount ++;

        ++cx;
    }

    m_p->painter().setPen(Qt::black);
}

void
NotePixmapFactory::drawBeams(const QPoint &s1,
                             const NotePixmapParameters &params,
                             int beamCount)
{
    clock_t startTime = clock();

    // draw beams: first we draw all the beams common to both ends of
    // the section, then we draw beams for those that appear at the
    // end only

    int startY = m_above + s1.y(), startX = m_left + s1.x();
    int commonBeamCount = std::min(beamCount, params.m_nextBeamCount);

    unsigned int thickness;
    (void)m_font->getBeamThickness(thickness);

    int width = params.m_width;
    double grad = params.m_gradient;
    bool smooth = m_font->isSmooth();
    int spacing = getLineSpacing();

    int sign = (params.m_stemGoesUp ? 1 : -1);

    if (!params.m_stemGoesUp)
        startY -= thickness;

    if (!smooth)
        startY -= sign;
    else if (grad > -0.01 && grad < 0.01)
        startY -= sign;

    if (m_inPrinterMethod) {
        startX += getStemThickness() / 2;
    }

    for (int j = 0; j < commonBeamCount; ++j) {
        int y = sign * j * spacing;
        drawShallowLine(startX, startY + y, startX + width,
                        startY + (int)(width*grad) + y,
                        thickness, smooth);
        drawBeamsBeamCount ++;
    }

    int partWidth = width / 3;
    if (partWidth < 2)
        partWidth = 2;
    else if (partWidth > m_noteBodyWidth)
        partWidth = m_noteBodyWidth;

    if (params.m_thisPartialBeams) {
        for (int j = commonBeamCount; j < beamCount; ++j) {
            int y = sign * j * spacing;
            drawShallowLine(startX, startY + y, startX + partWidth,
                            startY + (int)(partWidth*grad) + y,
                            thickness, smooth);
            drawBeamsBeamCount ++;
        }
    }

    if (params.m_nextPartialBeams) {
        startX += width - partWidth;
        startY += (int)((width - partWidth) * grad);

        for (int j = commonBeamCount; j < params.m_nextBeamCount; ++j) {
            int y = sign * j * spacing;
            drawShallowLine(startX, startY + y, startX + partWidth,
                            startY + (int)(partWidth*grad) + y,
                            thickness, smooth);
            drawBeamsBeamCount ++;
        }
    }

    clock_t endTime = clock();
    drawBeamsTime += (endTime - startTime);
}

void
NotePixmapFactory::drawSlashes(const QPoint &s0,
                               const NotePixmapParameters &params,
                               int slashCount)
{
    unsigned int thickness;
    (void)m_font->getBeamThickness(thickness);
    thickness = thickness * 3 / 4;
    if (thickness < 1)
        thickness = 1;

    int gap = thickness - 1;
    if (gap < 1)
        gap = 1;

    bool smooth = m_font->isSmooth();

    int width = m_noteBodyWidth * 4 / 5;
    int sign = (params.m_stemGoesUp ? -1 : 1);

    int offset =
        (slashCount == 1 ? m_noteBodyHeight * 2 :
         slashCount == 2 ? m_noteBodyHeight * 3 / 2 :
         m_noteBodyHeight);
    int y = m_above + s0.y() + sign * (offset + thickness / 2);

    for (int i = 0; i < slashCount; ++i) {
        int yoff = width / 2;
        drawShallowLine(m_left + s0.x() - width / 2, y + yoff / 2,
                        m_left + s0.x() + width / 2 + getStemThickness(), y - yoff / 2,
                        thickness, smooth);
        y += sign * (thickness + gap);
    }
}

void
NotePixmapFactory::makeRoomForTuplingLine(const NotePixmapParameters &params)
{
    int lineSpacing =
        (int)(params.m_tuplingLineWidth * params.m_tuplingLineGradient);
    int th = m_tupletCountFontMetrics.height();

    if (params.m_tuplingLineY < 0) {

        lineSpacing = -lineSpacing;
        if (lineSpacing < 0)
            lineSpacing = 0;
        m_above = std::max(m_above, -params.m_tuplingLineY + th / 2);
        m_above += lineSpacing + 1;

    } else {

        if (lineSpacing < 0)
            lineSpacing = 0;
        m_below = std::max(m_below, params.m_tuplingLineY + th / 2);
        m_below += lineSpacing + 1;
    }

    m_right = std::max(m_right, params.m_tuplingLineWidth);
}

void
NotePixmapFactory::drawTuplingLine(const NotePixmapParameters &params)
{
    int thickness = getStaffLineThickness() * 3 / 2;
    int countSpace = thickness * 2;

    QString count;
    count.setNum(params.m_tupletCount);
    QRect cr = m_tupletCountFontMetrics.boundingRect(count);

    int tlw = params.m_tuplingLineWidth;
    int indent = m_noteBodyWidth / 2;

    if (tlw < (cr.width() + countSpace * 2 + m_noteBodyWidth * 2)) {
        tlw += m_noteBodyWidth - 1;
        indent = 0;
    }

    int w = (tlw - cr.width()) / 2 - countSpace;

    int startX = m_left + indent;
    int endX = startX + w;

    int startY = params.m_tuplingLineY + m_above + getLineSpacing() / 2;
    int endY = startY + (int)(params.m_tuplingLineGradient * w);

    if (startY == endY)
        ++thickness;

    int tickOffset = getLineSpacing() / 2;
    if (params.m_tuplingLineY >= 0)
        tickOffset = -tickOffset;

    //    NOTATION_DEBUG << "adjusted params.m_tuplingLineWidth = "
    //			 << tlw
    //			 << ", cr.width = " << cr.width()
    //			 << ", tickOffset = " << tickOffset << endl;
    //    NOTATION_DEBUG << "line: (" << startX << "," << startY << ") -> ("
    //			 << endX << "," << endY << ")" << endl;

    bool smooth = m_font->isSmooth();

    if (!params.m_tuplingLineFollowsBeam) {
        m_p->drawLine(startX, startY, startX, startY + tickOffset);
        drawShallowLine(startX, startY, endX, endY, thickness, smooth);
    }

    m_p->painter().setFont(m_tupletCountFont);
    if (!m_inPrinterMethod)
        m_p->maskPainter().setFont(m_tupletCountFont);

    int textX = endX + countSpace;
    int textY = endY + cr.height() / 2;
    //    NOTATION_DEBUG << "text: (" << textX << "," << textY << ")" << endl;

    m_p->drawText(textX, textY, count);

    startX += tlw - w;
    endX = startX + w;

    startY += (int)(params.m_tuplingLineGradient * (tlw - w));
    endY = startY + (int)(params.m_tuplingLineGradient * w);

    //    NOTATION_DEBUG << "line: (" << startX << "," << startY << ") -> ("
    //			 << endX << "," << endY << ")" << endl;

    if (!params.m_tuplingLineFollowsBeam) {
        drawShallowLine(startX, startY, endX, endY, thickness, smooth);
        m_p->drawLine(endX, endY, endX, endY + tickOffset);
    }
}

void
NotePixmapFactory::drawTie(bool above, int length, int shift)
{
#ifdef NASTY_OLD_FLAT_TIE_CODE

    int tieThickness = getStaffLineThickness() * 2;
    int tieCurve = m_font->getSize() * 2 / 3;
    int height = tieCurve + tieThickness;
    int x = m_left + m_noteBodyWidth;
    int y = (above ? m_above - height - tieCurve / 2 :
             m_above + m_noteBodyHeight + tieCurve / 2 + 1);
    int i;

    length -= m_noteBodyWidth;
    if (length < tieCurve * 2)
        length = tieCurve * 2;
    if (length < m_noteBodyWidth * 3) {
        length += m_noteBodyWidth - 2;
        x -= m_noteBodyWidth / 2 - 1;
    }

    for (i = 0; i < tieThickness; ++i) {

        if (above) {

            m_p->drawArc
            (x, y + i, tieCurve*2, tieCurve*2, 90*16, 70*16);

            m_p->drawLine
            (x + tieCurve, y + i, x + length - tieCurve - 2, y + i);

            m_p->drawArc
            (x + length - 2*tieCurve - 1, y + i,
             tieCurve*2, tieCurve*2, 20*16, 70*16);

        } else {

            m_p->drawArc
            (x, y + i - tieCurve, tieCurve*2, tieCurve*2, 200*16, 70*16);

            m_p->drawLine
            (x + tieCurve, y + height - i - 1,
             x + length - tieCurve - 2, y + height - i - 1);

            m_p->drawArc
            (x + length - 2*tieCurve - 1, y + i - tieCurve,
             tieCurve*2, tieCurve*2, 270*16, 70*16);
        }
    }
#else

    int origLength = length;

    int x = m_left + m_noteBodyWidth + m_noteBodyWidth / 4 + shift;
    length = origLength - m_noteBodyWidth - m_noteBodyWidth / 3 - shift;

    // if the length is short, move the tie a bit closer to both notes
    if (length < m_noteBodyWidth*2) {
        x = m_left + m_noteBodyWidth + shift;
        length = origLength - m_noteBodyWidth - shift;
    }

    if (length < m_noteBodyWidth) {
        length = m_noteBodyWidth;
    }

    // We can't request a smooth slur here, because that always involves
    // creating a new pixmap

    QPoint hotspot;
    drawSlurAux(length, 0, above, false, true, false, hotspot,
                &m_p->painter(),
                x,
                above ? m_above : m_above + m_noteBodyHeight);
    //		above ? m_above - m_noteBodyHeight/2 :
    //		        m_above + m_noteBodyHeight + m_noteBodyHeight/2);

#endif
}

QCanvasPixmap*
NotePixmapFactory::makeRestPixmap(const NotePixmapParameters &params)
{
    Profiler profiler("NotePixmapFactory::makeRestPixmap");

    CharName charName(m_style->getRestCharName(params.m_noteType,
                      params.m_restOutsideStave));
    // Check whether the font has the glyph for this charName;
    // if not, substitute a rest-on-stave glyph for a rest-outside-stave glyph,
    // and vice-versa.
    NoteCharacter character;
    if (!getCharacter(charName, character, PlainColour, false))
        charName = m_style->getRestCharName(params.m_noteType,
                                            !params.m_restOutsideStave);

    bool encache = false;

    if (params.m_tupletCount == 0 && !m_selected && !m_shaded &&
            !params.m_restOutsideStave) {

        if (params.m_dots == 0) {
            return getCharacter(charName, PlainColour, false).getCanvasPixmap();
        } else {
            NotePixmapCache::iterator ci(m_dottedRestCache->find(charName));
            if (ci != m_dottedRestCache->end())
                return new QCanvasPixmap
                       (*ci->second, QPoint(ci->second->offsetX(),
                                            ci->second->offsetY()));
            else
                encache = true;
        }
    }

    QPoint hotspot(m_font->getHotspot(charName));
    drawRestAux(params, hotspot, 0, 0, 0);

    QCanvasPixmap* canvasMap = makeCanvasPixmap(hotspot);
    if (encache) {
        m_dottedRestCache->insert(std::pair<CharName, QCanvasPixmap*>
                                  (charName, new QCanvasPixmap
                                   (*canvasMap, hotspot)));
    }
    return canvasMap;
}

void
NotePixmapFactory::drawRest(const NotePixmapParameters &params,
                            QPainter &painter, int x, int y)
{
    Profiler profiler("NotePixmapFactory::drawRest");
    m_inPrinterMethod = true;
    QPoint hotspot; // unused
    drawRestAux(params, hotspot, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::drawRestAux(const NotePixmapParameters &params,
                               QPoint &hotspot, QPainter *painter, int x, int y)
{
    CharName charName(m_style->getRestCharName(params.m_noteType,
                      params.m_restOutsideStave));
    NoteCharacter character = getCharacter(charName,
                                           params.m_quantized ? QuantizedColour :
                                           PlainColour,
                                           false);

    NoteCharacter dot = getCharacter(NoteCharacterNames::DOT, PlainColour, false);

    int dotWidth = dot.getWidth();
    if (dotWidth < getNoteBodyWidth() / 2)
        dotWidth = getNoteBodyWidth() / 2;

    m_above = m_left = 0;
    m_below = dot.getHeight() / 2; // for dotted shallow rests like semibreve
    m_right = dotWidth / 2 + dotWidth * params.m_dots;
    m_noteBodyWidth = character.getWidth();
    m_noteBodyHeight = character.getHeight();

    if (params.m_tupletCount)
        makeRoomForTuplingLine(params);

    // we'll adjust this for tupling line after drawing rest character:
    hotspot = m_font->getHotspot(charName);

    if (params.m_restOutsideStave &&
            (charName == NoteCharacterNames::MULTI_REST ||
             charName == NoteCharacterNames::MULTI_REST_ON_STAFF)) {
        makeRoomForLegerLines(params);
    }
    if (painter) {
        painter->save();
        m_p->beginExternal(painter);
        painter->translate(x - m_left, y - m_above - hotspot.y());
    } else {
        createPixmapAndMask(m_noteBodyWidth + m_left + m_right,
                            m_noteBodyHeight + m_above + m_below);
    }

    m_p->drawNoteCharacter(m_left, m_above, character);

    if (params.m_tupletCount)
        drawTuplingLine(params);

    hotspot.setX(m_left);
    hotspot.setY(m_above + hotspot.y());

    int restY = hotspot.y() - dot.getHeight() - getStaffLineThickness();
    if (params.m_noteType == Note::Semibreve ||
            params.m_noteType == Note::Breve) {
        restY += getLineSpacing();
    }

    for (int i = 0; i < params.m_dots; ++i) {
        int x = m_left + m_noteBodyWidth + i * dotWidth + dotWidth / 2;
        m_p->drawNoteCharacter(x, restY, dot);
    }

    if (params.m_restOutsideStave &&
            (charName == NoteCharacterNames::MULTI_REST ||
             charName == NoteCharacterNames::MULTI_REST_ON_STAFF)) {
        drawLegerLines(params);
    }

    if (painter) {
        painter->restore();
    }
}

QCanvasPixmap*
NotePixmapFactory::makeClefPixmap(const Clef &clef)
{
    Profiler profiler("NotePixmapFactory::makeClefPixmap");
    NoteCharacter plain = getCharacter(m_style->getClefCharName(clef),
                                       PlainColour, false);

    int oct = clef.getOctaveOffset();
    if (oct == 0)
        return plain.getCanvasPixmap();

    QFont defaultOctaveFont(defaultSerifFontFamily);
    KConfig* config = kapp->config();
    config->setGroup(NotationViewConfigGroup);
    QFont octaveFont = config->readFontEntry("textfont", &defaultOctaveFont);
    octaveFont.setPixelSize(getLineSpacing() * 3 / 2);
    QFontMetrics octaveFontMetrics(octaveFont);

    // fix #1522784 and use 15 rather than 16 for double octave offset
    int adjustedOctave = (8 * (oct < 0 ? -oct : oct));
    if (adjustedOctave > 8)
        adjustedOctave--;
    else if (adjustedOctave < 8)
        adjustedOctave++;

    QString text = QString("%1").arg(adjustedOctave);
    QRect rect = octaveFontMetrics.boundingRect(text);

    createPixmapAndMask(plain.getWidth(),
                        plain.getHeight() + rect.height());

    if (m_selected) {
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
    }

    m_p->drawNoteCharacter(0, oct < 0 ? 0 : rect.height(), plain);

    m_p->painter().setFont(octaveFont);
    if (!m_inPrinterMethod)
        m_p->maskPainter().setFont(octaveFont);

    m_p->drawText(plain.getWidth() / 2 - rect.width() / 2,
                  oct < 0 ? plain.getHeight() + rect.height() - 1 :
                  rect.height(), text);

    m_p->painter().setPen(Qt::black);
    QPoint hotspot(plain.getHotspot());
    if (oct > 0) hotspot.setY(hotspot.y() + rect.height());
    return makeCanvasPixmap(hotspot, true);
}

QCanvasPixmap*
NotePixmapFactory::makePedalDownPixmap()
{
    return getCharacter(NoteCharacterNames::PEDAL_MARK, PlainColour, false)
           .getCanvasPixmap();
}

QCanvasPixmap*
NotePixmapFactory::makePedalUpPixmap()
{
    return getCharacter(NoteCharacterNames::PEDAL_UP_MARK, PlainColour, false)
           .getCanvasPixmap();
}

QCanvasPixmap*
NotePixmapFactory::makeUnknownPixmap()
{
    Profiler profiler("NotePixmapFactory::makeUnknownPixmap");
    return getCharacter(NoteCharacterNames::UNKNOWN, PlainColour, false)
           .getCanvasPixmap();
}

QCanvasPixmap*
NotePixmapFactory::makeToolbarPixmap(const char *name, bool menuSize)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QString fileBase = pixmapDir + "/toolbar/";
    if (menuSize) fileBase += "menu-";
    fileBase += name;
    if (QFile(fileBase + ".png").exists()) {
        return new QCanvasPixmap(fileBase + ".png");
    } else if (QFile(fileBase + ".xpm").exists()) {
        return new QCanvasPixmap(fileBase + ".xpm");
    } else if (menuSize) {
        return makeToolbarPixmap(name, false);
    } else {
        // this will fail, but we don't want to return a null pointer
        return new QCanvasPixmap(fileBase + ".png");
    }
}

QCanvasPixmap*
NotePixmapFactory::makeNoteMenuPixmap(timeT duration,
                                      timeT &errorReturn)
{
    Note nearestNote = Note::getNearestNote(duration);
    bool triplet = false;
    errorReturn = 0;

    if (nearestNote.getDuration() != duration) {
        Note tripletNote = Note::getNearestNote(duration * 3 / 2);
        if (tripletNote.getDuration() == duration * 3 / 2) {
            nearestNote = tripletNote;
            triplet = true;
        } else {
            errorReturn = duration - nearestNote.getDuration();
        }
    }

    QString noteName = NotationStrings::getReferenceName(nearestNote);
    if (triplet)
        noteName = "3-" + noteName;
    noteName = "menu-" + noteName;
    return makeToolbarPixmap(noteName);
}

QCanvasPixmap *
NotePixmapFactory::makeMarkMenuPixmap(Mark mark)
{
    if (mark == Marks::Sforzando ||
            mark == Marks::Rinforzando) {
        return makeToolbarPixmap(mark.c_str());
    } else {
        NoteFont *font = 0;
        try {
            font = NoteFontFactory::getFont
                   (NoteFontFactory::getDefaultFontName(), 6);
        } catch (Exception) {
            font = NoteFontFactory::getFont
                   (NoteFontFactory::getDefaultFontName(),
                    NoteFontFactory::getDefaultSize(NoteFontFactory::getDefaultFontName()));
        }
        NoteCharacter character = font->getCharacter
                                  (NoteStyleFactory::getStyle(NoteStyleFactory::DefaultStyle)->
                                   getMarkCharName(mark));
        return character.getCanvasPixmap();
    }
}

QCanvasPixmap*
NotePixmapFactory::makeKeyPixmap(const Key &key,
                                 const Clef &clef,
                                 Key previousKey)
{
    Profiler profiler("NotePixmapFactory::makeKeyPixmap");

    std::vector<int> ah0 = previousKey.getAccidentalHeights(clef);
    std::vector<int> ah1 = key.getAccidentalHeights(clef);

    int cancelCount = 0;
    if (key.isSharp() != previousKey.isSharp())
        cancelCount = ah0.size();
    else if (ah1.size() < ah0.size())
        cancelCount = ah0.size() - ah1.size();

    CharName keyCharName;
    if (key.isSharp())
        keyCharName = NoteCharacterNames::SHARP;
    else
        keyCharName = NoteCharacterNames::FLAT;

    NoteCharacter keyCharacter;
    NoteCharacter cancelCharacter;

    keyCharacter = getCharacter(keyCharName, PlainColour, false);
    if (cancelCount > 0) {
        cancelCharacter = getCharacter(NoteCharacterNames::NATURAL, PlainColour, false);
    }

    int x = 0;
    int lw = getLineSpacing();
    int keyDelta = keyCharacter.getWidth() - keyCharacter.getHotspot().x();

    int cancelDelta = 0;
    int between = 0;
    if (cancelCount > 0) {
        cancelDelta = cancelCharacter.getWidth() + cancelCharacter.getWidth() / 3;
        between = cancelCharacter.getWidth();
    }

    createPixmapAndMask(keyDelta * ah1.size() + cancelDelta * cancelCount + between +
                        keyCharacter.getWidth() / 4, lw * 8 + 1);

    if (key.isSharp() != previousKey.isSharp()) {

        // cancellation first

        for (int i = 0; i < cancelCount; ++i) {

            int h = ah0[ah0.size() - cancelCount + i];
            int y = (lw * 2) + ((8 - h) * lw) / 2 - cancelCharacter.getHotspot().y();

            m_p->drawNoteCharacter(x, y, cancelCharacter);

            x += cancelDelta;
        }

        if (cancelCount > 0) {
            x += between;
        }
    }

    for (unsigned int i = 0; i < ah1.size(); ++i) {

        int h = ah1[i];
        int y = (lw * 2) + ((8 - h) * lw) / 2 - keyCharacter.getHotspot().y();

        m_p->drawNoteCharacter(x, y, keyCharacter);

        x += keyDelta;
    }

    if (key.isSharp() == previousKey.isSharp()) {

        // cancellation afterwards

        if (cancelCount > 0) {
            x += between;
        }

        for (int i = 0; i < cancelCount; ++i) {

            int h = ah0[ah0.size() - cancelCount + i];
            int y = (lw * 2) + ((8 - h) * lw) / 2 - cancelCharacter.getHotspot().y();

            m_p->drawNoteCharacter(x, y, cancelCharacter);

            x += cancelDelta;
        }
    }

    return makeCanvasPixmap(m_pointZero);
}

QCanvasPixmap*
NotePixmapFactory::makeClefDisplayPixmap(const Clef &clef)
{
    QCanvasPixmap* clefPixmap = makeClefPixmap(clef);

    int lw = getLineSpacing();
    int width = clefPixmap->width() + 6 * getNoteBodyWidth();

    createPixmapAndMask(width, lw * 10 + 1);

    int h = clef.getAxisHeight();
    int y = (lw * 3) + ((8 - h) * lw) / 2;
    int x = 3 * getNoteBodyWidth();
    m_p->drawPixmap(x, y - clefPixmap->offsetY(), *clefPixmap);

    for (h = 0; h <= 8; h += 2) {
        y = (lw * 3) + ((8 - h) * lw) / 2;
        m_p->drawLine(x / 2, y, m_generatedWidth - x / 2 - 1, y);
    }

    delete clefPixmap;

    return makeCanvasPixmap(m_pointZero);
}

QCanvasPixmap*
NotePixmapFactory::makeKeyDisplayPixmap(const Key &key, const Clef &clef)
{
    std::vector<int> ah = key.getAccidentalHeights(clef);

    CharName charName = (key.isSharp() ?
                         NoteCharacterNames::SHARP :
                         NoteCharacterNames::FLAT);

    QCanvasPixmap* clefPixmap = makeClefPixmap(clef);
    QPixmap accidentalPixmap(*m_font->getCharacter(charName).getPixmap());
    QPoint hotspot(m_font->getHotspot(charName));

    int lw = getLineSpacing();
    int delta = accidentalPixmap.width() - hotspot.x();
    int maxDelta = getAccidentalWidth(Sharp);
    int width = clefPixmap->width() + 5 * maxDelta + 7 * maxDelta;
    int x = clefPixmap->width() + 5 * maxDelta / 2;

    createPixmapAndMask(width, lw * 10 + 1);

    int h = clef.getAxisHeight();
    int y = (lw * 3) + ((8 - h) * lw) / 2;
    m_p->drawPixmap(2 * maxDelta, y - clefPixmap->offsetY(), *clefPixmap);

    for (unsigned int i = 0; i < ah.size(); ++i) {

        h = ah[i];
        y = (lw * 3) + ((8 - h) * lw) / 2 - hotspot.y();

        m_p->drawPixmap(x, y, accidentalPixmap);

        x += delta;
    }

    for (h = 0; h <= 8; h += 2) {
        y = (lw * 3) + ((8 - h) * lw) / 2;
        m_p->drawLine(maxDelta, y, m_generatedWidth - 2*maxDelta - 1, y);
    }

    delete clefPixmap;
    return makeCanvasPixmap(m_pointZero);
}

QCanvasPixmap*
NotePixmapFactory::makeTrackHeaderPixmap(int height,
        const Key &key, const Clef &clef, QColor clefColour, bool drawClef,
        const QString &upperText, QColor upperTextColour,
        const QString &lowerText, QColor lowerTextColour
        )
{
    height -= 4;    // Make place to label frame :
                    // 4 = 2 * (margin + lineWidth)

    // Get widget default common character size
    // ("X" stands here for a "common character")
    QRect bounds = m_trackHeaderFontMetrics.boundingRect(i18n("X"));
    int charHeight = bounds.height();
    int charWidth = bounds.width();

    // Minimum width of a string displayed as upper or lower text
    int maxTextAllowedWidth = 20 * charWidth;

    QCanvasPixmap* clefAndKeyPixmap = NULL;
    clefAndKeyPixmap = makeKeyDisplayPixmap(key, clef);
    int clefAndKeyWidth = clefAndKeyPixmap->width();
    int clefAndKeyHeight = clefAndKeyPixmap->height();

    int width = maxTextAllowedWidth > clefAndKeyWidth ?
                            maxTextAllowedWidth : clefAndKeyWidth;

    createPixmapAndMask(width, height);

    int clefAndKeyY = (height - clefAndKeyHeight) / 2;
    int clefAndKeyX = width - clefAndKeyWidth;
    if (drawClef) {
        if (clefColour != Qt::black) clefAndKeyPixmap->fill(clefColour);
        m_p->drawPixmap(clefAndKeyX, clefAndKeyY, *clefAndKeyPixmap);
    }

    int upperTextY, lowerTextY;
    if (charHeight < clefAndKeyY) {
        // If enough space, place text just outside clef pixmap
        upperTextY = clefAndKeyY - charHeight + 4;  // +4 : adjust
        lowerTextY = clefAndKeyY + clefAndKeyHeight + charHeight;
    } else {
        // Else use top and bottom positions
        upperTextY = charHeight;
        lowerTextY = m_generatedHeight - 4;  // -4 : adjust
    }

    m_p->painter().setFont(m_trackHeaderFont);
    if (!m_inPrinterMethod)
        m_p->maskPainter().setFont(m_trackHeaderFont);

    m_p->painter().setPen(upperTextColour);
    m_p->drawText(charWidth, upperTextY, upperText);

    m_p->painter().setPen(lowerTextColour);
    m_p->drawText(charWidth, lowerTextY, lowerText);

    delete clefAndKeyPixmap;
    return makeCanvasPixmap(m_pointZero, true);
}

QCanvasPixmap*
NotePixmapFactory::makePitchDisplayPixmap(int p, const Clef &clef,
        bool useSharps)
{
    NotationRules rules;

    Pitch pitch(p);
    Accidental accidental(pitch.getAccidental(useSharps));
    NotePixmapParameters params(Note::Crotchet, 0, accidental);

    QCanvasPixmap* clefPixmap = makeClefPixmap(clef);

    int lw = getLineSpacing();
    int width = getClefWidth(Clef::Bass) + 10 * getNoteBodyWidth();

    int h = pitch.getHeightOnStaff(clef, useSharps);
    params.setStemGoesUp(rules.isStemUp(h));

    if (h < -1)
        params.setStemLength(lw * (4 - h) / 2);
    else if (h > 9)
        params.setStemLength(lw * (h - 4) / 2);
    if (h > 8)
        params.setLegerLines(h - 8);
    else if (h < 0)
        params.setLegerLines(h);

    params.setIsOnLine(h % 2 == 0);
    params.setSelected(m_selected);

    QCanvasPixmap *notePixmap = makeNotePixmap(params);

    int pixmapHeight = lw * 12 + 1;
    int yoffset = lw * 3;
    if (h > 12) {
        pixmapHeight += 6 * lw;
        yoffset += 6 * lw;
    } else if (h < -4) {
        pixmapHeight += 6 * lw;
    }

    createPixmapAndMask(width, pixmapHeight);

    int x =
        getClefWidth(Clef::Bass) + 5 * getNoteBodyWidth() -
        getAccidentalWidth(accidental);
    int y = yoffset + ((8 - h) * lw) / 2 - notePixmap->offsetY();
    m_p->drawPixmap(x, y, *notePixmap);

    h = clef.getAxisHeight();
    x = 3 * getNoteBodyWidth();
    y = yoffset + ((8 - h) * lw) / 2;
    m_p->drawPixmap(x, y - clefPixmap->offsetY(), *clefPixmap);

    for (h = 0; h <= 8; h += 2) {
        y = yoffset + ((8 - h) * lw) / 2;
        m_p->drawLine(x / 2, y, m_generatedWidth - x / 2, y);
    }

    delete clefPixmap;
    delete notePixmap;

    return makeCanvasPixmap(m_pointZero);
}

QCanvasPixmap*
NotePixmapFactory::makePitchDisplayPixmap(int p, const Clef &clef,
        int octave, int step)
{
    NotationRules rules;

    Pitch pitch(step, octave, p, 0);
    Accidental accidental = pitch.getDisplayAccidental(Key("C major"));
    NotePixmapParameters params(Note::Crotchet, 0, accidental);

    QCanvasPixmap* clefPixmap = makeClefPixmap(clef);

    int lw = getLineSpacing();
    int width = getClefWidth(Clef::Bass) + 10 * getNoteBodyWidth();

    int h = pitch.getHeightOnStaff
            (clef,
             Key("C major"));
    params.setStemGoesUp(rules.isStemUp(h));

    if (h < -1)
        params.setStemLength(lw * (4 - h) / 2);
    else if (h > 9)
        params.setStemLength(lw * (h - 4) / 2);
    if (h > 8)
        params.setLegerLines(h - 8);
    else if (h < 0)
        params.setLegerLines(h);

    params.setIsOnLine(h % 2 == 0);
    params.setSelected(m_selected);

    QCanvasPixmap *notePixmap = makeNotePixmap(params);

    int pixmapHeight = lw * 12 + 1;
    int yoffset = lw * 3;
    if (h > 12) {
        pixmapHeight += 6 * lw;
        yoffset += 6 * lw;
    } else if (h < -4) {
        pixmapHeight += 6 * lw;
    }

    createPixmapAndMask(width, pixmapHeight);

    int x =
        getClefWidth(Clef::Bass) + 5 * getNoteBodyWidth() -
        getAccidentalWidth(accidental);
    int y = yoffset + ((8 - h) * lw) / 2 - notePixmap->offsetY();
    m_p->drawPixmap(x, y, *notePixmap);

    h = clef.getAxisHeight();
    x = 3 * getNoteBodyWidth();
    y = yoffset + ((8 - h) * lw) / 2;
    m_p->drawPixmap(x, y - clefPixmap->offsetY(), *clefPixmap);

    for (h = 0; h <= 8; h += 2) {
        y = yoffset + ((8 - h) * lw) / 2;
        m_p->drawLine(x / 2, y, m_generatedWidth - x / 2, y);
    }

    delete clefPixmap;
    delete notePixmap;

    return makeCanvasPixmap(m_pointZero);
}

QCanvasPixmap*
NotePixmapFactory::makeHairpinPixmap(int length, bool isCrescendo)
{
    Profiler profiler("NotePixmapFactory::makeHairpinPixmap");
    drawHairpinAux(length, isCrescendo, 0, 0, 0);
    return makeCanvasPixmap(QPoint(0, m_generatedHeight / 2));
}

void
NotePixmapFactory::drawHairpin(int length, bool isCrescendo,
                               QPainter &painter, int x, int y)
{
    Profiler profiler("NotePixmapFactory::drawHairpin");
    m_inPrinterMethod = true;
    drawHairpinAux(length, isCrescendo, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::drawHairpinAux(int length, bool isCrescendo,
                                  QPainter *painter, int x, int y)
{
    int nbh = getNoteBodyHeight();
    int nbw = getNoteBodyWidth();

    int height = (int)(((double)nbh / (double)(nbw * 40)) * length) + nbh;
    int thickness = getStaffLineThickness() * 3 / 2;

    //    NOTATION_DEBUG << "NotePixmapFactory::makeHairpinPixmap: mapped length " << length << " to height " << height << " (nbh = " << nbh << ", nbw = " << nbw << ")" << endl;

    if (height < nbh)
        height = nbh;
    if (height > nbh*2)
        height = nbh * 2;

    height += thickness - 1;

    if (painter) {
        painter->save();
        m_p->beginExternal(painter);
        painter->translate(x, y - height / 2);
    } else {
        createPixmapAndMask(length, height);
    }

    if (m_selected) {
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
    }

    int left = 1, right = length - 2 * nbw / 3 + 1;

    bool smooth = m_font->isSmooth();

    if (isCrescendo) {
        drawShallowLine(left, height / 2 - 1,
                        right, height - thickness - 1, thickness, smooth);
        drawShallowLine(left, height / 2 - 1, right, 0, thickness, smooth);
    } else {
        drawShallowLine(left, 0, right, height / 2 - 1, thickness, smooth);
        drawShallowLine(left, height - thickness - 1,
                        right, height / 2 - 1, thickness, smooth);
    }

    m_p->painter().setPen(Qt::black);

    if (painter) {
        painter->restore();
    }
}

QCanvasPixmap*
NotePixmapFactory::makeSlurPixmap(int length, int dy, bool above, bool phrasing)
{
    Profiler profiler("NotePixmapFactory::makeSlurPixmap");

    //!!! could remove "height > 5" requirement if we did a better job of
    // sizing so that any horizontal part was rescaled down to exactly
    // 1 pixel wide instead of blurring
    bool smooth = m_font->isSmooth() && getNoteBodyHeight() > 5;
    QPoint hotspot;
    if (length < getNoteBodyWidth()*2)
        length = getNoteBodyWidth() * 2;
    drawSlurAux(length, dy, above, smooth, false, phrasing, hotspot, 0, 0, 0);

    m_p->end();

    if (smooth) {

        QImage i = m_generatedPixmap->convertToImage();
        if (i.depth() == 1)
            i = i.convertDepth(32);
        i = i.smoothScale(i.width() / 2, i.height() / 2);

        delete m_generatedPixmap;
        delete m_generatedMask;
        QPixmap newPixmap(i);
        QCanvasPixmap *p = new QCanvasPixmap(newPixmap, hotspot);
        p->setMask(PixmapFunctions::generateMask(newPixmap,
                   Qt::white.rgb()));
        return p;

    } else {

        QCanvasPixmap *p = new QCanvasPixmap(*m_generatedPixmap, hotspot);
        p->setMask(PixmapFunctions::generateMask(*m_generatedPixmap,
                   Qt::white.rgb()));
        delete m_generatedPixmap;
        delete m_generatedMask;
        return p;
    }
}

void
NotePixmapFactory::drawSlur(int length, int dy, bool above, bool phrasing,
                            QPainter &painter, int x, int y)
{
    Profiler profiler("NotePixmapFactory::drawSlur");
    QPoint hotspot;
    m_inPrinterMethod = true;
    if (length < getNoteBodyWidth()*2)
        length = getNoteBodyWidth() * 2;
    drawSlurAux(length, dy, above, false, false, phrasing, hotspot, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::drawSlurAux(int length, int dy, bool above,
                               bool smooth, bool flat, bool phrasing,
                               QPoint &hotspot, QPainter *painter, int x, int y)
{
    QWMatrix::TransformationMode mode = QWMatrix::transformationMode();
    QWMatrix::setTransformationMode(QWMatrix::Points);

    int thickness = getStaffLineThickness() * 2;
    if (phrasing)
        thickness = thickness * 3 / 4;
    int nbh = getNoteBodyHeight(), nbw = getNoteBodyWidth();

    // Experiment with rotating the painter rather than the control points.
    double theta = 0;
    bool rotate = false;
    if (dy != 0) {
        // We have opposite (dy) and adjacent (length).
        theta = atan(double(dy) / double(length)) * 180.0 / M_PI;
        //	NOTATION_DEBUG << "slur: dy is " << dy << ", length " << length << ", rotating through " << theta << endl;
        rotate = true;
    }

    // draw normal slur for very slopey phrasing slur:
    if (theta < -5 || theta > 5)
        phrasing = false;

    int y0 = 0, my = 0;

    float noteLengths = float(length) / nbw;
    if (noteLengths < 1)
        noteLengths = 1;

    my = int(0 - nbh * sqrt(noteLengths) / 2);
    if (flat)
        my = my * 2 / 3;
    else if (phrasing)
        my = my * 3 / 4;
    if (!above)
        my = -my;

    bool havePixmap = false;
    QPoint topLeft, bottomRight;

    if (smooth)
        thickness += 2;

    for (int i = 0; i < thickness; ++i) {

        Spline::PointList pl;

        if (!phrasing) {
            pl.push_back(QPoint(length / 6, my));
            pl.push_back(QPoint(length - length / 6, my));
        } else {
            pl.push_back(QPoint(abs(my) / 4, my / 3));
            pl.push_back(QPoint(length / 6, my));

            if (theta > 1) {
                pl.push_back(QPoint(length * 3 / 8, my * 3 / 2));
            } else if (theta < -1) {
                pl.push_back(QPoint(length * 5 / 8, my * 3 / 2));
            } else {
                pl.push_back(QPoint(length / 2, my * 4 / 3));
            }

            pl.push_back(QPoint(length - length / 6, my));
            pl.push_back(QPoint(length - abs(my) / 4, my / 3));
        }

        Spline::PointList *polyPoints = Spline::calculate
                                        (QPoint(0, y0), QPoint(length - 1, y0), pl, topLeft, bottomRight);

        if (!havePixmap) {
            int width = bottomRight.x() - topLeft.x();
            int height = bottomRight.y() - topLeft.y() + thickness - 1 + abs(dy);
            hotspot = QPoint(0, -topLeft.y() + (dy < 0 ? -dy : 0));

            //	    NOTATION_DEBUG << "slur: bottomRight (" << bottomRight.x() << "," << bottomRight.y() << "), topLeft (" << topLeft.x() << "," << topLeft.y() << "), width " << width << ", height " << height << ", hotspot (" << hotspot.x() << "," << hotspot.y() << "), dy " << dy << ", thickness " << thickness << endl;

            if (painter) {

                // This conditional is because we're also called with
                // a painter arg from non-printer drawTie.  It's a big
                // hack.

                if (m_inPrinterMethod) {
                    painter->save();
                    m_p->beginExternal(painter);
                    painter->translate(x, y);
                    if (rotate)
                        painter->rotate(theta);
                } else {
                    m_p->painter().save();
                    m_p->maskPainter().save();
                    m_p->painter().translate(x, y);
                    m_p->maskPainter().translate(x, y);
                    if (rotate) {
                        m_p->painter().rotate(theta);
                        m_p->maskPainter().rotate(theta);
                    }
                }

            } else {
                createPixmapAndMask(smooth ? width*2 + 1 : width,
                                    smooth ? height*2 + thickness*2 : height + thickness,
                                    width, height);

                QWMatrix m;
                if (smooth)
                    m.translate(2 * hotspot.x(), 2 * hotspot.y());
                else
                    m.translate(hotspot.x(), hotspot.y());
                m.rotate(theta);
                m_p->painter().setWorldMatrix(m);
                m_p->maskPainter().setWorldMatrix(m);
            }

            if (m_selected)
                m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
            else if (m_shaded) {
                m_p->painter().setPen(Qt::gray);
            }
            havePixmap = true;
        }
        /*
        	for (int j = 0; j < pl.size(); ++j) {
        	    if (smooth) {
        		m_p->drawPoint(pl[j].x()*2, pl[j].y()*2);
        	    } else {
        		m_p->drawPoint(pl[j].x(), pl[j].y());
        	    }
        	}
        */
        int ppc = polyPoints->size();
        QPointArray qp(ppc);

        for (int j = 0; j < ppc; ++j) {
            qp.setPoint(j, (*polyPoints)[j].x(), (*polyPoints)[j].y());
        }

        delete polyPoints;

        if (!smooth || (i > 0 && i < thickness - 1)) {
            if (smooth) {
                for (int j = 0; j < ppc; ++j) {
                    qp.setPoint(j, qp.point(j).x()*2, qp.point(j).y()*2);
                }
                m_p->drawPolyline(qp);
                for (int j = 0; j < ppc; ++j) {
                    qp.setPoint(j, qp.point(j).x(), qp.point(j).y() + 1);
                }
                m_p->drawPolyline(qp);
            } else {
                m_p->drawPolyline(qp);
            }
        }

        if (above) {
            ++my;
            if (i % 2)
                ++y0;
        } else {
            --my;
            if (i % 2)
                --y0;
        }
    }

    if (m_selected) {
        m_p->painter().setPen(Qt::black);
    }

    QWMatrix::setTransformationMode(mode);

    if (painter) {
        painter->restore();
        if (!m_inPrinterMethod)
            m_p->maskPainter().restore();
    }
}

QCanvasPixmap*
NotePixmapFactory::makeOttavaPixmap(int length, int octavesUp)
{
    Profiler profiler("NotePixmapFactory::makeOttavaPixmap");
    m_inPrinterMethod = false;
    drawOttavaAux(length, octavesUp, 0, 0, 0);
    return makeCanvasPixmap(QPoint(0, m_generatedHeight - 1));
}

void
NotePixmapFactory::drawOttava(int length, int octavesUp,
                              QPainter &painter, int x, int y)
{
    Profiler profiler("NotePixmapFactory::drawOttava");
    m_inPrinterMethod = true;
    drawOttavaAux(length, octavesUp, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::drawOttavaAux(int length, int octavesUp,
                                 QPainter *painter, int x, int y)
{
    int height = m_ottavaFontMetrics.height();
    int backpedal = 0;
    QString label;
    QRect r;

    if (octavesUp == 2 || octavesUp == -2) {
        label = "15ma  ";
        backpedal = m_ottavaFontMetrics.width("15") / 2;
    } else {
        label = "8va  ";
        backpedal = m_ottavaFontMetrics.width("8") / 2;
    }

    int width = length + backpedal;

    if (painter) {
        painter->save();
        m_p->beginExternal(painter);
        painter->translate(x - backpedal, y - height);
    } else {
        NOTATION_DEBUG << "NotePixmapFactory::drawOttavaAux: making pixmap and mask " << width << "x" << height << endl;
        createPixmapAndMask(width, height);
    }

    int thickness = getStemThickness();
    QPen pen(Qt::black, thickness, Qt::DotLine);

    if (m_selected) {
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
        pen.setColor(GUIPalette::getColour(GUIPalette::SelectedElement));
    } else if (m_shaded) {
        m_p->painter().setPen(Qt::gray);
        pen.setColor(Qt::gray);
    }

    m_p->painter().setFont(m_ottavaFont);
    if (!m_inPrinterMethod)
        m_p->maskPainter().setFont(m_ottavaFont);

    m_p->drawText(0, m_ottavaFontMetrics.ascent(), label);

    m_p->painter().setPen(pen);
    //    if (!m_inPrinterMethod) m_p->maskPainter().setPen(pen);

    int x0 = m_ottavaFontMetrics.width(label) + thickness;
    int x1 = width - thickness;
    int y0 = m_ottavaFontMetrics.ascent() * 2 / 3 - thickness / 2;
    int y1 = (octavesUp < 0 ? 0 : m_ottavaFontMetrics.ascent());

    NOTATION_DEBUG << "NotePixmapFactory::drawOttavaAux: drawing " << x0 << "," << y0 << " to " << x1 << "," << y0 << ", thickness " << thickness << endl;

    m_p->drawLine(x0, y0, x1, y0);

    pen.setStyle(Qt::SolidLine);
    m_p->painter().setPen(pen);
    //    if (!m_inPrinterMethod) m_p->maskPainter().setPen(pen);

    NOTATION_DEBUG << "NotePixmapFactory::drawOttavaAux: drawing " << x1 << "," << y0 << " to " << x1 << "," << y1 << ", thickness " << thickness << endl;

    m_p->drawLine(x1, y0, x1, y1);

    m_p->painter().setPen(QPen());
    if (!m_inPrinterMethod)
        m_p->maskPainter().setPen(QPen());

    if (painter) {
        painter->restore();
    }
}

void
NotePixmapFactory::drawBracket(int length, bool left, bool curly, int x, int y)
{
    // curly mode not yet implemented

    int thickness = getStemThickness() * 2;

    int m1 = length / 6;
    int m2 = length - length / 6 - 1;

    int off0 = 0, moff = 0;

    int nbh = getNoteBodyHeight(), nbw = getNoteBodyWidth();
    float noteLengths = float(length) / nbw;
    if (noteLengths < 1)
        noteLengths = 1;
    moff = int(nbh * sqrt(noteLengths) / 2);
    moff = moff * 2 / 3;

    if (left)
        moff = -moff;

    QPoint topLeft, bottomRight;

    for (int i = 0; i < thickness; ++i) {

        Spline::PointList pl;
        pl.push_back(QPoint((int)moff, m1));
        pl.push_back(QPoint((int)moff, m2));
        /*
        	NOTATION_DEBUG << "bracket spline controls: " << moff << "," << m1
        		       << ", " << moff << "," << m2 << "; end points "
        		       << off0 << ",0, " << off0 << "," << length-1
        		       << endl;
        */
        Spline::PointList *polyPoints = Spline::calculate
                                        (QPoint(off0, 0), QPoint(off0, length - 1), pl, topLeft, bottomRight);

        int ppc = polyPoints->size();
        QPointArray qp(ppc);
        /*
        	NOTATION_DEBUG << "bracket spline polypoints: " << endl;
        	for (int j = 0; j < ppc; ++j) {
        	    NOTATION_DEBUG << (*polyPoints)[j].x() << "," << (*polyPoints)[j].y() << endl;
        	}
        */

        for (int j = 0; j < ppc; ++j) {
            qp.setPoint(j, x + (*polyPoints)[j].x(), y + (*polyPoints)[j].y());
        }

        delete polyPoints;

        m_p->drawPolyline(qp);

        if (!left) {
            ++moff;
            if (i % 2)
                ++off0;
        } else {
            --moff;
            if (i % 2)
                --off0;
        }
    }
}

QCanvasPixmap*
NotePixmapFactory::makeTimeSigPixmap(const TimeSignature& sig)
{
    Profiler profiler("NotePixmapFactory::makeTimeSigPixmap");

    if (sig.isCommon()) {

        NoteCharacter character;

        CharName charName;
        if (sig.getNumerator() == 2) {
            charName = NoteCharacterNames::CUT_TIME;
        } else {
            charName = NoteCharacterNames::COMMON_TIME;
        }

        if (getCharacter(charName, character, PlainColour, false)) {
            createPixmapAndMask(character.getWidth(), character.getHeight());
            m_p->drawNoteCharacter(0, 0, character);
            return makeCanvasPixmap(QPoint(0, character.getHeight() / 2));
        }

        QString c("c");
        QRect r = m_bigTimeSigFontMetrics.boundingRect(c);

        int dy = getLineSpacing() / 4;
        createPixmapAndMask(r.width(), r.height() + dy*2);

        if (m_selected) {
            m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
        } else if (m_shaded) {
            m_p->painter().setPen(Qt::gray);
        }

        m_p->painter().setFont(m_bigTimeSigFont);
        if (!m_inPrinterMethod)
            m_p->maskPainter().setFont(m_bigTimeSigFont);

        m_p->drawText(0, r.height() + dy, c);

        if (sig.getNumerator() == 2) { // cut common

            int x = r.width() * 3 / 5 - getStemThickness();

            for (int i = 0; i < getStemThickness() * 2; ++i, ++x) {
                m_p->drawLine(x, 0, x, r.height() + dy*2 - 1);
            }
        }

        m_p->painter().setPen(Qt::black);
        return makeCanvasPixmap(QPoint(0, r.height() / 2 + dy));

    } else {

        int numerator = sig.getNumerator(),
                        denominator = sig.getDenominator();

        QString numS, denomS;

        numS.setNum(numerator);
        denomS.setNum(denominator);

        NoteCharacter character;
        if (getCharacter(m_style->getTimeSignatureDigitName(0), character,
                         PlainColour, false)) {

            // if the 0 digit exists, we assume 1-9 also all exist
            // and all have the same width

            int numW = character.getWidth() * numS.length();
            int denomW = character.getWidth() * denomS.length();

            int width = std::max(numW, denomW);
            int height = getLineSpacing() * 4 - getStaffLineThickness();

            createPixmapAndMask(width, height);

            for (unsigned int i = 0; i < numS.length(); ++i) {
                int x = width - (width - numW) / 2 - (i + 1) * character.getWidth();
                int y = height / 4 - (character.getHeight() / 2);
                NoteCharacter charCharacter = getCharacter
                                              (m_style->getTimeSignatureDigitName(numerator % 10),
                                               PlainColour, false);
                m_p->drawNoteCharacter(x, y, charCharacter);
                numerator /= 10;
            }

            for (unsigned int i = 0; i < denomS.length(); ++i) {
                int x = width - (width - denomW) / 2 - (i + 1) * character.getWidth();
                int y = height - height / 4 - (character.getHeight() / 2);
                NoteCharacter charCharacter = getCharacter
                                              (m_style->getTimeSignatureDigitName(denominator % 10),
                                               PlainColour, false);
                m_p->drawNoteCharacter(x, y, charCharacter);
                denominator /= 10;
            }

            return makeCanvasPixmap(QPoint(0, height / 2));
        }

        QRect numR = m_timeSigFontMetrics.boundingRect(numS);
        QRect denomR = m_timeSigFontMetrics.boundingRect(denomS);
        int width = std::max(numR.width(), denomR.width()) + 2;
        int x;

        createPixmapAndMask(width, denomR.height() * 2 + getNoteBodyHeight());

        if (m_selected) {
            m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
        } else if (m_shaded) {
            m_p->painter().setPen(Qt::gray);
        }

        m_p->painter().setFont(m_timeSigFont);
        if (!m_inPrinterMethod)
            m_p->maskPainter().setFont(m_timeSigFont);

        x = (width - numR.width()) / 2 - 1;
        m_p->drawText(x, denomR.height(), numS);

        x = (width - denomR.width()) / 2 - 1;
        m_p->drawText(x, denomR.height() * 2 + (getNoteBodyHeight() / 2) - 1, denomS);

        m_p->painter().setPen(Qt::black);

        return makeCanvasPixmap(QPoint(0, denomR.height() +
                                       (getNoteBodyHeight() / 4) - 1),
                                true);
    }
}

int NotePixmapFactory::getTimeSigWidth(const TimeSignature &sig) const
{
    if (sig.isCommon()) {

        QRect r(m_bigTimeSigFontMetrics.boundingRect("c"));
        return r.width() + 2;

    } else {

        int numerator = sig.getNumerator(),
                        denominator = sig.getDenominator();

        QString numS, denomS;

        numS.setNum(numerator);
        denomS.setNum(denominator);

        QRect numR = m_timeSigFontMetrics.boundingRect(numS);
        QRect denomR = m_timeSigFontMetrics.boundingRect(denomS);
        int width = std::max(numR.width(), denomR.width()) + 2;

        return width;
    }
}

QFont
NotePixmapFactory::getTextFont(const Text &text) const
{
    std::string type(text.getTextType());
    TextFontCache::iterator i = m_textFontCache.find(type.c_str());
    if (i != m_textFontCache.end())
        return i->second;

    /*
     * Text types:
     *
     * UnspecifiedType:    Nothing known, use small roman
     * StaffName:          Large roman, to left of start of staff
     * ChordName:          Not normally shown in score, use small roman
     * KeyName:            Not normally shown in score, use small roman
     * Lyric:              Small roman, below staff and dynamic texts
     * Chord:              Small bold roman, above staff
     * Dynamic:	           Small italic, below staff
     * Direction:          Large roman, above staff (by barline?)
     * LocalDirection:     Small bold italic, below staff (by barline?)
     * Tempo:              Large bold roman, above staff
     * LocalTempo:         Small bold roman, above staff
     * Annotation:         Very small sans-serif, in a yellow box
     * LilyPondDirective:  Very small sans-serif, in a green box
     */

    int weight = QFont::Normal;
    bool italic = false;
    bool large = false;
    bool tiny = false;
    bool serif = true;

    if (type == Text::Tempo ||
        type == Text::LocalTempo ||
        type == Text::LocalDirection ||
        type == Text::Chord) {
        weight = QFont::Bold;
    }

    if (type == Text::Dynamic ||
        type == Text::LocalDirection) {
        italic = true;
    }

    if (type == Text::StaffName ||
        type == Text::Direction ||
        type == Text::Tempo) {
        large = true;
    }

    if (type == Text::Annotation ||
        type == Text::LilyPondDirective) {
        serif = false;
        tiny = true;
    }
    
    KConfig* config = kapp->config();

    QFont textFont;

    if (serif) {
        textFont = QFont(defaultSerifFontFamily);
        textFont = config->readFontEntry("textfont", &textFont);
    } else {
        textFont = QFont(defaultSansSerifFontFamily);
        textFont = config->readFontEntry("sansfont", &textFont);
    }

    textFont.setStyleStrategy(QFont::StyleStrategy(QFont::PreferDefault |
                                                   QFont::PreferMatch));

    int size;
    if (large)
        size = (getLineSpacing() * 7) / 2;
    else if (tiny)
        size = (getLineSpacing() * 4) / 3;
    else if (serif)
        size = (getLineSpacing() * 2);
    else
        size = (getLineSpacing() * 3) / 2;

    textFont.setPixelSize(size);
    textFont.setStyleHint(serif ? QFont::Serif : QFont::SansSerif);
    textFont.setWeight(weight);
    textFont.setItalic(italic);

    NOTATION_DEBUG << "NotePixmapFactory::getTextFont: requested size " << size
     		   << " for type " << type << endl;
    
    NOTATION_DEBUG << "NotePixmapFactory::getTextFont: returning font '"
                   << textFont.toString() << "' for type " << type.c_str()
                   << " text : " << text.getText().c_str() << endl;

    m_textFontCache[type.c_str()] = textFont;
    return textFont;
}

QCanvasPixmap*
NotePixmapFactory::makeTextPixmap(const Text &text)
{
    Profiler profiler("NotePixmapFactory::makeTextPixmap");

    std::string type(text.getTextType());

    if (type == Text::Annotation ||
        type == Text::LilyPondDirective) {
        return makeAnnotationPixmap(text, (type == Text::LilyPondDirective));
    }

    drawTextAux(text, 0, 0, 0);
    return makeCanvasPixmap(QPoint(2, 2), true);
}

QCanvasPixmap*
NotePixmapFactory::makeGuitarChordPixmap(const Guitar::Fingering &fingering,
                                       int x,
                                       int y)
{
    using namespace Guitar;
    Profiler profiler("NotePixmapFactory::makeGuitarChordPixmap");

    int guitarChordWidth = getLineSpacing() * 6;
    int guitarChordHeight = getLineSpacing() * 6;

    createPixmapAndMask(guitarChordWidth, guitarChordHeight);

    if (m_selected) {
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
        m_p->painter().setBrush(GUIPalette::getColour(GUIPalette::SelectedElement));
    } else {
        m_p->painter().setPen(Qt::black);
        m_p->painter().setBrush(Qt::black);
    }
    
    Guitar::NoteSymbols ns(Guitar::Fingering::DEFAULT_NB_STRINGS, FingeringBox::DEFAULT_NB_DISPLAYED_FRETS);
    Guitar::NoteSymbols::drawFingeringPixmap(fingering, ns, &(m_p->painter()));

    return makeCanvasPixmap(QPoint (x, y), true);
}

void
NotePixmapFactory::drawText(const Text &text,
                            QPainter &painter, int x, int y)
{
    Profiler profiler("NotePixmapFactory::drawText");

    //     NOTATION_DEBUG << "NotePixmapFactory::drawText() " << text.getText().c_str()
    //                    << " - type : " << text.getTextType().c_str() << endl;

    std::string type(text.getTextType());

    if (type == Text::Annotation ||
        type == Text::LilyPondDirective) {
        QCanvasPixmap *map = makeAnnotationPixmap(text, (type == Text::LilyPondDirective));
        painter.drawPixmap(x, y, *map);
        return ;
    }

    m_inPrinterMethod = true;
    drawTextAux(text, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::drawTextAux(const Text &text,
                               QPainter *painter, int x, int y)
{
    QString s(strtoqstr(text.getText()));
    QFont textFont(getTextFont(text));
    QFontMetrics textMetrics(textFont);

    int offset = 2;
    int width = textMetrics.width(s) + 2 * offset;
    int height = textMetrics.height() + 2 * offset;

    if (painter) {
        painter->save();
        m_p->beginExternal(painter);
        painter->translate(x - offset, y - offset);
    } else {
        createPixmapAndMask(width, height);
    }

    if (m_selected)
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
    else if (m_shaded)
        m_p->painter().setPen(Qt::gray);

    m_p->painter().setFont(textFont);
    if (!m_inPrinterMethod)
        m_p->maskPainter().setFont(textFont);

    m_p->drawText(offset, textMetrics.ascent() + offset, s);

    m_p->painter().setPen(Qt::black);

    if (painter) {
        painter->restore();
    }
}

QCanvasPixmap*
NotePixmapFactory::makeAnnotationPixmap(const Text &text)
{
    return makeAnnotationPixmap(text, false);
}

QCanvasPixmap*
NotePixmapFactory::makeAnnotationPixmap(const Text &text, const bool isLilyPondDirective)
{
    QString s(strtoqstr(text.getText()));

    QFont textFont(getTextFont(text));
    QFontMetrics textMetrics(textFont);

    int annotationWidth = getLineSpacing() * 16;
    int annotationHeight = getLineSpacing() * 6;

    int topGap = getLineSpacing() / 4 + 1;
    int bottomGap = getLineSpacing() / 3 + 1;
    int sideGap = getLineSpacing() / 4 + 1;

    QRect r = textMetrics.boundingRect
              (0, 0, annotationWidth, annotationHeight, Qt::WordBreak, s);

    int pixmapWidth = r.width() + sideGap * 2;
    int pixmapHeight = r.height() + topGap + bottomGap;

    createPixmapAndMask(pixmapWidth, pixmapHeight);

    if (m_selected)
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
    else if (m_shaded)
        m_p->painter().setPen(Qt::gray);

    m_p->painter().setFont(textFont);
    if (!m_inPrinterMethod)
        m_p->maskPainter().setFont(textFont);

    if (isLilyPondDirective) {
        m_p->painter().setBrush(GUIPalette::getColour(GUIPalette::TextLilyPondDirectiveBackground));
    } else {
        m_p->painter().setBrush(GUIPalette::getColour(GUIPalette::TextAnnotationBackground));
    }

    m_p->drawRect(0, 0, pixmapWidth, pixmapHeight);

    m_p->painter().setBrush(Qt::black);
    m_p->painter().drawText(QRect(sideGap, topGap,
                                  annotationWidth + sideGap,
                                  pixmapHeight - bottomGap),
                            Qt::WordBreak, s);

    /* unnecessary following the rectangle draw
        m_pm.drawText(QRect(sideGap, topGap,
    			annotationWidth + sideGap, annotationHeight + topGap),
    		  Qt::WordBreak, s);
    */

    return makeCanvasPixmap(QPoint(0, 0));
}

void
NotePixmapFactory::createPixmapAndMask(int width, int height,
                                       int maskWidth, int maskHeight)
{
    if (maskWidth < 0)
        maskWidth = width;
    if (maskHeight < 0)
        maskHeight = height;

    m_generatedWidth = width;
    m_generatedHeight = height;
    m_generatedPixmap = new QPixmap(width, height);
    m_generatedMask = new QBitmap(maskWidth, maskHeight);

    static unsigned long total = 0;
    total += width * height;
//    NOTATION_DEBUG << "createPixmapAndMask: " << width << "x" << height << " (" << (width*height) << " px, " << total << " total)" << endl;

    // clear up pixmap and mask
    m_generatedPixmap->fill();
    m_generatedMask->fill(Qt::color0);

    // initiate painting
    m_p->begin(m_generatedPixmap, m_generatedMask);

    m_p->painter().setPen(Qt::black);
    m_p->painter().setBrush(Qt::black);
    m_p->maskPainter().setPen(Qt::white);
    m_p->maskPainter().setBrush(Qt::white);
}

QCanvasPixmap*
NotePixmapFactory::makeCanvasPixmap(QPoint hotspot, bool generateMask)
{
    m_p->end();

    QCanvasPixmap* p = new QCanvasPixmap(*m_generatedPixmap, hotspot);

    if (generateMask) {
        p->setMask(PixmapFunctions::generateMask(*p));
    } else {
        p->setMask(*m_generatedMask);
    }

    delete m_generatedPixmap;
    delete m_generatedMask;
    return p;
}

NoteCharacter
NotePixmapFactory::getCharacter(CharName name, ColourType type, bool inverted)
{
    NoteCharacter ch;
    getCharacter(name, ch, type, inverted);
    return ch;
}

bool
NotePixmapFactory::getCharacter(CharName name, NoteCharacter &ch,
                                ColourType type, bool inverted)
{
    NoteFont::CharacterType charType =
        m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen;

    if (m_selected) {
        return m_font->getCharacterColoured
               (name,
                GUIPalette::SelectedElementHue,
                GUIPalette::SelectedElementMinValue,
                ch, charType, inverted);
    }

    if (m_shaded) {
        return m_font->getCharacterShaded(name, ch, charType, inverted);
    }

    switch (type) {

    case PlainColour:
        return m_font->getCharacter(name, ch, charType, inverted);

    case QuantizedColour:
        return m_font->getCharacterColoured
               (name,
                GUIPalette::QuantizedNoteHue,
                GUIPalette::QuantizedNoteMinValue,
                ch, charType, inverted);

    case HighlightedColour:
        return m_font->getCharacterColoured
               (name,
                GUIPalette::HighlightedElementHue,
                GUIPalette::HighlightedElementMinValue,
                ch, charType, inverted);

    case TriggerColour:
        return m_font->getCharacterColoured
               (name,
                GUIPalette::TriggerNoteHue,
                GUIPalette::TriggerNoteMinValue,
                ch, charType, inverted);

    case OutRangeColour:
        return m_font->getCharacterColoured
               (name,
                GUIPalette::OutRangeNoteHue,
                GUIPalette::OutRangeNoteMinValue,
                ch, charType, inverted);
    }

    return m_font->getCharacter(name, ch, charType, inverted);
}

QPoint
NotePixmapFactory::m_pointZero;


int NotePixmapFactory::getNoteBodyWidth(Note::Type type)
const
{
    CharName charName(m_style->getNoteHeadCharName(type).first);
    int hx, hy;
    if (!m_font->getHotspot(charName, hx, hy))
        hx = 0;
    return m_font->getWidth(charName) - hx * 2;
}

int NotePixmapFactory::getNoteBodyHeight(Note::Type )
const
{
    // this is by definition
    return m_font->getSize();
}

int NotePixmapFactory::getLineSpacing() const
{
    return m_font->getSize() + getStaffLineThickness();
}

int NotePixmapFactory::getAccidentalWidth(const Accidental &a,
        int shift, bool extraShift) const
{
    if (a == Accidentals::NoAccidental)
        return 0;
    int w = m_font->getWidth(m_style->getAccidentalCharName(a));
    if (!shift)
        return w;
    else {
        int sw = w;
        if (extraShift) {
            --shift;
            w += getNoteBodyWidth() + getStemThickness();
        }
        w += shift *
             (sw - m_font->getHotspot(m_style->getAccidentalCharName(a)).x());
    }
    return w;
}

int NotePixmapFactory::getAccidentalHeight(const Accidental &a) const
{
    return m_font->getHeight(m_style->getAccidentalCharName(a));
}

int NotePixmapFactory::getStemLength() const
{
    unsigned int l = 1;
    (void)m_font->getStemLength(l);
    return l;
}

int NotePixmapFactory::getStemThickness() const
{
    unsigned int i = 1;
    (void)m_font->getStemThickness(i);
    return i;
}

int NotePixmapFactory::getStaffLineThickness() const
{
    unsigned int i;
    (void)m_font->getStaffLineThickness(i);
    return i;
}

int NotePixmapFactory::getLegerLineThickness() const
{
    unsigned int i;
    (void)m_font->getLegerLineThickness(i);
    return i;
}

int NotePixmapFactory::getDotWidth() const
{
    return m_font->getWidth(NoteCharacterNames::DOT);
}

int NotePixmapFactory::getClefWidth(const Clef &clef) const
{
    return m_font->getWidth(m_style->getClefCharName(clef.getClefType()));
}

int NotePixmapFactory::getBarMargin() const
{
    return getNoteBodyWidth() * 2;
}

int NotePixmapFactory::getRestWidth(const Note &restType) const
{
    return m_font->getWidth(m_style->getRestCharName(restType.getNoteType(),
                            false)) // small inaccuracy!
           + (restType.getDots() * getDotWidth());
}

int NotePixmapFactory::getKeyWidth(const Key &key,
                                   Key previousKey) const
{
    std::vector<int> ah0 = previousKey.getAccidentalHeights(Clef());
    std::vector<int> ah1 = key.getAccidentalHeights(Clef());

    int cancelCount = 0;
    if (key.isSharp() != previousKey.isSharp())
        cancelCount = ah0.size();
    else if (ah1.size() < ah0.size())
        cancelCount = ah0.size() - ah1.size();

    CharName keyCharName;
    if (key.isSharp())
        keyCharName = NoteCharacterNames::SHARP;
    else
        keyCharName = NoteCharacterNames::FLAT;

    NoteCharacter keyCharacter;
    NoteCharacter cancelCharacter;

    keyCharacter = m_font->getCharacter(keyCharName);
    if (cancelCount > 0) {
        cancelCharacter = m_font->getCharacter(NoteCharacterNames::NATURAL);
    }

    //int x = 0;
    //int lw = getLineSpacing();
    int keyDelta = keyCharacter.getWidth() - keyCharacter.getHotspot().x();

    int cancelDelta = 0;
    int between = 0;
    if (cancelCount > 0) {
        cancelDelta = cancelCharacter.getWidth() + cancelCharacter.getWidth() / 3;
        between = cancelCharacter.getWidth();
    }

    return (keyDelta * ah1.size() + cancelDelta * cancelCount + between +
            keyCharacter.getWidth() / 4);
}

int NotePixmapFactory::getTextWidth(const Text &text) const
{
    QFontMetrics metrics(getTextFont(text));
    return metrics.boundingRect(strtoqstr(text.getText())).width() + 4;
}

}
