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


#include <kdialogbase.h>

#ifndef _MIDIFILTER_H_
#define _MIDIFILTER_H_

class QButtonGroup;
class RosegardenGUIDoc;

namespace Rosegarden { class Studio; }

class MidiFilterDialog : public KDialogBase
{
    Q_OBJECT
public:
    MidiFilterDialog(QWidget *parent,
                     RosegardenGUIDoc *doc);

    void setModified(bool value);

public slots:

    void slotOk();
    void slotApply();
    void slotSetModified();

protected:

    RosegardenGUIDoc    *m_doc;

    QButtonGroup        *m_thruBox;
    QButtonGroup        *m_recordBox;

    bool                 m_modified;

};

#endif // _MIDIFILTER_H_

