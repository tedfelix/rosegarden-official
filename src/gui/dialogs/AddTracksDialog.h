/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ADDTRACKSDIALOG_H
#define RG_ADDTRACKSDIALOG_H

#include <QDialog>

class QWidget;
class QSpinBox;
class QComboBox;


namespace Rosegarden
{


class AddTracksDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddTracksDialog(QWidget *parent);

public slots:
    virtual void accept() override;

private slots:
    void slotDeviceChanged(int)  { updateInstrumentComboBox(); }

private:
    QSpinBox *m_numberOfTracks;

    QComboBox *m_location;
    int getInsertPosition();

    QComboBox *m_device;
    void initDeviceComboBox();

    QComboBox *m_instrument;
    void updateInstrumentComboBox();
};


}

#endif
