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
#include <set>
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

    void setNoteType(Rosegarden::Note::Type type) { m_noteType = type; }
    void setDots(int dots) { m_dots = dots; }
    void setAccidental(Rosegarden::Accidental acc) { m_accidental = acc; }
    
    void setNoteHeadShifted(bool shifted) { m_shifted          = shifted;   }
    void setDrawFlag(bool df)             { m_drawFlag         = df;        }
    void setStemGoesUp(bool up)           { m_stemGoesUp       = up;        }
    void setStemLength(int length)        { m_stemLength       = length;    }
    void setLegerLines(int lines)         { m_legerLines       = lines;     }
    void setSelected(bool selected)       { m_selected         = selected;  }
    void setIsOnLine(bool isOnLine)       { m_onLine           = isOnLine;  }

    void setBeamed(bool beamed)           { m_beamed           = beamed;    }
    void setNextBeamCount(int tc)         { m_nextBeamCount    = tc;        }
    void setThisPartialBeams(bool pt)     { m_thisPartialBeams = pt;        }
    void setNextPartialBeams(bool pt)     { m_nextPartialBeams = pt;        }
    void setWidth(int width)              { m_width            = width;     }
    void setGradient(double gradient)     { m_gradient         = gradient;  }

    void setTupledCount(int count)	  { m_tupledCount      = count;	    }
    void setTuplingLineY(int y)		  { m_tuplingLineY     = y;	    }
    void setTuplingLineWidth(int width)	  { m_tuplingLineWidth = width;	    }
    void setTuplingLineGradient(double g) { m_tuplingLineGradient = g;      }

    void setTied(bool tied)               { m_tied             = tied;      }
    void setTieLength(int tieLength)      { m_tieLength        = tieLength; }

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
    bool    m_selected;
    bool    m_onLine;

    bool    m_beamed;
    int     m_nextBeamCount;
    bool    m_thisPartialBeams;
    bool    m_nextPartialBeams;
    int     m_width;
    double  m_gradient;

    int	    m_tupledCount;
    int     m_tuplingLineY;
    int	    m_tuplingLineWidth;
    double  m_tuplingLineGradient;

    bool    m_tied;
    int     m_tieLength;
};    


/**
 * Generates QCanvasPixmaps for various notation items
 */

class NotePixmapFactory : private NoteCharacterNameLookup
{
public:
    NotePixmapFactory(std::string fontName = "", int size = -1);
    ~NotePixmapFactory();

    static std::set<std::string> getAvailableFontNames();
    static std::vector<int> getAvailableSizes(std::string fontName); // sorted
    static std::string getDefaultFont();
    static int getDefaultSize(std::string fontName);

    std::string getFontName() const;
    int getSize() const;

    void setSelected(bool selected) { m_selected = selected; }
    bool isSelected() const { return m_selected; }

    QCanvasPixmap makeNotePixmap(const NotePixmapParameters &parameters);
    QCanvasPixmap makeRestPixmap(const Rosegarden::Note &restType);
    QCanvasPixmap makeClefPixmap(const Rosegarden::Clef &clef) const;
    QCanvasPixmap makeKeyPixmap(const Rosegarden::Key &key,
				const Rosegarden::Clef &clef);
    QCanvasPixmap makeTimeSigPixmap(const Rosegarden::TimeSignature& sig);
    QCanvasPixmap makeHairpinPixmap(int length, bool isCrescendo);
    QCanvasPixmap makeSlurPixmap(int length, int dy, bool above);
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
    bool m_selected;

    int m_noteBodyWidth, m_noteBodyHeight;
    int m_left, m_right, m_above, m_below;

    void makeRoomForAccidental(Rosegarden::Accidental);
    void drawAccidental(Rosegarden::Accidental);
    void drawBeams(const QPoint &, const NotePixmapParameters &params,
                   int beamCount);
    void drawTuplingLine(const NotePixmapParameters &params);
    void drawShallowLine(int x0, int y0, int x1, int y1, int thickness,
                         bool smooth);
    void drawTie(bool above, int length);

    void createPixmapAndMask(int width, int height);
    QCanvasPixmap makeCanvasPixmap(QPoint hotspot);

    QPoint m_origin;

    QFont m_timeSigFont;
    QFontMetrics m_timeSigFontMetrics;

    QFont m_tupledCountFont;
    QFontMetrics m_tupledCountFontMetrics;

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
