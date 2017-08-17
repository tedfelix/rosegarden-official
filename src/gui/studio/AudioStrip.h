/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOSTRIP_H
#define RG_AUDIOSTRIP_H

#include "base/Instrument.h"

#include <QWidget>

class QGridLayout;


namespace Rosegarden
{


class Label;
class AudioRouteMenu;


/// The strips on the "Audio Mixer" window.
class AudioStrip : public QWidget
{
    Q_OBJECT

public:
    AudioStrip(QWidget *parent, int i_id = -1);
    virtual ~AudioStrip();

    void setId(int id);
    int getId() const  { return m_id; }

    void updateWidgets();

private slots:
    void slotLabelClicked();

private:
    /// Buss/Instrument ID.
    int m_id;

    bool isMaster() const  { return (m_id == 0); }
    bool isSubmaster() const
            { return (m_id > 0  &&  m_id < static_cast<int>(AudioInstrumentBase)); }
    bool isInput() const  { return (m_id >= static_cast<int>(AudioInstrumentBase)); }

    // Widgets
    Label *m_label;
    AudioRouteMenu *m_input;

    QGridLayout *m_layout;

    void createWidgets();
};


}

#endif
