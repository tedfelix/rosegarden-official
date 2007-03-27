
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_TEMPORULER_H_
#define _RG_TEMPORULER_H_

#include "gui/dialogs/TempoDialog.h"
#include <kxmlguiclient.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qwidget.h>
#include "base/Event.h"


class QWheelEvent;
class QPopupMenu;
class QPaintEvent;
class QMouseEvent;
class QEvent;
class KMainWindow;


namespace Rosegarden
{

class TextFloat;
class RulerScale;
class RosegardenGUIDoc;
class Composition;


/**
 * TempoRuler is a widget that shows a strip of tempo values at
 * x-coordinates corresponding to tempo changes in a Composition.
 */

class TempoRuler : public QWidget, public KXMLGUIClient
{
    Q_OBJECT

public:
    /**
     * Construct a TempoRuler that displays and allows editing of the
     * tempo changes found in the given Composition, with positions
     * calculated by the given RulerScale.
     *
     * The RulerScale will not be destroyed along with the TempoRuler.
     */
    TempoRuler(RulerScale *rulerScale,
               RosegardenGUIDoc *doc,
               KMainWindow *parentMainWindow,
               double xorigin = 0.0,
               int height = 0,
               bool small = false,
               QWidget* parent = 0,
               const char *name = 0);

    ~TempoRuler();
    
    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setMinimumWidth(int width) { m_width = width; }

    void connectSignals();

signals:
    void doubleClicked(timeT);

    void changeTempo(timeT,  // tempo change time
                     tempoT,  // tempo value
                     tempoT,  // tempo target
                     TempoDialog::TempoDialogAction); // tempo action

    void moveTempo(timeT, // old time
                   timeT); // new time

    void deleteTempo(timeT);

    void editTempo(timeT);
    void editTimeSignature(timeT);
    void editTempos(timeT);

public slots:
    void slotScrollHoriz(int x);

protected slots:
    void slotInsertTempoHere();
    void slotInsertTempoAtPointer();
    void slotDeleteTempoChange();
    void slotRampToNext();
    void slotUnramp();
    void slotEditTempo();
    void slotEditTimeSignature();
    void slotEditTempos();

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void wheelEvent(QWheelEvent *);

    void createMenu();
    
private:
    double m_xorigin;
    int  m_height;
    int  m_currentXOffset;
    int  m_width;
    bool m_small;
    int  m_illuminate;
    bool m_illuminatePoint;
    bool m_illuminateTarget;
    bool m_refreshLinesOnly;

    bool m_dragVert;
    bool m_dragTarget;
    bool m_dragHoriz;
    int  m_dragStartY;
    int  m_dragStartX;
    bool m_dragFine;
    int  m_clickX;

    timeT  m_dragStartTime;
    timeT  m_dragPreviousTime;
    tempoT m_dragStartTempo;
    tempoT m_dragStartTarget;
    tempoT m_dragOriginalTempo;
    tempoT m_dragOriginalTarget;

    int getYForTempo(tempoT tempo);
    tempoT getTempoForY(int y);
    void showTextFloat(tempoT tempo,
                       tempoT target = -1,
                       timeT time = -1,
                       bool showTime = false);

    Composition *m_composition;
    RulerScale *m_rulerScale;
    TextFloat *m_textFloat;
    QPopupMenu *m_menu;
    KMainWindow *m_parentMainWindow;

    QFont m_font;
    QFont m_boldFont;
    QFontMetrics m_fontMetrics;
    QPixmap m_buffer;
};


}

#endif
