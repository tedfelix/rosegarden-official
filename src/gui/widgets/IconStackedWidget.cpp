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

#include "IconStackedWidget.h"
#include "IconButton.h"

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

#include <iostream>

IconStackedWidget::IconStackedWidget(QWidget *parent) :
        QWidget(parent),
        m_buttonHeight(0),
        m_buttonWidth(0),
        m_backgroundColor(QColor(255,255,255))
{
    
    // Use a frame widget for the icon panel, it will hold a bunch of buttons
    m_iconPanel = new QFrame;
    m_iconPanel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_iconPanel->setLineWidth(2);

    // Set a bright background so that the icons are visible.
    QPalette palette = m_iconPanel->palette();
    palette.setColor(m_iconPanel->backgroundRole(), QColor(221,221,221));
    m_iconPanel->setPalette(palette);
    m_iconPanel->setAutoFillBackground(true);

    // Use a VBoxLayout for the icon buttons
    m_iconLayout = new QVBoxLayout;
    
    // Buttons butt up against each other
    m_iconLayout->setSpacing(0);
    
    // No margin between buttons and their frame
    m_iconLayout->setContentsMargins(0,0,0,0);
    
    // Want the widget fixed to the top of the space
    // A stretch item must be at the bottom of the list
    // This approach changes the direction to from Bottom to Top
    //  and adds a stretch item first in the list
    m_iconLayout->setDirection(QBoxLayout::BottomToTop);
    m_iconLayout->addStretch(1);

    m_iconPanel->setLayout(m_iconLayout);
    
    // Use a stacked widget for the pages so the selected on is displayed
    m_pagePanel = new QStackedWidget;

    // Use a QHBoxLayout for icon and page panels
    m_layout = new QHBoxLayout;
    m_layout->setMargin(0);
    
    // Add the icon and page panels to the main layout
    m_layout->addWidget(m_iconPanel);
    m_layout->addWidget(m_pagePanel);
    setLayout(m_layout);
}
    
void
IconStackedWidget::addPage(const QString& name, QWidget *page, const QPixmap& icon)
{
    IconButton *iconButton = new IconButton(m_iconPanel,icon, name);    

    // IconStackedWidget acts like a radio button widget with exclusive buttons
    iconButton->setCheckable(true);
    iconButton->setAutoExclusive(true);

    // If the new button is the biggest so far, update the default size
    if ((iconButton->minimumWidth() > m_buttonWidth) || (iconButton->minimumHeight() > m_buttonHeight)) {
        m_buttonWidth = std::max(iconButton->minimumWidth(),m_buttonWidth);
        m_buttonHeight = std::max(iconButton->minimumHeight(),m_buttonHeight);
        // Update the size of previous buttons
        for (iconbuttons::iterator i = m_iconButtons.begin();
            i != m_iconButtons.end(); ++i)
            (*i)->setMinimumSize(m_buttonWidth, m_buttonHeight);
    }

    iconButton->setMinimumSize(m_buttonWidth, m_buttonHeight);

    // If the list of buttons is not empty set the new buttons background to the default
    if (!m_iconButtons.size()) {
        iconButton->setChecked(true);
    }
    
    // Store the new button in a list for later modification
    m_iconButtons.push_back((IconButton *) iconButton);
    
    // Add the button to the icon layout, insert to the second point in the list
    //   the first hold the stretch item
    m_iconLayout->insertWidget(1,iconButton);

    // Add the new page to the page layout
    m_pagePanel->addWidget(page);

    // Connect the button's clicked data signal to the page select slot
    connect(iconButton, &QAbstractButton::clicked, this, &IconStackedWidget::slotPageSelect);
}

void
IconStackedWidget::slotPageSelect()
{
    // Cycle through the buttons to find the one that is checked
    iconbuttons::iterator i = m_iconButtons.begin();
    int index = 0;
    while ((i != m_iconButtons.end()) && ((*i)->isChecked() == false)) {
        ++i;
        index++;
    }
    
    // Select the new page
    m_pagePanel->setCurrentIndex(index);
}

void
IconStackedWidget::setPageByIndex(int index)
{
    std::cerr << "IconStackedWidget setting index to " << index << std::endl;

    // I originally tried just m_pagePanel->setCurrentIndex(index) here, and it
    // didn't work at all.  Rather than try to deciper that, I came up with this
    // alternate plan of attack.  Iterate through the icon buttons until we find
    // the one that matches our index, then check it.  Now call slotPageSelect()
    // which basically does the same thing I tried to do to start with.  I don't
    // know why that works.  Oh well.  This is working for me now, and results
    // are more important than appreciating nuance.
    iconbuttons::iterator i = m_iconButtons.begin();
    int c = 0;
    while (i != m_iconButtons.end()) {
        if (c == index) (*i)->setChecked(true);
        ++i;
        c++;
    }

    slotPageSelect();
}

