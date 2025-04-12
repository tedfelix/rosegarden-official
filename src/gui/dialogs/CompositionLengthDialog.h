
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_COMPOSITIONLENGTHDIALOG_H
#define RG_COMPOSITIONLENGTHDIALOG_H

#include <QDialog>
#include "base/Event.h"


class QWidget;
class QSpinBox;
class QCheckBox;


namespace Rosegarden
{

class Composition;


class CompositionLengthDialog : public QDialog
{
    Q_OBJECT
public:
    CompositionLengthDialog(QWidget *parent,
                            Composition *composition);

    timeT getStartMarker();
    timeT getEndMarker();
    bool autoExpandEnabled();

protected:

    QSpinBox                *m_startMarkerSpinBox;
    QSpinBox                *m_endMarkerSpinBox;
    QCheckBox               *m_autoExpandCheckBox;
    Composition *m_composition;
};



}

#endif
