// -*- c-basic-offset: 4 -*-

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
#include "notefont.h"


/**
 * Generates QCanvasPixmaps for various notation items
 */

class NotePixmapFactory : private NoteCharacterNameLookup
{
public:
    NotePixmapFactory(int size, std::string fontName = "rg21");
    ~NotePixmapFactory();

    QCanvasPixmap makeNotePixmap(Rosegarden::Note::Type note,
                                 int dots,
                                 Rosegarden::Accidental accidental =
                                                     Rosegarden::NoAccidental,
                                 bool noteHeadShifted = false,
                                 bool drawTail = true,
                                 bool stemGoesUp = true,
                                 int stemLength = -1);

    QCanvasPixmap makeBeamedNotePixmap(Rosegarden::Note::Type note,
				       int dots,
				       Rosegarden::Accidental accidental,
                                       bool noteHeadShifted,
				       bool stemGoesUp,
				       int stemLength,
				       int nextTailCount,
                                       bool thisPartialTails,
                                       bool nextPartialTails,
				       int width,
				       double gradient);

    QCanvasPixmap makeRestPixmap(const Rosegarden::Note &restType);
    QCanvasPixmap makeClefPixmap(const Rosegarden::Clef &clef) const;
    QCanvasPixmap makeKeyPixmap(const Rosegarden::Key &key,
				const Rosegarden::Clef &clef);
    QCanvasPixmap makeTimeSigPixmap(const Rosegarden::TimeSignature& sig);
    QCanvasPixmap makeUnknownPixmap();
    QCanvasPixmap makeToolbarPixmap(const char *name);

    int getNoteBodyWidth (Rosegarden::Note::Type =
                          Rosegarden::Note::Crotchet) const;
    int getNoteBodyHeight(Rosegarden::Note::Type =
                          Rosegarden::Note::Crotchet) const;

    int getAccidentalWidth (Rosegarden::Accidental) const;
    int getAccidentalHeight(Rosegarden::Accidental) const;

    int getLineSpacing()       const;
    int getStalkLength()       const { return getStemLength(); } //!!!
    int getStemLength()        const;
    int getDotWidth()          const;
    int getBarMargin()	       const;

    int getClefWidth(const Rosegarden::Clef &clef) const;
    int getTimeSigWidth(const Rosegarden::TimeSignature &timesig) const;
    int getRestWidth(const Rosegarden::Note &restType) const;
    int getKeyWidth(const Rosegarden::Key &key) const;

protected:
    QCanvasPixmap makeNotePixmapAux(Rosegarden::Note::Type note,
                                    int dots,
                                    Rosegarden::Accidental accidental,
                                    bool noteHeadShifted,
                                    bool drawTail,
                                    bool stemGoesUp,
                                    bool isBeamed,
                                    int stemLength,
                                    int nextTailCount,
                                    bool thisPartialTails,
                                    bool nextPartialTails,
                                    int width,
                                    double gradient);

    NoteFont *m_font;

    int m_noteBodyWidth, m_noteBodyHeight;
    int m_left, m_right, m_above, m_below;

    void makeRoomForAccidental(Rosegarden::Accidental);
    void drawAccidental(Rosegarden::Accidental);
    void drawBeams(const QPoint &, bool stemGoesUp,
                   int tailCount, int nextTailCount,
                   bool thisPartialTails, bool nextPartialTails,
                   int width, double gradient);

    void createPixmapAndMask(int width, int height);

    QPoint m_origin;

    QFont m_timeSigFont;
    QFontMetrics m_timeSigFontMetrics;

    QPixmap *m_generatedPixmap;
    QBitmap *m_generatedMask;
    QPainter m_p;
    QPainter m_pm;

    static QPoint m_pointZero;

private:
    NotePixmapFactory(const NotePixmapFactory &);
    NotePixmapFactory &operator=(const NotePixmapFactory &);
};


#endif
