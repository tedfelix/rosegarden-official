#include "symbols.h"

namespace Guitar
{
NoteSymbols::posPair
NoteSymbols::getX ( int imgWidth, unsigned int position, unsigned int string_num )
{
    /*
            std::cout << "NoteSymbols::getX - input values" << std::endl
            << "  position: " << position << std::endl
            << "  string #: " << string_num << std::endl
            << "  scale:    " << scale << std::endl;
    */
    unsigned int lBorder = getLeftBorder( imgWidth );
    unsigned int fretboard = getFretboardWidth( imgWidth );
    unsigned int columnWidth = fretboard / string_num;
    return std::make_pair( ( position * columnWidth + lBorder ), columnWidth );
}

NoteSymbols::posPair
NoteSymbols::getY ( int imgHeight, unsigned int fret_pos, unsigned int maxFretNum )
{
    /*
            std::cout << "NoteSymbols::getY - input values" << std::endl
            << "  position: " << fret_pos << std::endl
            << "  max frets:   " << maxFretNum << std::endl
            << "  scale:    " << scale << std::endl;
    */
    unsigned int tBorder = getTopBorder( imgHeight );
    unsigned int yFretboard = getFretboardHeight( imgHeight );
    unsigned int rowHeight = yFretboard / maxFretNum;
    return std::make_pair( ( ( fret_pos * rowHeight ) + tBorder ), rowHeight );
}

void
NoteSymbols::drawMuteSymbol ( QPainter* p,
                              unsigned int position,
                              unsigned int fretDisplayed,
                              unsigned int string_num )
{
    QRect v = p->viewport();

    posPair x_pos = getX ( v.width(), position, string_num );
    unsigned int y_pos = getTopBorder( v.height() ) / 2;
    double columnWidth = x_pos.second;
    unsigned int width = static_cast<unsigned int>( columnWidth * 0.7 );
    unsigned int height = static_cast<unsigned int>( columnWidth * 0.7 );

    //std::cout << "NoteSymbols::drawMuteSymbol - drawing Mute symbol at string #" << position
    //<< std::endl;

    p->drawLine ( x_pos.first - ( width / 2 ),
                  y_pos - ( height / 2 ),
                  ( x_pos.first + ( width / 2 ) ),
                  y_pos + ( height / 2 ) );

    p->drawLine( x_pos.first + ( width / 2 ),
                 y_pos - ( height / 2 ),
                 ( x_pos.first - ( width / 2 ) ),
                 y_pos + ( height / 2 ) );
}

void
NoteSymbols::drawOpenSymbol ( QPainter* p,
                              unsigned int position,
                              unsigned int fretDisplayed,
                              unsigned int string_num )
{
    QRect v = p->viewport();
    posPair x_pos = getX ( v.width(), position, string_num );
    unsigned int y_pos = getTopBorder( v.height() ) / 2;
    double columnWidth = x_pos.second;
    unsigned int radius = static_cast<unsigned int>( columnWidth * 0.7 );

    //std::cout << "NoteSymbols::drawOpenSymbol - drawing Open symbol at string #" << position
    //<< std::endl;

    p->setBrush( Qt::NoBrush );
    p->drawEllipse( x_pos.first - ( radius / 2 ),
                    y_pos - ( radius / 2 ),
                    radius,
                    radius );
}

void
NoteSymbols::drawNoteSymbol ( QPainter* p,
                              unsigned int position,
                              int fret,
                              unsigned int string_num,
                              unsigned int fretDisplayed )
{
    //std::cout << "NoteSymbols::drawNoteSymbol - string: " << position << ", fret:" << fret
    //<< std::endl;

    QRect v = p->viewport();
    posPair x_pos = getX ( v.width(), position, string_num );
    posPair y_pos = getY ( v.height(), fret, fretDisplayed );
    double columnWidth = x_pos.second;
    unsigned int radius = static_cast<unsigned int>( columnWidth * 0.7 );

    p->setBrush( Qt::SolidPattern );
    p->drawEllipse( x_pos.first - ( radius / 2 ),
                    y_pos.first + ( y_pos.second / 4 ),
                    radius,
                    radius );
}

void
NoteSymbols::drawBarreSymbol ( QPainter* p,
                               int fret,
                               unsigned int start,
                               unsigned int end,
                               unsigned int string_num,
                               unsigned int fretDisplayed )
{

    //std::cout << "NoteSymbols::drawBarreSymbol - start: " << start << ", end:" << end << std::endl;

    drawNoteSymbol ( p, start, fret, string_num, fretDisplayed );

    if ( ( end - start ) >= 1 )
    {
        QRect v = p->viewport();
        posPair startXPos = getX ( v.width(), start, string_num );
        posPair endXPos = getX ( v.width(), end, string_num );
        posPair y_pos = getY ( v.height(), fret, fretDisplayed );
        double columnWidth = startXPos.second;
        unsigned int thickness = static_cast<unsigned int>( columnWidth * 0.7 );

        p->drawRect( startXPos.first,
                     y_pos.first + ( y_pos.second / 4 ),
                     endXPos.first - startXPos.first,
                     thickness );
    }

    drawNoteSymbol ( p, end, fret, string_num, fretDisplayed );
}

void
NoteSymbols::drawFretNumber ( QPainter* p,
                              unsigned int fret_num,
                              unsigned int fretsDisplayed )
{
    if ( fret_num > 1 )
    {
        QRect v = p->viewport();
        unsigned int imgWidth = v.width();
        unsigned int imgHeight = v.height();

        QString tmp;
        tmp.setNum( fret_num );

        // Use NoteSymbols to grab X and Y for first fret
        posPair y_pos = getY( imgHeight, 0, fretsDisplayed );

        p->drawText( getLeftBorder( imgWidth ) / 4,
                     y_pos.first + ( y_pos.second / 2 ),
                     tmp );
    }
}

void
NoteSymbols::drawFretHorizontalLines ( QPainter* p,
                                       unsigned int fretsDisplayed,
                                       unsigned int maxStringNum )
{
    /*
            std::cout << "NoteSymbols::drawFretHorizontalLines" << std::endl
            << "  scale: " << scale << std::endl
            << "  frets: " << fretsDisplayed << std::endl
            << "  max string: " << maxStringNum << std::endl;
    */

    QRect v = p->viewport();
    unsigned int imgWidth = v.width();
    //unsigned int endXPos = getFretboardWidth(imgWidth) + getLeftBorder(imgWidth);
    posPair endXPos = getX ( imgWidth, maxStringNum - 1, maxStringNum );

    // Horizontal lines
    for ( unsigned int i = 0; i <= fretsDisplayed; ++i )
    {
        posPair y_pos = getY ( imgWidth, i, fretsDisplayed );

        /* This code borrowed from KGuitar 0.5 */
        p->drawLine( getLeftBorder( imgWidth ),
                     y_pos.first,
                     endXPos.first,
                     y_pos.first );
    }
}

void
NoteSymbols::drawFretVerticalLines ( QPainter* p,
                                     unsigned int fretsDisplayed,
                                     unsigned int maxStringNum )
{
    // Vertical lines
    QRect v = p->viewport();
    int imgHeight = v.height();
    int imgWidth = v.width();

    unsigned int startPos = getTopBorder( imgHeight );
    posPair endPos = getY ( imgHeight, fretsDisplayed, fretsDisplayed );

    for ( unsigned int i = 0; i < maxStringNum; ++i )
    {
        posPair x_pos = getX ( imgWidth, i, maxStringNum );

        /* This code borrowed from KGuitar 0.5 */
        p->drawLine( x_pos.first,
                     startPos,
                     x_pos.first,
                     endPos.first );
    }
}

unsigned int
NoteSymbols::getTopBorder ( unsigned int imgHeight )
{
    return static_cast<unsigned int>( TOP_BORDER_PERCENTAGE * imgHeight );
}

unsigned int
NoteSymbols::getBottomBorder ( unsigned int imgHeight )
{
    return static_cast<unsigned int>( imgHeight * BOTTOM_BORDER_PERCENTAGE );
}

unsigned int
NoteSymbols::getLeftBorder ( unsigned int imgWidth )
{
    unsigned int left = static_cast<unsigned int>( imgWidth * LEFT_BORDER_PERCENTAGE );
    if ( left < 15 )
    {
        left = 15;
    }
    return left;
}

unsigned int
NoteSymbols::getRightBorder ( unsigned int imgWidth )
{
    return static_cast<unsigned int>( imgWidth * RIGHT_BORDER_PERCENTAGE );
}

unsigned int
NoteSymbols::getFretboardWidth ( int imgWidth )
{
    return static_cast<unsigned int>( imgWidth * FRETBOARD_WIDTH_PERCENTAGE );
}

unsigned int
NoteSymbols::getFretboardHeight ( int imgHeight )
{
    return static_cast<unsigned int>( imgHeight * FRETBOARD_HEIGHT_PERCENTAGE );
}

std::pair<bool, unsigned int>
NoteSymbols::getStringNumber ( int imgWidth,
                               unsigned int x_pos,
                               unsigned int maxStringNum )
{
    /*
        std::cout << "NoteSymbols::getStringNumber - input values" << std::endl
        << "  X position: " << x_pos << std::endl
        << "  string #: " << maxStringNum << std::endl
        << "  image width:    " << imgWidth << std::endl;
    */
    bool valueOk = false;

    posPair xPairPos;
    unsigned int min = 0;
    unsigned int max = 0;
    unsigned int result = 0;

    for ( unsigned int i = 0; i < maxStringNum; ++i )
    {
        xPairPos = getX ( imgWidth, i, maxStringNum );

        // If the counter equals zero then we are at the first
        // string to the left
        if ( i == 0 )
        {
            // Add 10 pixel buffer to range comparison
            min = xPairPos.first - 10;
        }
        else
        {
            min = xPairPos.first - xPairPos.second / 2;
        }

        // If the counter equals the maxString number -1 then we are at the last
        // string to the right
        if ( i == ( maxStringNum - 1 ) )
        {
            // Add 10 pixel buffer to range comparison
            max = xPairPos.first + 10;
        }
        else
        {
            max = xPairPos.first + xPairPos.second / 2;
        }

        if ( ( x_pos >= min ) && ( x_pos <= max ) )
        {
            result = i;
            valueOk = true;
            break;
        }
    }

    //std::cout << "NoteSymbols::getStringNumber - string: #" << result << std::endl;
    return std::make_pair( valueOk, result );
}

std::pair<bool, unsigned int>
NoteSymbols::getFretNumber ( int imgHeight,
                             unsigned int y_pos,
                             unsigned int maxFretNum )
{
/*
    std::cout << "NoteSymbols::getFretNumber - input values" << std::endl
    << "  Y position: " << y_pos << std::endl
    << "  max frets:   " << maxFretNum << std::endl
    << "  image height:    " << imgHeight << std::endl;
*/

    bool valueOk = false;
    unsigned int tBorder = getTopBorder( imgHeight );
    unsigned int result = 0;

    if ( y_pos < tBorder )
    {
        // User pressing above the fretboard to mark line muted or opened
        valueOk = true;
    }
    else
    {
        typedef std::pair<unsigned int, unsigned int> RangePair;

        posPair min_pos;
        posPair max_pos;

        for ( unsigned int i = 0; i < maxFretNum; ++i )
        {
            min_pos = getY ( imgHeight, i, maxFretNum );
            max_pos = getY ( imgHeight, i + 1, maxFretNum );

            if ( ( y_pos >= min_pos.first ) && y_pos <= max_pos.first - 1 )
            {
                result = i;
                valueOk = true;
                break;
            }
        }
    }
//    std::cout << "  fret #: " << result << std::endl;
    return std::make_pair( valueOk, result );
}

float const NoteSymbols::LEFT_BORDER_PERCENTAGE = 0.1;
float const NoteSymbols::RIGHT_BORDER_PERCENTAGE = 0.1;
float const NoteSymbols::FRETBOARD_WIDTH_PERCENTAGE = 0.8;
float const NoteSymbols::TOP_BORDER_PERCENTAGE = 0.1;
float const NoteSymbols::BOTTOM_BORDER_PERCENTAGE = 0.1;
float const NoteSymbols::FRETBOARD_HEIGHT_PERCENTAGE = 0.8;


} /* namespace Guitar */

