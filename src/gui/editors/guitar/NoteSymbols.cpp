/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

    This file contains code from 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "NoteSymbols.h"
#include "Fingering.h"
#include "misc/Debug.h"

namespace Rosegarden
{

namespace Guitar
{
NoteSymbols::posPair
NoteSymbols::getX ( int imgWidth, unsigned int stringNb, unsigned int nbOfStrings ) const
{
    /*
            std::cout << "NoteSymbols::getX - input values" << std::endl
            << "  position: " << position << std::endl
            << "  string #: " << string_num << std::endl
            << "  scale:    " << scale << std::endl;
    */
    unsigned int lBorder = getLeftBorder( imgWidth );
    unsigned int guitarChordWidth = getGuitarChordWidth( imgWidth );
    unsigned int columnWidth = guitarChordWidth / nbOfStrings;
    return std::make_pair( ( stringNb * columnWidth + lBorder ), columnWidth );
}

NoteSymbols::posPair
NoteSymbols::getY ( int imgHeight, unsigned int fretNb, unsigned int nbOfFrets ) const
{
    /*
            std::cout << "NoteSymbols::getY - input values" << std::endl
            << "  position: " << fret_pos << std::endl
            << "  max frets:   " << maxFretNum << std::endl
            << "  scale:    " << scale << std::endl;
    */
    unsigned int tBorder = getTopBorder( imgHeight );
    unsigned int guitarChordHeight = getGuitarChordHeight( imgHeight );
    unsigned int rowHeight = guitarChordHeight / nbOfFrets;
    return std::make_pair( ( ( fretNb * rowHeight ) + tBorder ), rowHeight );
}

void
NoteSymbols::drawMuteSymbol ( bool big,
                              QPainter* p,
                              unsigned int position ) const
{
    RG_DEBUG << "NoteSymbols::drawMuteSymbol()";

    QRect v = p->viewport();

    posPair x_pos = getX ( v.width(), position, m_nbOfStrings );
    unsigned int y_pos = (getTopBorder( v.height() ) / 2) + 2;
    double columnWidth = x_pos.second;
    unsigned int width = static_cast<unsigned int>( columnWidth * 0.7 );
    unsigned int height = static_cast<unsigned int>( columnWidth * 0.7 );

    //std::cout << "NoteSymbols::drawMuteSymbol - drawing Mute symbol at string #" << position
    //<< std::endl;

    QPen pen(Qt::black);
    if (big) pen.setWidth(2);

    p->save();

    p->setPen(pen);

    p->drawLine ( x_pos.first - ( width / 2 ),
                  y_pos - ( height / 2 ),
                  ( x_pos.first + ( width / 2 ) ),
                  y_pos + ( height / 2 ) );

    p->drawLine( x_pos.first + ( width / 2 ),
                 y_pos - ( height / 2 ),
                 ( x_pos.first - ( width / 2 ) ),
                 y_pos + ( height / 2 ) );

    p->restore();
}

void
NoteSymbols::drawOpenSymbol ( bool big,
                              QPainter* p,
                              unsigned int position ) const
{
    RG_DEBUG << "NoteSymbols::drawOpenSymbol()";
    
    QRect v = p->viewport();
    posPair x_pos = getX ( v.width(), position, m_nbOfStrings );
    unsigned int y_pos = (getTopBorder(v.height()) / 2) + 2;
    double columnWidth = x_pos.second;
    unsigned int radius = static_cast<unsigned int>( columnWidth * 0.7 );

    //std::cout << "NoteSymbols::drawOpenSymbol - drawing Open symbol at string #" << position
    //<< std::endl;

    QPen stylus(Qt::black);
    if (big) stylus.setWidth(2);

    p->save();

    p->setPen(stylus);
    p->drawEllipse( x_pos.first - ( radius / 2 ),
                    y_pos - ( radius / 2 ),
                    radius,
                    radius );

    // little hack here to try to fix a problem I don't quite understand with
    // brute force
    p->setBrush(Qt::white);
    if (big) {
        p->drawEllipse( x_pos.first - ( radius / 2 ) + 1,
                        y_pos - ( radius / 2 ) + 1,
                        radius - 2,
                        radius - 2);
    }/* else {
        p->drawEllipse( x_pos.first - ( radius / 2 ) + 1,
                        y_pos - ( radius / 2 ) + 1,
                        radius - 3,
                        radius - 3);
    }*/

    p->restore();
}

void
NoteSymbols::drawNoteSymbol ( bool /* big */,
                              QPainter* p,
                              unsigned int stringNb,
                              int fretNb,
                              bool transient ) const
{
//    NOTATION_DEBUG << "NoteSymbols::drawNoteSymbol - string: " << stringNb << ", fret:" << fretNb;
    RG_DEBUG << "NoteSymbols::drawNoteSymbol()";

    QRect v = p->viewport();
    posPair x_pos = getX ( v.width(), stringNb, m_nbOfStrings );
    posPair y_pos = getY ( v.height(), fretNb, m_nbOfFrets );
    double columnWidth = x_pos.second;
    unsigned int radius;

    p->save();

    if (transient) {
        radius =  static_cast<unsigned int>( columnWidth /* * 0.9 */ );
        p->setPen(QColor(0, 0x10, 0xFF, 0xAA));
    } else {
        radius =  static_cast<unsigned int>( columnWidth * 0.7 );
        p->setBrush(Qt::black);
    }

    int x = x_pos.first - ( radius / 2 ),
        y = y_pos.first + ( (y_pos.second - radius) / 2) - y_pos.second + TOP_GUITAR_CHORD_MARGIN; 

//        y = y_pos.first - (radius / 2) - y_pos.second + TOP_GUITAR_CHORD_MARGIN;

//    RG_DEBUG << "NoteSymbols::drawNoteSymbol : rect = " << QRect(x,y, radius, radius);

    p->drawEllipse( x,
                    y,
                    radius,
                    radius );

    p->restore();
}

void
NoteSymbols::drawBarreSymbol ( QPainter* p,
                               int fretNb,
                               unsigned int start,
                               unsigned int end ) const
{

    //std::cout << "NoteSymbols::drawBarreSymbol - start: " << start << ", end:" << end << std::endl;

    drawNoteSymbol (false, p, start, fretNb );

    if ( ( end - start ) >= 1 ) {
        QRect v = p->viewport();
        posPair startXPos = getX ( v.width(), start, m_nbOfStrings );
        posPair endXPos = getX ( v.width(), end, m_nbOfStrings );
        posPair y_pos = getY ( v.height(), fretNb, m_nbOfFrets );
        double columnWidth = startXPos.second;
        unsigned int thickness = static_cast<unsigned int>( columnWidth * 0.7 );

        QPen pen(Qt::red); // to see if this is ever used 

        p->save();

        p->setPen(pen);

        p->drawRect( startXPos.first,
                     y_pos.first + ( y_pos.second / 4 ) + TOP_GUITAR_CHORD_MARGIN,
                     endXPos.first - startXPos.first,
                     thickness );

        p->restore();
    }

    drawNoteSymbol (false, p, end, fretNb );
}

void
NoteSymbols::drawFretNumber ( QPainter* p,
                              unsigned int fret_num ) const
{
    if ( fret_num > 1 ) {
        QRect v = p->viewport();
        unsigned int imgWidth = v.width();
        unsigned int imgHeight = v.height();

        p->save();
        QFont font;
        font.setPixelSize(getFontPixelSize(v.width(), v.height()));
        p->setFont(font);

        QString tmp;
        tmp.setNum( fret_num );

        // Get the Y coord of the first fret.
        posPair y_pos = getY( imgHeight, 1, m_nbOfFrets );

        // Compute the "true" center.
        int y = y_pos.first + TOP_GUITAR_CHORD_MARGIN;

        // Make a rect around the center.  Don't worry about the size as 
        // boundingRect() will give us the required bounding rect.
        QRect rect(getLeftBorder( imgWidth ) / 4, y - 10, 20, 20);

        p->setPen(Qt::black);

        // Using AlignVCenter ends up slightly high as the descent is probably
        // also included.  Looks OK, though.

        // Get the required bounding rect.
        QRect requiredRect = p->boundingRect(
            rect, Qt::AlignVCenter | Qt::AlignLeft, tmp);

        // Use the required bounding rect to draw the text.
        p->drawText(requiredRect, Qt::AlignVCenter | Qt::AlignLeft, tmp);

        p->restore();
    }
}

void
NoteSymbols::drawFrets ( QPainter* p ) const
{
    /*
            std::cout << "NoteSymbols::drawFretHorizontalLines" << std::endl
            << "  scale: " << scale << std::endl
            << "  frets: " << fretsDisplayed << std::endl
            << "  max string: " << maxStringNum << std::endl;
    */

    QRect v = p->viewport();
    unsigned int imgWidth = v.width();
    unsigned int imgHeight = v.height();
    //unsigned int endXPos = getGuitarChordWidth(imgWidth) + getLeftBorder(imgWidth);
    posPair endXPos = getX ( imgWidth, m_nbOfStrings - 1, m_nbOfStrings );

    unsigned int yGuitarChord = getGuitarChordHeight( imgHeight );
    unsigned int rowHeight = yGuitarChord / m_nbOfFrets;

    QPen pen(p->pen());
    pen.setWidth(imgHeight >= 100 ? FRET_PEN_WIDTH : FRET_PEN_WIDTH / 2);
    pen.setColor(Qt::black);
    p->save();
    p->setPen(pen);
    unsigned int y_pos = (getY ( imgHeight, 0, m_nbOfFrets )).first + TOP_GUITAR_CHORD_MARGIN;
    
//    NOTATION_DEBUG << "NoteSymbols::drawFrets : " << m_nbOfFrets;
    
    // Horizontal lines
    for ( unsigned int i = 0; i <= m_nbOfFrets; ++i ) {

        /* This code borrowed from KGuitar 0.5 */
        p->drawLine( getLeftBorder( imgWidth ),
                     y_pos,
                     endXPos.first,
                     y_pos);
//        NOTATION_DEBUG << "NoteSymbols::drawFrets : " << QPoint(getLeftBorder(imgWidth), y_pos)
//                       << " to " << QPoint(endXPos.first, y_pos) << endl;
                     

       y_pos += rowHeight;
    }

    p->restore();

}

void
NoteSymbols::drawStrings ( QPainter* p ) const
{
    // Vertical lines
    QRect v = p->viewport();
    int imgHeight = v.height();
    int imgWidth = v.width();

    unsigned int startPos = getTopBorder( imgHeight ) + TOP_GUITAR_CHORD_MARGIN;
    unsigned int endPos = (getY ( imgHeight, m_nbOfFrets, m_nbOfFrets )).first + TOP_GUITAR_CHORD_MARGIN;

    unsigned int guitarChordWidth = getGuitarChordWidth( imgWidth );
    unsigned int columnWidth = guitarChordWidth / m_nbOfStrings;

    unsigned int x_pos = (getX ( imgWidth, 0, m_nbOfStrings )).first;

    QPen pen(p->pen());
    pen.setWidth(imgWidth >= 100 ? STRING_PEN_WIDTH : STRING_PEN_WIDTH / 2);  
    pen.setColor(Qt::black);
    p->save();
    p->setPen(pen);

    for ( unsigned int i = 0; i < m_nbOfStrings; ++i ) {

        /* This code borrowed from KGuitar 0.5 */
        p->drawLine( x_pos,
                     startPos,
                     x_pos,
                     endPos );
                     
       x_pos += columnWidth;
    }

    p->restore();
    
}

QRect NoteSymbols::getTransientNoteSymbolRect(QSize guitarChordSize,
                                              unsigned int stringNb,
                                              int fretNb) const
{
    posPair x_pos = getX ( guitarChordSize.width(), stringNb, m_nbOfStrings );
    posPair y_pos = getY ( guitarChordSize.height(), fretNb, m_nbOfFrets );
    double columnWidth = x_pos.second;
    unsigned int radius =  static_cast<unsigned int>( columnWidth /* * 0.9 */ );

    int x = x_pos.first - ( radius / 2 ),
        y = y_pos.first + ( (y_pos.second - radius) / 2) - y_pos.second + TOP_GUITAR_CHORD_MARGIN; 

    return QRect(x, y, radius, radius);
}

unsigned int
NoteSymbols::getTopBorder ( unsigned int imgHeight ) const
{
    return static_cast<unsigned int>( TOP_BORDER_PERCENTAGE * imgHeight );
}

unsigned int
NoteSymbols::getBottomBorder ( unsigned int imgHeight ) const
{
    return static_cast<unsigned int>( imgHeight * BOTTOM_BORDER_PERCENTAGE );
}

unsigned int
NoteSymbols::getLeftBorder ( unsigned int imgWidth ) const
{
    unsigned int left = static_cast<unsigned int>( imgWidth * LEFT_BORDER_PERCENTAGE );
    if ( left < 15 ) {
        left = 15;
    }
    return left;
}

unsigned int
NoteSymbols::getRightBorder ( unsigned int imgWidth ) const
{
    return static_cast<unsigned int>( imgWidth * RIGHT_BORDER_PERCENTAGE );
}

unsigned int
NoteSymbols::getGuitarChordWidth ( int imgWidth ) const
{
    return static_cast<unsigned int>( imgWidth * GUITAR_CHORD_WIDTH_PERCENTAGE );
}

unsigned int
NoteSymbols::getGuitarChordHeight ( int imgHeight ) const
{
    return static_cast<unsigned int>( imgHeight * GUITAR_CHORD_HEIGHT_PERCENTAGE );
}

unsigned int
NoteSymbols::getFontPixelSize ( int /* imgWidth */, int imgHeight ) const
{
    return std::max(8, imgHeight / 10);
}

std::pair<bool, unsigned int>
NoteSymbols::getStringNumber ( int imgWidth,
                               unsigned int x_pos,
                               unsigned int maxStringNum ) const
{
    /*
        std::cout << "NoteSymbols::getNumberOfStrings - input values" << std::endl
        << "  X position: " << x_pos << std::endl
        << "  string #: " << maxStringNum << std::endl
        << "  image width:    " << imgWidth << std::endl;
    */
    bool valueOk = false;

    posPair xPairPos;
    unsigned int min = 0;
    unsigned int max = 0;
    unsigned int result = 0;

    for ( unsigned int i = 0; i < maxStringNum; ++i ) {
        xPairPos = getX ( imgWidth, i, maxStringNum );

        // If the counter equals zero then we are at the first
        // string to the left
        if ( i == 0 ) {
            // Add 10 pixel buffer to range comparison
            min = xPairPos.first - 10;
        } else {
            min = xPairPos.first - xPairPos.second / 2;
        }

        // If the counter equals the maxString number -1 then we are at the last
        // string to the right
        if ( i == ( maxStringNum - 1 ) ) {
            // Add 10 pixel buffer to range comparison
            max = xPairPos.first + 10;
        } else {
            max = xPairPos.first + xPairPos.second / 2;
        }

        if ( ( x_pos >= min ) && ( x_pos <= max ) ) {
            result = i;
            valueOk = true;
            break;
        }
    }

    //std::cout << "NoteSymbols::getNumberOfStrings - string: #" << result << std::endl;
    return std::make_pair( valueOk, result );
}

std::pair<bool, unsigned int>
NoteSymbols::getFretNumber ( int imgHeight,
                             unsigned int y_pos,
                             unsigned int maxFretNum ) const
{
    /*
        std::cout << "NoteSymbols::getNumberOfFrets - input values" << std::endl
        << "  Y position: " << y_pos << std::endl
        << "  max frets:   " << maxFretNum << std::endl
        << "  image height:    " << imgHeight << std::endl;
    */

    bool valueOk = false;
    unsigned int tBorder = getTopBorder( imgHeight );
    unsigned int result = 0;

    if ( y_pos < tBorder ) {
        // User pressing above the guitar chord to mark line muted or opened
        valueOk = true;
    } else {
        posPair min_pos;
        posPair max_pos;

        for ( unsigned int i = 0; i < maxFretNum; ++i ) {
            min_pos = getY ( imgHeight, i, maxFretNum );
            max_pos = getY ( imgHeight, i + 1, maxFretNum );

            if ( ( y_pos >= min_pos.first ) && y_pos <= max_pos.first - 1 ) {
                result = i + 1;
                valueOk = true;
                break;
            }
        }
    }
    //    std::cout << "  fret #: " << result << std::endl;
    return std::make_pair( valueOk, result );
}

void
NoteSymbols::drawFingeringPixmap(const Guitar::Fingering& fingering, const Guitar::NoteSymbols& noteSymbols, QPainter *p)
{
    unsigned int startFret = fingering.getStartFret();
    unsigned int stringNb = 0;

    for (Fingering::const_iterator pos = fingering.begin();
         pos != fingering.end();
         ++pos, ++stringNb) {
                
        switch (*pos) {
        case Fingering::OPEN:
                noteSymbols.drawOpenSymbol(false, p, stringNb);
                break;

        case Fingering::MUTED:
                noteSymbols.drawMuteSymbol(false, p, stringNb);
                break;

        default:
                noteSymbols.drawNoteSymbol(false, p, stringNb, *pos - (startFret - 1), false);
                break;
        }
    }
   
    // draw frets last, so the sharp lines don't get broken by the fuzzy
    // outlines of the new antialiased note symbols
    noteSymbols.drawFretNumber(p, startFret);
    noteSymbols.drawFrets(p);
    noteSymbols.drawStrings(p);
}


float const NoteSymbols::LEFT_BORDER_PERCENTAGE = 0.2;
float const NoteSymbols::RIGHT_BORDER_PERCENTAGE = 0.1;
float const NoteSymbols::GUITAR_CHORD_WIDTH_PERCENTAGE = 0.8;
float const NoteSymbols::TOP_BORDER_PERCENTAGE = 0.1;
float const NoteSymbols::BOTTOM_BORDER_PERCENTAGE = 0.1;
float const NoteSymbols::GUITAR_CHORD_HEIGHT_PERCENTAGE = 0.8;
int   const NoteSymbols::TOP_GUITAR_CHORD_MARGIN = 5;
int   const NoteSymbols::FRET_PEN_WIDTH = 2;
int   const NoteSymbols::STRING_PEN_WIDTH = 2;

} /* namespace Guitar */

}

