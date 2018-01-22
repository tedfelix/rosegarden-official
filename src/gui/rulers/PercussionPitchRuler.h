
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

#ifndef RG_PERCUSSIONPITCHRULER_H
#define RG_PERCUSSIONPITCHRULER_H

#include "PitchRuler.h"
#include <QSize>


class QWidget;
class QPaintEvent;
class QMouseEvent;
class QFontMetrics;
class QFont;
class QEvent;


namespace Rosegarden
{

class MidiKeyMapping;


class PercussionPitchRuler : public PitchRuler
{
    Q_OBJECT
public:
    PercussionPitchRuler(QWidget *parent,
                         const MidiKeyMapping *mapping,
                         int lineSpacing);

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void drawHoverNote(int evPitch);
    void hideHoverNote();

protected:
    virtual void paintEvent(QPaintEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);

    const MidiKeyMapping *m_mapping;

    int                       m_width;
    int                       m_lineSpacing;

    bool                      m_mouseDown;
    bool                      m_selecting;

    int                       m_hoverNotePitch;
    int                       m_lastHoverHighlight;

    QFont                    *m_font;
    QFontMetrics             *m_fontMetrics;
};



}

#endif
