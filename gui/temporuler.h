// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#ifndef _TEMPORULER_H_
#define _TEMPORULER_H_

#include <qwidget.h>
#include "tempodialog.h"
#include "Event.h"
#include "Composition.h"

#include <kxmlguiclient.h>

namespace Rosegarden {
    class RulerScale;
}

class QFont;
class QFontMetrics;
class QPopupMenu;
class RosegardenGUIDoc;
class RosegardenTextFloat;
class KXMLGUIFactory;


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
    TempoRuler(Rosegarden::RulerScale *rulerScale,
	       RosegardenGUIDoc *doc,
	       KXMLGUIFactory *factory,
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
    void doubleClicked(Rosegarden::timeT);

    void changeTempo(Rosegarden::timeT,  // tempo change time
                     Rosegarden::tempoT,  // tempo value
                     Rosegarden::tempoT,  // tempo target
                     TempoDialog::TempoDialogAction); // tempo action

    void moveTempo(Rosegarden::timeT, // old time
		   Rosegarden::timeT); // new time

    void deleteTempo(Rosegarden::timeT);

    void editTempo(Rosegarden::timeT);
    void editTimeSignature(Rosegarden::timeT);
    void editTempos(Rosegarden::timeT);

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

    Rosegarden::timeT  m_dragStartTime;
    Rosegarden::timeT  m_dragPreviousTime;
    Rosegarden::tempoT m_dragStartTempo;
    Rosegarden::tempoT m_dragStartTarget;
    Rosegarden::tempoT m_dragOriginalTempo;
    Rosegarden::tempoT m_dragOriginalTarget;

    int getYForTempo(Rosegarden::tempoT tempo);
    Rosegarden::tempoT getTempoForY(int y);
    void showTextFloat(Rosegarden::tempoT tempo,
		       Rosegarden::tempoT target = -1,
		       Rosegarden::timeT time = -1,
		       bool showTime = false);

    Rosegarden::Composition *m_composition;
    Rosegarden::RulerScale *m_rulerScale;
    RosegardenTextFloat *m_textFloat;
    QPopupMenu *m_menu;
    KXMLGUIFactory *m_factory;

    QFont m_font;
    QFont m_boldFont;
    QFontMetrics m_fontMetrics;
    QPixmap m_buffer;
};

#endif // _TEMPORULER_H_

