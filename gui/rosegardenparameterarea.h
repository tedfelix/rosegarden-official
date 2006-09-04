// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file Copyright 2006 Martin Shepherd <mcs@astro.caltech.edu>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _ROSEGARDENPARAMETERS_H_
#define _ROSEGARDENPARAMETERS_H_

#include <vector>
#include <qwidget.h>
#include <qwidgetstack.h>
#include <ktabwidget.h>
#include <qvgroupbox.h>

#include "widgets.h"

/**
 * A widget that arranges a set of Rosegarden parameter-box widgets
 * within a frame, in a dynamically configurable manner.
 */
class RosegardenParameterArea : public QWidgetStack
{
    Q_OBJECT
public:

    // Create the parameter display area.

    RosegardenParameterArea(QWidget *parent=0, const char *name=0, WFlags f=0);

    // Add a rosegarden parameter box to the list that are to be displayed.

    void addRosegardenParameterBox(RosegardenParameterBox *b);


    // List the supported methods of arranging the various parameter-box
    // widgets within the parameter area.

    enum Arrangement {
	CLASSIC_STYLE,  // A simple vertical tiling of parameter-box widgets.
	TAB_BOX_STYLE   // A horizontal list of tabs, displaying one box at a time.
    };

    // Redisplay the widgets with a different layout style.

    void setArrangement(Arrangement style);

protected:
private:
    Arrangement m_style;                // The current layout style.

    // The list of parameter box widgets that are being displayed by this
    // widget.

    std::vector<RosegardenParameterBox *>  m_parameterBoxes;

    // Create a parallel array of group boxes, to be used when the
    // corresponding parameter box widget needs to be enclosed by a
    // titled outline.

    std::vector<QVGroupBox *> m_groupBoxes;

    // Move a RosegardenParameterBox widget from one container to another.

    void moveWidget(QWidget *old_container, QWidget *new_container,
		    int index);

    QVBox *m_classic;          // The container widget for m_style==CLASSIC_STYLE.
    KTabWidget *m_tabBox;     // The container widget for m_style==TAB_BOX_STYLE.
    QWidget *m_active;         // The current container widget.
    QWidget *m_spacing;
};

#endif // _ROSEGARDENPARAMETERS_H_
