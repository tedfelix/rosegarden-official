
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

#ifndef NOTEPIXMAPFACTORY_H
#define NOTEPIXMAPFACTORY_H

#include <vector>
#include <qcanvas.h>

#include "NotationTypes.h"
#include "notepixmapfactory.h"

typedef std::vector<int> ChordPitches;
typedef std::vector<Rosegarden::Accidental> Accidentals;


/**
 * Helper class to compute various offsets
 * Used by the NotePixmapFactory
 * @author Guillaume Laurent
 */
class NotePixmapOffsets
{
public:

    typedef std::pair<QPoint, QPoint> stalkpoints;

    NotePixmapOffsets();

    void offsetsFor(Rosegarden::Note::Type,
                    bool dotted,
                    Rosegarden::Accidental,
                    bool drawTail,
                    bool stalkGoesUp,
                    bool fixedHeight);
    
    const QPoint&      getBodyOffset()     { return m_bodyOffset; }
    const QSize&       getPixmapSize()     { return m_pixmapSize; }
    const QPoint&      getHotSpot()        { return m_hotSpot; }
    const stalkpoints& getStalkPoints()    { return m_stalkPoints; }
    const QPoint&      getAccidentalOffset() { return m_accidentalOffset; }

    void setNoteBodySizes(QSize empty, QSize filled, QSize breve);
    void setTailWidth(unsigned int);
    void setStalkLength(unsigned int);
    void setAccidentalsWidth(unsigned int sharp,
                             unsigned int flat,
			     unsigned int doublesharp,
			     unsigned int doubleflat,
                             unsigned int natural);
    void setAccidentalHeight(unsigned int height);
    void setDotSize(QSize size);
    void setExtraBeamSpacing(unsigned int bs);

protected:

    void computePixmapSize();
    void computeAccidentalAndStalkSize();
    void computeBodyOffset();

    QSize m_noteBodyEmptySize;
    QSize m_noteBodyFilledSize;
    QSize m_breveSize;

    unsigned int m_tailWidth;
    unsigned int m_sharpWidth;
    unsigned int m_flatWidth;
    unsigned int m_doubleSharpWidth;
    unsigned int m_doubleFlatWidth;
    unsigned int m_naturalWidth;
    unsigned int m_accidentalHeight;
    unsigned int m_stalkLength;
    unsigned int m_extraBeamSpacing;

    Rosegarden::Note::Type m_note;
    Rosegarden::Accidental m_accidental;
    bool m_drawTail;
    bool m_stalkGoesUp;
    bool m_noteHasStalk;
    bool m_dotted;

    QPoint m_bodyOffset;
    QPoint m_hotSpot;
    QPoint m_accidentalOffset;
    QSize m_bodySize;
    QSize m_pixmapSize;
    QSize m_accidentalStalkSize;
    QSize m_dotSize;
    
    stalkpoints m_stalkPoints;
};


/**
 * Generates QCanvasPixmaps for single notes and chords
 * (chords unused)
 *
 *@author Guillaume Laurent
 */
class NotePixmapFactory
{
public:
    NotePixmapFactory(int resolution);
    ~NotePixmapFactory();

    /**
     * Generate a pixmap for a single note
     *
     * @param note : note type
     * @param drawTail : if the pixmap should have a tail or not
     *   (useful when the tail should be collapsed with the one of a
     *    neighboring note)
     * @param stalkGoesUp : if the note's stalk should go up or down
     * @param fixedHeight : true for things like toolbar buttons,
     *   never appropriate on a real stave
     */
    QCanvasPixmap makeNotePixmap(Rosegarden::Note::Type note,
                                 bool dotted,
                                 Rosegarden::Accidental accidental =
                                                     Rosegarden::NoAccidental,
                                 bool drawTail = true,
                                 bool stalkGoesUp = true,
                                 bool fixedHeight = false);

    QCanvasPixmap makeBeamedNotePixmap(Rosegarden::Note::Type note,
				       bool dotted,
				       Rosegarden::Accidental accidental,
				       bool stalkGoesUp,
				       int stalkLength,
				       int nextTailCount,
                                       bool thisPartialTails,
                                       bool nextPartialTails,
				       int width,
				       double gradient);
    
    QCanvasPixmap makeRestPixmap(Rosegarden::Note::Type note, bool dotted);
    QCanvasPixmap makeClefPixmap(const Rosegarden::Clef &clef) const;
    QCanvasPixmap makeKeyPixmap(const Rosegarden::Key &key,
				const Rosegarden::Clef &clef);
    QCanvasPixmap makeTimeSigPixmap(const Rosegarden::TimeSignature& sig);
    QCanvasPixmap makeUnknownPixmap();

    int getNoteBodyHeight() const   { return m_noteBodyEmpty.height(); }
    int getNoteBodyWidth() const    { return m_noteBodyEmpty.width(); }
    int getBreveWidth() const       { return m_breve.width(); }
    int getLineSpacing() const      { return getNoteBodyHeight() + 1; }
    int getAccidentalWidth() const  { return m_accidentalSharp.width(); }
    int getAccidentalHeight() const { return m_accidentalSharp.height(); }
    int getStalkLength() const      { return getNoteBodyHeight() * 11/4; }
    int getDotWidth() const         { return m_dot.width(); }
    int getClefWidth() const;
    int getTimeSigWidth(const Rosegarden::TimeSignature &timesig) const;
    int getKeyWidth(const Rosegarden::Key &key) const {
        return (key.getAccidentalCount() *
                (getAccidentalWidth() - (key.isSharp()? 1 : 2)));
    }

protected:

    const QPixmap* tailUp(Rosegarden::Note::Type note) const;
    const QPixmap* tailDown(Rosegarden::Note::Type note) const;

    void drawStalk(Rosegarden::Note::Type note, bool drawTail, bool stalkGoesUp);
    void drawAccidental(Rosegarden::Accidental, bool stalkGoesUp);
    void drawDot();

    void createPixmapAndMask(int width = -1, int height = -1);

    int m_resolution;
    QString m_pixmapDirectory;

    NotePixmapOffsets m_offsets;
    unsigned int m_generatedPixmapHeight;

    QFont m_timeSigFont;
    QFontMetrics m_timeSigFontMetrics;

    QPixmap *m_generatedPixmap;
    QBitmap *m_generatedMask;
    QPainter m_p;
    QPainter m_pm;

    QPixmap m_noteBodyFilled;
    QPixmap m_noteBodyEmpty;
    QPixmap m_breve;

    QPixmap m_accidentalSharp;
    QPixmap m_accidentalFlat;
    QPixmap m_accidentalDoubleSharp;
    QPixmap m_accidentalDoubleFlat;
    QPixmap m_accidentalNatural;

    QPixmap m_dot;

    std::vector<QPixmap*> m_tailsUp;
    std::vector<QPixmap*> m_tailsDown;
    std::vector<QPixmap*> m_rests;

    mutable int m_clefWidth;

    static QPoint m_pointZero;

private:
    NotePixmapFactory(const NotePixmapFactory &);
    NotePixmapFactory &operator=(const NotePixmapFactory &);
};


#endif
