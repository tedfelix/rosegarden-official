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

#ifndef RG_PITCHDRAGLABEL_H
#define RG_PITCHDRAGLABEL_H

#include <QPixmap>
#include <QSize>
#include <QWidget>


class QWheelEvent;
class QPaintEvent;
class QMouseEvent;


namespace Rosegarden
{

class NotePixmapFactory;


class PitchDragLabel : public QWidget
{
    Q_OBJECT
public:
    PitchDragLabel(QWidget *parent,
    int defaultPitch = 60, bool defaultSharps = true);
    ~PitchDragLabel();

    int getPitch() const { return m_pitch; }

    virtual QSize sizeHint() const;

signals:
    void pitchDragged(int);
    // pitch, octave, step
    void pitchDragged(int,int,int);
    void pitchChanged(int); // mouse release
    // pitch, octave, step
    void pitchChanged(int,int,int); // mouse release
    void preview(int);

public slots:
    void slotSetPitch(int);
    void slotSetPitch(int,int,int);
    
protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void wheelEvent(QWheelEvent *e);

    void calculatePixmap() const;
    void calculatePixmap(int pitch, int octave, int step) const;

    /** emits 'pitchChanged' events, both diatonic and chromatic */
    void emitPitchChange();

    mutable QPixmap m_pixmap;

    int m_pitch;
    int m_clickedY;
    int m_clickedPitch;
    bool m_clicked;
    
    bool m_usingSharps;

    /** Are we using the Thorn style? */
    bool m_Thorn;

    NotePixmapFactory *m_npf;
};



}

#endif
