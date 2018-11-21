/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifdef __GNUG__
#pragma GCC diagnostic ignored "-Wswitch-enum"
#endif

#include "NotePixmapFactory.h"

#include "misc/Debug.h"
#include "base/NotationRules.h"
#include "misc/Strings.h"
#include "misc/ConfigGroups.h"
#include "base/Exception.h"
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "gui/editors/guitar/Fingering.h"
#include "gui/editors/guitar/FingeringBox.h"
#include "gui/editors/guitar/NoteSymbols.h"
#include "gui/editors/notation/StaffHeader.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/PixmapFunctions.h"
#include "gui/general/Spline.h"
#include "gui/general/ResourceFinder.h"
#include "gui/general/IconLoader.h"
#include "gui/widgets/StartupLogo.h"
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

#include <QApplication>
#include <QSettings>
#include <QMessageBox>
#include <QBitmap>
#include <QColor>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPolygon>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QMatrix>

#include <cmath>


namespace Rosegarden
{

using namespace Accidentals;

//static clock_t drawBeamsTime = 0;
//static clock_t makeNotesTime = 0;
static int makeNotesCount = 0;
static int makeRestsCount = 0;
//static int drawBeamsCount = 0;
static int drawBeamsBeamCount = 0;

const char* const NotePixmapFactory::defaultSerifFontFamily = "Bitstream Vera Serif";
const char* const NotePixmapFactory::defaultSansSerifFontFamily = "Bitstream Vera Sans";
const char* const NotePixmapFactory::defaultTimeSigFontFamily = "Bitstream Vera Serif";

NotePixmapFactory::NotePixmapFactory(QString fontName, int size, int graceSize) :
    m_selected(false),
    m_shaded(false),
    m_haveGrace(graceSize != NO_GRACE_SIZE),
    m_graceSize(graceSize),
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
    m_clefOttavaFont(defaultSerifFontFamily, 8, QFont::Normal),
    m_clefOttavaFontMetrics(m_ottavaFont),
    m_trackHeaderFont(defaultSansSerifFontFamily, 9, QFont::Normal),
    m_trackHeaderFontMetrics(m_trackHeaderFont),
    m_trackHeaderBoldFont(defaultSansSerifFontFamily, 9, QFont::Bold),
    m_trackHeaderBoldFontMetrics(m_trackHeaderBoldFont),
    m_generatedPixmap(nullptr),
    m_generatedWidth( -1),
    m_generatedHeight( -1),
    m_inPrinterMethod(false),
    m_p(new NotePixmapPainter())
{
    init(fontName, size);
}

NotePixmapFactory::NotePixmapFactory(const NotePixmapFactory &npf) :
    m_selected(false),
    m_shaded(false),
    m_graceSize(npf.m_graceSize),
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
    m_clefOttavaFont(defaultSerifFontFamily, 8, QFont::Normal),
    m_clefOttavaFontMetrics(m_ottavaFont),
    m_trackHeaderFont(defaultSansSerifFontFamily, 9, QFont::Normal),
    m_trackHeaderFontMetrics(m_trackHeaderFont),
    m_trackHeaderBoldFont(defaultSansSerifFontFamily, 9, QFont::Bold),
    m_trackHeaderBoldFontMetrics(m_trackHeaderBoldFont),
    m_generatedPixmap(0),
    m_generatedWidth( -1),
    m_generatedHeight( -1),
    m_inPrinterMethod(false),
    m_p(new NotePixmapPainter())
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
        m_ottavaFontMetrics = QFontMetrics(m_ottavaFont);
        m_clefOttavaFont = npf.m_clefOttavaFont;
        m_clefOttavaFontMetrics = QFontMetrics(m_clefOttavaFont);
        m_trackHeaderFont = npf.m_trackHeaderFont;
        m_trackHeaderFontMetrics = QFontMetrics(m_trackHeaderFont);
        m_trackHeaderBoldFont = npf.m_trackHeaderBoldFont;
        m_trackHeaderBoldFontMetrics = QFontMetrics(m_trackHeaderBoldFont);
        init(npf.m_font->getName(), npf.m_font->getSize());
        m_textFontCache.clear();
    }
    return *this;
}

void
NotePixmapFactory::init(QString fontName, int size)
{
    try {
        m_style = NoteStyleFactory::getStyle(NoteStyleFactory::DefaultStyle);
    } catch (NoteStyleFactory::StyleUnavailable u) {
        StartupLogo::hideIfStillThere();
        QMessageBox::critical(0, tr("Rosegarden"), tr( u.getMessage().c_str() )  );
        throw;
    }

    int origSize = size;

    if (fontName != "") {
        try {
            if (size < 0) size = NoteFontFactory::getDefaultSize(fontName);
            m_font = NoteFontFactory::getFont(fontName, size);
            if (m_graceSize > 0) m_graceFont = NoteFontFactory::getFont(fontName, m_graceSize);
            else m_graceFont = m_font;
        } catch (Exception f) {
            fontName = "";
            // fall through
        }
    }

    if (fontName == "") { // either because it was passed in or because read failed
        try {
            fontName = NoteFontFactory::getDefaultFontName();
            size = origSize;
            if (size < 0) size = NoteFontFactory::getDefaultSize(fontName);
            m_font = NoteFontFactory::getFont(fontName, size);
            m_graceFont = NoteFontFactory::getFont(fontName, m_graceSize);
        } catch (Exception f) { // already reported
            throw;
        }
    }

    // Resize the fonts, because the original constructor used point
    // sizes only and we want pixels
    QFont timeSigFont(defaultTimeSigFontFamily),
        textFont(defaultSerifFontFamily);

    QSettings settings;
    settings.beginGroup( NotationViewConfigGroup );

    m_timeSigFont = settings.value("timesigfont", timeSigFont).toString();
    m_timeSigFont.setBold(true);
    m_timeSigFont.setPixelSize(size * 5 / 2);
    m_timeSigFontMetrics = QFontMetrics(m_timeSigFont);

    m_bigTimeSigFont = settings.value("timesigfont", timeSigFont).toString();
    m_bigTimeSigFont.setPixelSize(size * 4 + 2);
    m_bigTimeSigFontMetrics = QFontMetrics(m_bigTimeSigFont);

    m_tupletCountFont = settings.value("textfont", textFont).toString();
    m_tupletCountFont.setBold(true);
    m_tupletCountFont.setPixelSize(size * 2);
    m_tupletCountFontMetrics = QFontMetrics(m_tupletCountFont);

    m_textMarkFont = settings.value("textfont", textFont).toString();
    m_textMarkFont.setBold(true);
    m_textMarkFont.setItalic(true);
    m_textMarkFont.setPixelSize(size * 2);
    m_textMarkFontMetrics = QFontMetrics(m_textMarkFont);

    m_fingeringFont = settings.value("textfont", textFont).toString();
    m_fingeringFont.setBold(true);
    m_fingeringFont.setPixelSize(size * 5 / 3);
    m_fingeringFontMetrics = QFontMetrics(m_fingeringFont);

    m_ottavaFont = settings.value("textfont", textFont).toString();
    m_ottavaFont.setPixelSize(size * 2);
    m_ottavaFontMetrics = QFontMetrics(m_ottavaFont);

    m_clefOttavaFont = settings.value("textfont", textFont).toString();
    m_clefOttavaFont.setPixelSize(getLineSpacing() * 3 / 2);
    m_clefOttavaFontMetrics = QFontMetrics(m_clefOttavaFont);

    m_trackHeaderFont = settings.value("sansfont", m_trackHeaderFont).toString();
    m_trackHeaderFont.setPixelSize(9);
    m_trackHeaderFontMetrics = QFontMetrics(m_trackHeaderFont);

    m_trackHeaderBoldFont = m_trackHeaderFont;
    m_trackHeaderBoldFont.setBold(true);
    m_trackHeaderBoldFontMetrics = QFontMetrics(m_trackHeaderBoldFont);

    settings.endGroup();
}

NotePixmapFactory::~NotePixmapFactory()
{
    NOTATION_DEBUG << "NotePixmapFactory::~NotePixmapFactory:"
              << " makeNotesCount = " << makeNotesCount
              << ", makeRestsCount = " << makeRestsCount;

    delete m_p;
}

QString
NotePixmapFactory::getFontName() const
{
    return m_font->getName();
}

int
NotePixmapFactory::getSize() const
{
    return m_font->getSize();
}

void
NotePixmapFactory::dumpStats(std::ostream &s)
{
#ifdef DUMP_STATS
/*
  s << "NotePixmapFactory: total times since last stats dump:\n"
  << "makeNotePixmap: "
  << (makeNotesTime * 1000 / CLOCKS_PER_SEC) << "ms\n"
  << "drawBeams: "
  << (drawBeamsTime * 1000 / CLOCKS_PER_SEC) << "ms"
  << " (drew " << drawBeamsCount << " individual points in " << drawBeamsBeamCount << " beams)"
 ;
  makeNotesTime = 0;
  drawBeamsTime = 0;
  drawBeamsCount = 0;
  drawBeamsBeamCount = 0;
*/
#endif

    (void)s; // avoid warnings
}

QGraphicsItem *
NotePixmapFactory::makeNote(const NotePixmapParameters &params)
{
    Profiler profiler("NotePixmapFactory::makeNote");

    ++makeNotesCount;

    if (m_inPrinterMethod) {
        return makeNotePixmapItem(params);
    }

    NoteItem *item = new NoteItem(params, m_style, m_selected, m_shaded, this);
    return item;
}

void
NotePixmapFactory::getNoteDimensions(const NotePixmapParameters &params,
                                     NoteItemDimensions &dimensions)
{
    calculateNoteDimensions(params);
    dimensions = m_nd;
}

void
NotePixmapFactory::drawNoteForItem(const NotePixmapParameters &params,
                                   const NoteItemDimensions &dimensions,
                                   NoteItem::DrawMode mode,
                                   QPainter *painter)
{
    if (mode == NoteItem::DrawTiny) {
        sketchNoteTiny(params, dimensions, painter);
        return;
    }

    m_nd = dimensions;
    drawNoteAux(params, painter, 0, 0);
}

QGraphicsPixmapItem *
NotePixmapFactory::makeNotePixmapItem(const NotePixmapParameters &params)
{
    Profiler profiler("NotePixmapFactory::makeNotePixmapItem");

    calculateNoteDimensions(params);
    drawNoteAux(params, 0, 0, 0);

    QPoint hotspot(m_nd.left, m_nd.above + m_nd.noteBodyHeight / 2);

    //#define ROSE_DEBUG_NOTE_PIXMAP_FACTORY
#ifdef ROSE_DEBUG_NOTE_PIXMAP_FACTORY

    m_p->painter().setPen(QColor(Qt::red));
    m_p->painter().setBrush(QColor(Qt::red));

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

    return makeItem(hotspot);
}

void
NotePixmapFactory::drawNote(const NotePixmapParameters &params,
                            QPainter &painter, int x, int y)
{
    Profiler profiler("NotePixmapFactory::drawNote");
    m_inPrinterMethod = true;
    calculateNoteDimensions(params);
    drawNoteAux(params, &painter, x, y);
    m_inPrinterMethod = false;
}

