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

#include <QDialog>

class QPushButton;

namespace Rosegarden
{


class CheckButton;

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

    // Duration widgets
    // INSERT GENERATED HEADER CODE AFTER THIS LINE


    // breve buttons
    CheckButton *m_use_duration_breve;
    CheckButton *m_use_duration_breve_dotted;
    CheckButton *m_use_duration_breve_double_dotted;
    CheckButton *m_use_duration_breve_tuplet;
    CheckButton *m_use_duration_breve_rest;
    CheckButton *m_use_duration_breve_dotted_rest;
    CheckButton *m_use_duration_breve_double_dotted_rest;
    CheckButton *m_use_duration_breve_rest_tuplet;
    CheckButton *m_use_all_breve;

    // semibreve buttons
    CheckButton *m_use_duration_semibreve;
    CheckButton *m_use_duration_semibreve_dotted;
    CheckButton *m_use_duration_semibreve_double_dotted;
    CheckButton *m_use_duration_semibreve_tuplet;
    CheckButton *m_use_duration_semibreve_rest;
    CheckButton *m_use_duration_semibreve_dotted_rest;
    CheckButton *m_use_duration_semibreve_double_dotted_rest;
    CheckButton *m_use_duration_semibreve_rest_tuplet;
    CheckButton *m_use_all_semibreve;

    // minim buttons
    CheckButton *m_use_duration_minim;
    CheckButton *m_use_duration_minim_dotted;
    CheckButton *m_use_duration_minim_double_dotted;
    CheckButton *m_use_duration_minim_tuplet;
    CheckButton *m_use_duration_minim_rest;
    CheckButton *m_use_duration_minim_dotted_rest;
    CheckButton *m_use_duration_minim_double_dotted_rest;
    CheckButton *m_use_duration_minim_rest_tuplet;
    CheckButton *m_use_all_minim;

    // crotchet buttons
    CheckButton *m_use_duration_crotchet;
    CheckButton *m_use_duration_crotchet_dotted;
    CheckButton *m_use_duration_crotchet_double_dotted;
    CheckButton *m_use_duration_crotchet_tuplet;
    CheckButton *m_use_duration_crotchet_rest;
    CheckButton *m_use_duration_crotchet_dotted_rest;
    CheckButton *m_use_duration_crotchet_double_dotted_rest;
    CheckButton *m_use_duration_crotchet_rest_tuplet;
    CheckButton *m_use_all_crotchet;

    // quaver buttons
    CheckButton *m_use_duration_quaver;
    CheckButton *m_use_duration_quaver_dotted;
    CheckButton *m_use_duration_quaver_double_dotted;
    CheckButton *m_use_duration_quaver_tuplet;
    CheckButton *m_use_duration_quaver_rest;
    CheckButton *m_use_duration_quaver_dotted_rest;
    CheckButton *m_use_duration_quaver_double_dotted_rest;
    CheckButton *m_use_duration_quaver_rest_tuplet;
    CheckButton *m_use_all_quaver;

    // semiquaver buttons
    CheckButton *m_use_duration_semiquaver;
    CheckButton *m_use_duration_semiquaver_dotted;
    CheckButton *m_use_duration_semiquaver_double_dotted;
    CheckButton *m_use_duration_semiquaver_tuplet;
    CheckButton *m_use_duration_semiquaver_rest;
    CheckButton *m_use_duration_semiquaver_dotted_rest;
    CheckButton *m_use_duration_semiquaver_double_dotted_rest;
    CheckButton *m_use_duration_semiquaver_rest_tuplet;
    CheckButton *m_use_all_semiquaver;

    // demisemi buttons
    CheckButton *m_use_duration_demisemi;
    CheckButton *m_use_duration_demisemi_dotted;
    CheckButton *m_use_duration_demisemi_double_dotted;
    CheckButton *m_use_duration_demisemi_tuplet;
    CheckButton *m_use_duration_demisemi_rest;
    CheckButton *m_use_duration_demisemi_dotted_rest;
    CheckButton *m_use_duration_demisemi_double_dotted_rest;
    CheckButton *m_use_duration_demisemi_rest_tuplet;
    CheckButton *m_use_all_demisemi;

    // hemidemisemi buttons
    CheckButton *m_use_duration_hemidemisemi;
    CheckButton *m_use_duration_hemidemisemi_dotted;
    CheckButton *m_use_duration_hemidemisemi_double_dotted;
    CheckButton *m_use_duration_hemidemisemi_tuplet;
    CheckButton *m_use_duration_hemidemisemi_rest;
    CheckButton *m_use_duration_hemidemisemi_dotted_rest;
    CheckButton *m_use_duration_hemidemisemi_double_dotted_rest;
    CheckButton *m_use_duration_hemidemisemi_rest_tuplet;
    CheckButton *m_use_all_hemidemisemi;


    // INSERT GENERATED HEADER CODE BEFORE THIS LINE
    QPushButton *m_selectEveryDuration;
    QPushButton *m_selectNoDurations;
};   

}

#endif
