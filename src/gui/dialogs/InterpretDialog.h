
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_INTERPRETDIALOG_H
#define RG_INTERPRETDIALOG_H

#include <QDialog>


class QWidget;
class QCheckBox;


namespace Rosegarden
{



class InterpretDialog : public QDialog
{
    Q_OBJECT
public:
    InterpretDialog(QWidget *parent);

    // an OR from InterpretCommand's constants
    int getInterpretations();

protected slots:
    void slotAllBoxChanged();
    void slotHelpRequested();

private:
    QCheckBox *m_allInterpretations;
    QCheckBox *m_applyTextDynamics;
    QCheckBox *m_applyHairpins;
    QCheckBox *m_stressBeats;
    QCheckBox *m_articulate;
};



}

#endif