void
NotePixmapFactory::calculateNoteDimensions(const NotePixmapParameters &params)
{
    // use the full font for this unless a grace size was supplied in ctor
    NoteFont *font = (m_haveGrace ? m_graceFont : m_font);

    NoteFont::CharacterType charType =
        m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen;

    if (m_selected) {
        NOTATION_DEBUG << "NotePixmapFactory::calculateNoteDimensions: selected";
    }

    bool drawFlag = params.m_drawFlag;
    if (params.m_beamed) drawFlag = false;

    // A note pixmap is formed of note head, stem, flags,
    // accidentals, dots and beams.  Assume the note head first, then
    // do the rest of the calculations left to right, ie accidentals,
    // stem, flags, dots, beams

    m_nd.noteBodyWidth = getNoteBodyWidth(params.m_noteType);
    m_nd.noteBodyHeight = getNoteBodyHeight(params.m_noteType);

    // Spacing surrounding the note head.  For top and bottom, we
    // adjust this according to the discrepancy between the nominal
    // and actual heights of the note head pixmap.  For left and
    // right, we use the hotspot x coordinate of the head.
    int temp;
    if (!font->getHotspot(m_style->getNoteHeadCharName(params.m_noteType).first,
                          m_nd.borderX, temp))
        m_nd.borderX = 0;

    if (params.m_noteType == Note::Minim && params.m_stemGoesUp)
        m_nd.borderX++;
    int actualNoteBodyHeight =
        font->getHeight(m_style->getNoteHeadCharName(params.m_noteType).first);

    m_nd.left = m_nd.right = m_nd.borderX;
    m_nd.above = m_nd.borderY = (actualNoteBodyHeight - m_nd.noteBodyHeight) / 2;
    m_nd.below = (actualNoteBodyHeight - m_nd.noteBodyHeight) - m_nd.above;

    //    NOTATION_DEBUG << "actualNoteBodyHeight: " << actualNoteBodyHeight
    //		   << ", noteBodyHeight: " << m_nd.noteBodyHeight << ", borderX: "
    //		   << m_nd.borderX << ", borderY: "
    //		   << m_nd.borderY;

    bool isStemmed = m_style->hasStem(params.m_noteType);
    int flagCount = m_style->getFlagCount(params.m_noteType);
    int slashCount = params.m_slashes;
    if (!slashCount) slashCount = m_style->getSlashCount(params.m_noteType);

    if (params.m_accidental != NoAccidental) {
        makeRoomForAccidental(params.m_accidental,
                              params.m_cautionary,
                              params.m_accidentalShift,
                              params.m_accidentalExtra);
    }

    NoteCharacter dot(getCharacter(NoteCharacterNames::DOT, PlainColour, charType));
    int dotWidth = dot.getWidth();
    if (dotWidth < getNoteBodyWidth() / 2) dotWidth = getNoteBodyWidth() / 2;

    int stemLength = getStemLength(params);

    if (params.m_marks.size() > 0) {
        makeRoomForMarks(isStemmed, params, stemLength);
    }

    if (params.m_legerLines != 0) {
        makeRoomForLegerLines(params);
    }

    if (slashCount > 0) {
        m_nd.left = std::max(m_nd.left, m_nd.noteBodyWidth / 2);
        m_nd.right = std::max(m_nd.right, m_nd.noteBodyWidth / 2);
    }

    if (params.m_tupletCount > 0) {
        makeRoomForTuplingLine(params);
    }

    m_nd.right = std::max(m_nd.right, params.m_dots * dotWidth + dotWidth / 2);
    if (params.m_dotShifted) {
        m_nd.right += m_nd.noteBodyWidth;
    }
    if (params.m_onLine) {
        m_nd.above = std::max(m_nd.above, dot.getHeight() / 2);
    }

    if (params.m_shifted) {
        if (params.m_stemGoesUp) {
            m_nd.right += m_nd.noteBodyWidth;
        } else {
            m_nd.left = std::max(m_nd.left, m_nd.noteBodyWidth);
        }
    }

    bool tieAbove = params.m_tieAbove;
    if (!params.m_tiePositionExplicit) {
        tieAbove = !params.m_stemGoesUp;
    }

    if (params.m_tied) {
        m_nd.right = std::max(m_nd.right, params.m_tieLength);
        if (!tieAbove) {
            m_nd.below = std::max(m_nd.below, m_nd.noteBodyHeight * 2);
        } else {
            m_nd.above = std::max(m_nd.above, m_nd.noteBodyHeight * 2);
        }
    }

    if (isStemmed && params.m_drawStem) {
        makeRoomForStemAndFlags(drawFlag ? flagCount : 0, stemLength, params,
                                m_nd.stemStart, m_nd.stemEnd);
    }

    if (isStemmed && params.m_drawStem && params.m_beamed) {
        makeRoomForBeams(params);
    }
}

void
NotePixmapFactory::sketchNoteTiny(const NotePixmapParameters &params,
                                  const NoteItemDimensions &dimensions,
                                  QPainter *painter)
{
    if (params.m_drawStem && m_style->hasStem(params.m_noteType)) {
        painter->drawLine(dimensions.left + dimensions.stemStart.x(),
                          dimensions.above + dimensions.stemStart.y(),
                          dimensions.left + dimensions.stemEnd.x(),
                          dimensions.above + dimensions.stemEnd.y());
    }
    painter->drawRect(dimensions.left,
                      dimensions.above,
                      dimensions.noteBodyWidth,
                      dimensions.noteBodyHeight);
}

void
NotePixmapFactory::drawNoteAux(const NotePixmapParameters &params,
                               QPainter *painter, int x, int y)
{
    NoteFont::CharacterType charType =
        m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen;

    bool drawFlag = params.m_drawFlag;
    if (params.m_beamed) drawFlag = false;

    bool isStemmed = m_style->hasStem(params.m_noteType);
    int flagCount = m_style->getFlagCount(params.m_noteType);
    int slashCount = params.m_slashes;
    if (!slashCount) slashCount = m_style->getSlashCount(params.m_noteType);

    NoteCharacter dot;
    if (params.m_forceColor) {
        dot = getCharacter(NoteCharacterNames::DOT,
                           params.m_forcedColor, charType);
    } else {
        dot = getCharacter(NoteCharacterNames::DOT, PlainColour, charType);
    }

    int dotWidth = dot.getWidth();
    if (dotWidth < getNoteBodyWidth() / 2) dotWidth = getNoteBodyWidth() / 2;

    int stemLength = getStemLength(params);

    bool tieAbove = params.m_tieAbove;
    if (!params.m_tiePositionExplicit) {
        tieAbove = !params.m_stemGoesUp;
    }

    // for all other calculations we use the nominal note-body height
    // (same as the gap between staff lines), but here we want to know
    // if the pixmap itself is taller than that
    /*!!!
      int actualNoteBodyHeight = font->getHeight
      (m_style->getNoteHeadCharName(params.m_noteType).first);
      //	- 2*m_origin.y();
      if (actualNoteBodyHeight > m_noteBodyHeight) {
      m_below = std::max(m_below, actualNoteBodyHeight - m_noteBodyHeight);
      }
    */
    if (painter) {
        painter->save();
        m_p->beginExternal(painter);
//        NOTATION_DEBUG << "Translate: (" << x << "," << y << ")";
        painter->translate(x - m_nd.left, y - m_nd.above - m_nd.noteBodyHeight / 2);
    } else {
        createPixmap(m_nd.noteBodyWidth + m_nd.left + m_nd.right,
                     m_nd.noteBodyHeight + m_nd.above + m_nd.below);
    }

    if (params.m_tupletCount > 0) {
        drawTuplingLine(params);
    }

    if (isStemmed && params.m_drawStem && drawFlag) {
        drawFlags(flagCount, params, m_nd.stemStart, m_nd.stemEnd);
    }

    NoteStyle::CharNameRec charNameRec
        (m_style->getNoteHeadCharName(params.m_noteType));
    CharName charName = charNameRec.first;
    bool inverted = charNameRec.second;

    NoteCharacter body;
    if (params.m_forceColor) {
        body = getCharacter(charName, params.m_forcedColor, inverted);
    } else {
        body = getCharacter
            (charName,
            params.m_memberOfParallel ? MemberOfParallelColour :
            params.m_highlighted ? HighlightedColour :
            params.m_quantized ? QuantizedColour :
            (params.m_trigger == NotePixmapParameters::triggerYes) ? TriggerColour :
            (params.m_trigger == NotePixmapParameters::triggerSkip) ? TriggerSkipColour :
            params.m_inRange ? PlainColour : OutRangeColour,
            inverted);
    }

    QPoint bodyLocation(m_nd.left - m_nd.borderX,
                        m_nd.above - m_nd.borderY + getStaffLineThickness() / 2);
    if (params.m_shifted) {
        if (params.m_stemGoesUp) {
            bodyLocation.rx() += m_nd.noteBodyWidth;
        } else {
            bodyLocation.rx() -= m_nd.noteBodyWidth - 1;
        }
    }

    m_p->drawNoteCharacter(bodyLocation.x(), bodyLocation.y(), body);

    if (params.m_dots > 0) {

        int x = m_nd.left + m_nd.noteBodyWidth + dotWidth / 2;
        int y = m_nd.above + m_nd.noteBodyHeight / 2 - dot.getHeight() / 2;

        if (params.m_onLine) y -= m_nd.noteBodyHeight / 2;

        if (params.m_shifted) x += m_nd.noteBodyWidth;
        else if (params.m_dotShifted) x += m_nd.noteBodyWidth;

        for (int i = 0; i < params.m_dots; ++i) {
            m_p->drawNoteCharacter(x, y, dot);
            x += dotWidth;
        }
    }

    // Prepare color to draw accidental, stem, flags, etc...
    if ((isStemmed && params.m_drawStem) || params.m_cautionary) {
        if (m_selected) m_p->painter().setPen(GUIPalette::getColour(
                                                GUIPalette::SelectedElement));
        else if (m_shaded) m_p->painter().setPen(QColor(Qt::gray));
        else m_p->painter().setPen(QColor(Qt::black));

    }

    drawAccidental(params);

    if (isStemmed && params.m_drawStem) {

        if (flagCount > 0 && !drawFlag && params.m_beamed) {
            drawBeams(m_nd.stemEnd, params, flagCount);
        }

        if (slashCount > 0) {
            drawSlashes(m_nd.stemStart, params, slashCount);
        }

        // If we draw stems after beams, instead of beams after stems,
        // beam anti-aliasing won't damage stems but we have to shorten the
        // stems slightly first so that the stems don't extend all the way
        // through the beam into the anti-aliased region on the
        // other side of the beam that faces away from the note-heads.
        int shortening;
        if (flagCount > 0 && !drawFlag && params.m_beamed) shortening = 2;
        else shortening = 0;
        drawStem(params, m_nd.stemStart, m_nd.stemEnd, shortening);
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


QGraphicsPixmapItem *
NotePixmapFactory::makeNoteHalo(const NotePixmapParameters &params)
{
    int nbh0 = getNoteBodyHeight();
    int nbh = getNoteBodyHeight(params.m_noteType);
    int nbw0 = getNoteBodyHeight();
    int nbw = getNoteBodyWidth(params.m_noteType);

    createPixmap(nbw + nbw0, nbh + nbh0);
    drawNoteHalo(0, 0, nbw + nbw0, nbh + nbh0);

    return makeItem(QPoint(nbw0 / 2, nbh0));
}


void
NotePixmapFactory::drawNoteHalo(int x, int y, int w, int h) {

    m_p->painter().setPen(QPen(QColor::fromHsv(GUIPalette::CollisionHaloHue,
                                      GUIPalette::CollisionHaloSaturation,
                                      255), 1));
    m_p->painter().setBrush(QColor::fromHsv(GUIPalette::CollisionHaloHue,
                                   GUIPalette::CollisionHaloSaturation,
                                   255));
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
    int nbh = m_nd.noteBodyHeight;

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

            unsigned int flagSpace = m_nd.noteBodyHeight;
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
    // use the full font for this unless a grace size was supplied in ctor
    NoteFont *font = (m_haveGrace ? m_graceFont : m_font);

    // General observation: where we're only using a character to
    // determine its dimensions, we should (for the moment) just
    // request it in screen mode, because it may be quicker and we
    // don't need to render it, and the dimensions are the same.
    NoteCharacter ac
        (font->getCharacter(m_style->getAccidentalCharName(a)));

    QPoint ah(font->getHotspot(m_style->getAccidentalCharName(a)));

    m_nd.left += ac.getWidth() + (m_nd.noteBodyWidth / 4 - m_nd.borderX);

    if (shift > 0) {
        if (extra) {
            // The extra flag indicates that the first shift is to get
            // out of the way of a note head, thus has to move
            // possibly further, or at least a different amount.  So
            // replace the first shift with a different one.
            --shift;
            m_nd.left += m_nd.noteBodyWidth - m_nd.noteBodyWidth / 5;
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
                    (font->getCharacter(m_style->getAccidentalCharName
                                        (Accidentals::Sharp)));
                QPoint ahSharp
                    (font->getHotspot(m_style->getAccidentalCharName
                                      (Accidentals::Sharp)));
                step = std::max(step, acSharp.getWidth() - ahSharp.x());
            }
            m_nd.left += shift * step;
        }
    }

    if (cautionary) m_nd.left += m_nd.noteBodyWidth;

    int above = ah.y() - m_nd.noteBodyHeight / 2;
    int below = (ac.getHeight() - ah.y()) -
        (m_nd.noteBodyHeight - m_nd.noteBodyHeight / 2); // subtract in case it's odd

    if (above > 0) m_nd.above = std::max(m_nd.above, above);
    if (below > 0) m_nd.below = std::max(m_nd.below, below);
}

void
NotePixmapFactory::drawAccidental(const NotePixmapParameters &params)
{
    if (params.m_accidental == NoAccidental) return;   // Nothing to draw

    Accidental a = params.m_accidental;
    bool cautionary = params.m_cautionary;
        
    // use the full font for this unless a grace size was supplied in ctor
    NoteFont *font = (m_haveGrace ? m_graceFont : m_font);
    
    NoteCharacter ac;
    if (params.m_forceColor) {
        ac = getCharacter
            (m_style->getAccidentalCharName(a), params.m_forcedColor, false);
        // Prepare color in case drawing brackets is needed
        m_p->painter().setPen(params.m_forcedColor);
    } else {
        ac = getCharacter
            (m_style->getAccidentalCharName(a), PlainColour, false);
    }

    QPoint ah(font->getHotspot(m_style->getAccidentalCharName(a)));

    int ax = 0;

    if (cautionary) {
        ax += m_nd.noteBodyWidth / 2;
        int bl = ac.getHeight() * 2 / 3;
        int by = m_nd.above + m_nd.noteBodyHeight / 2 - bl / 2;
        drawBracket(bl, true, false, m_nd.noteBodyWidth*3 / 8, by);
        drawBracket(bl, false, false, ac.getWidth() + m_nd.noteBodyWidth*5 / 8, by);
    }

    m_p->drawNoteCharacter(ax, m_nd.above + m_nd.noteBodyHeight / 2 - ah.y(), ac);
}

void
NotePixmapFactory::makeRoomForMarks(bool isStemmed,
                                    const NotePixmapParameters &params,
                                    int stemLength)
{
    // if anyone puts marks over grace notes, they're stupid, so we're ignoring
    // this case

    int height = 0, width = 0;
    int gap = m_nd.noteBodyHeight / 5 + 1;

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
            m_nd.below += height + 1;
        } else {
            m_nd.above += height + 1;
        }
    }

    height = 0;

    if (params.m_safeVertDistance > 0 && !aboveMarks.empty()) {
        m_nd.above = std::max(m_nd.above, params.m_safeVertDistance);
    }

    for (std::vector<Mark>::iterator i = aboveMarks.begin();
         i != aboveMarks.end(); ++i) {

        if (!Marks::isFingeringMark(*i)) {

            Mark m(*i);

            if (m == Marks::TrillLine)
                m = Marks::LongTrill;

            if (m == Marks::LongTrill) {
                m_nd.right = std::max(m_nd.right, params.m_width);
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
            m_nd.above += stemLength + height + 1;
        } else {
            m_nd.above += height + 1;
        }
    }

    m_nd.left = std::max(m_nd.left, width / 2 - m_nd.noteBodyWidth / 2);
    m_nd.right = std::max(m_nd.right, width / 2 - m_nd.noteBodyWidth / 2);
}

