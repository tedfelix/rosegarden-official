/***************************************************************************
                          notepixmapfactory.h  -  description
                             -------------------
    begin                : Mon Jun 19 2000
    copyright            : (C) 2000 by Guillaume Laurent, Chris Cannam, Rich Bown
    email                : glaurent@telegraph-road.org, cannam@all-day-breakfast.com, bownie@bownie.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NOTEPIXMAPFACTORY_H
#define NOTEPIXMAPFACTORY_H

#include <vector>
#include <qcanvas.h>

typedef vector<int> chordpitches;

// enum Note { WholeDotted = 0, Whole,
//             HalfDotted, Half,
//             QuarterDotted, Quarter,
//             EighthDotted, Eighth,
//             SixteenthDotted, Sixteenth,
//             ThirtySecondDotted, ThirtySecond,
//             SixtyFourthDotted, SixtyFourth, LastNote = SixtyFourth };

enum Note {
    SixtyFourth = 0, SixtyFourthDotted,
    ThirtySecond, ThirtySecondDotted,
    Sixteenth, SixteenthDotted,
    Eighth, EighthDotted,
    Quarter, QuarterDotted,
    Half, HalfDotted,
    Whole, WholeDotted,
    LastNote = WholeDotted 
};


/**Generates pixmaps for single notes and chords

  *@author Guillaume Laurent
  */
class NotePixmapFactory
{
public:
    NotePixmapFactory();
    ~NotePixmapFactory();

    /**
     * Generate a pixmap for a single note
     *
     * @param duration : note duration
     * @param drawTail : if the pixmap should have a tail or not
     *   (useful when the tail should be collapsed with the one of a neighboring note)
     * @param stalkGoesUp : if the note's stalk should go up or down
     */
    QPixmap makeNotePixmap(Note note,
                           bool drawTail,
                           bool stalkGoesUp = true);

    /**
     * Generate a pixmap for a chord
     *
     * @param pitches : a \b sorted vector of relative pitches
     * @param duration : chord duration
     * @param drawTail : if the pixmap should have a tail or not
     *   (useful when the tail should be collapsed with the one of a neighboring chord)
     * @param stalkGoesUp : if the note's stalk should go up or down
     */
    QPixmap makeChordPixmap(const chordpitches &pitches,
                            Note note, bool drawTail,
                            bool stalkGoesUp = true);

protected:
    /**
     * Transform a duration in rosegarden time units (96 = 4th)
     * to note type (Note enum)
     */
    Note duration2note(unsigned int duration);

    const QPixmap* tailUp(Note note) const;
    const QPixmap* tailDown(Note note) const;

    void drawStalk(Note note, bool drawTail, bool stalkGoesUp);
    void readjustGeneratedPixmapHeight(Note note);
    void createPixmapAndMask(unsigned int tailOffset);

    unsigned int m_generatedPixmapHeight;
    unsigned int m_noteBodyHeight;
    unsigned int m_noteBodyWidth;
    unsigned int m_tailWidth;

    QPixmap *m_generatedPixmap;
    QBitmap *m_generatedMask;

    QPainter m_p, m_pm;

    QPixmap m_noteBodyFilled;
    QPixmap m_noteBodyEmpty;

    vector<QPixmap*> m_tailsUp;
    vector<QPixmap*> m_tailsDown;
};

#endif
