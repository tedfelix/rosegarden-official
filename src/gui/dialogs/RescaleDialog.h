
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_RESCALEDIALOG_H
#define RG_RESCALEDIALOG_H

#include "base/TimeT.h"

#include <QDialog>

class QWidget;
class QCheckBox;
class QDialogButtonBox;


namespace Rosegarden
{


class TimeWidget2;


class RescaleDialog : public QDialog
{
    Q_OBJECT

public:

    RescaleDialog(QWidget *parent,
                  timeT startTime,
                  timeT originalDuration,
                  timeT minimumDuration,
                  bool showCloseGapOption,
                  bool constrainToCompositionDuration);

    timeT getNewDuration();
    bool shouldCloseGap();

private slots:

    void slotIsValid(bool valid);

private:

    TimeWidget2 *m_newDuration;
    QCheckBox *m_closeGap;
    QDialogButtonBox *m_buttonBox;

};
    

}

#endif
