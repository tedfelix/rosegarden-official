
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_NOTEPIXMAPPARAMETERS_H
#define RG_NOTEPIXMAPPARAMETERS_H

#include "base/NotationTypes.h"
#include <vector>
#include <cmath>




namespace Rosegarden
{



class NotePixmapParameters
{
public:
    enum Triggering { triggerNone, triggerYes, triggerSkip, };
    
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
    void setTrigger(Triggering trigger)   { m_trigger          = trigger;   }
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

    void setMemberOfParallel(bool memberOfParallel) { m_memberOfParallel = memberOfParallel; }

    void setMarks(const std::vector<Mark> &marks);
    void removeMarks();

    void setInRange(bool inRange)         { m_inRange          = inRange;    }

    /** Return a list of normal marks that draw either above or below the note
     * head, opposite the stem direction
     */
    std::vector<Mark> getNormalMarks() const;

    /** Return a list of marks that must always be drawn above the note, even if
     * this means drawing above the stem too
     */
    std::vector<Mark> getAboveMarks() const; // bowings, pause etc

    // While I'm in here, it seems to me there's something or other that should
    // always be drawn *below* the note, and we get it wrong, and/or there are
    // some things we treat as normal marks and shouldn't.  Hrm.

    bool operator==(const NotePixmapParameters &p) {
	return (m_noteType == p.m_noteType &&
		m_dots == p.m_dots &&
		m_accidental == p.m_accidental &&
		m_cautionary == p.m_cautionary &&
		m_shifted == p.m_shifted &&
		m_dotShifted == p.m_dotShifted &&
		m_accidentalShift == p.m_accidentalShift &&
		m_accidentalExtra == p.m_accidentalExtra &&
		m_drawFlag == p.m_drawFlag &&
		m_drawStem == p.m_drawStem &&
		m_stemGoesUp == p.m_stemGoesUp &&
		m_stemLength == p.m_stemLength &&
		m_legerLines == p.m_legerLines &&
		m_slashes == p.m_slashes &&
		m_selected == p.m_selected &&
		m_highlighted == p.m_highlighted &&
		m_quantized == p.m_quantized &&
		m_trigger == p.m_trigger &&
		m_onLine == p.m_onLine &&
		m_safeVertDistance == p.m_safeVertDistance &&
		m_restOutsideStave == p.m_restOutsideStave &&

		m_beamed == p.m_beamed &&
		m_nextBeamCount == p.m_nextBeamCount &&
		m_thisPartialBeams == p.m_thisPartialBeams &&
		m_nextPartialBeams == p.m_nextPartialBeams &&
		m_width == p.m_width &&
		fabs(m_gradient - p.m_gradient) < 0.0001 &&

		m_tupletCount == p.m_tupletCount &&
		m_tuplingLineY == p.m_tuplingLineY &&
		m_tuplingLineWidth == p.m_tuplingLineWidth &&
		fabs(m_tuplingLineGradient - p.m_tuplingLineGradient) < 0.0001 &&
		m_tuplingLineFollowsBeam == p.m_tuplingLineFollowsBeam &&

		m_tied == p.m_tied &&
		m_tieLength == p.m_tieLength &&
		m_tiePositionExplicit == p.m_tiePositionExplicit &&
		m_tieAbove == p.m_tieAbove &&

		m_inRange == p.m_inRange &&
		m_marks == p.m_marks &&

		m_memberOfParallel == p.m_memberOfParallel);
    }

private:
    friend class NotePixmapFactory;
    friend class NotationStaff;

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
    Triggering m_trigger;
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

    bool    m_memberOfParallel;
};


class NotePixmapPainter;


}

#endif
