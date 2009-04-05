/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

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

#include <QGraphicsPixmapItem>

#include "base/NotationTypes.h"
#include "NoteCharacter.h"
#include "base/Event.h"
#include "gui/editors/notation/NoteCharacterNames.h"
#include <map>
#include <string>

#include <QFont>
#include <QFontMetrics>
#include <QPixmap>
#include <QPoint>

#include <QCoreApplication>

class QPainter;
class QBitmap;


namespace Rosegarden
{

namespace Guitar { class Fingering; }

class TimeSignature;
class Text;
class NoteStyle;
class NotePixmapParameters;
class NoteFont;
class NotePixmapPainter;
class Clef;
class TrackHeader;

/**
 * Generates pixmaps and graphics items for various notation items.
 */

class NotePixmapFactory 
{
    Q_DECLARE_TR_FUNCTIONS(NotePixmapFactory)

public:
    NotePixmapFactory(QString fontName = "", int size = -1);
    NotePixmapFactory(const NotePixmapFactory &);
    NotePixmapFactory &operator=(const NotePixmapFactory &);
    ~NotePixmapFactory();

    QString getFontName() const;
    int getSize() const;

    void setSelected(bool selected) { m_selected = selected; }
    bool isSelected() const { return m_selected; }

    void setShaded(bool shaded) { m_shaded = shaded; }
    bool isShaded() const { return m_shaded; }

    void setNoteStyle(NoteStyle *style) { m_style = style; }
    const NoteStyle *getNoteStyle() const { return m_style; } 

    // Display methods -- create canvas pixmaps:

    QGraphicsPixmapItem *makeNote(const NotePixmapParameters &parameters);
    QGraphicsPixmapItem *makeRest(const NotePixmapParameters &parameters);
    QGraphicsPixmapItem *makeClef(const Clef &clef);
    QGraphicsPixmapItem *makeKey(const Key &key,
                                 const Clef &clef,
                                 Key previousKey =
                                 Key::DefaultKey);
    QGraphicsPixmapItem *makeTimeSig(const TimeSignature& sig);
    QGraphicsPixmapItem *makeHairpin(int length, bool isCrescendo);
    QGraphicsPixmapItem *makeSlur(int length, int dy, bool above, bool phrasing);
    QGraphicsPixmapItem *makeOttava(int length, int octavesUp);
    QGraphicsPixmapItem *makePedalDown();
    QGraphicsPixmapItem *makePedalUp();
    QGraphicsPixmapItem *makeUnknown();
    QGraphicsPixmapItem *makeText(const Text &text);
    QGraphicsPixmapItem *makeGuitarChord(const Guitar::Fingering &fingering,
                                       int x, int y);

    QGraphicsPixmapItem *makeNoteHalo(const NotePixmapParameters &parameters);

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

    static QPixmap makeToolbarPixmap(const char *name,
                                            bool menuSize = false);
    static QPixmap makeNoteMenuPixmap(timeT duration,
                                             timeT &errorReturn);
    static QPixmap makeMarkMenuPixmap(Mark);

    QPixmap makePitchDisplayPixmap(int pitch,
                                   const Clef &clef,
                                   bool useSharps);
    QPixmap makePitchDisplayPixmap(int pitch,
                                   const Clef &clef,
                                   int octave,
                                   int step);
    QPixmap makeClefDisplayPixmap(const Clef &clef);
    QPixmap makeKeyDisplayPixmap(const Key &key,
                                 const Clef &clef);
    QPixmap makeTextPixmap(const Text &text);

#ifdef NOT_JUST_NOW //!!!    
    QPixmap makeTrackHeaderPixmap(int width, int height,
                                  TrackHeader *header);
#endif

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
     * Returns the width of clef and key signature drawn in a track header.
     */
    int getClefAndKeyWidth(const Key &key, const Clef &clef);

#ifdef NOT_JUST_NOW //!!!
    /**
     * Returns the Number of Text Lines that can be written at top and bottom
     * of a track header.
     * The parameter is the track header height.
     * Always returns a value >= 1.
     */
    int getTrackHeaderNTL(int height);

    /**
     * Returns the width of a text string written in a track header.
     */
    int getTrackHeaderTextWidth(QString str);

    /**
     * Returns the spacing of a text lines written in a track header.
     */
    int getTrackHeaderTextLineSpacing();

    /**
     * Returns from the beginning of "text" a string of horizontal size
     * "width" (when written with m_trackHeaderFont) and removes it
     * from "text".
     */
    QString getOneLine(QString &text, int width);
#endif

    static void dumpStats(std::ostream &);


    static const char* const defaultSerifFontFamily;
    static const char* const defaultSansSerifFontFamily;
    static const char* const defaultTimeSigFontFamily;
    

protected:
    void init(QString fontName, int size);
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

    QGraphicsPixmapItem *makeAnnotation(const Text &text);
    QGraphicsPixmapItem *makeAnnotation(const Text &text, const bool isLilyPondDirective);

    void createPixmap(int width, int height);
    QGraphicsPixmapItem *makeItem(QPoint hotspot);

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

    void drawNoteHalo(int x, int y, int w, int h);

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

    QFont m_clefOttavaFont;
    QFontMetrics m_clefOttavaFontMetrics;

    QFont m_trackHeaderFont;
    QFontMetrics m_trackHeaderFontMetrics;

    QFont m_trackHeaderBoldFont;
    QFontMetrics m_trackHeaderBoldFontMetrics;

    QPixmap *m_generatedPixmap;

    int m_generatedWidth;
    int m_generatedHeight;
    bool m_inPrinterMethod;
    
    NotePixmapPainter *m_p;

    typedef std::map<const char *, QFont> TextFontCache;
    mutable TextFontCache m_textFontCache;

    static QPoint m_pointZero;
};



}

#endif
