/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/



#ifndef RG_FINGERINGBOX_H
#define RG_FINGERINGBOX_H

#include <QFrame>

#include "NoteSymbols.h"
#include "Fingering.h"

namespace Rosegarden
{

class Fingering;

class FingeringBox : public QFrame
{
    static const unsigned int IMG_WIDTH  = 200;
    static const unsigned int IMG_HEIGHT = 200;
    
public:
    FingeringBox(unsigned int nbFrets, unsigned int nbStrings, bool editable, QWidget *parent, bool big = false);
    FingeringBox(bool editable, QWidget *parent, bool big = false);

    void setStartFret(unsigned int f) { m_startFret = f; update(); }
    unsigned int getStartFret() const { return m_startFret; }
    
    void setFingering(const Guitar::Fingering&);
    const Guitar::Fingering& getFingering() { return m_fingering; }
    
    const Guitar::NoteSymbols& getNoteSymbols() const { return m_noteSymbols; }
    
    static const unsigned int DEFAULT_NB_DISPLAYED_FRETS = 4;
    
protected:
    void init();

    /** In Qt4 there is no more drawContents() so we'll use paintEvent() to
     * trigger the old code, hopefully with minimal modification
     */
    virtual void paintEvent(QPaintEvent*);

    /** This was the old Qt3 way of updating the widget.  Rather than
     * restructuring everything in new idiom, we use new idiom to call the old
     * idiom while leaving it in place.
     */
    virtual void drawContents(QPainter*);

    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void leaveEvent(QEvent*);

    void processMouseRelease( unsigned int release_string_num, unsigned int release_fret_num);

    typedef std::pair<bool, unsigned int> PositionPair;

    unsigned int getStringNumber(const QPoint&);

    unsigned int getFretNumber(const QPoint&);

    //! Maximum number of frets displayed by FingeringBox
    unsigned int m_nbFretsDisplayed;

    unsigned int m_startFret;
    
    unsigned int m_nbStrings;
    
    unsigned int m_transientFretNb;
    unsigned int m_transientStringNb;
    
    //! Present mode
    bool m_editable;

    //! Handle to the present fingering
    Guitar::Fingering m_fingering;

    //! String number where a mouse press event was located
    unsigned int m_press_string_num;

    //! Fret number where a mouse press event was located
    unsigned int m_press_fret_num;

    Guitar::NoteSymbols m_noteSymbols;

    QRect m_r1, m_r2;

    /** We use a little bool hack to tell this whether or not it's going to be
     * big, so we can draw finer lines and turn off antialiasing for smaller
     * renderings
     */
    bool m_big;

};

}

#endif /*RG_FINGERINGBOX_H*/
