// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.2
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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


#ifndef _CONTROLRULER_H_
#define _CONTROLRULER_H_

#include <qwidget.h>

#include "PropertyName.h"

namespace Rosegarden
{
    class RulerScale;
    class Segment;
}

class QFont;
class QFontMetrics;
class VelocityColour;


/**
 * ControlRuler is a widget that shows and allows modification of
 * a range of Property values for a set of Rosegarden Events.  
 * Designed to be able to modify groups of common controllers like
 * volume, pan as well as the more exotic ones.
 *
 */

class ControlRuler : public QWidget
{
    Q_OBJECT

public:
    ControlRuler(Rosegarden::RulerScale *rulerScale,
	         Rosegarden::Segment *segment,
                 const Rosegarden::PropertyName &property,
                 VelocityColour *velocityColour,
	         double xorigin = 0.0,
	         int height = 0,
	         QWidget* parent = 0,
	         const char *name = 0);

    ~ControlRuler();

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setMinimumWidth(int width) { m_width = width; }

    /**
     * Get the property name
     */
    Rosegarden::PropertyName getPropertyName() const { return m_propertyName; }

public slots:
    void slotScrollHoriz(int x);

protected:
    virtual void paintEvent(QPaintEvent *);

private:
    Rosegarden::PropertyName m_propertyName;

    double m_xorigin;
    int    m_height;
    int    m_currentXOffset;
    int    m_width;

    Rosegarden::Segment     *m_segment;
    Rosegarden::RulerScale  *m_rulerScale;

    QFont                    m_font;
    QFont                    m_boldFont;
    QFontMetrics             m_fontMetrics;

    VelocityColour          *m_velocityColour;
};

#endif // _CONTROLRULER_H_

