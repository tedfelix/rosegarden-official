/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2014 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_INSTRUMENTPARAMETERPANEL_H
#define RG_INSTRUMENTPARAMETERPANEL_H

#include "gui/widgets/InstrumentAliasButton.h"

#include <QFrame>

#include <vector>
#include <utility>

class QWidget;
class QLabel;


namespace Rosegarden
{

class RosegardenDocument;
class Instrument;
class Rotary;

// A struct would be easier to understand than nested std::pair's.
// struct RotaryInfo
// {
//     Rotary *rotary;  // "second.first" becomes "rotary"
//     QLabel *label;   // "second.second" becomes "label"
//     int controller;  // "first" becomes "controller"
// };
// typedef std::vector<RotaryInfo> RotaryInfoVector;
typedef std::pair<Rotary *, QLabel *> RotaryPair;
typedef std::vector<std::pair<int /* controller */, RotaryPair> > RotaryMap;


////////////////////////////////////////////////////////////////////////

/// Code shared by the MIDI and Audio Instrument Parameter Panels
class InstrumentParameterPanel : public QFrame
{
    Q_OBJECT
public:
    InstrumentParameterPanel(RosegardenDocument *doc, QWidget *parent);
    virtual ~InstrumentParameterPanel() {}

    void setDocument(RosegardenDocument *doc);

signals:
    // Connected to InstrumentParameterBox::slotUpdateAllBoxes().
    void updateAllBoxes();

protected:
    RosegardenDocument *m_doc;

    Instrument *m_selectedInstrument;
    QLabel     *m_instrumentLabel;
    /// Make instrument our selected instrument.
    void setSelectedInstrument(Instrument *instrument, QString label);

private slots:
    /// m_selectedInstrument is being destroyed
    void slotInstrumentGone(void);
};



}

#endif
