// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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
#include <qpainter.h>

#include "NotationTypes.h"
#include "notefont.h"
#include "notestyle.h"

class NotePixmapParameters
{
public:
    NotePixmapParameters(Rosegarden::Note::Type noteType,
                         int dots,
                         Rosegarden::Accidental accidental =
			 Rosegarden::Accidentals::NoAccidental);
    ~NotePixmapParameters();

    void setNoteType(Rosegarden::Note::Type type) { m_noteType = type; }
    void setDots(int dots) { m_dots = dots; }
    void setAccidental(Rosegarden::Accidental acc) { m_accidental = acc; }
    
    void setNoteHeadShifted(bool shifted) { m_shifted          = shifted;   }
    void setDrawFlag(bool df)             { m_drawFlag         = df;        }
    void setDrawStem(bool ds)             { m_drawStem         = ds;        }
    void setStemGoesUp(bool up)           { m_stemGoesUp       = up;        }
    void setStemLength(int length)        { m_stemLength       = length;    }
    void setLegerLines(int lines)         { m_legerLines       = lines;     }
    void setSlashes(int slashes)          { m_slashes          = slashes;   }
    void setSelected(bool selected)       { m_selected         = selected;  }
    void setHighlighted(bool highlighted) { m_highlighted      = highlighted;}
    void setQuantized(bool quantized)     { m_quantized        = quantized; }
    void setIsOnLine(bool isOnLine)       { m_onLine           = isOnLine;  }

    void setBeamed(bool beamed)           { m_beamed           = beamed;    }
    void setNextBeamCount(int tc)         { m_nextBeamCount    = tc;        }
    void setThisPartialBeams(bool pt)     { m_thisPartialBeams = pt;        }
    void setNextPartialBeams(bool pt)     { m_nextPartialBeams = pt;        }
    void setWidth(int width)              { m_width            = width;     }
    void setGradient(double gradient)     { m_gradient         = gradient;  }

    void setTupletCount(int count)	  { m_tupletCount      = count;	    }
    void setTuplingLineY(int y)		  { m_tuplingLineY     = y;	    }
    void setTuplingLineWidth(int width)	  { m_tuplingLineWidth = width;	    }
    void setTuplingLineGradient(double g) { m_tuplingLineGradient = g;      }

    void setTied(bool tied)               { m_tied             = tied;      }
    void setTieLength(int tieLength)      { m_tieLength        = tieLength; }

    void setMarks(const std::vector<Rosegarden::Mark> &marks) {
	m_marks.clear();
	for (unsigned int i = 0; i < marks.size(); ++i)
	    m_marks.push_back(marks[i]);
    }
    void removeMarks() { m_marks.clear(); }

private:
    friend class NotePixmapFactory;

    //--------------- Data members ---------------------------------

    Rosegarden::Note::Type m_noteType;
    int m_dots;
    Rosegarden::Accidental m_accidental;

    bool    m_shifted;
    bool    m_drawFlag;
    bool    m_drawStem;
    bool    m_stemGoesUp;
    int     m_stemLength;
    int     m_legerLines;
    int     m_slashes;
    bool    m_selected;
    bool    m_highlighted;
    bool    m_quantized;
    bool    m_onLine;

    bool    m_beamed;
    int     m_nextBeamCount;
    bool    m_thisPartialBeams;
    bool    m_nextPartialBeams;
    int     m_width;
    double  m_gradient;

    int	    m_tupletCount;
    int     m_tuplingLineY;
    int	    m_tuplingLineWidth;
    double  m_tuplingLineGradient;

    bool    m_tied;
    int     m_tieLength;
    
    std::vector<Rosegarden::Mark> m_marks;
};    


/**
 * Generates QCanvasPixmaps for various notation items
 */

class NotePixmapFactory 
{
public:
    NotePixmapFactory(std::string fontName = "", int size = -1);
    NotePixmapFactory(const NotePixmapFactory &);
    NotePixmapFactory &operator=(const NotePixmapFactory &);
    ~NotePixmapFactory();

    static std::set<std::string> getAvailableFontNames();
    static std::vector<int> getAvailableSizes(std::string fontName); // sorted
    static std::string getDefaultFont();
    static int getDefaultSize(std::string fontName);

    std::string getFontName() const;
    int getSize() const;

    void setSelected(bool selected) { m_selected = selected; }
    bool isSelected() const { return m_selected; }

