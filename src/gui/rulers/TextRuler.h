
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

#ifndef RG_TEXTRULER_H
#define RG_TEXTRULER_H

#include <QFont>
#include <QFontMetrics>
#include <QSize>
#include <QWidget>


class QPaintEvent;


namespace Rosegarden
{

class Segment;
class RulerScale;


/**
 * TextRuler is a widget that shows a strip of text strings at
 * x-coordinates corresponding to specified times.  The strings
 * are obtained from a Segment, in which they are stored as
 * text events.  (The Segment does not have to be part of a
 * Composition.)
 *
 * By design, this is more suitable for the display of single-purpose
 * read-only data such as calculated chord names or (at a pinch)
 * lyrics; it's not really suitable for displaying text data
 * associated with a staff.
 */

class TextRuler : public QWidget
{
    Q_OBJECT

public:
    /**
     * Construct a TextRuler that displays the text events found
     * in the given Segment at positions calculated by the given
     * RulerScale.
     *
     * The Segment will be destroyed along with the TextRuler, if
     * it was not in a Composition when the TextRuler was created
     * and still is not when it's destroyed.  If the Segment was
     * found to be in a Composition on either occasion, it will be
     * assumed to be someone else's responsibility.
     *
     * The RulerScale will not be destroyed along with the TextRuler.
     */
    TextRuler(RulerScale *rulerScale,
              Segment *segment,
              int height = 0,
              QWidget* parent = nullptr);

    ~TextRuler() override;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void setMinimumWidth(int width) { m_width = width; }

public slots:
    void slotScrollHoriz(int x);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    int  m_height;
    int  m_currentXOffset;
    int  m_width;

    bool m_mySegmentMaybe;

    Segment    *m_segment;
    RulerScale *m_rulerScale;

    QFont m_font;
    QFontMetrics m_fontMetrics;
};


}

#endif
