
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _RG_NOTEPIXMAPFACTORY_H_
#define _RG_NOTEPIXMAPFACTORY_H_

#include "base/NotationTypes.h"
#include <map>
#include "NoteCharacter.h"
#include <string>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qpixmap.h>
#include <qpoint.h>
#include "base/Event.h"
#include "gui/editors/notation/NoteCharacterNames.h"


class QPainter;
class QCanvasPixmap;
class QBitmap;


namespace Rosegarden
{

namespace Guitar { class Fingering; }

class TimeSignature;
class Text;
class NoteStyle;
class NotePixmapParameters;
class NoteFont;
class Fingering;
class NotePixmapPainter;
class NotePixmapCache;
class Clef;


/**
 * Generates QCanvasPixmaps for various notation items.
 */

class NotePixmapFactory 
{
public:
    NotePixmapFactory(std::string fontName = "", int size = -1);
    NotePixmapFactory(const NotePixmapFactory &);
    NotePixmapFactory &operator=(const NotePixmapFactory &);
    ~NotePixmapFactory();

    std::string getFontName() const;
    int getSize() const;

    void setSelected(bool selected) { m_selected = selected; }
    bool isSelected() const { return m_selected; }

    void setShaded(bool shaded) { m_shaded = shaded; }
    bool isShaded() const { return m_shaded; }

    void setNoteStyle(NoteStyle *style) { m_style = style; }
    const NoteStyle *getNoteStyle() const { return m_style; } 

    // Display methods -- create canvas pixmaps:

    QCanvasPixmap* makeNotePixmap(const NotePixmapParameters &parameters);
    QCanvasPixmap* makeRestPixmap(const NotePixmapParameters &parameters);
    QCanvasPixmap* makeClefPixmap(const Clef &clef);
    QCanvasPixmap* makeKeyPixmap(const Key &key,
                                 const Clef &clef,
                                 Key previousKey =
                                 Key::DefaultKey);
    QCanvasPixmap* makeTimeSigPixmap(const TimeSignature& sig);
    QCanvasPixmap* makeHairpinPixmap(int length, bool isCrescendo);
    QCanvasPixmap* makeSlurPixmap(int length, int dy, bool above, bool phrasing);
    QCanvasPixmap* makeOttavaPixmap(int length, int octavesUp);
    QCanvasPixmap* makePedalDownPixmap();
    QCanvasPixmap* makePedalUpPixmap();
    QCanvasPixmap* makeUnknownPixmap();
    QCanvasPixmap* makeTextPixmap(const Text &text);
    QCanvasPixmap* makeFretboardPixmap(const Guitar::Fingering &arrangement,
                                       int x, int y);

    // Printing methods -- draw direct to a paint device:

    void drawNote(const NotePixmapParameters &parameters,
                  QPainter &painter, int x, int y);
    void drawRest(const NotePixmapParameters &parameters,
                  QPainter &painter, int x, int y);
    void drawHairpin(int length, bool isCrescendo,
                     QPainter &painter, int x, int y);
    void drawSlur(int length, int dy, bool above, bool phrasing,
                  QPainter &painter, int x, int y);
    void drawOttava(int length, int octavesUp,
                    QPainter &painter, int x, int y);
    void drawText(const Text &text,
                  QPainter &painter, int x, int y);

    // Other support methods for producing pixmaps for other contexts:

    static QCanvasPixmap *makeToolbarPixmap(const char *name);
    static QCanvasPixmap *makeNoteMenuPixmap(timeT duration,
                                             timeT &errorReturn);
    static QCanvasPixmap *makeMarkMenuPixmap(Mark);

    QCanvasPixmap* makePitchDisplayPixmap(int pitch,
                                          const Clef &clef,
                                          bool useSharps);
    QCanvasPixmap* makeClefDisplayPixmap(const Clef &clef);
    QCanvasPixmap* makeKeyDisplayPixmap(const Key &key,
                                       const Clef &clef);

    // Bounding box and other geometry methods:

    int getNoteBodyWidth (Note::Type =
                          Note::Crotchet) const;

    int getNoteBodyHeight(Note::Type =
                          Note::Crotchet) const;

    int getAccidentalWidth (const Accidental &,
                            int shift = 0, bool extra = false) const;
    int getAccidentalHeight(const Accidental &) const;

    int getLineSpacing()        const;
    int getStemLength()         const;
    int getStemThickness()      const;
    int getStaffLineThickness() const;
    int getLegerLineThickness() const;
    int getDotWidth()           const;
    int getBarMargin()          const;

    int getClefWidth(const Clef &clef) const;
    int getTimeSigWidth(const TimeSignature &timesig) const;
    int getRestWidth(const Note &restType) const;
    int getKeyWidth(const Key &key,
                    Key previousKey = Key::DefaultKey) const;
    int getTextWidth(const Text &text) const;

