
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

#ifndef _RG_MATRIXPARAMETERBOX_H_
#define _RG_MATRIXPARAMETERBOX_H_

#include <QFrame>
#include <vector>
#include "base/Event.h"


class QWidget;
class QComboBox;


namespace Rosegarden
{

class RosegardenGUIDoc;
class InstrumentParameterBox;
class Instrument;
class EventSelection;


class MatrixParameterBox : public QFrame
{
    Q_OBJECT

public:
    MatrixParameterBox(RosegardenGUIDoc *doc=0, QWidget *parent=0, const char* name=0);
    ~MatrixParameterBox();

    void initBox();
    void setSelection(EventSelection *);
    void useInstrument(Instrument *instrument);

protected:

    QComboBox                  *m_quantizeCombo;
    QComboBox                  *m_snapGridCombo;
    InstrumentParameterBox     *m_instrumentParameterBox;

    std::vector<timeT> m_quantizations;
    std::vector<timeT> m_snapValues;

    RosegardenGUIDoc           *m_doc;

};



}

#endif
