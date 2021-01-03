
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_INSERTTUPLETDIALOG_H
#define RG_INSERTTUPLETDIALOG_H

#include "base/NotationTypes.h"
#include <QDialog>
#include "base/Event.h"


class QWidget;
class QString;
class QSpinBox;
class QGroupBox;



namespace Rosegarden
{



class InsertTupletDialog : public QDialog
{
    Q_OBJECT

public:
    InsertTupletDialog(QWidget *parent,
            unsigned int untupledCount,
            unsigned int tupledCount);

    unsigned int getUntupledCount() const;
    unsigned int getTupledCount() const;


protected:

    //--------------- Data members ---------------------------------

    QSpinBox *m_untupledSpin;
    QSpinBox *m_tupledSpin;


    QGroupBox *m_timingDisplayGrid;
};



}

#endif
