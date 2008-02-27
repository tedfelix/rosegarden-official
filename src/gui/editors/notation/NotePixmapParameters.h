
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

#ifndef _RG_NOTEPIXMAPPARAMETERS_H_
#define _RG_NOTEPIXMAPPARAMETERS_H_

#include "base/NotationTypes.h"
#include <vector>




namespace Rosegarden
{



class NotePixmapParameters
{
public:
    NotePixmapParameters(Note::Type noteType,
                         int dots,
                         Accidental accidental =
                         Accidentals::NoAccidental);
    ~NotePixmapParameters();

    void setNoteType(Note::Type type) { m_noteType = type; }
    void setDots(int dots) { m_dots = dots; }
    void setAccidental(Accidental acc) { m_accidental = acc; }

    void setAccidentalCautionary(bool cautionary) { m_cautionary = cautionary; }
    void setNoteHeadShifted(bool shifted) { m_shifted          = shifted;   }
    void setNoteDotShifted(bool shifted)  { m_dotShifted       = shifted;   }
    void setAccidentalShift(int shift)    { m_accidentalShift  = shift;     }
    void setAccExtraShift(bool extra)     { m_accidentalExtra  = extra;     }

    void setDrawFlag(bool df)             { m_drawFlag         = df;        }
    void setDrawStem(bool ds)             { m_drawStem         = ds;        }
    void setStemGoesUp(bool up)           { m_stemGoesUp       = up;        }
    void setStemLength(int length)        { m_stemLength       = length;    }
    void setLegerLines(int lines)         { m_legerLines       = lines;     }
    void setSlashes(int slashes)          { m_slashes          = slashes;   }
    void setRestOutside(bool os)          { m_restOutsideStave = os;        }

    void setSelected(bool selected)       { m_selected         = selected;  }
    void setHighlighted(bool highlighted) { m_highlighted      = highlighted;}
    void setQuantized(bool quantized)     { m_quantized        = quantized; }
    void setTrigger(bool trigger)         { m_trigger          = trigger;   }
    void setIsOnLine(bool isOnLine)       { m_onLine           = isOnLine;  }
    void setSafeVertDistance(int safe)    { m_safeVertDistance = safe;      }

    void setBeamed(bool beamed)           { m_beamed           = beamed;    }
    void setNextBeamCount(int tc)         { m_nextBeamCount    = tc;        }
    void setThisPartialBeams(bool pt)     { m_thisPartialBeams = pt;        }
    void setNextPartialBeams(bool pt)     { m_nextPartialBeams = pt;        }
    void setWidth(int width)              { m_width            = width;     }
    void setGradient(double gradient)     { m_gradient         = gradient;  }

    void setTupletCount(int count)        { m_tupletCount      = count;     }
    void setTuplingLineY(int y)           { m_tuplingLineY     = y;         }
    void setTuplingLineWidth(int width)   { m_tuplingLineWidth = width;     }
    void setTuplingLineGradient(double g) { m_tuplingLineGradient = g;      }
    void setTuplingLineFollowsBeam(bool b){ m_tuplingLineFollowsBeam = b;   }

    void setTied(bool tied)               { m_tied             = tied;      }
    void setTieLength(int tieLength)      { m_tieLength        = tieLength; }

    void setTiePosition(bool expl, bool above) {
        m_tiePositionExplicit = expl;
        m_tieAbove = above;
    }

    void setMarks(const std::vector<Mark> &marks);
    void removeMarks();

    void setInRange(bool inRange)         { m_inRange          = inRange;    }

    std::vector<Mark> getNormalMarks() const;
    std::vector<Mark> getAboveMarks() const; // bowings, pause etc


private:
    friend class NotePixmapFactory;

    //--------------- Data members ---------------------------------

    Note::Type m_noteType;
    int m_dots;
    Accidental m_accidental;

    bool    m_cautionary;
    bool    m_shifted;
    bool    m_dotShifted;
    int     m_accidentalShift;
    bool    m_accidentalExtra;
    bool    m_drawFlag;
    bool    m_drawStem;
    bool    m_stemGoesUp;
    int     m_stemLength;
    int     m_legerLines;
    int     m_slashes;
    bool    m_selected;
    bool    m_highlighted;
    bool    m_quantized;
    bool    m_trigger;
    bool    m_onLine;
    int     m_safeVertDistance;
    bool    m_restOutsideStave;

    bool    m_beamed;
    int     m_nextBeamCount;
    bool    m_thisPartialBeams;
    bool    m_nextPartialBeams;
    int     m_width;
    double  m_gradient;

    int     m_tupletCount;
    int     m_tuplingLineY;
    int     m_tuplingLineWidth;
    double  m_tuplingLineGradient;
    bool    m_tuplingLineFollowsBeam;

    bool    m_tied;
    int     m_tieLength;
    bool    m_tiePositionExplicit;
    bool    m_tieAbove;

    bool    m_inRange;

    std::vector<Mark> m_marks;
};


class NotePixmapPainter;


}

#endif
