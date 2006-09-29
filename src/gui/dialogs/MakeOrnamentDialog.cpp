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


#include "MakeOrnamentDialog.h"

#include <klocale.h>
#include "gui/widgets/PitchChooser.h"
#include <kdialogbase.h>
#include <qgroupbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qstring.h>
#include <qvbox.h>
#include <qwidget.h>


namespace Rosegarden
{

MakeOrnamentDialog::MakeOrnamentDialog(QWidget *parent, QString defaultName,
                                       int defaultBasePitch) :
        KDialogBase(parent, "makeornamentdialog", true, i18n("Make Ornament"),
                    Ok | Cancel, Ok)
{
    QVBox *vbox = makeVBoxMainWidget();
    QGroupBox *nameBox = new QGroupBox(2, Vertical, i18n("Name"), vbox);

    new QLabel(i18n("The name is used to identify both the ornament\nand the triggered segment that stores\nthe ornament's notes."), nameBox);

    QHBox *hbox = new QHBox(nameBox);
    new QLabel(i18n("Name:  "), hbox);
    m_name = new QLineEdit(defaultName, hbox);

    m_pitch = new PitchChooser(i18n("Base pitch"), vbox, defaultBasePitch);
}

QString
MakeOrnamentDialog::getName() const
{
    return m_name->text();
}

int
MakeOrnamentDialog::getBasePitch() const
{
    return m_pitch->getPitch();
}

}
#include "MakeOrnamentDialog.moc"
