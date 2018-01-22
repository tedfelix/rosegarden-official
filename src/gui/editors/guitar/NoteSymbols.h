/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    This file contains code from 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef RG_SYMBOLS_H
#define RG_SYMBOLS_H

#include <string>

#include <QBrush>
#include <QPainter>

namespace Rosegarden
{

/**
 *----------------------------------------
 * Finding X position on guitar chord pixmap
 *----------------------------------------
 *
 * Originally x = position * scale + FC::BORDER + FC::CIRCBORD + FC::FRETTEXT
 *
 * The last three can be condense into on term called XBorder
 *      XBorder = FC::BORDER + FC::CIRCBORD + FC::FRETTEXT
 *             = 5 + 2 + 10 (see fingers.h)
 *             = 17
 *
 * The drawable guitar chord space on the x-axis:
 *      XGuitarChord = pixmap width - XBorder
 *                = width - 17
 *
 * The guitar chord x-axis is broken up into colums which represent the drawable
 * space for a guitar chord component (e.g. note, barre)
 *      Column Width = XGuitarChord / number of strings
 *
 * Therefore a new x can be calculated from the position and the column width
 *      x = (position * Column Width) + XBorder
 *
 *-------------------------------------------
 * Finding Y position on guitar chord pixmap
 *-------------------------------------------
 *
 * Originally y = (FC::BORDER * scale) + (2 * FC::SPACER) + (fret * scale) + FC::CIRCBORD
 *
 * As with the x-axis the equation can be separated into the position plus the border. In
 * this case YBorder
 *      YBorder = (FC::BORDER*scale) + (2*FC::SPACER) + FC::CIRCBORD
 *              = 17 (If we want to use the same border as the x-axis)
 *
 * The drawable guitar chord space on the y-axis:
 *      YGuitarChord = pixmap height - YBorder
 *
 * The guitar chord y-axis is broken up into rows which represent the drawable
 * space for a guitar chord component (e.g. note, barre)
 *      Row Height = YGuitarChord / number of frets
 *
 * Therefore a new y can be calculated from the fret position and the row height
 *      y = fret * Row Height
 **/

namespace Guitar
{

class Fingering;


class NoteSymbols
{
private:
    typedef std::pair<unsigned int, unsigned int> posPair;

    static float const LEFT_BORDER_PERCENTAGE;
    static float const RIGHT_BORDER_PERCENTAGE;
    static float const GUITAR_CHORD_WIDTH_PERCENTAGE;
    static float const TOP_BORDER_PERCENTAGE;
    static float const BOTTOM_BORDER_PERCENTAGE;
    static float const GUITAR_CHORD_HEIGHT_PERCENTAGE;
    static int   const TOP_GUITAR_CHORD_MARGIN;
    static int   const FRET_PEN_WIDTH;
    static int   const STRING_PEN_WIDTH;
    
public:

    NoteSymbols(unsigned int nbOfStrings, unsigned int nbOfFrets) :
        m_nbOfStrings(nbOfStrings), 
        m_nbOfFrets(nbOfFrets) {};

    //! Display a mute symbol in the QPainter object
    void
    drawMuteSymbol ( bool big,
                     QPainter* p,
                     unsigned int position ) const;

    /* This code borrowed from KGuitar 0.5 */
    //! Display a open symbol in the QPainter object (KGuitar)
    void drawOpenSymbol ( bool big,
                          QPainter* p,
                          unsigned int position ) const;

    /* This code borrowed from KGuitar 0.5 */
    //! Display a note symbol in the QPainter object (KGuitar)
    void drawNoteSymbol ( bool big,
                          QPainter* p,
                          unsigned int stringNb,
                          int fretNb,
                          bool transient = false ) const;

    /* This code borrowed from KGuitar 0.5 */
    /**
     * Display a bar symbol in the QPainter object (KGuitar)
     * The code from the KGuitar project was modified to display a bar. This feature was not
     * available in that project
     */
    void drawBarreSymbol ( QPainter* p,
                           int fretNb,
                           unsigned int start,
                           unsigned int end ) const;

    void drawFretNumber ( QPainter* p,
                          unsigned int fret_num ) const;

    void drawFrets ( QPainter* p ) const;

    void drawStrings ( QPainter* p ) const;

    unsigned int getTopBorder ( unsigned int imgHeight ) const;

    unsigned int getBottomBorder ( unsigned int imgHeight ) const;

    unsigned int getLeftBorder ( unsigned int imgWidth ) const;

    unsigned int getRightBorder ( unsigned int imgWidth ) const;

    unsigned int getGuitarChordWidth ( int imgWidth ) const;

    unsigned int getGuitarChordHeight ( int imgHeight ) const;

    unsigned int getFontPixelSize ( int imgWidth, int imgHeight ) const;
    
    std::pair<bool, unsigned int>
    getStringNumber ( int imgWidth,
                      unsigned int x_pos,
                      unsigned int string_num ) const;

    std::pair<bool, unsigned int>
    getFretNumber ( int imgHeight,
                    unsigned int y_pos,
                    unsigned int maxFretNum ) const;

    QRect getTransientNoteSymbolRect(QSize guitarChordSize,
                                     unsigned int stringNb,
                                     int fretNb) const;
    
    static void drawFingeringPixmap(const Fingering& fingering, const NoteSymbols& noteSymbols, QPainter *p);
    
private:

    posPair
    getX ( int imgWidth, unsigned int stringNb, unsigned int nbOfStrings ) const;

    posPair
    getY ( int imgHeight, unsigned int fretNb, unsigned int nbOfFrets ) const;


    unsigned int m_nbOfStrings;
    unsigned int m_nbOfFrets;

};

} /* namespace Guitar */

}

#endif /* SYMBOLS_H_ */

