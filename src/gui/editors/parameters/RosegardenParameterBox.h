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

#ifndef RG_ROSEGARDENPARAMETERBOX_H
#define RG_ROSEGARDENPARAMETERBOX_H

#include <QFont>
#include <QFrame>
#include <QString>

class QWidget;

namespace Rosegarden
{


/// A QFrame with label and font support.
class RosegardenParameterBox : public QFrame
{
    Q_OBJECT

public:
    RosegardenParameterBox(
            const QString &label,  // e.g. tr("Track Parameters")
            QWidget *parent);

    const QString &getLabel() const  { return m_label; }

protected:
    /// Font used by all derivers to give a consistent look.
    QFont m_font;

private:
    QString m_label;

};


}

#endif
