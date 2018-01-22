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

#ifndef RG_AUDIOROUTEMENU_H
#define RG_AUDIOROUTEMENU_H

#include "base/Instrument.h"  // InstrumentId

#include <QObject>
#include <QString>


class QWidget;
class QComboBox;
class QAction;


namespace Rosegarden
{


class WheelyButton;
class Studio;

/// A specialised menu for selecting audio inputs or outputs.
/**
 * This class queries the studio and instrument to find out what it should
 * show.  Available in a "compact" size, which is a push button with a popup
 * menu attached, or a regular size which is a combobox.
 */
class AudioRouteMenu : public QObject
{
    Q_OBJECT

public:
    enum Direction { In, Out };
    enum Format { Compact, Regular };

    AudioRouteMenu(QWidget *parent,
                   Direction direction,
                   Format format,
                   InstrumentId instrumentId = NoInstrument);

    /// Connect to a different Instrument.
    void setInstrument(InstrumentId instrumentId);

    /// Get the WheelyButton (Compact) or QComboBox (Regular).
    QWidget *getWidget();

    /// Update the widget from the Instrument.
    void updateWidget();

private slots:
    /// Handle wheel movement from WheelyButton.
    void slotWheel(bool up);
    /// Handle click from WheelyButton.  Launch pop-up menu.
    void slotShowMenu();
    /// Handle selection from WheelyButton pop-up menu.
    void slotEntrySelected(QAction *);

    /// Handle selection change from QComboBox.
    void slotEntrySelected(int);

private:
    InstrumentId m_instrumentId;

    Direction m_direction;
    Format m_format;

    WheelyButton *m_button;
    QComboBox *m_combo;

    /// Number of entries in the combo/menu.
    int getNumEntries();
    /// Selected entry based on Instrument.
    int getCurrentEntry();
    /// Text for a specific entry.
    QString getEntryText(int n);
};


}

#endif
