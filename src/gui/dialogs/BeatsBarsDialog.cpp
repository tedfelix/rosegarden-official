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


#include "BeatsBarsDialog.h"
#include <qlayout.h>

#include <klocale.h>
#include "base/Segment.h"
#include <kcombobox.h>
#include <kdialogbase.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qspinbox.h>
#include <qwidget.h>


namespace Rosegarden
{

BeatsBarsDialog::BeatsBarsDialog(QWidget* parent) :
        KDialogBase(parent, 0, true, i18n("Audio Segment Duration"),
                    Ok | Cancel, Ok)
{
    QHBox *hbox = makeHBoxMainWidget();

    QGroupBox *gbox = new QGroupBox(1, Horizontal,
                                    i18n("The selected audio segment contains:"), hbox);

    QFrame *frame = new QFrame(gbox);
    QGridLayout *layout = new QGridLayout(frame, 1, 2, 5, 5);

    m_spinBox = new QSpinBox(1, INT_MAX, 1, frame, "glee");
    layout->addWidget(m_spinBox, 0, 0);

    m_comboBox = new KComboBox(false, frame);
    m_comboBox->insertItem(i18n("beat(s)"));
    m_comboBox->insertItem(i18n("bar(s)"));
    m_comboBox->setCurrentItem(0);
    layout->addWidget(m_comboBox, 0, 1);
}

}
#include "BeatsBarsDialog.moc"
