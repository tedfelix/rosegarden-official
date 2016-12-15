/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "ProgressBar.h"
#include "misc/ConfigGroups.h"

#include <QProgressBar>
#include <QSettings>

#include <iostream>

namespace Rosegarden
{


ProgressBar::ProgressBar(/*int totalSteps, */
                         QWidget *parent) :
         QProgressBar(parent)
{
/*    setRange(0, totalSteps); */

    connect (this, SIGNAL(valueChanged(int)), this, SLOT(WTF(int)));
}

ProgressBar::ProgressBar(int totalSteps,
                         QWidget *parent) :
         QProgressBar(parent)
{
    setRange(0, totalSteps);

    connect (this, SIGNAL(valueChanged(int)), this, SLOT(WTF(int)));
}

void
ProgressBar::WTF(int /* wtf */)
{
    /*
    std::cout << "I am a ProgressBar, and my value changed to " << wtf << "!  Why am I still blank?!" << std::endl;

    std::cout << "my minimum: " << minimum() << " maximum: " << maximum() << " value: " << value() << std::endl;

    std::cout << "My parent is: \"" << parentWidget()->objectName().toStdString() << "\"" << std::endl;
    */

    // Because the only ProgressBar doing anything is the one in the main window
    // status bar.  That's interesting.  I guess we used to update both the
    // status bar in CurrentProgressDialog as well as the CPU meter.  Well why
    // the bloody hell would we want to change the function of the CPU meter
    // anyway?  Now that I think back on it, it used to get stuck in odd places
    // anyway.  So if I disentangle those two things from each other and make
    // them separate, that's part of the battle won.
    //
    // Or so I'm thinking.
    //
//    std::cout << "Good evening. I am a ProgressBar, and my name is " << objectName().toStdString() << ". I just changed my value to " << wtf << std::endl;
}


}
