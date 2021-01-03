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

#ifndef RG_MIDIFILTERDIALOG_H
#define RG_MIDIFILTERDIALOG_H

#include <QDialog>


class QWidget;
class QGroupBox;
class QDialogButtonBox;


namespace Rosegarden
{

class RosegardenDocument;


class MidiFilterDialog : public QDialog
{
    Q_OBJECT
public:
    MidiFilterDialog(QWidget *parent,
                     RosegardenDocument *doc);

    void setModified(bool value);

public slots:

    void accept() override;
    void help();
    void slotApply();
    void slotSetModified(int foo = 42);

protected:

    RosegardenDocument *m_doc;

    QGroupBox        *m_thruBox;
    QGroupBox        *m_recordBox;

    bool              m_modified;
    QDialogButtonBox *m_buttonBox;
    QPushButton      *m_applyButton;

};


}

#endif