    /**
     * We need this function because as of Qt 3.1, QCanvasPixmap
     * is no longer copyable by value, while QPixmap still is.
     *
     * So all the makeXXPixmap are now returning QCanvasPixmap*
     * instead of QCanvasPixmap, but we need an easy way to
     * convert them to QPixmap, since we use them that
     * way quite often (to generate toolbar button icons for instance).
     */
    static QPixmap toQPixmap(QCanvasPixmap*);
    static void dumpStats(std::ostream &);


    static const char* const defaultSerifFontFamily;
    static const char* const defaultTimeSigFontFamily;
    

protected:
    void init(std::string fontName, int size);
    void initMaybe() { if (!m_font) init("", -1); }

    void drawNoteAux(const NotePixmapParameters &parameters,
                     QPainter *painter, int x, int y);
    void drawRestAux(const NotePixmapParameters &parameters, QPoint &hotspot,
                     QPainter *painter, int x, int y);
    void drawHairpinAux(int length, bool isCrescendo,
                        QPainter *painter, int x, int y);
    void drawSlurAux(int length, int dy, bool above, bool smooth, bool tie, bool phrasing,
                     QPoint &hotspot,
                     QPainter *painter, int x, int y);
    void drawOttavaAux(int length, int octavesUp,
                       QPainter *painter, int x, int y);
    void drawTextAux(const Text &text,
                     QPainter *painter, int x, int y);

    int getStemLength(const NotePixmapParameters &) const;

    void makeRoomForAccidental(Accidental, bool cautionary, int shift, bool extra);
    void drawAccidental(Accidental, bool cautionary);

    void makeRoomForMarks(bool isStemmed, const NotePixmapParameters &params, int stemLength);
    void drawMarks(bool isStemmed, const NotePixmapParameters &params, int stemLength);

    void makeRoomForLegerLines(const NotePixmapParameters &params);
    void drawLegerLines(const NotePixmapParameters &params);

    void makeRoomForStemAndFlags(int flagCount, int stemLength,
                                 const NotePixmapParameters &params,
                                 QPoint &startPoint, QPoint &endPoint);
    void drawFlags(int flagCount, const NotePixmapParameters &params,
                   const QPoint &startPoint, const QPoint &endPoint);
    void drawStem(const NotePixmapParameters &params,
                  const QPoint &startPoint, const QPoint &endPoint,
                  int shortening);

    void makeRoomForBeams(const NotePixmapParameters &params);
    void drawBeams(const QPoint &, const NotePixmapParameters &params,
                   int beamCount);

    void drawSlashes(const QPoint &, const NotePixmapParameters &params,
                     int slashCount);

    void makeRoomForTuplingLine(const NotePixmapParameters &params);
    void drawTuplingLine(const NotePixmapParameters &params);

    void drawShallowLine(int x0, int y0, int x1, int y1, int thickness,
                         bool smooth);
    void drawTie(bool above, int length, int shift);

    void drawBracket(int length, bool left, bool curly, int x, int y);

    QFont getTextFont(const Text &text) const;

    QCanvasPixmap* makeAnnotationPixmap(const Text &text);
    QCanvasPixmap* makeAnnotationPixmap(const Text &text, const bool isLilypondDirective);

    void createPixmapAndMask(int width, int height,
                             int maskWidth = -1,
                             int maskHeight = -1);
    QCanvasPixmap* makeCanvasPixmap(QPoint hotspot, bool generateMask = false);

    enum ColourType {
        PlainColour,
        QuantizedColour,
        HighlightedColour,
        TriggerColour,
        OutRangeColour
    };

    /// draws selected/shaded status from m_selected/m_shaded:
    NoteCharacter getCharacter(CharName name, ColourType type, bool inverted);

    /// draws selected/shaded status from m_selected/m_shaded:
    bool getCharacter(CharName name, NoteCharacter &ch, ColourType type, bool inverted);

    //--------------- Data members ---------------------------------

    NoteFont *m_font;
    NoteStyle *m_style;
    bool m_selected;
    bool m_shaded;

    int m_noteBodyWidth, m_noteBodyHeight;
    int m_left, m_right, m_above, m_below;
    int m_borderX, m_borderY;

    QFont m_tupletCountFont;
    QFontMetrics m_tupletCountFontMetrics;

    QFont m_textMarkFont;
    QFontMetrics m_textMarkFontMetrics;

    QFont m_fingeringFont;
    QFontMetrics m_fingeringFontMetrics;

    QFont m_timeSigFont;
    QFontMetrics m_timeSigFontMetrics;

    QFont m_bigTimeSigFont;
    QFontMetrics m_bigTimeSigFontMetrics;

    QFont m_ottavaFont;
    QFontMetrics m_ottavaFontMetrics;

    QPixmap *m_generatedPixmap;
    QBitmap *m_generatedMask;

    int m_generatedWidth;
    int m_generatedHeight;
    bool m_inPrinterMethod;
    
    NotePixmapPainter *m_p;

    mutable NotePixmapCache *m_dottedRestCache;

    typedef std::map<const char *, QFont> TextFontCache;
    mutable TextFontCache m_textFontCache;

    static QPoint m_pointZero;
};



}

#endif
