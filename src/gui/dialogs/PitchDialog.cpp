/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
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


#include "PitchDialog.h"

#include <klocale.h>
#include "gui/widgets/PitchChooser.h"
#include <kdialogbase.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

PitchDialog::PitchDialog(QWidget *parent, QString title, int defaultPitch) :
        KDialogBase(parent, 0, true, title, User1 | Ok)
{
    QVBox *vbox = makeVBoxMainWidget();
    m_pitchChooser = new PitchChooser(title, vbox, defaultPitch);

    setButtonText(User1, i18n("Reset"));
    connect(this, SIGNAL(user1Clicked()),
            m_pitchChooser, SLOT(slotResetToDefault()));
}

int
PitchDialog::getPitch() const
{
    return m_pitchChooser->getPitch();
}

}
#include "PitchDialog.moc"
