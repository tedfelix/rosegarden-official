
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_PROPERTYVIEWRULER_H_
#define _RG_PROPERTYVIEWRULER_H_

#include "base/PropertyName.h"
#include "gui/general/HZoomable.h"
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
 * PropertyViewRuler is a widget that shows a range of Property
 * (velocity, typically) values for a set of Rosegarden Events.
 */
class PropertyViewRuler : public QWidget, public HZoomable
{
    Q_OBJECT

public:
    PropertyViewRuler(RulerScale *rulerScale,
                      Segment *segment,
                      const PropertyName &property,
                      double xorigin = 0.0,
                      int height = 0,
                      QWidget* parent = 0,
                      const char *name = 0);

    ~PropertyViewRuler();

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setMinimumWidth(int width) { m_width = width; }

    /**
     * Get the property name
     */
    PropertyName getPropertyName() const { return m_propertyName; }

public slots:
    void slotScrollHoriz(int x);

protected:
    virtual void paintEvent(QPaintEvent *);

    //--------------- Data members ---------------------------------

    PropertyName m_propertyName;

    double m_xorigin;
    int    m_height;
    int    m_currentXOffset;
    int    m_width;

    Segment     *m_segment;
    RulerScale  *m_rulerScale;

    QFont                    m_font;
    QFont                    m_boldFont;
    QFontMetrics             m_fontMetrics;
};


}

#endif
