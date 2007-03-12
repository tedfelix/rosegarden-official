
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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

#ifndef _RG_MATRIXPARAMETERBOX_H_
#define _RG_MATRIXPARAMETERBOX_H_

#include <qframe.h>
#include <vector>
#include "base/Event.h"


class QWidget;
class KComboBox;


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

    KComboBox                  *m_quantizeCombo;
    KComboBox                  *m_snapGridCombo;
    InstrumentParameterBox     *m_instrumentParameterBox;

    std::vector<timeT> m_quantizations;
    std::vector<timeT> m_snapValues;

    RosegardenGUIDoc           *m_doc;

};



}

#endif