void
NotePixmapFactory::drawMarks(bool isStemmed,
                             const NotePixmapParameters &params,
                             int stemLength,
                             bool overRestHack)
{
    int gap = m_nd.noteBodyHeight / 5 + 1;
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

            int x = m_nd.left + m_nd.noteBodyWidth / 2 - character.getWidth() / 2;
            int y = (normalMarksAreAbove ?
                     (m_nd.above - dy - character.getHeight() - 1) :
                     (m_nd.above + m_nd.noteBodyHeight + m_nd.borderY * 2 + dy));

            m_p->drawNoteCharacter(x, y, character);
            dy += character.getHeight() + gap;

        } else {

            QString text = strtoqstr(Marks::getTextFromMark(*i));
            QRect bounds = m_textMarkFontMetrics.boundingRect(text);

            m_p->painter().setFont(m_textMarkFont);

            int x = m_nd.left + m_nd.noteBodyWidth / 2 - bounds.width() / 2;
            int y = (normalMarksAreAbove ?
                     (m_nd.above - dy - 3) :
                     (m_nd.above + m_nd.noteBodyHeight + m_nd.borderY * 2 + dy + bounds.height() + 1));

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

        if (m_selected) m_p->painter().setPen(GUIPalette::getColour(
                                                GUIPalette::SelectedElement));
        else if (m_shaded) m_p->painter().setPen(QColor(Qt::gray));
        else m_p->painter().setPen(QColor(Qt::black));

        if (!Marks::isFingeringMark(*i)) {

            int x = m_nd.left + m_nd.noteBodyWidth / 2;
            int y = m_nd.above - dy - 1;

            if (*i != Marks::TrillLine) {

                NoteCharacter character(getCharacter(m_style->getMarkCharName(*i), PlainColour, false));

                if (overRestHack) {

                    // Hmmm...  The "canvas" we can draw on is contrained by
                    // something somewhere else, and a fermata symbol won't fit
                    // on it.  After a few hours trying to pick this apart, I'm
                    // completely stuck.  Oh well.  I'll leave the code leading
                    // up to here in place in case I ever have any new
                    // inspiration.

                } else {

                    x -= character.getWidth() / 2;
                    y -= character.getHeight();

                    m_p->drawNoteCharacter(x, y, character);

                    y += character.getHeight() / 2;
                    x += character.getWidth();

                    dy += character.getHeight() + gap;
                }

            } else {

                NoteCharacter character(getCharacter(m_style->getMarkCharName(
                                                         Marks::Trill), PlainColour, false));
                y -= character.getHeight() / 2;
                dy += character.getHeight() + gap;
            }

            if (*i == Marks::LongTrill ||
                *i == Marks::TrillLine) {
                NoteCharacter extension;
                if (getCharacter(NoteCharacterNames::TRILL_LINE, extension,
                                 PlainColour, false)) {
                    x += extension.getHotspot().x();
                    while (x < m_nd.left + params.m_width - extension.getWidth()) {
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

            int x = m_nd.left + m_nd.noteBodyWidth / 2 - bounds.width() / 2;
            int y = m_nd.above - dy - 3;

            m_p->drawText(x, y, text);
            dy += bounds.height() + gap;
        }
    }
}

void
NotePixmapFactory::makeRoomForLegerLines(const NotePixmapParameters &params)
{
    if (params.m_legerLines < 0 || params.m_restOutsideStave) {
        m_nd.above = std::max(m_nd.above,
                              (m_nd.noteBodyHeight + 1) *
                              ( -params.m_legerLines / 2));
    }
    if (params.m_legerLines > 0 || params.m_restOutsideStave) {
        m_nd.below = std::max(m_nd.below,
                              (m_nd.noteBodyHeight + 1) *
                              (params.m_legerLines / 2));
    }
    if (params.m_legerLines != 0) {
        m_nd.left = std::max(m_nd.left, m_nd.noteBodyWidth / 5 + 1);
        m_nd.right = std::max(m_nd.right, m_nd.noteBodyWidth / 5 + 1);
    }
    if (params.m_restOutsideStave) {
        m_nd.above += 1;
        m_nd.left = std::max(m_nd.left, m_nd.noteBodyWidth * 3 + 1);
        m_nd.right = std::max(m_nd.right, m_nd.noteBodyWidth * 3 + 1);
    }
}

void
NotePixmapFactory::drawLegerLines(const NotePixmapParameters &params)
{
    int x0, x1, y;

    if (params.m_legerLines == 0)
        return ;

    if (params.m_restOutsideStave) {
        if (m_selected) m_p->painter().setPen(GUIPalette::getColour(
                                                GUIPalette::SelectedElement));
        else if (m_shaded) m_p->painter().setPen(QColor(Qt::gray));
        else m_p->painter().setPen(QColor(Qt::black));
    }
    x0 = m_nd.left - m_nd.noteBodyWidth / 5 - 1;
    x1 = m_nd.left + m_nd.noteBodyWidth + m_nd.noteBodyWidth / 5 /* + 1 */;

    if (params.m_shifted) {
        if (params.m_stemGoesUp) {
            x0 += m_nd.noteBodyWidth;
            x1 += m_nd.noteBodyWidth;
        } else {
            x0 -= m_nd.noteBodyWidth;
            x1 -= m_nd.noteBodyWidth;
        }
    }

    int offset = m_nd.noteBodyHeight + getStaffLineThickness();
    int legerLines = params.m_legerLines;
    bool below = (legerLines < 0);

    if (below) {
        legerLines = -legerLines;
        offset = -offset;
    }

    if (params.m_restOutsideStave)
        y = m_nd.above;
    else {
        if (!below) { // note above staff
            if (legerLines % 2) { // note is between lines
                y = m_nd.above + m_nd.noteBodyHeight;
            } else { // note is on a line
                y = m_nd.above + m_nd.noteBodyHeight / 2 - getStaffLineThickness() / 2;
            }
        } else { // note below staff
            if (legerLines % 2) { // note is between lines
                y = m_nd.above - getStaffLineThickness();
            } else { // note is on a line
                y = m_nd.above + m_nd.noteBodyHeight / 2;
            }
        }
    }
    if (params.m_restOutsideStave) {
        NOTATION_DEBUG << "draw leger lines: " << legerLines << " lines, below "
                       << below
                       << ", note body height " << m_nd.noteBodyHeight
                       << ", thickness " << getLegerLineThickness()
                       << " (staff line " << getStaffLineThickness() << ")"
                       << ", offset " << offset;
    }

    //    NOTATION_DEBUG << "draw leger lines: " << legerLines << " lines, below "
    //		   << below
    //		   << ", note body height " << m_noteBodyHeight
    //		   << ", thickness " << getLegerLineThickness()
    //		   << " (staff line " << getStaffLineThickness() << ")"
    //		   << ", offset " << offset;

    //    bool first = true;

    if (getLegerLineThickness() > getStaffLineThickness()) {
        y -= (getLegerLineThickness() - getStaffLineThickness() + 1) / 2;
    }

    if (params.m_forceColor) {
        m_p->painter().save();
        m_p->painter().setPen(params.m_forcedColor);
    } else {
        // Sometimes needed to render segment link when an accent is under the note.
        //!!! Why ???
        if (m_shaded && !m_selected) m_p->painter().setPen(QColor(Qt::gray));
    }

    for (int i = legerLines - 1; i >= 0; --i) {
        if (i % 2) {
            //	    NOTATION_DEBUG << "drawing leger line at y = " << y;
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

    if (params.m_forceColor) {
        m_p->painter().restore();
    }
}

void
NotePixmapFactory::makeRoomForStemAndFlags(int flagCount, int stemLength,
                                           const NotePixmapParameters &params,
                                           QPoint &s0, QPoint &s1)
{
    // use the full font for this unless a grace size was supplied in ctor
    NoteFont *font = (m_haveGrace ? m_graceFont : m_font);

    // The coordinates we set in s0 and s1 are relative to (m_above, m_left)

    if (params.m_stemGoesUp) {
        m_nd.above = std::max
            (m_nd.above, stemLength - m_nd.noteBodyHeight / 2);
    } else {
        m_nd.below = std::max
            (m_nd.below, stemLength - m_nd.noteBodyHeight / 2 + 1);
    }

    if (flagCount > 0) {
        if (params.m_stemGoesUp) {
            int width = 0, height = 0;
            if (!font->getDimensions
                (m_style->getFlagCharName(flagCount), width, height)) {
                width = font->getWidth(m_style->getPartialFlagCharName(false));
            }
            m_nd.right += width;
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
            s0.setX(m_nd.noteBodyWidth - stemThickness);
        } else {
            s0.setX(0);
        }
        break;

    case NoteStyle::Central:
        if (params.m_stemGoesUp ^ (hfix == NoteStyle::Reversed)) {
            s0.setX(m_nd.noteBodyWidth / 2 + 1);
        } else {
            s0.setX(m_nd.noteBodyWidth / 2);
        }
        break;
    }

    switch (vfix) {

    case NoteStyle::Near:
    case NoteStyle::Far:
        if (params.m_stemGoesUp ^ (vfix == NoteStyle::Far)) {
            s0.setY(0);
        } else {
            s0.setY(m_nd.noteBodyHeight);
        }
        if (vfix == NoteStyle::Near) {
            stemLength -= m_nd.noteBodyHeight / 2;
        } else {
            stemLength += m_nd.noteBodyHeight / 2;
        }
        break;

    case NoteStyle::Middle:
        if (params.m_stemGoesUp) {
            s0.setY(m_nd.noteBodyHeight * 3 / 8);
        } else {
            s0.setY(m_nd.noteBodyHeight * 5 / 8);
        }
        stemLength -= m_nd.noteBodyHeight / 8;
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
    bool found;
    if (params.m_forceColor) {
        found = getCharacter(m_style->getFlagCharName(flagCount),
                             flagChar,
                             params.m_forcedColor,
                             !params.m_stemGoesUp);
    } else {
        found = getCharacter(m_style->getFlagCharName(flagCount),
                             flagChar,
                             PlainColour,
                             !params.m_stemGoesUp);
    }

    if (!found) {

        // Handle fonts that don't have all the flags in separate characters

        if (params.m_forceColor) {
            found = getCharacter(m_style->getPartialFlagCharName(false),
                                flagChar,
                                params.m_forcedColor,
                                !params.m_stemGoesUp);
        } else {
            found = getCharacter(m_style->getPartialFlagCharName(false),
                                flagChar,
                                PlainColour,
                                !params.m_stemGoesUp);
        }

        if (!found) {
            RG_WARNING << "Warning: NotePixmapFactory::drawFlags: No way to draw note with " << flagCount << " flags in this font!?";
            return ;
        }

        QPoint hotspot = flagChar.getHotspot();

        NoteCharacter oneFlagChar;
        bool foundOne;
        if (params.m_forceColor) {
            foundOne = getCharacter(m_style->getPartialFlagCharName(true),
                                    oneFlagChar,
                                    params.m_forcedColor,
                                    !params.m_stemGoesUp);
        } else {
            foundOne = getCharacter(m_style->getPartialFlagCharName(true),
                                    oneFlagChar,
                                    PlainColour,
                                    !params.m_stemGoesUp);
        }
        
        foundOne = flagCount > 1 ? foundOne : false;


        unsigned int flagSpace = m_nd.noteBodyHeight;
        (void)m_font->getFlagSpacing(flagSpace);

        for (int flag = 0; flag < flagCount; ++flag) {

            // use flag_1 in preference to flag_0 for the final flag, so
            // as to end with a flourish
            if (flag == flagCount - 1 && foundOne)
                flagChar = oneFlagChar;

            int y = m_nd.above + s1.y();
            if (params.m_stemGoesUp)
                y += flag * flagSpace;
            else
                y -= (flag * flagSpace) + flagChar.getHeight();

/*
  if (!m_inPrinterMethod) {

  m_p->end();

  // Super-slow

  PixmapFunctions::drawPixmapMasked(*m_generatedPixmap,
  *m_generatedMask,
  m_left + s1.x() - hotspot.x(),
  y,
  flagChar.getPixmap());

  m_p->begin(m_generatedPixmap, m_generatedMask);

  } else {
*/
            // No problem with mask here
            m_p->drawNoteCharacter(m_nd.left + s1.x() - hotspot.x(),
                                   y,
                                   flagChar);
//            }
        }

    } else { // the normal case

        QPoint hotspot = flagChar.getHotspot();

        int y = m_nd.above + s1.y();
        if (!params.m_stemGoesUp)
            y -= flagChar.getHeight();

        m_p->drawNoteCharacter(m_nd.left + s1.x() - hotspot.x(), y, flagChar);
    }
}

void
NotePixmapFactory::drawStem(const NotePixmapParameters &params,
                            const QPoint &s0, const QPoint &s1,
                            int shortening)
{
    if (params.m_stemGoesUp) shortening = -shortening;

    if (params.m_forceColor) {
        m_p->painter().save();
        m_p->painter().setPen(params.m_forcedColor);
    }

    for (int i = 0; i < getStemThickness(); ++i) {
        m_p->drawLine(m_nd.left + s0.x() + i, m_nd.above + s0.y(),
                      m_nd.left + s1.x() + i, m_nd.above + s1.y() - shortening);
    }

    if (params.m_forceColor) {
        m_p->painter().restore();
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
        m_nd.above += beamSpacing + 2;

        // allow a bit extra in case the h fixpoint is non-normal
        m_nd.right = std::max(m_nd.right, params.m_width + m_nd.noteBodyWidth);

    } else {

        if (beamSpacing < 0)
            beamSpacing = 0;
        m_nd.below += beamSpacing + 2;

        m_nd.right = std::max(m_nd.right, params.m_width);
    }
}

void
NotePixmapFactory::drawShallowLine(float x0, float y0, float x1, float y1,
                                   float thickness)
{
    m_p->painter().save();
    m_p->painter().setRenderHints(QPainter::Antialiasing);

    m_p->painter().setPen(Qt::NoPen);
    if (m_selected) {
        m_p->painter().setBrush(GUIPalette::getColour(GUIPalette::SelectedElement));
    } else if (m_shaded) {
        m_p->painter().setBrush(Qt::gray);
    } else {
        m_p->painter().setBrush(Qt::black);
    }

    QPoint p[4];
    p[0] = QPoint(x0 + 0.5, y0 + 0.5);
    p[1] = QPoint(x1 + 1.5, y1 + 0.5);
    p[2] = QPoint(x1 + 1.5, y1 + thickness + 1.5);
    p[3] = QPoint(x0 + 0.5, y0 + thickness + 1.5);
    m_p->painter().drawPolygon(p, 4);

    m_p->painter().restore();
    return;
}

void
NotePixmapFactory::drawBeams(const QPoint &s1,
                             const NotePixmapParameters &params,
                             int beamCount)
{
//    clock_t startTime = clock();

    // draw beams: first we draw all the beams common to both ends of
    // the section, then we draw beams for those that appear at the
    // end only

    int startY = m_nd.above + s1.y(), startX = m_nd.left + s1.x();
    int commonBeamCount = std::min(beamCount, params.m_nextBeamCount);

    unsigned int thickness;
    (void)m_font->getBeamThickness(thickness);

    int width = params.m_width;
    double grad = params.m_gradient;
    int spacing = getLineSpacing();

    int sign = (params.m_stemGoesUp ? 1 : -1);

    if (!params.m_stemGoesUp)
        startY -= thickness;

    if (grad > -0.01 && grad < 0.01)
        startY -= sign;

    if (m_inPrinterMethod) {
        startX += getStemThickness() / 2;
    }

    for (int j = 0; j < commonBeamCount; ++j) {
        int y = sign * j * spacing;
        drawShallowLine(startX, startY + y, startX + width,
                        startY + width*grad + y,
                        thickness);
        drawBeamsBeamCount ++;
    }

    int partWidth = width / 3;
    if (partWidth < 2)
        partWidth = 2;
    else if (partWidth > m_nd.noteBodyWidth)
        partWidth = m_nd.noteBodyWidth;

    if (params.m_thisPartialBeams) {
        for (int j = commonBeamCount; j < beamCount; ++j) {
            int y = sign * j * spacing;
            drawShallowLine(startX, startY + y, startX + partWidth,
                            startY + (int)(partWidth*grad) + y,
                            thickness);
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
                            thickness);
            drawBeamsBeamCount ++;
        }
    }

//    clock_t endTime = clock();
//    drawBeamsTime += (endTime - startTime);
}

void
NotePixmapFactory::drawSlashes(const QPoint &s0,
                               const NotePixmapParameters &params,
                               int slashCount)
{
    unsigned int thickness;
    (void)m_font->getBeamThickness(thickness);
    thickness = (slashCount == 1 && m_graceSize > 0 ? thickness / 3 : thickness * 3 / 4);
    if (thickness < 1)
        thickness = 1;

    int gap = thickness - 1;
    if (gap < 1)
        gap = 1;

    //!!! I think the thickness is coming out too heavy in this rewrite, which
    // is what was really swallowing the gap, but given the number of pixel
    // level drawing problems we already have (eg. stem location is randomly off
    // by one pixel on a given user's system, but not on another's) let's just
    // leave the thickness, and expand the gap by one.  Better too much space in
    // some cases than globbing it all together illegibly.
    gap++;

    int width = m_nd.noteBodyWidth * 4 / 5;
    int sign = (params.m_stemGoesUp ? -1 : 1);

    int offset =
        ((slashCount == 1 && m_graceSize > 0) ? m_nd.noteBodyHeight * 3 / 2 :
         slashCount == 1 ? m_nd.noteBodyHeight * 2 :
         slashCount == 2 ? m_nd.noteBodyHeight * 3 / 2 :
         m_nd.noteBodyHeight);
    int y = m_nd.above + s0.y() + sign * (offset + thickness / 2);

    for (int i = 0; i < slashCount; ++i) {
        int yoff = width / 2;
        drawShallowLine(m_nd.left + s0.x() - width / 2, y + yoff / 2,
                        m_nd.left + s0.x() + width / 2 + getStemThickness(), y - yoff / 2,
                        thickness);
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
        m_nd.above = std::max(m_nd.above, -params.m_tuplingLineY + th / 2);
        m_nd.above += lineSpacing + 1;

    } else {

        if (lineSpacing < 0)
            lineSpacing = 0;
        m_nd.below = std::max(m_nd.below, params.m_tuplingLineY + th / 2);
        m_nd.below += lineSpacing + 1;
    }

    m_nd.right = std::max(m_nd.right, params.m_tuplingLineWidth);
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
    int indent = m_nd.noteBodyWidth / 2;

    if (tlw < (cr.width() + countSpace * 2 + m_nd.noteBodyWidth * 2)) {
        tlw += m_nd.noteBodyWidth - 1;
        indent = 0;
    }

    int w = (tlw - cr.width()) / 2 - countSpace;

    int startX = m_nd.left + indent;
    int endX = startX + w;

    int startY = params.m_tuplingLineY + m_nd.above + getLineSpacing() / 2;
    int endY = startY + (int)(params.m_tuplingLineGradient * w);

    if (startY == endY)
        ++thickness;

    int tickOffset = getLineSpacing() / 2;
    if (params.m_tuplingLineY >= 0)
        tickOffset = -tickOffset;

    //    NOTATION_DEBUG << "adjusted params.m_tuplingLineWidth = "
    //			 << tlw
    //			 << ", cr.width = " << cr.width()
    //			 << ", tickOffset = " << tickOffset;
    //    NOTATION_DEBUG << "line: (" << startX << "," << startY << ") -> ("
    //			 << endX << "," << endY << ")";

    if (!params.m_tuplingLineFollowsBeam) {
        m_p->drawLine(startX, startY, startX, startY + tickOffset);
        drawShallowLine(startX, startY, endX, endY, thickness);
    }

    // fix bug #1405: the tuplet numbers never displayed correctly when selected
    // or grayed, because this code snippet was always missing, I suspect
    if (m_selected) m_p->painter().setPen(GUIPalette::getColour(
                                            GUIPalette::SelectedElement));
    else if (m_shaded) m_p->painter().setPen(QColor(Qt::gray));
    else m_p->painter().setPen(QColor(Qt::black));

    m_p->painter().setFont(m_tupletCountFont);
//    if (!m_inPrinterMethod)
//        m_p->maskPainter().setFont(m_tupletCountFont);

    int textX = endX + countSpace;
    int textY = endY + cr.height() / 2;
    //    NOTATION_DEBUG << "text: (" << textX << "," << textY << ")";

    m_p->drawText(textX, textY, count);

    startX += tlw - w;
    endX = startX + w;

    startY += (int)(params.m_tuplingLineGradient * (tlw - w));
    endY = startY + (int)(params.m_tuplingLineGradient * w);

    //    NOTATION_DEBUG << "line: (" << startX << "," << startY << ") -> ("
    //			 << endX << "," << endY << ")";

    if (!params.m_tuplingLineFollowsBeam) {
        drawShallowLine(startX, startY, endX, endY, thickness);
        m_p->drawLine(endX, endY, endX, endY + tickOffset);
    }
}

void
NotePixmapFactory::drawTie(bool above, int length, int shift)
{
    int origLength = length;

    int x = m_nd.left + m_nd.noteBodyWidth + m_nd.noteBodyWidth / 4 + shift;
    length = origLength - m_nd.noteBodyWidth - m_nd.noteBodyWidth / 3 - shift;

    // if the length is short, move the tie a bit closer to both notes
    if (length < m_nd.noteBodyWidth*2) {
        x = m_nd.left + m_nd.noteBodyWidth + shift;
        length = origLength - m_nd.noteBodyWidth - shift;
    }

    if (length < m_nd.noteBodyWidth) {
        length = m_nd.noteBodyWidth;
    }

    // We can't request a smooth slur here, because that always involves
    // creating a new pixmap

    QPoint hotspot;
    drawSlurAux(length, 0, above, false, true, false, hotspot,
                &m_p->painter(),
                x,
                above ? m_nd.above : m_nd.above + m_nd.noteBodyHeight);
}

QGraphicsItem *
NotePixmapFactory::makeRest(const NotePixmapParameters &params)
{
    ++makeRestsCount;
    Profiler profiler("NotePixmapFactory::makeRest");

    CharName charName(m_style->getRestCharName(params.m_noteType,
                                               params.m_restOutsideStave));
    // Check whether the font has the glyph for this charName;
    // if not, substitute a rest-on-stave glyph for a rest-outside-stave glyph,
    // and vice-versa.
    NoteCharacter character;
    if (!getCharacter(charName, character, PlainColour, false))
        charName = m_style->getRestCharName(params.m_noteType,
                                            !params.m_restOutsideStave);

    // bool encache = false;

    if (params.m_tupletCount == 0 && !m_selected && !m_shaded &&
        !params.m_restOutsideStave) {

        if (params.m_dots == 0) {
            if (params.m_forceColor) {
                return getCharacter(charName,
                                    params.m_forcedColor, false).makeItem();
            } else {
                return getCharacter(charName, PlainColour, false).makeItem();
            }
        }
    }

    QPoint hotspot(m_font->getHotspot(charName));
    drawRestAux(params, hotspot, 0, 0, 0);

    QGraphicsPixmapItem *canvasMap = makeItem(hotspot);
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

    NoteCharacter character;
    NoteCharacter dot;

    if (params.m_forceColor) {
        NOTATION_DEBUG << "NotePixmapFactory::drawRestAux FORCE COLOR\n";
        character = getCharacter(charName, params.m_forcedColor, false);
        dot = getCharacter(NoteCharacterNames::DOT, params.m_forcedColor, false);
    } else {
        NOTATION_DEBUG << "NotePixmapFactory::drawRestAux std mode\n";
        character = getCharacter(charName,
                            params.m_quantized ? QuantizedColour : PlainColour,
                            false);
        dot = getCharacter(NoteCharacterNames::DOT, PlainColour, false);
    }

    int dotWidth = dot.getWidth();
    if (dotWidth < getNoteBodyWidth() / 2)
        dotWidth = getNoteBodyWidth() / 2;

    m_nd.above = m_nd.left = 0;
    m_nd.below = dot.getHeight() / 2; // for dotted shallow rests like semibreve
    m_nd.right = dotWidth / 2 + dotWidth * params.m_dots;
    m_nd.noteBodyWidth = character.getWidth();
    m_nd.noteBodyHeight = character.getHeight();

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
        painter->translate(x - m_nd.left, y - m_nd.above - hotspot.y());
    } else {
        createPixmap(m_nd.noteBodyWidth + m_nd.left + m_nd.right,
                     m_nd.noteBodyHeight + m_nd.above + m_nd.below);
    }

    m_p->drawNoteCharacter(m_nd.left, m_nd.above, character);

    if (params.m_tupletCount)
        drawTuplingLine(params);

    hotspot.setX(m_nd.left);
    hotspot.setY(m_nd.above + hotspot.y());

    int restY = hotspot.y() - dot.getHeight() - getStaffLineThickness();
    if (params.m_noteType == Note::Semibreve ||
        params.m_noteType == Note::Breve) {
        restY += getLineSpacing();
    }

    for (int i = 0; i < params.m_dots; ++i) {
        int x = m_nd.left + m_nd.noteBodyWidth + i * dotWidth + dotWidth / 2;
        m_p->drawNoteCharacter(x, restY, dot);
    }

    if (params.m_restOutsideStave &&
        (charName == NoteCharacterNames::MULTI_REST ||
         charName == NoteCharacterNames::MULTI_REST_ON_STAFF)) {
        drawLegerLines(params);
    }

    // a rest can have a text mark or a fermata; we permit this, so we should
    // draw this (fixes ancient bug report)
    if (params.m_marks.size() > 0) {
        NOTATION_DEBUG << "Groundbreaking.  We're drawing a fermata on a rest!  Or maybe a text mark!";
        // this next bit is the bit that isn't working yet...  we have a mark,
        // we find it, add it to restParams in NotationStaff, its m_marks.size()
        // is 1, it's passed here there and everywhere to wind up at this next
        // bit, which calls NoteFont looking for a FERMATA symbol, but never
        // draws the thing so far.
        drawMarks(false, params, 0, true);
    }

    if (painter) {
        painter->restore();
    }
}

QGraphicsPixmapItem *
NotePixmapFactory::makeClef(const Clef &clef, const ColourType colourType)
{
    Profiler profiler("NotePixmapFactory::makeClef");

    NoteCharacter plain = getCharacter(m_style->getClefCharName(clef),
                                       colourType, false);

    int oct = clef.getOctaveOffset();
    if (oct == 0) return plain.makeItem();

    // Since there was an offset, we have additional bits to draw, and must
    // match up the colour.  This bit is rather hacky, but I decided not to
    // embark on a little refactoring project to unify all of this colour
    // management under one consistent umbrella.

    // fix #1522784 and use 15 rather than 16 for double octave offset
    int adjustedOctave = (8 * (oct < 0 ? -oct : oct));
    if (adjustedOctave > 8)
        adjustedOctave--;
    else if (adjustedOctave < 8)
        adjustedOctave++;

    QString text = QString("%1").arg(adjustedOctave);
    int th = m_clefOttavaFontMetrics.height();
    int tw = m_clefOttavaFontMetrics.width(text);
    int ascent = m_clefOttavaFontMetrics.ascent();

    createPixmap(plain.getWidth(), plain.getHeight() + th);

    // The selected state is part of the ColourType enum, but it requires fluid
    // external control, and can't be managed in the same way as more static
    // colour states.  Thus we have to use m_selected to manage this case.
    if (m_selected) {
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
    } else {

        // the hackiest bit: we have to sync up the colour to the ColourType by
        // hand, because we know what these enum values really mean, but in a
        // cleaner world, this code here probably shouldn't have to know those
        // details.  Oh well.
        switch (colourType) {
        case PlainColourLight:
            m_p->painter().setPen(Qt::white);
            break;

        case ConflictColour:
            m_p->painter().setPen(Qt::red);
            break;

        case PlainColour:
        case QuantizedColour:
        case HighlightedColour:
        case TriggerColour:
        case TriggerSkipColour:
        case OutRangeColour:
        default:
            // fix bug with ottava marks not reflecting invisibility properly
            QColor plain = (m_shaded ? Qt::gray : Qt::black);
            m_p->painter().setPen(plain);
        }
    }

    m_p->drawNoteCharacter(0, oct < 0 ? 0 : th, plain);

    m_p->painter().setFont(m_clefOttavaFont);

    //!!! This fix may be turn out to be highly dependent on an individual
    // user's random system preferences.  Here on my system today and now, the
    // 8/15 above and below clef pixmaps was getting cut off.  Supplying an
    // adjustment of -2 and 4 yields just perfect results, but who knows how
    // this will turn out on somebody else's setup.
    m_p->drawText(plain.getWidth() / 2 - tw / 2,
                  ascent + (oct < 0 ? plain.getHeight() - 2 : 4), text);

    QPoint hotspot(plain.getHotspot());
    if (oct > 0) hotspot.setY(hotspot.y() + th);
    return makeItem(hotspot);
}


QGraphicsPixmapItem *
NotePixmapFactory::makeSymbol(const Symbol &symbol, const ColourType colourType)
{
    Profiler profiler("NotePixmapFactory::makeSymbol");

    NoteCharacter plain = getCharacter(m_style->getSymbolCharName(symbol),
                                       colourType, false);
    return plain.makeItem();
}

QGraphicsPixmapItem *
NotePixmapFactory::makePedalDown()
{
    return getCharacter(NoteCharacterNames::PEDAL_MARK, PlainColour, false)
        .makeItem();
}

QGraphicsPixmapItem *
NotePixmapFactory::makePedalUp()
{
    return getCharacter(NoteCharacterNames::PEDAL_UP_MARK, PlainColour, false)
        .makeItem();
}

QGraphicsPixmapItem *
NotePixmapFactory::makeUnknown()
{
    Profiler profiler("NotePixmapFactory::makeUnknown");
    return getCharacter(NoteCharacterNames::UNKNOWN, PlainColour, false)
        .makeItem();
}

QPixmap
NotePixmapFactory::makeToolbarPixmap(QString name, bool menuSize)
{
    if (menuSize && !name.startsWith("menu-")) {
        QPixmap menuMap = makeToolbarPixmap("menu-" + name, false);
        if (!menuMap.isNull()) return menuMap;
    }
    return IconLoader().loadPixmap(name);
}

QPixmap
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
    if (triplet) noteName = "3-" + noteName;
    noteName = "menu-" + noteName;
    return makeToolbarPixmap(noteName.toLocal8Bit().data(), true);
}

QPixmap
NotePixmapFactory::makeMarkMenuPixmap(Mark mark)
{
    if (mark == Marks::Sforzando || mark == Marks::Rinforzando) {
        return makeToolbarPixmap( mark.c_str(), true );
    } else {
        NoteFont *font = nullptr;
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
        return character.getPixmap();
    }
}

QGraphicsPixmapItem *
NotePixmapFactory::makeKey(const Key &key,
                           const Clef &clef,
                           Key previousKey,
                           const ColourType colourType)
{
//    NOTATION_DEBUG << "NPF: makeKey(key: " << key.getName() << ", clef: "
//              <<  clef.getClefType() << " , prevKey: "
//              << previousKey.getName() <<  ", colType: " << (int)colourType
//              << ")";

    Profiler profiler("NotePixmapFactory::makeKey");

    std::vector<int> ah0 = previousKey.getAccidentalHeights(clef);
    std::vector<int> ah1 = key.getAccidentalHeights(clef);

    int cancelCount = 0;
    if (key.isSharp() != previousKey.isSharp())
        cancelCount = ah0.size();
    else if (ah1.size() < ah0.size())
        cancelCount = ah0.size() - ah1.size();

    CharName keyCharName;
    if (key.isSharp()) keyCharName = NoteCharacterNames::SHARP;
    else keyCharName = NoteCharacterNames::FLAT;

    NoteCharacter keyCharacter;
    NoteCharacter cancelCharacter;

    keyCharacter = getCharacter(keyCharName, colourType, false);
    if (cancelCount > 0) {
        cancelCharacter = getCharacter(NoteCharacterNames::NATURAL, colourType, false);
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

    createPixmap(keyDelta * ah1.size() + cancelDelta * cancelCount + between +
                 keyCharacter.getWidth() / 4,
                 lw * 8 + 1);

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

    return makeItem(QPoint());
}

QPixmap
NotePixmapFactory::makeClefDisplayPixmap(const Clef &clef,
                                         const ColourType colourType)
{
    QGraphicsPixmapItem *clefItem = makeClef(clef, colourType);

    int lw = getLineSpacing();
    int width = clefItem->pixmap().width() + 6 * getNoteBodyWidth();

    createPixmap(width, lw * 10 + 1);

    int h = clef.getAxisHeight();
    int y = (lw * 3) + ((8 - h) * lw) / 2;
    int x = 3 * getNoteBodyWidth();
    m_p->drawPixmap(x, y + clefItem->offset().y(), clefItem->pixmap());

    // error: ‘QColour’ was not declared in this scope
    //
    // Rrrrrrrrrrrrr!!!

    // we'll just use the colourType to figure out what color--yes, I said COLOR,
    // get over it you stiff-lipped Brits--to draw the lines
    QColor lines;
    switch (colourType) {
    case PlainColourLight:
        lines = Qt::white;    
        break;
    case ConflictColour:
    case HighlightedColour:
    case OutRangeColour:
    case PlainColour:
    case QuantizedColour:
    case TriggerColour:
    case TriggerSkipColour:
    default:
        lines = Qt::black;
    }

    m_p->painter().setPen(lines);

    for (h = 0; h <= 8; h += 2) {
        y = (lw * 3) + ((8 - h) * lw) / 2;
        m_p->drawLine(x / 2, y, m_generatedWidth - x / 2 - 1, y);
    }

    delete clefItem;
    return makePixmap();
}

QPixmap
NotePixmapFactory::makeKeyDisplayPixmap(const Key &key, const Clef &clef,
                                        const ColourType colourType)
{
    std::vector<int> ah = key.getAccidentalHeights(clef);

    CharName charName = (key.isSharp() ?
                         NoteCharacterNames::SHARP :
                         NoteCharacterNames::FLAT);

    QGraphicsPixmapItem *clefItem = makeClef(clef, colourType);

    NoteCharacter ch(getCharacter(charName, colourType, false));
    QPixmap accidentalPixmap = ch.getPixmap();

    QPoint hotspot(m_font->getHotspot(charName));

    int lw = getLineSpacing();
    int delta = accidentalPixmap.width() - hotspot.x();
    int maxDelta = getAccidentalWidth(Sharp);
    int width = clefItem->pixmap().width() + 5 * maxDelta + 7 * maxDelta;
    int x = clefItem->pixmap().width() + 5 * maxDelta / 2;

    createPixmap(width, lw * 10 + 1);

    int h = clef.getAxisHeight();
    int y = (lw * 3) + ((8 - h) * lw) / 2;
    m_p->drawPixmap(2 * maxDelta, y + clefItem->offset().y(), clefItem->pixmap());

    for (unsigned int i = 0; i < ah.size(); ++i) {

        h = ah[i];
        y = (lw * 3) + ((8 - h) * lw) / 2 - hotspot.y();

        m_p->drawPixmap(x, y, accidentalPixmap);

        x += delta;
    }

    // use the colourType to figure out what kuller to draw the lines with
    QColor kuller;

    switch (colourType) {
    case PlainColourLight:
        kuller = Qt::white;    
        break;
    case ConflictColour:
    case HighlightedColour:
    case OutRangeColour:
    case PlainColour:
    case QuantizedColour:
    case TriggerColour:
    case TriggerSkipColour:
    default:
        kuller = Qt::black;
    }

    m_p->painter().setPen(kuller);
    m_p->painter().setBrush(kuller);

    for (h = 0; h <= 8; h += 2) {
        y = (lw * 3) + ((8 - h) * lw) / 2;
        m_p->drawLine(maxDelta, y, m_generatedWidth - 2*maxDelta - 1, y);
    }

    delete clefItem;
    return makePixmap();
}

int
NotePixmapFactory::getClefAndKeyWidth(const Key &key, const Clef &clef)
{
    std::vector<int> ah = key.getAccidentalHeights(clef);
    Accidental accidental = key.isSharp() ? Sharp : Flat;
    NoteCharacter plain = getCharacter(m_style->getClefCharName(clef),
                                       PlainColour, false);

    int clefWidth = plain.getWidth();
    int accWidth = getAccidentalWidth(accidental);
    int maxDelta = getAccidentalWidth(Sharp);

    int width = clefWidth + 2 * maxDelta + ah.size() * accWidth;

    return width;
}

int
NotePixmapFactory::getTrackHeaderNTL(int height)
{
    int clefMaxHeight = 12 * getLineSpacing();
    int textLineHeight = getTrackHeaderTextLineSpacing();
    int numberOfLines = ((height - clefMaxHeight) / 2) / textLineHeight;
    return (numberOfLines > 0) ? numberOfLines : 1;
}

int
NotePixmapFactory::getTrackHeaderTextWidth(QString str)
{
    QRect bounds = m_trackHeaderFontMetrics.boundingRect(str);
    return bounds.width();
}

int
NotePixmapFactory::getTrackHeaderTextLineSpacing()
{
    // 3/2 is some arbitrary line spacing
    return m_trackHeaderFont.pixelSize() * 3 / 2;
}

QString
NotePixmapFactory::getOneLine(QString &text, int width)
{
    QString str;
    int n;

    // Immediately stop if string is empty or only contains white spaces ...
    if (text.trimmed().isEmpty()) return QString("");

    // ... or if width is too small.
    if (width < m_trackHeaderFontMetrics.boundingRect(text.left(1)).width())
        return QString("");

    // Get a first approx. string length
    int totalLength = text.length();
    n = totalLength * width / getTrackHeaderTextWidth(text) + 1;
    if (n > totalLength) n = totalLength;

    // Verify string size is less than width then correct it if necessary
    while (((getTrackHeaderTextWidth(text.left(n))) > width) && n) n--;

    if (n == 0) {
        str = text;
        text = QString("");
    } else {
        str = text.left(n);
        text.remove(0, n);
    }

    return str;
}

QPixmap
NotePixmapFactory::makePitchDisplayPixmap(int p, const Clef &clef,
                                          bool useSharps,
                                          const ColourType colourType)
{
    NotationRules rules;

    Pitch pitch(p);
    Accidental accidental(pitch.getAccidental(useSharps));
    NotePixmapParameters params(Note::Crotchet, 0, accidental);

    QGraphicsPixmapItem *clefItem = makeClef(clef, colourType);

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

    // use the colourType to figure out what kuller to draw the lines and note
    // with
    QColor kuller;

    switch (colourType) {
    case PlainColourLight:
        kuller = Qt::white;    
        break;
    case ConflictColour:
    case HighlightedColour:
    case OutRangeColour:
    case PlainColour:
    case QuantizedColour:
    case TriggerColour:
    case TriggerSkipColour:
    default:
        kuller = Qt::black;
    }
    int hue, saturation, value;
    kuller.getHsv(&hue, &saturation, &value);

    // I can't think of any real use for the ability to draw all notation in
    // white, and given the complexity of adding that ability to all the various
    // bits and bobs called upon to draw a note, what we're going to do instead
    // is draw a conventional black note with unaltered bits and bobs, and then
    // recolour the resulting pixmap for this dialog-oriented display method

    QGraphicsPixmapItem *noteItem = makeNotePixmapItem(params);
    QPixmap colouredNote(PixmapFunctions::colourPixmap(noteItem->pixmap(), hue, value, saturation));
    noteItem->setPixmap(colouredNote);

    int pixmapHeight = lw * 12 + 1;
    int yoffset = lw * 3;
    if (h > 12) {
        pixmapHeight += 6 * lw;
        yoffset += 6 * lw;
    } else if (h < -4) {
        pixmapHeight += 6 * lw;
    }

    createPixmap(width, pixmapHeight);

    m_p->painter().setPen(kuller);
    m_p->painter().setBrush(kuller);

    int x =
        getClefWidth(Clef::Bass) + 5 * getNoteBodyWidth() -
        getAccidentalWidth(accidental);
    int y = yoffset + ((8 - h) * lw) / 2 + noteItem->offset().y();
    m_p->drawPixmap(x, y, noteItem->pixmap());

    h = clef.getAxisHeight();
    x = 3 * getNoteBodyWidth();
    y = yoffset + ((8 - h) * lw) / 2;
    m_p->drawPixmap(x, y + clefItem->offset().y(), clefItem->pixmap());

    m_p->painter().setPen(kuller);
    m_p->painter().setBrush(kuller);

    for (h = 0; h <= 8; h += 2) {
        y = yoffset + ((8 - h) * lw) / 2;
        m_p->drawLine(x / 2, y, m_generatedWidth - x / 2, y);
    }

    delete noteItem;
    delete clefItem;

    return makePixmap();
}

QPixmap
NotePixmapFactory::makePitchDisplayPixmap(int p, const Clef &clef,
                                          int octave, int step,
                                          const ColourType colourType)
{
    NotationRules rules;

    Pitch pitch(step, octave, p, 0);
    Accidental accidental = pitch.getDisplayAccidental(Key("C major"));
    NotePixmapParameters params(Note::Crotchet, 0, accidental);

    QGraphicsPixmapItem *clefItem = makeClef(clef, colourType);

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

    // use the colourType to figure out what kuller to draw the lines and note
    // with
    QColor kuller;

    switch (colourType) {
    case PlainColourLight:
        kuller = Qt::white;    
        break;
    case ConflictColour:
    case HighlightedColour:
    case OutRangeColour:
    case PlainColour:
    case QuantizedColour:
    case TriggerColour:
    case TriggerSkipColour:
    default:
        kuller = Qt::black;
    }
    int hue, saturation, value;
    kuller.getHsv(&hue, &saturation, &value);

    //!!! NOTE: I started this white on gray notation for dialogs thing on a
    // whim, and I tore it down bit by bit doing everything else before arriving
    // here in the "draw the notes" code.  The "draw the notes" code was so
    // involved to doctor that I sensibly just used the existing code unaltered,
    // and colored the resulting pixmap.  This suggests that it might be more
    // consistent to use this approach everywhere else, instead of passing
    // around an optional ColourType here, but not there, and then again over
    // here.  It's really a mixed bag which methods got the optional parameter,
    // and which ones didn't, whittled down bit by bit solving the individual
    // problems of making notation displayed in dialog contexts draw itself in
    // white.  Now that I can see the forest for the trees, this annoys me, and
    // I vow to fix all of this one day soon.  But not today.
    //
    // Years later, and still not today!  dmm
    //!!!

    // I can't think of any real use for the ability to draw all notation in
    // white, and given the complexity of adding that ability to all the various
    // bits and bobs called upon to draw a note, what we're going to do instead
    // is draw a conventional black note with unaltered bits and bobs, and then
    // recolour the resulting pixmap for this dialog-oriented display method
    QGraphicsPixmapItem *noteItem = makeNotePixmapItem(params);
    QPixmap colouredNote(PixmapFunctions::colourPixmap(noteItem->pixmap(), hue, value, saturation));
    noteItem->setPixmap(colouredNote);

    int pixmapHeight = lw * 12 + 1;
    int yoffset = lw * 3;
    if (h > 12) {
        pixmapHeight += 6 * lw;
        yoffset += 6 * lw;
    } else if (h < -4) {
        pixmapHeight += 6 * lw;
    }

    createPixmap(width, pixmapHeight);
    m_p->painter().setPen(kuller);
    m_p->painter().setBrush(kuller);


    int x =
        getClefWidth(Clef::Bass) + 5 * getNoteBodyWidth() -
        getAccidentalWidth(accidental);
    int y = yoffset + ((8 - h) * lw) / 2 + noteItem->offset().y();
    m_p->drawPixmap(x, y, noteItem->pixmap());

    h = clef.getAxisHeight();
    x = 3 * getNoteBodyWidth();
    y = yoffset + ((8 - h) * lw) / 2;
    m_p->drawPixmap(x, y + clefItem->offset().y(), clefItem->pixmap());

    m_p->painter().setPen(kuller);
    m_p->painter().setBrush(kuller);

    for (h = 0; h <= 8; h += 2) {
        y = yoffset + ((8 - h) * lw) / 2;
        m_p->drawLine(x / 2, y, m_generatedWidth - x / 2, y);
    }

    delete noteItem;
    delete clefItem;

    return makePixmap();
}

QGraphicsPixmapItem *
NotePixmapFactory::makeHairpin(int length, bool isCrescendo)
{
    Profiler profiler("NotePixmapFactory::makeHairpin");
    drawHairpinAux(length, isCrescendo, 0, 0, 0);
    return makeItem(QPoint(0, m_generatedHeight / 2));
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

    //    NOTATION_DEBUG << "NotePixmapFactory::makeHairpinPixmap: mapped length " << length << " to height " << height << " (nbh = " << nbh << ", nbw = " << nbw << ")";

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
        createPixmap(length, height);
    }

    if (m_selected) {
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
    }

    int left = 1, right = length - 2 * nbw / 3 + 1;

    if (isCrescendo) {
        drawShallowLine(left, height / 2 - 1,
                        right, height - thickness - 1, thickness);
        drawShallowLine(left, height / 2 - 1, right, 0, thickness);
    } else {
        drawShallowLine(left, 0, right, height / 2 - 1, thickness);
        drawShallowLine(left, height - thickness - 1,
                        right, height / 2 - 1, thickness);
    }

    m_p->painter().setPen(QColor(Qt::black));

    if (painter) {
        painter->restore();
    }
}

QGraphicsPixmapItem *
NotePixmapFactory::makeSlur(int length, int dy, bool above, bool phrasing)
{
    Profiler profiler("NotePixmapFactory::makeSlur");

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

        QImage i = m_generatedPixmap->toImage();
        if (i.depth() == 1) i = i.convertToFormat(QImage::Format_ARGB32);
        i = i.scaled(i.width() / 2, i.height() / 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        delete m_generatedPixmap;
//        delete m_generatedMask;
        QPixmap newPixmap = QPixmap::fromImage(i);
        QGraphicsPixmapItem *p = new QGraphicsPixmapItem(newPixmap);
        p->setOffset(QPointF(-hotspot.x(), -hotspot.y()));
//!!!        p->setMask(PixmapFunctions::generateMask(newPixmap,
//                   QColor(Qt::white).rgb()));
        return p;

    } else {

        QGraphicsPixmapItem *p = new QGraphicsPixmapItem(*m_generatedPixmap);
        p->setOffset(QPointF(-hotspot.x(), -hotspot.y()));
//!!!        p->setMask(PixmapFunctions::generateMask(*m_generatedPixmap,
//                   QColor(Qt::white).rgb()));
        delete m_generatedPixmap;
//        delete m_generatedMask;
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
//     QMatrix::TransformationMode mode = QMatrix::transformationMode();	//&&&
//     QMatrix::setTransformationMode(QMatrix::Points);

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
        //	NOTATION_DEBUG << "slur: dy is " << dy << ", length " << length << ", rotating through " << theta;
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
    if (flat) my = my * 2 / 3;
    else if (phrasing) my = my * 3 / 4;
    if (!above) my = -my;

    bool havePixmap = false;
    QPoint topLeft, bottomRight;

    if (smooth) thickness += 2;

    for (int i = 0; i < thickness; ++i) {

        //!!! use QPainterPath::cubicTo

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

            //	    NOTATION_DEBUG << "slur: bottomRight (" << bottomRight.x() << "," << bottomRight.y() << "), topLeft (" << topLeft.x() << "," << topLeft.y() << "), width " << width << ", height " << height << ", hotspot (" << hotspot.x() << "," << hotspot.y() << "), dy " << dy << ", thickness " << thickness;

            if (painter) {

                // This conditional is because we're also called with
                // a painter arg from non-printer drawTie.  It's a big
                // hack.

                if (m_inPrinterMethod) {
                    painter->save();
                    m_p->beginExternal(painter);
                    painter->translate(x, y);
                    if (rotate) {
                        painter->rotate(theta);
                    }
                } else {
                    m_p->painter().save();
                    m_p->painter().translate(x, y);
                    if (rotate) {
                        m_p->painter().rotate(theta);
                    }
                }

            } else {
                createPixmap(smooth ? width*2 + 1 : width,
                             smooth ? height*2 + thickness*2 : height + thickness);

                QMatrix m;
                if (smooth) {
                    m.translate(2 * hotspot.x(), 2 * hotspot.y());
                } else {
                    m.translate(hotspot.x(), hotspot.y());
                }
                m.rotate(theta);
                m_p->painter().setWorldMatrix(m);
            }

            if (m_selected)
                m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
            else if (m_shaded) {
                m_p->painter().setPen(QColor(Qt::gray));
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
        QPolygon qp(ppc);

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
        m_p->painter().setPen(QColor(Qt::black));
    }

//     QMatrix::setTransformationMode(mode);	//&&&

    if (painter) {
        painter->restore();
//        if (!m_inPrinterMethod)
//            m_p->maskPainter().restore();
    }
}

QGraphicsPixmapItem *
NotePixmapFactory::makeOttava(int length, int octavesUp)
{
    Profiler profiler("NotePixmapFactory::makeOttava");
    m_inPrinterMethod = false;
    drawOttavaAux(length, octavesUp, 0, 0, 0);
    return makeItem(QPoint(0, m_generatedHeight - 1));
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
        if (octavesUp == 2) label = "15ma  ";
        else label = "15mb  ";
        backpedal = m_ottavaFontMetrics.width("15") / 2;
    } else {
        if (octavesUp == 1) label = "8va  ";
        else label = "8vb  ";
        backpedal = m_ottavaFontMetrics.width("8") / 2;
    }

    int width = length + backpedal;

    if (painter) {
        painter->save();
        m_p->beginExternal(painter);
        painter->translate(x - backpedal, y - height);
    } else {
        NOTATION_DEBUG << "NotePixmapFactory::drawOttavaAux: making pixmap and mask " << width << "x" << height;
        createPixmap(width, height);
    }

    int thickness = getStemThickness();
    QPen pen(QColor(Qt::black), thickness, Qt::DotLine);

    if (m_selected) {
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
        pen.setColor(GUIPalette::getColour(GUIPalette::SelectedElement));
    } else if (m_shaded) {
        m_p->painter().setPen(QColor(Qt::gray));
        pen.setColor(QColor(Qt::gray));
    }

    m_p->painter().setFont(m_ottavaFont);
//    if (!m_inPrinterMethod)
//        m_p->maskPainter().setFont(m_ottavaFont);

    m_p->drawText(0, m_ottavaFontMetrics.ascent(), label);

    m_p->painter().setPen(pen);
    //    if (!m_inPrinterMethod) m_p->maskPainter().setPen(pen);

    int x0 = m_ottavaFontMetrics.width(label) + thickness;
    int x1 = width - thickness;
    int y0 = m_ottavaFontMetrics.ascent() * 2 / 3 - thickness / 2;
    int y1 = (octavesUp < 0 ? 0 : m_ottavaFontMetrics.ascent());

    NOTATION_DEBUG << "NotePixmapFactory::drawOttavaAux: drawing " << x0 << "," << y0 << " to " << x1 << "," << y0 << ", thickness " << thickness;

    m_p->drawLine(x0, y0, x1, y0);

    pen.setStyle(Qt::SolidLine);
    m_p->painter().setPen(pen);
    //    if (!m_inPrinterMethod) m_p->maskPainter().setPen(pen);

    NOTATION_DEBUG << "NotePixmapFactory::drawOttavaAux: drawing " << x1 << "," << y0 << " to " << x1 << "," << y1 << ", thickness " << thickness;

    m_p->drawLine(x1, y0, x1, y1);

    m_p->painter().setPen(QPen());
//    if (!m_inPrinterMethod)
//        m_p->maskPainter().setPen(QPen());

    if (painter) {
        painter->restore();
    }
}

QGraphicsPixmapItem *
NotePixmapFactory::makeTrillLine(int length)
{
    drawTrillLineAux(length, 0, 0, 0);
    return makeItem(QPoint(0, m_generatedHeight / 2));
}

void
NotePixmapFactory::drawTrillLineAux(int length, QPainter *painter, int x, int y)
{
    // arbitrary gap calculation from elsewhere
    int gap = m_nd.noteBodyHeight / 5 + 1;

    // make a character for the tr bit first, so we can use its height
    NoteCharacter character(getCharacter(m_style->getMarkCharName(Marks::Trill),
                                         PlainColour, false));
    int height = character.getHeight();

    // create a pixmap of a suitable width and height to contain our tr~~~
    //
    // I think the "if (painter)" bit is a block-copied hold-over that shouldn't
    // be necessary in new code, but I'm not completely sure.  A lot of my
    // drawing-related code is still lucky guesswork more than method
    if (painter) {
        painter->save();
        m_p->beginExternal(painter);
        painter->translate(x, y - height / 2);
    } else {
        createPixmap(length, height * 2);
    }

    if (m_selected) {
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
    }

    // draw the tr
    m_p->drawNoteCharacter(x, y, character);
    // increment x so we start drawing after the tr; 3 is an arbitrary figure
    // that seems about right, but I'm only testing with one font at one size
    x += character.getWidth() + gap;
    
 
    // make a character for the /\/\/\/ bit, and keep drawing it at spaced
    // increments until one glyph width just before running out of space, to
    // avoid clipping the last glyph
    NoteCharacter extension;
    if (getCharacter(NoteCharacterNames::TRILL_LINE, extension,
                     PlainColour, false)) {
        x += extension.getHotspot().x();

        // align the /\/\ bit with the center of the tr bit; assumes the tr bit
        // is always taller
        y = ((character.getHeight() - extension.getHeight()) / 2 );

        while (x < (length - extension.getWidth())) {
            x -= extension.getHotspot().x();
            m_p->drawNoteCharacter(x, y, extension);
            x += extension.getWidth();
        }
    }

    m_p->painter().setPen(QColor(Qt::black));

    if (painter) {
        painter->restore();
    }
}

void
NotePixmapFactory::drawBracket(int length, bool left, bool /*curly*/, int x, int y)
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
         ;
        */
        Spline::PointList *polyPoints = Spline::calculate
            (QPoint(off0, 0), QPoint(off0, length - 1), pl, topLeft, bottomRight);

        int ppc = polyPoints->size();
        QPolygon qp(ppc);
        /*
          NOTATION_DEBUG << "bracket spline polypoints: ";
          for (int j = 0; j < ppc; ++j) {
          NOTATION_DEBUG << (*polyPoints)[j].x() << "," << (*polyPoints)[j].y();
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

QGraphicsPixmapItem *
NotePixmapFactory::makeTimeSig(const TimeSignature& sig)
{
    Profiler profiler("NotePixmapFactory::makeTimeSig");

    if (sig.isCommon()) {

        NoteCharacter character;

        CharName charName;
        if (sig.getNumerator() == 2) {
            charName = NoteCharacterNames::CUT_TIME;
        } else {
            charName = NoteCharacterNames::COMMON_TIME;
        }

        if (getCharacter(charName, character, PlainColour, false)) {
            createPixmap(character.getWidth(), character.getHeight());
            m_p->drawNoteCharacter(0, 0, character);
            return makeItem(QPoint(0, character.getHeight() / 2));
        }

        QString c("c");
        QRect r = m_bigTimeSigFontMetrics.boundingRect(c);

        int dy = getLineSpacing() / 4;
        createPixmap(r.width(), r.height() + dy*2);

        if (m_selected) {
            m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
        } else if (m_shaded) {
            m_p->painter().setPen(QColor(Qt::gray));
        }

        m_p->painter().setFont(m_bigTimeSigFont);
//        if (!m_inPrinterMethod)
//            m_p->maskPainter().setFont(m_bigTimeSigFont);

        m_p->drawText(0, r.height() + dy, c);

        if (sig.getNumerator() == 2) { // cut common

            int x = r.width() * 3 / 5 - getStemThickness();

            for (int i = 0; i < getStemThickness() * 2; ++i, ++x) {
                m_p->drawLine(x, 0, x, r.height() + dy*2 - 1);
            }
        }

        m_p->painter().setPen(QColor(Qt::black));
        return makeItem(QPoint(0, r.height() / 2 + dy));

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

            createPixmap(width, height);

            for (int i = 0; i < numS.length(); ++i) {
                int x = width - (width - numW) / 2 - (i + 1) * character.getWidth();
                int y = height / 4 - (character.getHeight() / 2);
                NoteCharacter charCharacter = getCharacter
                    (m_style->getTimeSignatureDigitName(numerator % 10),
                     PlainColour, false);
                m_p->drawNoteCharacter(x, y, charCharacter);
                numerator /= 10;
            }

            for (int i = 0; i < denomS.length(); ++i) {
                int x = width - (width - denomW) / 2 - (i + 1) * character.getWidth();
                int y = height - height / 4 - (character.getHeight() / 2);
                NoteCharacter charCharacter = getCharacter
                    (m_style->getTimeSignatureDigitName(denominator % 10),
                     PlainColour, false);
                m_p->drawNoteCharacter(x, y, charCharacter);
                denominator /= 10;
            }

            return makeItem(QPoint(0, height / 2));
        }

        QRect numR = m_timeSigFontMetrics.boundingRect(numS);
        QRect denomR = m_timeSigFontMetrics.boundingRect(denomS);
        int width = std::max(numR.width(), denomR.width()) + 2;
        int x;

        createPixmap(width, denomR.height() * 2 + getNoteBodyHeight());

        if (m_selected) {
            m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
        } else if (m_shaded) {
            m_p->painter().setPen(QColor(Qt::gray));
        }

        m_p->painter().setFont(m_timeSigFont);
//        if (!m_inPrinterMethod)
//            m_p->maskPainter().setFont(m_timeSigFont);

        x = (width - numR.width()) / 2 - 1;
        m_p->drawText(x, denomR.height(), numS);

        x = (width - denomR.width()) / 2 - 1;
        m_p->drawText(x, denomR.height() * 2 + (getNoteBodyHeight() / 2) - 1, denomS);

        m_p->painter().setPen(QColor(Qt::black));

        return makeItem(QPoint(0, denomR.height() +
                               (getNoteBodyHeight() / 4) - 1));
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
    
    QSettings settings;
    //@@@ JAS Check here first for errors.  Added .beginGroup()
    settings.beginGroup( NotationViewConfigGroup );

    QFont textFont;

    if (serif) {
        textFont = QFont(defaultSerifFontFamily);
        textFont = settings.value("textfont", textFont).toString();
    } else {
        textFont = QFont(defaultSansSerifFontFamily);
        textFont = settings.value("sansfont", textFont).toString();
    }
    settings.endGroup();

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
     		   << " for type " << type;
    
    NOTATION_DEBUG << "NotePixmapFactory::getTextFont: returning font '"
                   << textFont.toString() << "' for type " << type.c_str()
                   << " text : " << text.getText().c_str();

    m_textFontCache[type.c_str()] = textFont;
    return textFont;
}

QPixmap
NotePixmapFactory::makeTextPixmap(const Text &text)
{
    QGraphicsPixmapItem *item = makeText(text);
    QPixmap map = item->pixmap();
    delete item;
    return map;
}

QGraphicsPixmapItem *
NotePixmapFactory::makeText(const Text &text)
{
    Profiler profiler("NotePixmapFactory::makeText");

    std::string type(text.getTextType());

    if (type == Text::Annotation ||
        type == Text::LilyPondDirective) {
        return makeAnnotation(text, (type == Text::LilyPondDirective));
    }

    drawTextAux(text, 0, 0, 0);
    return makeItem(QPoint(2, 2));
}

QGraphicsPixmapItem *
NotePixmapFactory::makeGuitarChord(const Guitar::Fingering &fingering,
                                   int x,
                                   int y)
{
    using namespace Guitar;
    Profiler profiler("NotePixmapFactory::makeGuitarChord");

    int guitarChordWidth = getLineSpacing() * 6;
    int guitarChordHeight = getLineSpacing() * 6;

    createPixmap(guitarChordWidth, guitarChordHeight);

    if (m_selected) {
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
        m_p->painter().setBrush(GUIPalette::getColour(GUIPalette::SelectedElement));
    } else {
        m_p->painter().setPen(QColor(Qt::black));
        m_p->painter().setBrush(QColor(Qt::black));
    }
    
    Guitar::NoteSymbols ns(Guitar::Fingering::DEFAULT_NB_STRINGS, FingeringBox::DEFAULT_NB_DISPLAYED_FRETS);
    Guitar::NoteSymbols::drawFingeringPixmap(fingering, ns, &(m_p->painter()));

    return makeItem(QPoint (x, y));
}

void
NotePixmapFactory::drawText(const Text &text,
                            QPainter &painter, int x, int y)
{
    Profiler profiler("NotePixmapFactory::drawText");

    //     NOTATION_DEBUG << "NotePixmapFactory::drawText() " << text.getText().c_str()
    //                    << " - type : " << text.getTextType().c_str();

    std::string type(text.getTextType());

    if (type == Text::Annotation ||
        type == Text::LilyPondDirective) {
        QGraphicsPixmapItem *map = makeAnnotation(text, (type == Text::LilyPondDirective));
        painter.drawPixmap(x, y, map->pixmap());
        delete map;
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
        createPixmap(width, height);
    }

    if (m_selected)
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
    else if (m_shaded)
        m_p->painter().setPen(QColor(Qt::gray));

    m_p->painter().setFont(textFont);
//    if (!m_inPrinterMethod)
//        m_p->maskPainter().setFont(textFont);

    m_p->drawText(offset, textMetrics.ascent() + offset, s);

    m_p->painter().setPen(QColor(Qt::black));

    if (painter) {
        painter->restore();
    }
}

QGraphicsPixmapItem *
NotePixmapFactory::makeAnnotation(const Text &text)
{
    return makeAnnotation(text, false);
}

QGraphicsPixmapItem *
NotePixmapFactory::makeAnnotation(const Text &text, const bool isLilyPondDirective)
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
        (0, 0, annotationWidth, annotationHeight, Qt::TextWordWrap, s);

    int pixmapWidth = r.width() + sideGap * 2;
    int pixmapHeight = r.height() + topGap + bottomGap;

    createPixmap(pixmapWidth, pixmapHeight);

    if (m_selected)
        m_p->painter().setPen(GUIPalette::getColour(GUIPalette::SelectedElement));
    else if (m_shaded)
        m_p->painter().setPen(QColor(Qt::gray));

    m_p->painter().setFont(textFont);
//    if (!m_inPrinterMethod)
//        m_p->maskPainter().setFont(textFont);

    if (isLilyPondDirective) {
        m_p->painter().setBrush(GUIPalette::getColour(GUIPalette::TextLilyPondDirectiveBackground));
    } else {
        m_p->painter().setBrush(GUIPalette::getColour(GUIPalette::TextAnnotationBackground));
    }

    m_p->drawRect(0, 0, pixmapWidth, pixmapHeight);

    m_p->painter().setBrush(QColor(Qt::black));
    m_p->painter().drawText(QRect(sideGap, topGap,
                                  annotationWidth + sideGap,
                                  pixmapHeight - bottomGap),
                            Qt::TextWordWrap, s);

    /* unnecessary following the rectangle draw
       m_pm.drawText(QRect(sideGap, topGap,
       annotationWidth + sideGap, annotationHeight + topGap),
       Qt::TextWordWrap, s);
    */

    return makeItem(QPoint(0, 0));
}

void
NotePixmapFactory::createPixmap(int width, int height)
{
    if (width == 0 || height == 0) {
        RG_WARNING << "NotePixmapFactory::createPixmap: WARNING: invalid size " << width << "x" << height;
        m_generatedPixmap = new QPixmap();
        return;
    }

    m_generatedWidth = width;
    m_generatedHeight = height;
    m_generatedPixmap = new QPixmap(width, height);
    m_generatedPixmap->fill(Qt::transparent);

    // initiate painting
    NOTATION_DEBUG << "NotePixmapFactory::createPixmap(" << width << "," << height << "): about to begin painter";
    m_p->begin(m_generatedPixmap);

}

QGraphicsPixmapItem *
NotePixmapFactory::makeItem(QPoint hotspot)
{
//    NOTATION_DEBUG << "NotePixmapFactory::makeItem(" << hotspot << ")";

    if (!m_generatedPixmap->isNull()) {
        m_p->end();
    }// else NOTATION_DEBUG << "m_generatedPixmap was nullptr!";

    QGraphicsPixmapItem *p = new QGraphicsPixmapItem;

    p->setPixmap(*m_generatedPixmap);
    p->setOffset(QPointF(-hotspot.x(), -hotspot.y()));

    // The hit test QGraphicsScene::items(), called by NotationScene::setupMouseEvent,
    // calls contains() which calls shape(), which by default generates a mask (slow!).
    // Change the shape mode to BoundingRectShape to be able to click on text items anywhere
    // in the bounding rect, rather than having to aim for a black pixel.
    p->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);

//    NOTATION_DEBUG << "NotePixmapFactory::makeItem: item = " << p << " (scene = " << p->scene() << ")";

    delete m_generatedPixmap;
    return p;
}

QPixmap
NotePixmapFactory::makePixmap()
{
    if (!m_generatedPixmap->isNull()) {
        m_p->end();
    }

    QPixmap p = *m_generatedPixmap;
    delete m_generatedPixmap;
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
    // use the full font for this unless a grace size was supplied in ctor
    NoteFont *font = (m_haveGrace ? m_graceFont : m_font);

    NoteFont::CharacterType charType =
        m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen;

    if (m_selected) {
        return font->getCharacterColoured
            (name,
             GUIPalette::SelectedElementHue,
             GUIPalette::SelectedElementMinValue,
             ch, charType, inverted);
    }

    QColor white = Qt::white;
    QColor red = Qt::red;
    QColor gray = Qt::gray;
    QColor magenta = Qt::magenta;

    int h, s, v;

    // getCharacterShaded() has been removed and replaced with a call to
    // getCharacterColoured() using Qt::gray, in order to preserve the
    // antialiasing of the character, and avoid blowing it out
    if (m_shaded) {
        gray.getHsv(&h, &s, &v);
        return font->getCharacterColoured
            (name,
             h,
             v,
             ch, charType, inverted, s);
    }

    switch (type) {

    case PlainColour:
        return font->getCharacter(name, ch, charType, inverted);

    case QuantizedColour:
        return font->getCharacterColoured
            (name,
             GUIPalette::QuantizedNoteHue,
             GUIPalette::QuantizedNoteMinValue,
             ch, charType, inverted);

    case HighlightedColour:
        return font->getCharacterColoured
            (name,
             GUIPalette::HighlightedElementHue,
             GUIPalette::HighlightedElementMinValue,
             ch, charType, inverted);

    case TriggerColour:
        return font->getCharacterColoured
            (name,
             GUIPalette::TriggerNoteHue,
             GUIPalette::TriggerNoteMinValue,
             ch, charType, inverted);

    case TriggerSkipColour:
        return font->getCharacterColoured
            (name,
             GUIPalette::TriggerSkipHue,
             GUIPalette::TriggerSkipMinValue,
             ch, charType, inverted);

    case OutRangeColour:
        return font->getCharacterColoured
            (name,
             GUIPalette::OutRangeNoteHue,
             GUIPalette::OutRangeNoteMinValue,
             ch, charType, inverted);

    case PlainColourLight:
        white.getHsv(&h, &s, &v);
        return font->getCharacterColoured
            (name,
             h,
             v,
             ch, charType, inverted, s);  

    case ConflictColour:
        red.getHsv(&h, &s, &v);
        return font->getCharacterColoured
            (name,
             h,
             v,
             ch, charType, inverted, s);

    case MemberOfParallelColour:
        magenta.getHsv(&h, &s, &v);
        return font->getCharacterColoured
            (name,
             h,
             v,
             ch, charType, inverted, s);
    }

    return font->getCharacter(name, ch, charType, inverted);
}

NoteCharacter
NotePixmapFactory::getCharacter(CharName name, QColor color, bool inverted)
{
    NoteCharacter ch;

    // use the full font for this unless a grace size was supplied in ctor
    NoteFont *font = (m_haveGrace ? m_graceFont : m_font);

    NoteFont::CharacterType charType =
        m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen;

    int h, s, v;
    color.getHsv(&h, &s, &v);
    font->getCharacterColoured(name, h, v, ch, charType, inverted, s);
    return ch;
}

bool
NotePixmapFactory::getCharacter(CharName name,
                                NoteCharacter &ch, QColor color, bool inverted)
{
    // use the full font for this unless a grace size was supplied in ctor
    NoteFont *font = (m_haveGrace ? m_graceFont : m_font);

    NoteFont::CharacterType charType =
        m_inPrinterMethod ? NoteFont::Printer : NoteFont::Screen;

    int h, s, v;
    color.getHsv(&h, &s, &v);
    return font->getCharacterColoured(name, h, v, ch, charType, inverted, s);
}


int NotePixmapFactory::getNoteBodyWidth(Note::Type type)
    const
{
    // use the full font for this unless a grace size was supplied in ctor
    NoteFont *font = (m_haveGrace ? m_graceFont : m_font);

    CharName charName(m_style->getNoteHeadCharName(type).first);
    int hx, hy;
    if (!font->getHotspot(charName, hx, hy))
        hx = 0;
    return font->getWidth(charName) - hx * 2;
}

int NotePixmapFactory::getNoteBodyHeight(Note::Type )
    const
{
    // use full size for this, never grace size, because we never want to change
    // the vertical scaling

    // this is by definition
    return m_font->getSize();
}

int NotePixmapFactory::getLineSpacing() const
{
    // use full size for this, never grace size
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
    // use the full font for this unless a grace size was supplied in ctor
    NoteFont *font = (m_haveGrace ? m_graceFont : m_font);

    unsigned int l = 1;
    (void)font->getStemLength(l);
    return l;
}

int NotePixmapFactory::getStemThickness() const
{
    // use the full font for this unless a grace size was supplied in ctor
    NoteFont *font = (m_haveGrace ? m_graceFont : m_font);

    unsigned int i = 1;
    (void)font->getStemThickness(i);
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
