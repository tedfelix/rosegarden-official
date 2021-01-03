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

#ifndef RG_NOTEPIXMAPFACTORY_H
#define RG_NOTEPIXMAPFACTORY_H

#include <QGraphicsPixmapItem>

#include "base/NotationTypes.h"
#include "NoteCharacter.h"
#include "NoteItem.h"
#include "base/Event.h"
#include "gui/editors/notation/NoteCharacterNames.h"
#include <map>
#include <string>

#include <QFont>
#include <QFontMetrics>
#include <QPixmap>
#include <QPoint>
#include <QCoreApplication> // for Q_DECLARE_TR_FUNCTIONS
#include <QSharedPointer>

class QPainter;
class QBitmap;
class QString;


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
class StaffHeader;

/**
 * Generates pixmaps and graphics items for various notation items.
 * This class is not re-entrant.
 */
class NotePixmapFactory
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::NotePixmapFactory)

public:
    static const int NO_GRACE_SIZE = -1;

    NotePixmapFactory(QString fontName = "", int size = -1, int graceSize = NO_GRACE_SIZE);
    NotePixmapFactory(const NotePixmapFactory &);
    NotePixmapFactory &operator=(const NotePixmapFactory &);
    ~NotePixmapFactory();

    QString getFontName() const;
    int getSize() const;

   /**
    \enum ColourType

    This enum describes the different colours that may be used to draw
    notation-related glyphs.  This aspect of drawing is handled through this
    enum, rather than by specifying a colour explicitly.  If you have some
    occasion to add more colours for some new purpose, do so through this enum
    (realising that you have to hook it all up somewhere to make use of the new
    values defined!)

    \sa GUIPalette, TrackParameterBox, PresetHandlerDialog
    */
    enum ColourType {
        PlainColour,        /**< The default basic Qt::black (hard coded) */
        QuantizedColour,    /**< Defined in GUIPalette; used when quantized notes are indicated */
        HighlightedColour,  /**< Defined in GUIPalette; used when notes (&c.) are shown in the selected state */
        TriggerColour,      /**< Defined in GUIPalette; used when trigger notes are indicated */
        TriggerSkipColour,   /**< Defined in GUIPalette; used when masked-out trigger notes within ties are indicated */
        OutRangeColour,     /**< Defined in GUIPalette; used when out-of-range notes are indicated */
        PlainColourLight,   /**< The default basic Qt::white (hard coded) used when drawing on a black background */
        ConflictColour,      /**< Qt::red (hard coded) used by track headers to indicate, eg. a clef conflict */
        MemberOfParallelColour /**< for members of parallels */
    };

    /** Used to notify the drawing code that the character is selected, and
     * should therefore be drawn with a blue foreground
     */
    void setSelected(bool selected) { m_selected = selected; }

    /** Returns true if the character is in the selected state
     */
    bool isSelected() const { return m_selected; }

    int getGraceSize() const { return m_graceSize; }

    /** Used to notify the drawing code that the character is shaded, and should
     * therefore be drawn with a gray foreground.  This is used for "invisible"
     * items
     */
    void setShaded(bool shaded) { m_shaded = shaded; }

    /** Returns true if the character is in the shaded (ie. "invisible") state
     */
    bool isShaded() const { return m_shaded; }

    void setNoteStyle(QSharedPointer<NoteStyle> style) { m_style = style; }
    const QSharedPointer<NoteStyle> getNoteStyle() const { return m_style; }

    // Display methods -- create graphics items:

    QGraphicsItem *makeNote(const NotePixmapParameters &parameters);
    QGraphicsItem *makeRest(const NotePixmapParameters &parameters);

    QGraphicsPixmapItem *makeNotePixmapItem(const NotePixmapParameters &parameters);

    void getNoteDimensions(const NotePixmapParameters &parameters,
                           NoteItemDimensions &dimensions);

    void drawNoteForItem(const NotePixmapParameters &parameters,
                         const NoteItemDimensions &dimensions,
                         NoteItem::DrawMode mode,
                         QPainter *painter);

    /** Make a clef pixmap from Clef &clef.  The optional colourType parameter
     * is used to pass a ColourType through makeClef() into drawCharacter() for
     * certain special situations requiring external control of the glyph colour
     * (eg. track headers)
     */
    QGraphicsPixmapItem *makeClef(const Clef &clef, const ColourType colourType = PlainColour);

    /** Make a symbol pixmap from Symbol &symbol.
     *
     * \sa makeClef
     */
    QGraphicsPixmapItem *makeSymbol(const Symbol &symbol, const ColourType colourType = PlainColour);

    QGraphicsPixmapItem *makeKey(const Key &key,
                                 const Clef &clef,
                                 Key previousKey =
                                 Key::DefaultKey,
                                 const ColourType colourType = PlainColour);
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
    QGraphicsPixmapItem *makeTrillLine(int length);

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

    static QPixmap makeToolbarPixmap(QString name, bool menuSize = false);
    static QPixmap makeNoteMenuPixmap(timeT duration, timeT &errorReturn);
    static QPixmap makeMarkMenuPixmap(Mark);

    QPixmap makePitchDisplayPixmap(int pitch,
                                   const Clef &clef,
                                   bool useSharps,
                                   const ColourType = PlainColour);
    QPixmap makePitchDisplayPixmap(int pitch,
                                   const Clef &clef,
                                   int octave,
                                   int step,
                                   const ColourType = PlainColour);
    QPixmap makeClefDisplayPixmap(const Clef &clef, const ColourType colourType = PlainColour);
    QPixmap makeKeyDisplayPixmap(const Key &key,
                                 const Clef &clef,
                                 const ColourType colourType = PlainColour);
    QPixmap makeTextPixmap(const Text &text);

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

    QFont getTrackHeaderFont() { return m_trackHeaderFont; }
    QFontMetrics getTrackHeaderFontMetrics() { return m_trackHeaderFontMetrics; }

    QFont getTrackHeaderBoldFont() { return m_trackHeaderBoldFont; }
    QFontMetrics getTrackHeaderBoldFontMetrics() {
        return m_trackHeaderBoldFontMetrics;
    }

    static void dumpStats(std::ostream &);


    static const char* const defaultSerifFontFamily;
    static const char* const defaultSansSerifFontFamily;
    static const char* const defaultTimeSigFontFamily;

