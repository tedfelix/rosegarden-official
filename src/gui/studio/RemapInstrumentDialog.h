
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

#ifndef RG_REMAPINSTRUMENTDIALOG_H
#define RG_REMAPINSTRUMENTDIALOG_H

#include "base/Studio.h"
#include <QDialog>


class QWidget;
class QRadioButton;
class QComboBox;


namespace Rosegarden
{


class Command;
class RosegardenDocument;


/// The "Remap Instrument assignments..." dialog.
/**
 * From the main window: Tracks > Remap Instruments...
 *
 * This lets the user quickly change either the Device or Instrument on a
 * number of tracks at the same time.  E.g. if you want to change all
 * tracks using the General MIDI device to instead use Soft Synths, this
 * dialog can do that.
 */
class RemapInstrumentDialog : public QDialog
{
    Q_OBJECT
public:
    RemapInstrumentDialog(QWidget *parent,
                          RosegardenDocument *doc);

    void populateCombo();

public slots:
    void slotRemapReleased();

    void accept() override;
    void slotApply();

signals:
    void applyClicked();

protected:

    RosegardenDocument    *m_doc;

    QRadioButton        *m_deviceButton;
    QRadioButton        *m_instrumentButton;

    QComboBox           *m_fromCombo;
    QComboBox           *m_toCombo;

    DeviceVector m_devices;
    InstrumentVector m_instruments;
};


}

#endif
