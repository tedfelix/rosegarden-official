// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <qvbox.h>

#include "widgets.h"
#include "Quantizer.h"
#include "Selection.h"
#include "MappedInstrument.h"

#include <vector>

#ifndef _MATRIXPARAMETERBOX_H_
#define _MATRIXPARAMETERBOX_H_

// Display and allow modification of Event parameters on the Matrix
//

class RosegardenComboBox;
class InstrumentParameterBox;
class RosegardenGUIDoc;

namespace Rosegarden
{
    class Instrument;
    class MappedEvent;
}

class MatrixParameterBox : public QVBox
{
Q_OBJECT

public:
    MatrixParameterBox(QWidget *parent=0, RosegardenGUIDoc *doc=0);
    ~MatrixParameterBox();

    void initBox();
    void setSelection(Rosegarden::EventSelection *);
    void useInstrument(Rosegarden::Instrument *instrument);

protected:

    RosegardenComboBox         *m_quantizeCombo;
    RosegardenComboBox         *m_snapGridCombo;
    InstrumentParameterBox     *m_instrumentParameterBox;

    std::vector<Rosegarden::StandardQuantization> m_quantizations;
    std::vector<Rosegarden::timeT>                m_snapValues;

    RosegardenGUIDoc           *m_doc;

};


#endif // _MATRIXPARAMETERBOX_H_
