// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _MIXER_H_
#define _MIXER_H_

#include <kmainwindow.h>
#include "studiowidgets.h"
#include "Instrument.h"

class RosegardenGUIDoc;
namespace Rosegarden { class Studio; }
class SequencerMapper;

class MixerWindow : public KMainWindow
{
    Q_OBJECT

public:
    MixerWindow(QWidget *parent, RosegardenGUIDoc *document);
    ~MixerWindow();

    void updateMeters(SequencerMapper *mapper);

signals:
    void closing();

protected slots:
    void slotFaderLevelChanged(float level);
    
protected:
    virtual void closeEvent(QCloseEvent *);

private:
    RosegardenGUIDoc *m_document;
    Rosegarden::Studio *m_studio;

    typedef std::map<Rosegarden::InstrumentId, AudioFaderWidget *> FaderMap;
    FaderMap m_faders;

    typedef std::vector<AudioFaderWidget *> FaderVector;
    FaderVector m_submasters;
    AudioFaderWidget *m_monitor;
    AudioFaderWidget *m_master;

    void updateFader(int id); // instrument id if large enough, master/sub otherwise
};

#endif
