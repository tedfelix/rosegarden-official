/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.
 
    This file Copyright 2006 Martin Shepherd <mcs@astro.caltech.edu>.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[RosegardenParameterArea]"

#include "RosegardenParameterArea.h"

#include "RosegardenParameterBox.h"
#include <iostream>
#include <set>

#include <QTabWidget>
#include <QFont>
#include <QFrame>
#include <QPoint>
#include <QScrollArea>
#include <QString>
#include <QLayout>
#include <QWidget>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QGroupBox>
//#include <QPushButton>

#include "misc/Debug.h"

namespace Rosegarden
{

RosegardenParameterArea::RosegardenParameterArea(
    QWidget *parent,
    const char *name)
    : QStackedWidget(parent),
        m_scrollArea( new QScrollArea(this) ),
        m_boxContainer(new QWidget()),
        m_boxContainerLayout( new QVBoxLayout(m_boxContainer) )
{
    setObjectName( name );
        
    m_boxContainer->setLayout(m_boxContainerLayout);
    
    // Setting vertical ScrollBarAlwaysOn resolves initial sizing problem
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // alleviate a number of really bad problems in the TPB and IPB by allowing
    // QScrollArea to resize as necessary.  This was done to solve the problem
    // of CollapsingFrame widgets not having enough room to be shown when
    // expanded, but I expect it also solves the "more than n controllers,
    // everything is hopelessly jammed together" problem too.  For cheap.
    // (Too bad this fix is the last place I looked after a couple lost hours.)
    m_scrollArea->setWidgetResizable(true);
    
    // Install the scroll area widget in the widget-stack.
    addWidget(m_scrollArea);

    setCurrentWidget( m_scrollArea );

    m_boxContainerLayout->addStretch(100);
}

void RosegardenParameterArea::addRosegardenParameterBox(
    RosegardenParameterBox *b)
{
    RG_DEBUG << "RosegardenParameterArea::addRosegardenParameterBox";
    
    // Check that the box hasn't been added before.

    for (unsigned int i = 0; i < m_parameterBoxes.size(); i++) {
        if (m_parameterBoxes[i] == b)
            return ;
    }

    // Append the parameter box to the list to be displayed.
     m_parameterBoxes.push_back(b);
 
    // Create a titled group box for the parameter box, so that it can be
    // used to provide a title and outline. Add this container to an array that
    // parallels the above array of parameter boxes.

    QGroupBox *box = new QGroupBox(b->getLongLabel(), m_boxContainer);
    m_boxContainerLayout->insertWidget(m_boxContainerLayout->count() - 1, box); // before the stretch
    
    box->setLayout( new QVBoxLayout(box) );
    box->layout()->setMargin( 4 ); // about half the default value
    QFont f;
    f.setBold( true );
    box->setFont( f );
    
    m_groupBoxes.push_back(box);

    // add the ParameterBox to the Layout
    box->layout()->addWidget(b);
}

void RosegardenParameterArea::setScrollAreaWidget()
{
    m_scrollArea->setWidget(m_boxContainer);
}

void RosegardenParameterArea::hideEvent(QHideEvent *)
{
    emit hidden();
}

}
