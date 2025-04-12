
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SPLITBYPITCHDIALOG_H
#define RG_SPLITBYPITCHDIALOG_H

#include <QDialog>


class QWidget;
class QCheckBox;
class QComboBox;


namespace Rosegarden
{

class PitchChooser;


class SplitByPitchDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SplitByPitchDialog(QWidget *parent);

    int getPitch();

    int getStrategy();
    bool getShouldDuplicateNonNoteEvents();
    int getClefHandling(); // actually SegmentSplitByPitchCommand::ClefHandling

private:
    PitchChooser *m_pitch;

    QComboBox *m_strategy;
    QCheckBox *m_duplicate;
    QComboBox *m_clefs;
};



}

#endif
