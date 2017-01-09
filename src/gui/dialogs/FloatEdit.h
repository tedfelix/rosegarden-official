
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_FLOATEDIT_H
#define RG_FLOATEDIT_H

#include <QDialog>


class QWidget;
class QString;
class QLabel;
class QDoubleSpinBox;


namespace Rosegarden
{


/** A simple input dialog for requesting a float value
 */
class FloatEdit : public QDialog
{
    Q_OBJECT

public:
    FloatEdit(QWidget *parent,
              const QString &title,
              const QString &text,
              float min,
              float max,
              float value,
              float step);

    /// Get a float value from the dialog
    float getValue() const;

    /// Reparent the float edit dialog correctly by context, so it can be made
    /// to appear in a sensible place
    void reparent(QWidget *newParent);

protected:

    QLabel            *m_text;
    QDoubleSpinBox    *m_spin;
};


}

#endif
