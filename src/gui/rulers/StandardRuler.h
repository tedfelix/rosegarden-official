
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_STANDARDRULER_H_
#define _RG_STANDARDRULER_H_

#include <QWidget>
#include <QVBoxLayout>
#include "base/Event.h"


class QWidget;
class QPaintEvent;


namespace Rosegarden
{

class RulerScale;
class RosegardenGUIDoc;
class LoopRuler;
class MarkerRuler;
class SnapGrid;


class StandardRuler : public QVBox
{
    Q_OBJECT

public:
    StandardRuler(RosegardenGUIDoc *doc,
                  RulerScale *rulerScale,
                  double xorigin,
                  int buttonHeight,
                  bool invert = false, // draw upside-down
                  QWidget* parent = 0,
                  const char* name = 0,
                  WFlags f=0);

    void setSnapGrid(SnapGrid *grid);

    LoopRuler* getLoopRuler() { return m_loopRuler; }

    /**
     * Make connections from the LoopRuler to the document's
     * position pointer -- the standard use for a LoopRuler.
     * If you don't call this, you'll have to connect the
     * LoopRuler's signals up to something yourself.
     */
    void connectRulerToDocPointer(RosegardenGUIDoc *doc);
    
    void setMinimumWidth(int width);

    void setHScaleFactor(double dy);

public slots:
    void slotScrollHoriz(int x);

signals:
    /// reflected from the loop ruler
    void dragPointerToPosition(timeT);

    /// reflected from the loop ruler
    void dragLoopToPosition(timeT);


protected:
    virtual void paintEvent(QPaintEvent *);

private:
    //--------------- Data members ---------------------------------
    bool m_invert;
    int m_loopRulerHeight;
    int m_currentXOffset;

    RosegardenGUIDoc       *m_doc;
    RulerScale *m_rulerScale;

    MarkerRuler *m_hButtonBar;
    LoopRuler *m_loopRuler;
};



}

#endif
