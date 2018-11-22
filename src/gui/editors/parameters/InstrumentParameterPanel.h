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
    InstrumentParameterPanel(QWidget *parent);
    ~InstrumentParameterPanel() override {}

protected:
    void setSelectedInstrument(Instrument *);
    Instrument *getSelectedInstrument();

    SqueezedLabel *m_instrumentLabel;

private slots:
    /// m_selectedInstrument is being destroyed
    void slotInstrumentGone();

private:
    // ??? This needs to go.  If the panels need the selected instrument,
    //     they need to get it directly from the document.  That
    //     simplifies things by not needing to maintain this pointer.
    //     And not needing to connect for the Instrument's destroy() signal.
    //     See InstrumentParameterBox::slotDocumentModified() for the
    //     proper steps to get the selected instrument pointer.
    //     As an interim solution, we could move those steps into
    //     getSelectedInstrument() and stub out setSelectedInstrument().
    Instrument *m_selectedInstrument;
};



}

#endif
