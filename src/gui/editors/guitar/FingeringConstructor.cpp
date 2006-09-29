#include "Fingering.h"
#include "NoteSymbols.h"

#include <qpainter.h>
#include <qdrawutil.h>
#include <qevent.h>
#include <qbrush.h>
#include <qstring.h>
#include <qscrollbar.h>

#include <kcursor.h>

#include <iostream>

namespace Rosegarden
{

namespace Guitar
{

/*------------------------------------------
     Parent: Fingering Constructor
-------------------------------------------*/
FingeringConstructor::FingeringConstructor
( GuitarNeck *instr,
  QWidget *parent,
  Mode state,
  const char *name )
        : QFrame( parent, name ),
        m_instr ( instr ),
        m_mode ( state ),
        m_chord_arrangement ( new Fingering ( instr ) ),
        m_frets_displayed ( MAX_FRET_DISPLAYED ),
        m_press_string_num ( 0 ),
        m_press_fret_num ( 0 ),
        INSERT ( new FC_InsertMode( this ) ),
        DELETE ( new FC_DeleteMode( this ) ),
        m_presentMode ( INSERT )
{
    setFixedSize( IMG_WIDTH, IMG_HEIGHT );
    setFrameStyle( Panel | Sunken );
    setBackgroundMode( PaletteBase );

    if ( m_mode == EDITABLE ) {
        m_fret_spinbox =
            new QSpinBox ( 1,
                           m_instr->getFretNumber() - m_frets_displayed + 1,
                           1,
                           this );
        m_fret_spinbox->setGeometry( width() - SCROLL, 0, SCROLL, height() );

        connect( m_fret_spinbox,
                 SIGNAL( valueChanged( int ) ),
                 SLOT( setFirstFret( int ) ) );
    }

    clear();
}

void FingeringConstructor::clear()
{
    m_instr->clear();
    if ( m_chord_arrangement != 0 ) {
        delete m_chord_arrangement;
    }
    m_chord_arrangement = new Fingering ( m_instr );
    update();
    emit chordChange();
}

void FingeringConstructor::setFingering ( Fingering* arrange )
{
    m_chord_arrangement = new Fingering( *arrange );
    update();
    emit chordChange();
}

void FingeringConstructor::setFirstFret( int fret )
{
    m_chord_arrangement->setFirstFret ( fret );
    update();
    emit chordChange();
}

void FingeringConstructor::drawContents( QPainter *p )
{
    /*
            // Horizontal separator line
            NoteSymbols ns;
            ns.drawSeperator(p, SCALE, m_instr->getStringNumber());
            ns.drawFretHorizontalLines (p, SCALE, m_frets_displayed, m_instr->getStringNumber());
            ns.drawFretVerticalLines (p, SCALE, m_frets_displayed, m_instr->getStringNumber());
    */

    if ( m_mode == EDITABLE ) {
        // Set beginning fret number and scroll bar
        m_fret_spinbox->setValue ( m_chord_arrangement->getFirstFret() );
    }

    // Draw Fingering
    m_chord_arrangement->drawContents ( p, m_frets_displayed );

}

Fingering*
FingeringConstructor::getFingering ( void ) const
{
    return m_chord_arrangement;
}

unsigned int
FingeringConstructor::getStringNumber ( QMouseEvent const* event )
{
    const QPoint pos = event->pos();
    NoteSymbols ns;
    PositionPair result = ns.getStringNumber ( maximumHeight(),
                          pos.x(),
                          m_instr->getStringNumber() );
    unsigned int string_num = 0;

    if ( result.first ) {
        string_num = m_instr->getStringNumber() - result.second;
    }

    return string_num;
}

unsigned int
FingeringConstructor::getFretNumber ( QMouseEvent const* event )
{
    NoteSymbols ns;
    const QPoint pos = event->pos();
    unsigned int y_pos = pos.y();
    unsigned int fret_num = 0;

    if ( y_pos > ns.getTopBorder( maximumHeight() ) ) {
        // If fret position is below the top line of the fretboard image.
        PositionPair result = ns.getFretNumber ( maximumWidth(),
                              y_pos,
                              m_frets_displayed );

        if ( result.first ) {
            fret_num = result.second + m_fret_spinbox->value();
        }
    }

    return fret_num;
}

void FingeringConstructor::mousePressEvent( QMouseEvent *event )
{
    if ( ( event->button() == LeftButton ) && ( m_mode == EDITABLE ) ) {

        // Find string position
        m_press_string_num = this->getStringNumber ( event );

        // Find fret position
        m_press_fret_num = this->getFretNumber ( event );
    }
}

void FingeringConstructor::mouseReleaseEvent( QMouseEvent *event )
{
    // If we are display only then we will not updated on a mouse event
    if ( m_mode == DISPLAY_ONLY ) {
        return ;
    }

    unsigned int release_string_num = this->getStringNumber( event );
    unsigned int release_fret_num = this->getFretNumber( event );

    m_presentMode->mouseRelease ( release_string_num, release_fret_num );
}

bool FingeringConstructor::toggleState ( void )
{
    return m_presentMode->change();
}

/*------------------------------------------
     Default FC_Mode
-------------------------------------------*/
FC_Mode::FC_Mode ( FingeringConstructor* finger )
        : m_finger ( finger )
{}

/*------------------------------------------
     State: Insert
-------------------------------------------*/
FC_InsertMode::FC_InsertMode ( FingeringConstructor* finger )
        : FC_Mode ( finger )
{}

void FC_InsertMode::mouseRelease ( unsigned int release_string_num,
                                   unsigned int release_fret_num )
{
    unsigned int press_string_num = m_finger->m_press_string_num;
    unsigned int press_fret_num = m_finger->m_press_fret_num;
    unsigned int frets_displayed = m_finger->m_frets_displayed;
    Fingering* arrangement = m_finger->m_chord_arrangement;
    GuitarNeck* instr = m_finger->m_instr;
    QSpinBox* fret_spinbox = m_finger->m_fret_spinbox;

    if ( press_fret_num == release_fret_num ) {
        // If press string & fret pos == release string & fret position, display chord
        if ( press_string_num == release_string_num ) {
            // QUESTION: Move check for whether a note pressed is at position 0
            // here or not?
            if ( ( press_string_num > 0 ) &&
                    ( press_string_num <= instr->getStringNumber() ) &&
                    ( press_fret_num < ( fret_spinbox->value() + frets_displayed ) )
               ) {
                /**
                IF m_press_fret_num == 0
                	Get fretStatus
                	    status | new status
                	    MUTED  | OPEN
                	    OPEN   | MUTED
                ELSE m_press_fret_num > 0
                	status  | new status
                	MUTED   | PRESSED
                	OPEN    | PRESSED
                        PRESSED | PRESSED
                */
                GuitarString::Action aVal = GuitarString::PRESSED;
                if ( press_fret_num == 0 ) {
                    switch ( arrangement->getStringStatus ( press_string_num ) ) {
                    case GuitarString::MUTED: {
                            aVal = GuitarString::OPEN;
                            break;
                        }
                    case GuitarString::OPEN:
                    default: {
                            aVal = GuitarString::MUTED;
                            break;
                        }
                    }
                }

                if ( arrangement->hasNote ( press_string_num ) ) {
                    Note * note_ptr = arrangement->getNote ( press_string_num );
                    note_ptr->setNote ( press_string_num, press_fret_num );
                    arrangement->setStringStatus( press_string_num, aVal );
                } else {
                    Note* note_ptr = new Note ( press_string_num, press_fret_num );
                    arrangement->addNote( note_ptr );
                    arrangement->setStringStatus( press_string_num, aVal );
                }
                m_finger->update();
            }
        }
        // else if press fret pos == release fret pos & press string pos != release string pos, display bar
        else {
            if ( ( ( press_string_num > 0 ) && ( release_string_num > 0 ) ) &&
                    ( ( press_string_num <= instr->getStringNumber() ) &&
                      ( release_string_num <= instr->getStringNumber() ) ) &&
                    ( ( press_fret_num < ( fret_spinbox->value() + frets_displayed ) ) &&
                      ( release_fret_num < ( fret_spinbox->value () + frets_displayed ) ) ) ) {
                /*
                                std::cout << "FingeringConsturctor::mouseHandle - Adding a bar from string #"
                                << event->press_string_num << ", fret #" << event->press_fret_num
                                << " to string #" << release_string_num << ", fret #" << release_fret_num << std::endl;
                */

                if ( arrangement->hasBarre ( press_fret_num ) ) {
                    Barre * b_ptr = arrangement->getBarre( press_fret_num );
                    b_ptr->setBarre( press_fret_num, press_string_num, release_string_num );
                } else {
                    Barre* b_ptr = new Barre ( press_fret_num,
                                               press_string_num,
                                               release_string_num );
                    arrangement->addBarre( b_ptr );
                }
                m_finger->update();
            }
        }
    }
}


bool FC_InsertMode::change ( void )
{
    m_finger->m_presentMode = m_finger->DELETE;
    m_finger->setCursor( KCursor::crossCursor() );
    return false;
}

/*------------------------------------------
     State: Delete
-------------------------------------------*/
FC_DeleteMode::FC_DeleteMode ( FingeringConstructor* finger )
        : FC_Mode ( finger )
{}

void FC_DeleteMode::mouseRelease ( unsigned int release_string_num,
                                   unsigned int release_fret_num )
{

    unsigned int press_string_num = m_finger->m_press_string_num;
    unsigned int press_fret_num = m_finger->m_press_fret_num;
    Guitar::Fingering* arrangement = m_finger->m_chord_arrangement;

    if ( ( press_fret_num == release_fret_num ) &&
            ( press_string_num == release_string_num ) ) {
        // We have two cases here:
        //   #1: Clicked above in the string status area
        //   #2: Clicked on the fret area

        // So if we are not in the string status area
        if ( press_fret_num != 0 ) {
            // Detect object at position.
            // If note, delete note
            if ( arrangement->hasNote ( press_string_num ) ) {
                Note * note_ptr = arrangement->getNote ( press_string_num );
                if ( ( note_ptr->getFret() == press_fret_num ) &&
                        ( note_ptr->getStringNumber() == press_string_num ) ) {
                    // Remove note
                    arrangement->removeNote ( press_string_num );
                }
            }

            if ( arrangement->hasBarre ( press_fret_num ) ) {
                arrangement->removeBarre ( press_fret_num );
            }
            m_finger->update();
        }
    }
}

bool FC_DeleteMode::change ( void )
{
    m_finger->m_presentMode = m_finger->INSERT;
    m_finger->setCursor( KCursor::arrowCursor() );
    return true;
}

} /* namespace Guitar */

}

#include "FingeringConstructor.moc"

