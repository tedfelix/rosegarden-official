/***************************************************************************
                          staff.cpp  -  description
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

#include "staff.h"
#include "staffline.h"
#include "qcanvasspritegroupable.h"

#include "rosedebug.h"

Staff::Staff(QCanvas *canvas, Staff::Clef clef)
    : QCanvasItemGroup(canvas),
      m_currentKey(clef),
      m_barLineHeight(0),
      m_horizLineLength(0),
      m_pitchToHeight(32)
{

    // clef
    //
    QCanvasPixmapArray *clefPixmap;
    
    if (clef == Treble) {
        
        clefPixmap = new QCanvasPixmapArray("pixmaps/clef-treble.xpm");

    } else if (clef == Bass) {

        clefPixmap = new QCanvasPixmapArray("pixmaps/clef-bass.xpm");

    } else if (clef == Alto) {

        clefPixmap = new QCanvasPixmapArray("pixmaps/clef-alto.xpm");

    } else if (clef == Tenor) {
        
        clefPixmap = new QCanvasPixmapArray("pixmaps/clef-tenor.xpm");

    }
    
    
    QCanvasSpriteGroupable *clef = new QCanvasSpriteGroupable(clefPixmap, canvas, this);

    clef->moveBy(8, 0);

    // horizontal lines
    //
    int w = canvas->width();
    m_horizLineLength = w - (w / 10);

// Pitch : 0  - C  - line 5 (leger)
// Pitch : 1  - C# - line 5
// Pitch : 2  - D  - line 4.5
// Pitch : 3  - D# - line 4.5
// Pitch : 4  - E  - line 4
// Pitch : 5  - F  - line 3.5
// Pitch : 6  - F# - line 3.5
// Pitch : 7  - G  - line 3
// Pitch : 8  - G# - line 3
// Pitch : 9  - A  - line 2.5
// Pitch : 10 - A# - line 2.5
// Pitch : 11 - B  - line 2
// Pitch : 12 - C  - line 1.5
// Pitch : 13 - C# - line 1.5
// Pitch : 14 - D  - line 1
// Pitch : 15 - D# - line 1
// Pitch : 16 - E  - line 0.5
// Pitch : 17 - F  - line 0
// Pitch : 18 - F# - line 0
// Pitch : 19 - G  - line -0.5
// Pitch : 20 - G# - line -0.5
// Pitch : 21 - A  - line -1

    unsigned int pitch = 17, // F
        l = 0;

    // staff lines are numbered from 0 to 4, in a top-down order. Yes,
    // the code below is butt ugly, but it works and it's fairly easy
    // to maintain. For once I don't think trying to make this fit in
    // a loop will do any good.
    //

    // Line 0 : Top-most line (F - pitch 17 in a Treble clef)
    //
    StaffLine *staffLine = new StaffLine(canvas, this);

    int y = l * lineWidth;

    staffLine->setPoints(0,y, m_horizLineLength,y);
    staffLine->moveBy(0,linesOffset);

    m_pitchToHeight[pitch] = y; // F
    m_pitchToHeight[pitch + 1] = y; // F#

    m_pitchToHeight[pitch + 2] = y + lineWidth / 2 + 1; // G
    m_pitchToHeight[pitch + 3] = y + lineWidth / 2 + 1; // G#
    m_pitchToHeight[pitch + 4] = y + lineWidth;     // A

    // Intermediate invisible line
    //
    makeInvisibleLine(y + lineWidth / 2 + 1);
    
    // Line 1 : D - pitch 14
    //
    ++l; pitch = 14;
    staffLine = new StaffLine(canvas, this);

    y = l * lineWidth;

    staffLine->setPoints(0,y, m_horizLineLength,y);
    staffLine->moveBy(0,linesOffset);

    m_pitchToHeight[pitch] = y; // D
    m_pitchToHeight[pitch + 1] = y; // D#
    m_pitchToHeight[pitch + 2] = y + lineWidth / 2 + 1; // E
    
    makeInvisibleLine(y + lineWidth / 2 + 1);

    // Line 2 : B - pitch 11
    //
    ++l; pitch = 11;
    staffLine = new StaffLine(canvas, this);

    y = l * lineWidth;

    staffLine->setPoints(0,y, m_horizLineLength,y);
    staffLine->moveBy(0,linesOffset);

    m_pitchToHeight[pitch] = y; // B
    m_pitchToHeight[pitch + 1] = y + lineWidth / 2 + 1; // C
    m_pitchToHeight[pitch + 2] = y + lineWidth / 2 + 1; // C#
    
    makeInvisibleLine(y + lineWidth / 2 + 1);

    // Line 3 : G - pitch 7
    //
    ++l; pitch = 7;
    staffLine = new StaffLine(canvas, this);

    y = l * lineWidth;

    staffLine->setPoints(0,y, m_horizLineLength,y);
    staffLine->moveBy(0,linesOffset);

    m_pitchToHeight[pitch] = y; // G
    m_pitchToHeight[pitch + 1] = y; // G#
    m_pitchToHeight[pitch + 2] = y + lineWidth / 2 + 1; // A
    m_pitchToHeight[pitch + 3] = y + lineWidth / 2 + 1; // A#
    
    makeInvisibleLine(y + lineWidth / 2 + 1);

    // Line 4 : E - pitch 4
    //
    ++l; pitch = 4;
    staffLine = new StaffLine(canvas, this);

    y = l * lineWidth;

    staffLine->setPoints(0,y, m_horizLineLength,y);
    staffLine->moveBy(0,linesOffset);

    m_pitchToHeight[pitch] = y; // E
    m_pitchToHeight[pitch + 1] = y + lineWidth / 2 + 1; // F
    m_pitchToHeight[pitch + 2] = y + lineWidth / 2 + 1; // F#

    makeInvisibleLine(y + lineWidth / 2 + 1);

    // Line 5 : middle C - pitch 0 (not actually displayed)
    //
    ++l; pitch = 0;
    y = l * lineWidth;

    makeInvisibleLine(y);

    m_pitchToHeight[pitch] = y; // C
    m_pitchToHeight[pitch + 1] = y; // C#
    m_pitchToHeight[pitch + 2] = y + lineWidth / 2 + 1; // D
    m_pitchToHeight[pitch + 3] = y + lineWidth / 2 + 1; // D#
    
    makeInvisibleLine(y + lineWidth / 2 + 1);

    // Add vertical lines
    //
    QCanvasLineGroupable *staffVertLine = new QCanvasLineGroupable(canvas, this);

    m_barLineHeight = nbLines * lineWidth - lineWidth / 2 - 5;

    // First line - thick
    //
    QPen pen(black, 3);
    pen.setCapStyle(Qt::SquareCap);
    staffVertLine->setPen(pen);

    staffVertLine->setPoints(0,linesOffset + 1,
                             0,m_barLineHeight + linesOffset - 1);

    // Second line - thin
    //
    staffVertLine = new QCanvasLineGroupable(canvas, this);

    staffVertLine->setPoints(4,linesOffset,
                             4,m_barLineHeight + linesOffset);


    setActive(false);  // don't react to mousePress events

}

int
Staff::pitchYOffset(unsigned int pitch) const
{
    if (pitch <= 17)
        return m_pitchToHeight[pitch];
    else {
        kdDebug(KDEBUG_AREA) << "Staff::pitchYOffset(" << pitch << ") : pitch too high\n";
        return m_pitchToHeight[pitch % 18];
    }
}

void
Staff::makeInvisibleLine(int y)
{
    // Intermediate invisible line
    //
    StaffLine *invisibleLine = new StaffLine(canvas(), this);
    invisibleLine->setPen(red);
    
    invisibleLine->setPoints(0,y, m_horizLineLength,y);
    invisibleLine->moveBy(0,linesOffset);
}



const unsigned int Staff::noteHeight = 8;
const unsigned int Staff::lineWidth = noteHeight + 1;
const unsigned int Staff::noteWidth = 9;
const unsigned int Staff::stalkLen = noteHeight * 7/2 - 6;
const unsigned int Staff::nbLines = 5;
const unsigned int Staff::linesOffset = 8;

