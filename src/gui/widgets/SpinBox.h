
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2011 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_ROSEGARDENSPINBOX_H_
#define _RG_ROSEGARDENSPINBOX_H_

#include <QSpinBox>
#include <QString>


class QWidget;


namespace Rosegarden
{



/**
 * A Combobox that just about handles doubles - you have
 * to set the precision outside of this class if you're
 * using it with Qt designer.  Urch.
 */
class SpinBox : public QSpinBox
{
    Q_OBJECT
public:
    SpinBox(QWidget *parent = 0, const char *name=0);

    double getDoubleValue() const { return m_doubleValue; }

protected:
    virtual QString mapValueToText (int value);
    virtual int mapTextToValue(bool *ok);

    double m_doubleValue;
};



}

#endif
