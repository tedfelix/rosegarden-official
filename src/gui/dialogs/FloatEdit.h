
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

#ifndef _RG_ROSEGARDENFLOATEDIT_H_
#define _RG_ROSEGARDENFLOATEDIT_H_

#include <QDialog>
#include <QDialogButtonBox>


class QWidget;
class QString;
class QLabel;


namespace Rosegarden
{

class HSpinBox;


class FloatEdit : public QDialog
{
    Q_OBJECT

public:
    FloatEdit(QDialogButtonBox::QWidget *parent,
                        const QString &title,
                        const QString &text,
                        float min,
                        float max,
                        float value,
                        float step);

    float getValue() const;

protected:

    QLabel            *m_text;
    HSpinBox          *m_spin;
};



}

#endif