    void setNoteStyle(NoteStyle *style) { m_style = style; }
    const NoteStyle *getNoteStyle() const { return m_style; } 

    QCanvasPixmap* makeNotePixmap(const NotePixmapParameters &parameters);
    QCanvasPixmap* makeRestPixmap(const NotePixmapParameters &parameters);
    QCanvasPixmap* makeClefPixmap(const Rosegarden::Clef &clef);
    QCanvasPixmap* makeKeyPixmap(const Rosegarden::Key &key,
				const Rosegarden::Clef &clef);
    QCanvasPixmap* makeTimeSigPixmap(const Rosegarden::TimeSignature& sig);
    QCanvasPixmap* makeHairpinPixmap(int length, bool isCrescendo);
    QCanvasPixmap* makeSlurPixmap(int length, int dy, bool above);
    QCanvasPixmap* makeUnknownPixmap();
    QCanvasPixmap* makeTextPixmap(const Rosegarden::Text &text);

    QCanvasPixmap* makeToolbarPixmap(const char *name);
    QCanvasPixmap* makeNoteMenuPixmap(Rosegarden::timeT duration,
				     Rosegarden::timeT &errorReturn);

    QCanvasPixmap* makePitchDisplayPixmap(int pitch,
					  const Rosegarden::Clef &clef);
    QCanvasPixmap* makeClefDisplayPixmap(const Rosegarden::Clef &clef);
    QCanvasPixmap* makeKeyDisplayPixmap(const Rosegarden::Key &key,
				       const Rosegarden::Clef &clef);

    int getNoteBodyWidth (Rosegarden::Note::Type =
                          Rosegarden::Note::Crotchet) const;

    int getNoteBodyHeight(Rosegarden::Note::Type =
                          Rosegarden::Note::Crotchet) const;

    int getAccidentalWidth (const Rosegarden::Accidental &) const;
    int getAccidentalHeight(const Rosegarden::Accidental &) const;

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
    int getTextWidth(const Rosegarden::Text &text) const;

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

protected:
    void init(std::string fontName, int size);

    void makeRoomForAccidental(Rosegarden::Accidental);
    void drawAccidental(Rosegarden::Accidental);

    void makeRoomForMarks(bool isStemmed, const NotePixmapParameters &params);
    void drawMarks(bool isStemmed, const NotePixmapParameters &params);
    
    void drawBeams(const QPoint &, const NotePixmapParameters &params,
                   int beamCount);

    void drawSlashes(const QPoint &, const NotePixmapParameters &params,
		     int slashCount);

    void makeRoomForTuplingLine(const NotePixmapParameters &params);
    void drawTuplingLine(const NotePixmapParameters &params);

    void drawShallowLine(int x0, int y0, int x1, int y1, int thickness,
                         bool smooth);
    void drawTie(bool above, int length);

    QFont getTextFont(const Rosegarden::Text &text) const;

    QCanvasPixmap* makeAnnotationPixmap(const Rosegarden::Text &text);

    void createPixmapAndMask(int width, int height,
			     int maskWidth = -1,
			     int maskHeight = -1);
    QCanvasPixmap* makeCanvasPixmap(QPoint hotspot, bool generateMask = false);

    //--------------- Data members ---------------------------------

    NoteFont *m_font;
    NoteStyle *m_style;
    bool m_selected;

    int m_noteBodyWidth, m_noteBodyHeight;
    int m_left, m_right, m_above, m_below;

    QPoint m_origin;

    QFont m_tupletCountFont;
    QFontMetrics m_tupletCountFontMetrics;

    QFont m_textMarkFont;
    QFontMetrics m_textMarkFontMetrics;

    QFont m_timeSigFont;
    QFontMetrics m_timeSigFontMetrics;

    QFont m_bigTimeSigFont;
    QFontMetrics m_bigTimeSigFontMetrics;

    QPixmap *m_generatedPixmap;
    QBitmap *m_generatedMask;
    
    QPainter m_p;
    QPainter m_pm;

    typedef __HASH_NS::hash_map<CharName, QCanvasPixmap*,
	                        CharNameHash, CharNamesEqual> NotePixmapCache;
    mutable NotePixmapCache m_dottedRestCache;

    typedef Rosegarden::hash_char<QFont> TextFontCache;
    mutable TextFontCache m_textFontCache;

    static QPoint m_pointZero;
};


#endif
