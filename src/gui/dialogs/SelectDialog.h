/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2013 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef SELECTDIALOG_H
#define SELECTDIALOG_H

// local includes
//#include <local/LocalInclude.h>
//#include "misc/Debug.h"

// Qt includes
//#include <QtInclude>
#include <QDialog>

// STL & other includes
//#include <vector>

class QToolButton;

namespace Rosegarden
{

class SelectDialog : public QDialog
{
    Q_OBJECT

public:
    SelectDialog(QWidget *parent = 0
                );
    ~SelectDialog();

public slots:
    void help();

protected:
/*
    // standard durations
    QToolButton        *m_durationBreve;
    QToolButton        *m_durationWhole;
    QToolButton         *m_durationHalf;
    QToolButton      *m_durationQuarter;
    QToolButton       *m_durationEighth;
    QToolButton    *m_durationSixteenth;
    QToolButton *m_durationThirtysecond;
    QToolButton  *m_durationSixtyfourth;

    // one dot durations
    QToolButton        *m_durationBreveDot;
    QToolButton        *m_durationWholeDot;
    QToolButton         *m_durationHalfDot;
    QToolButton      *m_durationQuarterDot;
    QToolButton       *m_durationEighthDot;
    QToolButton    *m_durationSixteenthDot;
    QToolButton *m_durationThirtysecondDot;
    QToolButton  *m_durationSixtyfourthDot;

    // two dot durations
    QToolButton        *m_durationBreveDotDot;
    QToolButton        *m_durationWholeDotDot;
    QToolButton         *m_durationHalfDotDot;
    QToolButton      *m_durationQuarterDotDot;
    QToolButton       *m_durationEighthDotDot;
    QToolButton    *m_durationSixteenthDotDot;
    QToolButton *m_durationThirtysecondDotDot;
    QToolButton  *m_durationSixtyfourthDotDot;

    // tuplet durations
    QToolButton        *m_durationBreveTuplet;
    QToolButton        *m_durationWholeTuplet;
    QToolButton         *m_durationHalfTuplet;
    QToolButton      *m_durationQuarterTuplet;
    QToolButton       *m_durationEighthTuplet;
    QToolButton    *m_durationSixteenthTuplet;
    QToolButton *m_durationThirtysecondTuplet;
    QToolButton  *m_durationSixtyfourthTuplet;

    // standard durations
    QToolButton        *m_durationBreve;
    QToolButton        *m_durationWhole;
    QToolButton         *m_durationHalf;
    QToolButton      *m_durationQuarter;
    QToolButton       *m_durationEighth;
    QToolButton    *m_durationSixteenth;
    QToolButton *m_durationThirtysecond;
    QToolButton  *m_durationSixtyfourth;

    // standard durations
    QToolButton        *m_durationBreve;
    QToolButton        *m_durationWhole;
    QToolButton         *m_durationHalf;
    QToolButton      *m_durationQuarter;
    QToolButton       *m_durationEighth;
    QToolButton    *m_durationSixteenth;
    QToolButton *m_durationThirtysecond;
    QToolButton  *m_durationSixtyfourth;

    // standard durations
    QToolButton        *m_durationBreve;
    QToolButton        *m_durationWhole;
    QToolButton         *m_durationHalf;
    QToolButton      *m_durationQuarter;
    QToolButton       *m_durationEighth;
    QToolButton    *m_durationSixteenth;
    QToolButton *m_durationThirtysecond;
    QToolButton  *m_durationSixtyfourth;

    // standard durations
    QToolButton        *m_durationBreve;
    QToolButton        *m_durationWhole;
    QToolButton         *m_durationHalf;
    QToolButton      *m_durationQuarter;
    QToolButton       *m_durationEighth;
    QToolButton    *m_durationSixteenth;
    QToolButton *m_durationThirtysecond;
    QToolButton  *m_durationSixtyfourth;
*/
};   

}

#endif
