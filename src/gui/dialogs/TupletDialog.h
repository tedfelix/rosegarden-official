
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

#ifndef _RG_TUPLETDIALOG_H_
#define _RG_TUPLETDIALOG_H_

#include "base/NotationTypes.h"
#include <kdialogbase.h>
#include "base/Event.h"


class QWidget;
class QString;
class QLabel;
class QGroupBox;
class QCheckBox;
class KComboBox;


namespace Rosegarden
{



class TupletDialog : public KDialogBase
{
    Q_OBJECT

public:
    TupletDialog(QWidget *parent,
                 Note::Type defaultUnitType,
                 timeT maxDuration = 0);

    Note::Type getUnitType() const;
    int getUntupledCount() const;
    int getTupledCount() const;
    bool hasTimingAlready() const;

public slots:
    void slotUnitChanged(const QString &);
    void slotUntupledChanged(const QString &);
    void slotTupledChanged(const QString &);
    void slotHasTimingChanged();

protected:

    void updateUntupledCombo();
    void updateTupledCombo();
    void updateTimingDisplays();

    //--------------- Data members ---------------------------------

    KComboBox *m_unitCombo;
    KComboBox *m_untupledCombo;
    KComboBox *m_tupledCombo;

    QCheckBox *m_hasTimingAlready;

    QGroupBox *m_timingDisplayBox;
    QLabel *m_selectionDurationDisplay;
    QLabel *m_untupledDurationCalculationDisplay;
    QLabel *m_untupledDurationDisplay;
    QLabel *m_tupledDurationCalculationDisplay;
    QLabel *m_tupledDurationDisplay;
    QLabel *m_newGapDurationCalculationDisplay;
    QLabel *m_newGapDurationDisplay;
    QLabel *m_unchangedDurationCalculationDisplay;
    QLabel *m_unchangedDurationDisplay;

    timeT m_maxDuration;
};



}

#endif
