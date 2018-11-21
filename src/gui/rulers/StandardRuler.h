
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

#ifndef RG_STANDARDRULER_H
#define RG_STANDARDRULER_H

#include <QWidget>
#include "base/Event.h"

namespace Rosegarden
{

class RulerScale;
class RosegardenDocument;
class LoopRuler;
class MarkerRuler;
class SnapGrid;


class StandardRuler : public QWidget
{
    Q_OBJECT

public:
    StandardRuler(RosegardenDocument *doc,
                  RulerScale *rulerScale,
                  bool invert = false, // draw upside-down
                  bool isForMainWindow = false,
                  QWidget* parent = nullptr);

    void setSnapGrid(const SnapGrid *grid);

    LoopRuler* getLoopRuler() { return m_loopRuler; }

    /**
     * Make connections from the LoopRuler to the document's
     * position pointer -- the standard use for a LoopRuler.
     * If you don't call this, you'll have to connect the
     * LoopRuler's signals up to something yourself.
     */
    void connectRulerToDocPointer(RosegardenDocument *doc);
    
    void setMinimumWidth(int width);

    /**
     * Update all components of standard ruler.
     * Useful when the scene has changed due to font change,
     * font size change in NotationView, etc.
     */
    void updateStandardRuler();

public slots:
    void slotScrollHoriz(int x);

signals:
    /// reflected from the loop ruler
    void dragPointerToPosition(timeT);

    /// reflected from the loop ruler
    void dragLoopToPosition(timeT);

private:
    //--------------- Data members ---------------------------------
    bool m_invert;
    bool m_isForMainWindow;
    int m_loopRulerHeight;
    int m_currentXOffset;

    RosegardenDocument       *m_doc;
    RulerScale *m_rulerScale;

    MarkerRuler *m_markerRuler;
    LoopRuler *m_loopRuler;
};



}

#endif
