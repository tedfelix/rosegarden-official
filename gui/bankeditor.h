
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

#include <vector>

#include <kdialogbase.h>
#include <qspinbox.h>

class RosegardenComboBox;
class QLineEdit;

namespace Rosegarden { class Studio; }

#ifndef _BANKEDITOR_H_
#define _BANKEDITOR_H_

class BankEditorDialog : public KDialogBase
{
public:
    BankEditorDialog(QWidget *parent,
                     Rosegarden::Studio *studio);

    void populateDialog();

protected:

    //--------------- Data members ---------------------------------
    Rosegarden::Studio      *m_studio;

    QSpinBox                *m_msb;
    QSpinBox                *m_lsb;
    std::vector<QLineEdit*>  m_programNames;
};

#endif // _BANKEDITOR_H_

