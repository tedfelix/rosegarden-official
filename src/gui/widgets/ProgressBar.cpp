/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2010 the Rosegarden development team.
 
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
#include <QProgressDialog>
#include <QSettings>


namespace Rosegarden
{


ProgressBar::ProgressBar(int totalSteps,
                         QWidget *parent) :
         QProgressBar(parent)
{
    setRange(0, totalSteps);

    QSettings settings;
    settings.beginGroup(GeneralOptionsConfigGroup);
    bool Thorn = settings.value("use_thorn_style", true).toBool();
    settings.endGroup();

    QString localStyle("QProgressBar { background: #FFFFFF; border: 1px solid #AAAAAA; border-radius: 3px; }  QProgressBar::chunk { background-color: #D6E7FA; width: 20px; }");
    if (Thorn) setStyleSheet(localStyle);
}


}
#include "ProgressBar.moc"
