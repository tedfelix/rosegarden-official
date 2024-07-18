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

#ifndef RG_MIDIFILTERDIALOG_H
#define RG_MIDIFILTERDIALOG_H

#include <QDialog>

class QWidget;
class QGroupBox;
class QDialogButtonBox;
class QCheckBox;


namespace Rosegarden
{


class RosegardenDocument;


/// The Modify MIDI Filters dialog.  Studio > Modify MIDI Filters...
/**
 * This dialog controls thru and record filtering.
 */
class MidiFilterDialog : public QDialog
{
    Q_OBJECT
public:

    MidiFilterDialog(QWidget *parent,
                     RosegardenDocument *doc);

private slots:

    void slotClicked(bool);
    void accept() override;
    void help();
    void slotApply();

private:

    RosegardenDocument *m_doc;

    // Widgets

    QGroupBox *m_thruBox;
    QGroupBox *m_recordBox;

    QCheckBox *m_noteThru;
    QCheckBox *m_progThru;
    QCheckBox *m_keyThru;
    QCheckBox *m_chanThru;
    QCheckBox *m_pitchThru;
    QCheckBox *m_contThru;
    QCheckBox *m_sysThru;

    QCheckBox *m_noteRecord;
    QCheckBox *m_progRecord;
    QCheckBox *m_keyRecord;
    QCheckBox *m_chanRecord;
    QCheckBox *m_pitchRecord;
    QCheckBox *m_contRecord;
    QCheckBox *m_sysRecord;

    QDialogButtonBox *m_buttonBox;
    QPushButton *m_applyButton;


    // Used to enable/disable the apply button.
    bool m_modified{false};
    void setModified(bool modified);

};


}

#endif
