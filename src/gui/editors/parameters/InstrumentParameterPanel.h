/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.

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

#include <QFrame>

#include <vector>
#include <utility>

class QWidget;
class QLabel;


namespace Rosegarden
{

class RosegardenDocument;
class Instrument;
class SqueezedLabel;


////////////////////////////////////////////////////////////////////////

/// Code shared by the MIDI and Audio Instrument Parameter Panels
class InstrumentParameterPanel : public QFrame
{
    Q_OBJECT
public:
    InstrumentParameterPanel(RosegardenDocument *doc, QWidget *parent);
    virtual ~InstrumentParameterPanel() {}

    void setDocument(RosegardenDocument *doc);

protected:
    RosegardenDocument *m_doc;

    void setSelectedInstrument(Instrument *);
    Instrument *getSelectedInstrument();

    SqueezedLabel *m_instrumentLabel;

private slots:
    /// m_selectedInstrument is being destroyed
    void slotInstrumentGone(void);

private:
    Instrument *m_selectedInstrument;
};



}

#endif
