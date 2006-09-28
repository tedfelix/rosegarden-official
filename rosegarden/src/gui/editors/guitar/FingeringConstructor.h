#ifndef FINGERS_H
#define FINGERS_H

#include <qframe.h>
#include <qspinbox.h>

class QScrollBar;

/*
	KGuitar interface was used as a guideline for design the FingeringConstructor. Where the code
	from the project was used is defined by comments.
*/
namespace Guitar
{
class Fingering;
class GuitarNeck;
class NoteSymbols;
class FC_Mode;
class FC_InsertMode;
class FC_DeleteMode;

class FingeringConstructor : public QFrame
{
    Q_OBJECT

    friend class FC_InsertMode;
    friend class FC_DeleteMode;

public:

    static const unsigned int MAX_FRET_DISPLAYED = 5;

    enum Mode {
        EDITABLE,
        DISPLAY_ONLY
    };

    //! Constant values obtained from KGuitar project
    enum { IMG_WIDTH = 200,
           IMG_HEIGHT = 200,
           SCROLL = 15,
         };

    //! Constructor
    FingeringConstructor ( GuitarNeck *instr,
                           QWidget *parent = 0,
                           Mode state = EDITABLE,
                           const char *name = 0 );

    virtual ~FingeringConstructor() {}

public slots:

    //! Clear the chord display of Bar and Note objects
    void clear();

    /**
     * Set fingering object to be displayed
     * @param  arrange Fingering object pointer
     */
    void setFingering( Guitar::Fingering* arrange );

    //! Get the displayed fingering object
    Fingering* getFingering ( void ) const;

    //! Set the base fret for fingering
    void setFirstFret( int );

    //! Overriding method to draw fingering to QPainter object
    virtual void drawContents ( QPainter * );

    //! Capture mouse press event
    virtual void mousePressEvent ( QMouseEvent * );

    //! Capture mouse release event
    virtual void mouseReleaseEvent ( QMouseEvent * );

    //! Change state
    bool toggleState ( void );

signals:

    //! Alert this object that a chordChange has occurred
    void chordChange();

protected:

    typedef std::pair<bool, unsigned int> PositionPair;

    unsigned int getStringNumber ( QMouseEvent const* event );

    unsigned int getFretNumber ( QMouseEvent const* event );

    //! Spin Box to manage setting the base fret for fingering (KGuitar)
    QSpinBox *m_fret_spinbox;

    //! Handle to the guitar associated with present fingering
    GuitarNeck *m_instr;

    //! Present mode
    Mode m_mode;

    //! Handle to the present fingering
    Fingering* m_chord_arrangement;

    //! Maximum number of frets displayed by FingeringConstructor
    unsigned int m_frets_displayed;

    //! String number where a mouse press event was located
    unsigned int m_press_string_num;

    //! Fret number where a mouse press event was located
    unsigned int m_press_fret_num;

    // Modes
    FC_Mode* INSERT;
    FC_Mode* DELETE;
    FC_Mode* m_presentMode;
};

class FC_Mode
{
protected:
    FingeringConstructor* m_finger;

public:
    FC_Mode ( FingeringConstructor* finger );

    virtual ~FC_Mode () {}

    virtual void mouseRelease ( unsigned int string_num, unsigned int fret_num ) = 0;

    // To mark the states we will use boolean values since there are only two.
    // TRUE = insert
    // FALSE = delete
    virtual bool change ( void ) = 0;
};

class FC_InsertMode : public FC_Mode
{
public:
    FC_InsertMode ( FingeringConstructor* finger );

    virtual ~FC_InsertMode () {}

    //! Capture mouse release event
    virtual void mouseRelease ( unsigned int string_num, unsigned int fret_num );

    virtual bool change ( void );
};

class FC_DeleteMode : public FC_Mode
{
public:

    FC_DeleteMode ( FingeringConstructor* finger );

    virtual ~FC_DeleteMode () {}

    //! Capture mouse release event
    virtual void mouseRelease ( unsigned int string_num, unsigned int fret_num );

    virtual bool change ( void );
};

} /* namespace */

#endif
