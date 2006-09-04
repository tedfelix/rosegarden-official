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

#include <iostream>
#include <qlayout.h>
#include <klocale.h>

#include "rosegardenparameterarea.h"

RosegardenParameterArea::RosegardenParameterArea(QWidget *parent,
						 const char *name, WFlags f)
    : QWidgetStack(parent, name, f),
      m_style(RosegardenParameterArea::CLASSIC_STYLE),
      m_classic(new QVBox(this)),
      m_tabBox(new KTabWidget(this)),
      m_active(0),
      m_spacing(0)
{

    // Install the classic-style VBox widget in the widget-stack.

    addWidget(m_classic, CLASSIC_STYLE);

    // Install the widget that implements the tab-style to the widget-stack.

    addWidget(m_tabBox, TAB_BOX_STYLE);

}

// Add a RosegardenParameterWidget to a RosegardenParameterArea widget
// and queue a redisplay of the area, to display the new widget.

void RosegardenParameterArea::addRosegardenParameterBox(
    RosegardenParameterBox *b)
{
    // Check that the box hasn't been added before.

    for (unsigned int i = 0; i < m_parameterBoxes.size(); i++) {
	if (m_parameterBoxes[i] == b)
	    return;
    }

    // Append the parameter box to the list to be displayed.

    m_parameterBoxes.push_back(b);

    // Create a titled group box for the parameter box, parented by the
    // classic layout widget, so that it can be used to provide a title
    // and outline, in classic mode. Add this container to an array that
    // parallels the above array of parameter boxes.

    QVGroupBox *box = new QVGroupBox( i18n("%1 Parameters").arg(b->getLabel()),
				      m_classic);
    box->layout()->setMargin( 4 ); // about half the default value
    QFont f;
    f.setBold( true );
    box->setFont( f );
    m_groupBoxes.push_back(box);

    if (m_spacing) delete m_spacing;
    m_spacing = new QFrame(m_classic);
    m_classic->setStretchFactor(m_spacing, 100);

    // Add the parameter box to the current container of the displayed
    // widgets, unless the current container has been set up yet.

    if (m_active)
	moveWidget(0, m_active, m_parameterBoxes.size()-1);

    // Queue a redisplay of the parameter area, to incorporate the new box.

    update();
}

// Select a new display arrangement, and queue a redisplay to instantiate it.

void RosegardenParameterArea::setArrangement(Arrangement style)
{
    // Lookup the container of the specified style.

    QWidget *container;
    switch(style) {
    case CLASSIC_STYLE:
	container = m_classic;
	break;
    case TAB_BOX_STYLE:
	container = m_tabBox;
	break;
    default:
	std::cerr << "setArrangement() was passed an unknown arrangement style."
		  << std::endl;
	return;
    }

    // Does the current container of the parameter-box widgets differ
    // from the one that is associated with the currently configured
    // style?

    if (container != m_active) {

	// Move the parameter boxes from the old container to the new one.

	for (unsigned int i = 0; i < m_parameterBoxes.size(); i++) {
	    moveWidget(m_active, container, i);
	    m_parameterBoxes[i]->showAdditionalControls(style == TAB_BOX_STYLE);
	}

	// Switch the widget stack to displaying the new container.

	raiseWidget(style);

    }

    // Record the identity of the active container, and the associated
    // arrangement style.

    m_active = container;
    m_style = style;
}

// Move a parameter box widget from one widget-container to another.

void RosegardenParameterArea::moveWidget(QWidget *old_container,
					 QWidget *new_container,
					 int index)
{
    // Get the parameter box widget that is to be moved.

    RosegardenParameterBox *b = m_parameterBoxes[index];

    // Remove any state that is associated with the parameter boxes,
    // from the active container.

    if (old_container == m_classic) {
	;
    } else if (old_container == m_tabBox) {
	m_tabBox->removePage(b);
    }

    // Reparent the paramter box, and perform any container-specific
    // configuration.

    if (new_container == m_classic) {
	b->reparent(m_groupBoxes[index], 0, QPoint(0,0), FALSE);
    } else if(new_container == m_tabBox) {
	b->reparent(new_container, 0, QPoint(0,0), FALSE);
	m_tabBox->insertTab(b, b->getLabel());
    }
}

#include "rosegardenparameterarea.moc"
