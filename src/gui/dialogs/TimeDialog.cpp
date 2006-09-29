/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "TimeDialog.h"

#include <klocale.h>
#include "base/Composition.h"
#include "gui/widgets/TimeWidget.h"
#include <kdialogbase.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

TimeDialog::TimeDialog(QWidget *parent, QString title,
                       Composition *composition,
                       timeT defaultTime) :
        KDialogBase(parent, 0, true, title, User1 | Ok | Cancel)
{
    QVBox *vbox = makeVBoxMainWidget();
    m_timeWidget = new TimeWidget
                   (title, vbox, composition, defaultTime);

    setButtonText(User1, i18n("Reset"));
    connect(this, SIGNAL(user1Clicked()),
            m_timeWidget, SLOT(slotResetToDefault()));
}

TimeDialog::TimeDialog(QWidget *parent, QString title,
                       Composition *composition,
                       timeT startTime,
                       timeT defaultTime) :
        KDialogBase(parent, 0, true, title, User1 | Ok | Cancel)
{
    QVBox *vbox = makeVBoxMainWidget();
    m_timeWidget = new TimeWidget
                   (title, vbox, composition, startTime, defaultTime);

    setButtonText(User1, i18n("Reset"));
    connect(this, SIGNAL(user1Clicked()),
            m_timeWidget, SLOT(slotResetToDefault()));
}

timeT
TimeDialog::getTime() const
{
    return m_timeWidget->getTime();
}

}
#include "TimeDialog.moc"
