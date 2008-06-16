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


#include "MarkerModifyDialog.h"
#include <qlayout.h>

#include <klocale.h>
#include "base/Composition.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/widgets/TimeWidget.h"
#include <kdialogbase.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>
#include "misc/Strings.h"


namespace Rosegarden
{

MarkerModifyDialog::MarkerModifyDialog(QWidget *parent,
                                       Composition *composition,
                                       int time,
                                       const QString &name,
                                       const QString &des):
    KDialogBase(parent, 0, true, i18n("Edit Marker"), Ok | Cancel)
{
    initialise(composition, time, name, des);
}

MarkerModifyDialog::MarkerModifyDialog(QWidget *parent,
                                       Composition *composition,
                                       Marker *marker) :
    KDialogBase(parent, 0, true, i18n("Edit Marker"), Ok | Cancel)
{
    initialise(composition, marker->getTime(),
               strtoqstr(marker->getName()),
               strtoqstr(marker->getDescription()));
}

void
MarkerModifyDialog::initialise(Composition *composition,
                               int time,
                               const QString &name,
                               const QString &des)
{
    m_originalTime = time;

    QVBox *vbox = makeVBoxMainWidget();

    m_timeEdit = new TimeWidget(i18n("Marker Time"), vbox, composition,
                                time);

    /*!!!
     
        layout->addWidget(new QLabel(i18n("Absolute Time:"), frame), 0, 0);
        m_timeEdit = new QSpinBox(frame);
        layout->addWidget(m_timeEdit, 0, 1);
     
        m_timeEdit->setMinValue(INT_MIN);
        m_timeEdit->setMaxValue(INT_MAX);
        m_timeEdit->setLineStep(
                Note(Note::Shortest).getDuration());
        m_timeEdit->setValue(time);
    */
    QGroupBox *groupBox = new QGroupBox
                          (1, Horizontal, i18n("Marker Properties"), vbox);

    QFrame *frame = new QFrame(groupBox);

    QGridLayout *layout = new QGridLayout(frame, 2, 2, 5, 5);

    layout->addWidget(new QLabel(i18n("Text:"), frame), 0, 0);
    m_nameEdit = new QLineEdit(name, frame);
    layout->addWidget(m_nameEdit, 0, 1);

    layout->addWidget(new QLabel(i18n("Description:"), frame), 1, 0);
    m_desEdit = new QLineEdit(des, frame);
    layout->addWidget(m_desEdit, 1, 1);

    m_nameEdit->selectAll();
    m_nameEdit->setFocus();
}

}
#include "MarkerModifyDialog.moc"