protected:
    void init(QString fontName, int size);
    void initMaybe() { if (!m_font) init("", -1); }

    void calculateNoteDimensions(const NotePixmapParameters &parameters);
    void sketchNoteTiny(const NotePixmapParameters &parameters,
                        const NoteItemDimensions &dimensions,
                        QPainter *painter);
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
    void drawTrillLineAux(int length, QPainter *painter, int x, int y);

    int getStemLength(const NotePixmapParameters &) const;

    void makeRoomForAccidental(Accidental, bool cautionary, int shift, bool extra);
    void drawAccidental(const NotePixmapParameters &params);

    void makeRoomForMarks(bool isStemmed, const NotePixmapParameters &params, int stemLength);
    void drawMarks(bool isStemmed, const NotePixmapParameters &params, int stemLength, bool overRestHack = false);

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

    void drawShallowLine(float x0, float y0, float x1, float y1, float thickness);
    void drawTie(bool above, int length, int shift);

    /// Draw a parenthesis for cautionary (courtesy) accidentals.
    void drawBracket(int length, bool left, bool curly, int x, int y);

    QFont getTextFont(const Text &text) const;

    QGraphicsPixmapItem *makeAnnotation(const Text &text);
    QGraphicsPixmapItem *makeAnnotation(const Text &text,
                                        const bool isLilyPondDirective);

    void createPixmap(int width, int height);
    QGraphicsPixmapItem *makeItem(QPoint hotspot);
    QPixmap makePixmap();

    /// draws selected/shaded status from m_selected/m_shaded:
    NoteCharacter getCharacter(CharName name, ColourType type, bool inverted);

    /// draws selected/shaded status from m_selected/m_shaded:
    bool getCharacter(CharName name, NoteCharacter &ch, ColourType type, bool inverted);

    /// Draw character with a specified QColor
    //  This is a first step to a simpler use of colors
    NoteCharacter getCharacter(CharName name, QColor color, bool inverted);

    /// Draw character with a specified QColor
    //  This is a first step to a simpler use of colors
    bool getCharacter(CharName name, NoteCharacter &ch, QColor color, bool inverted);

    void drawNoteHalo(int x, int y, int w, int h);

    //--------------- Data members ---------------------------------

    NoteFont *m_font;
    NoteFont *m_graceFont;
    QSharedPointer<NoteStyle> m_style;
    bool m_selected;
    bool m_shaded;
    bool m_haveGrace;

    int m_graceSize;
    
    NoteItemDimensions m_nd;

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
};



}

#endif
