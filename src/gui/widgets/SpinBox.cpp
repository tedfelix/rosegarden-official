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


#include "SpinBox.h"
#include "misc/Strings.h"

#include <QSpinBox>
#include <QString>
#include <QWidget>


namespace Rosegarden
{

SpinBox::SpinBox(QWidget *parent, const char *name):
        QSpinBox(parent, name), m_doubleValue(0)
{}

QString
SpinBox::mapValueToText(int value)
{
    QString doubleStr;

    // Assume we want to show the precision
    //
    if ((int)m_doubleValue != value)
        m_doubleValue = (double) value;

    doubleStr.sprintf("%4.6f", m_doubleValue);

    // clear any special value
    //setSpecialValueText("");

    return doubleStr;
}

int
SpinBox::mapTextToValue(bool * /*ok*/)
{
    double number = qstrtodouble(text());

    if (number) {
        m_doubleValue = number;
        return ((int)number);
    }

    return 120; // default
}

}
#include "SpinBox.moc"
