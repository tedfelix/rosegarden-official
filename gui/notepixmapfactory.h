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


class NotePixmapParameters
{
public:
    NotePixmapParameters(Rosegarden::Note::Type noteType,
                         int dots,
                         Rosegarden::Accidental accidental =
                                                Rosegarden::NoAccidental);
    ~NotePixmapParameters();
    
    void setNoteHeadShifted(bool shifted) { m_shifted          = shifted;  }
    void setDrawFlag(bool df)             { m_drawFlag         = df;       }
    void setStemGoesUp(bool up)           { m_stemGoesUp       = up;       }
    void setStemLength(int length)        { m_stemLength       = length;   }
    void setLegerLines(int lines)         { m_legerLines       = lines;    }

    void setBeamed(bool beamed)           { m_beamed           = beamed;   }
    void setNextBeamCount(int tc)         { m_nextBeamCount    = tc;       }
    void setThisPartialBeams(bool pt)     { m_thisPartialBeams = pt;       }
    void setNextPartialBeams(bool pt)     { m_nextPartialBeams = pt;       }
    void setWidth(int width)              { m_width            = width;    }
    void setGradient(double gradient)     { m_gradient         = gradient; }

private:
    friend class NotePixmapFactory;

    Rosegarden::Note::Type m_noteType;
    int m_dots;
    Rosegarden::Accidental m_accidental;

    bool    m_shifted;
    bool    m_drawFlag;
    bool    m_stemGoesUp;
    int     m_stemLength;
    int     m_legerLines;

    bool    m_beamed;
    int     m_nextBeamCount;
    bool    m_thisPartialBeams;
    bool    m_nextPartialBeams;
    int     m_width;
    double  m_gradient;
};    


/**
 * Generates QCanvasPixmaps for various notation items
 */

class NotePixmapFactory : private NoteCharacterNameLookup
{
public:
    NotePixmapFactory(int size, std::string fontName = "feta");
    ~NotePixmapFactory();

    QCanvasPixmap makeNotePixmap(const NotePixmapParameters &parameters);
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

    int getLineSpacing()        const;
    int getStemLength()         const;
    int getStemThickness()      const;
    int getStaffLineThickness() const;
    int getDotWidth()           const;
    int getBarMargin()	        const;

    int getClefWidth(const Rosegarden::Clef &clef) const;
    int getTimeSigWidth(const Rosegarden::TimeSignature &timesig) const;
    int getRestWidth(const Rosegarden::Note &restType) const;
    int getKeyWidth(const Rosegarden::Key &key) const;

protected:
    NoteFont *m_font;

    int m_noteBodyWidth, m_noteBodyHeight;
    int m_left, m_right, m_above, m_below;

    void makeRoomForAccidental(Rosegarden::Accidental);
    void drawAccidental(Rosegarden::Accidental);
    void drawBeams(const QPoint &, const NotePixmapParameters &params,
                   int beamCount);
    void drawShallowLine(int x0, int y0, int x1, int y1, int thickness,
                         bool smooth);

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
