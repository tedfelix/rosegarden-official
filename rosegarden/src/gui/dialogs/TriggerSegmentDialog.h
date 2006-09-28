
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
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

#ifndef _RG_TRIGGERSEGMENTDIALOG_H_
#define _RG_TRIGGERSEGMENTDIALOG_H_

#include "base/TriggerSegment.h"
#include <string>
#include <kdialogbase.h>


class QWidget;
class QCheckBox;
class KComboBox;


namespace Rosegarden
{

class Composition;


class TriggerSegmentDialog : public KDialogBase
{
    Q_OBJECT

public:
    TriggerSegmentDialog(QWidget *parent, Composition *);

    TriggerSegmentId getId() const;
    bool getRetune() const;
    std::string getTimeAdjust() const;

public slots:
    void slotOk();

protected:
    void setupFromConfig();

    Composition  *m_composition;
    KComboBox                *m_segment;
    QCheckBox                *m_retune;
    KComboBox                *m_adjustTime;
};


}

#endif
