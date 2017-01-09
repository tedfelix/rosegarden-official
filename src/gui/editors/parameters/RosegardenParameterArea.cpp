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
    const char *name
    )    //, WFlags f)
    : QStackedWidget(parent),//, name),//, f),
        m_style(RosegardenParameterArea::CLASSIC_STYLE),
        m_scrollArea( new QScrollArea(this) ),
         m_classic(new QWidget()),
        m_classicLayout( new QVBoxLayout(m_classic) ),
        m_tabBox(new QTabWidget(this)),
        m_active(0),
        m_spacing(0)
{
    setObjectName( name );
        
    m_classic->setLayout(m_classicLayout);
    
    // Setting vertical ScrollBarAlwaysOn resolves initial sizing problem
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // alleviate a number of really bad problems in the TPB and IPB by allowing
    // QScrollArea to resize as necessary.  This was done to solve the problem
    // of CollapsingFrame widgets not having enough room to be shown when
    // expanded, but I expect it also solves the "more than n controllers,
    // everything is hopelessly jammed together" problem too.  For cheap.
    // (Too bad this fix is the last place I looked after a couple lost hours.)
    m_scrollArea->setWidgetResizable(true);
    
    // add 2 wigets as stacked widgets
    // Install the classic-style VBox widget in the widget-stack.
    addWidget(m_scrollArea);//, CLASSIC_STYLE);    //&&& 

    setCurrentWidget( m_scrollArea );
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
 
    // Create a titled group box for the parameter box, parented by the
    // classic layout widget, so that it can be used to provide a title
    // and outline, in classic mode. Add this container to an array that
    // parallels the above array of parameter boxes.

    QGroupBox *box = new QGroupBox(b->getLongLabel(), m_classic);
    //box->setMinimumSize( 40,40 );
    m_classicLayout->addWidget(box);
    
    box->setLayout( new QVBoxLayout(box) );
    box->layout()->setMargin( 4 ); // about half the default value
    QFont f;
    f.setBold( true );
    box->setFont( f );
    
    m_groupBoxes.push_back(box);

    // add the ParameterBox to the Layout
    box->layout()->addWidget(b);
    
    if (m_spacing)
        delete m_spacing;
    m_spacing = new QFrame(m_classic);
    m_classicLayout->addWidget(m_spacing);
    m_classicLayout->setStretchFactor(m_spacing, 100);

}

void RosegardenParameterArea::setScrollAreaWidget()
{
    m_scrollArea->setWidget(m_classic);
}
    
void RosegardenParameterArea::setArrangement(Arrangement style)
{
    RG_DEBUG << "RosegardenParameterArea::setArrangement(" << style << ") is deprecated, and non-functional!";
}

void RosegardenParameterArea::hideEvent(QHideEvent *)
{
    emit hidden();
}

void RosegardenParameterArea::moveWidget(QWidget *old_container,
        QWidget *new_container,
        RosegardenParameterBox *box)
{
    RG_DEBUG << "RosegardenParameterArea::moveWidget";

    // Remove any state that is associated with the parameter boxes,
    // from the active container.

    if (old_container == m_classic) {
        ;
    } else if (old_container == m_tabBox) {
        m_tabBox->removeTab(indexOf(box));
    }

    // Reparent the parameter box, and perform any container-specific
    // configuration.

    if (new_container == m_classic) {
        size_t index = 0;
        while (index < m_parameterBoxes.size()) {
            if (box == m_parameterBoxes[index])
                break;
            ++index;
        }
        if (index < m_parameterBoxes.size()) {
            //box->reparent(m_groupBoxes[index], 0, QPoint(0, 0), FALSE);
            m_groupBoxes[index]->setParent(this, 0);
            m_groupBoxes[index]->move(QPoint(0,0));
        }
    } else if (new_container == m_tabBox) {
        //box->reparent(new_container, 0, QPoint(0, 0), FALSE);
        new_container->setParent(this, 0);
        new_container->move(QPoint(0,0));
        m_tabBox->insertTab(-1, box, box->getShortLabel());
    }
}

}
