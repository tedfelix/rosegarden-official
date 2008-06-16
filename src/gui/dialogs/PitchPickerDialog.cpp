/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "PitchPickerDialog.h"
#include <klocale.h>
#include <kdialogbase.h>
#include <qlayout.h>
#include <qframe.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

PitchPickerDialog::PitchPickerDialog(QWidget *parent, int initialPitch, QString text) :
        KDialogBase(parent, 0, true, i18n("Pitch Selector"), Ok | Cancel)
{
    QVBox *vBox = makeVBoxMainWidget();

    QFrame *frame = new QFrame(vBox);

    QGridLayout *layout = new QGridLayout(frame, 4, 3, 10, 5);

    m_pitch = new PitchChooser(text, frame, initialPitch);
    layout->addMultiCellWidget(m_pitch, 0, 0, 0, 2, Qt::AlignHCenter);
}

PitchPickerDialog::~PitchPickerDialog()
{
    // Nothing here...
}

}
#include "PitchPickerDialog.moc"
