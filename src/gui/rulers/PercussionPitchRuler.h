
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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

#include <QSharedPointer>
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
                         QSharedPointer<const MidiKeyMapping> mapping,
                         int lineSpacing);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    /// Draw a highlight to indicate the pitch that the mouse is hovering over.
    /**
     * For the PercussionPitchRuler, this is a reverse video highlight on
     * the text.
     */
    void showHighlight(int evPitch) override;
    void hideHighlight() override;

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    void enterEvent(QEnterEvent *) override;
#else
    void enterEvent(QEvent *) override;
#endif
    void leaveEvent(QEvent *) override;

    QSharedPointer<const MidiKeyMapping> m_mapping;

    int                       m_width;
    int                       m_lineSpacing;

    bool                      m_mouseDown;
    bool                      m_selecting;

    int                       m_highlightPitch;
    int                       m_lastHighlightPitch;

    QFont                    *m_font;
    QFontMetrics             *m_fontMetrics;
};



}

#endif
