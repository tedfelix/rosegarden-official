// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
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

#include "widgets.h"
#include "Quantizer.h"
#include "Selection.h"


#include <vector>

#ifndef _MATRIXPARAMETERBOX_H_
#define _MATRIXPARAMETERBOX_H_

// Display and allow modification of Instrument parameters
//

class RosegardenComboBox;
class QCheckBox;

class MatrixParameterBox : public RosegardenParameterBox
{
Q_OBJECT

public:
    MatrixParameterBox(QWidget *parent=0);
    ~MatrixParameterBox();

    void initBox();

public slots:

signals:

protected:

    RosegardenComboBox         *m_quantizeValue;
    RosegardenComboBox         *m_snapGridValue;

    std::vector<Rosegarden::StandardQuantization> m_standardQuantizations;

};


#endif // _MATRIXPARAMETERBOX_H_
