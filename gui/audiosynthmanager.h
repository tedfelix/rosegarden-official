// -*- c-basic-offset: 4 -*-
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#ifndef _AUDIO_SYNTH_MANAGER_H_
#define _AUDIO_SYNTH_MANAGER_H_

#include <kmainwindow.h>
#include <vector>

#include "Instrument.h"

class RosegardenGUIDoc;
class KComboBox;

namespace Rosegarden {
    class Studio;
    class AudioPluginManager;
}


class SynthPluginManagerDialog : public KMainWindow
{
    Q_OBJECT

public:
    SynthPluginManagerDialog(QWidget *parent,
			     RosegardenGUIDoc *doc);

    virtual ~SynthPluginManagerDialog();

signals:
    void closing();
    void pluginSelected(Rosegarden::InstrumentId, int pluginIndex, int plugin);

protected slots:
    void slotClose();
    void slotPluginChanged(int index);

protected:
    virtual void closeEvent(QCloseEvent *);

protected:
    RosegardenGUIDoc *m_document;
    Rosegarden::Studio *m_studio;
    Rosegarden::AudioPluginManager *m_pluginManager;
    std::vector<int> m_synthPlugins;
    std::vector<KComboBox *> m_synthCombos;

    static const char* const SynthPluginManagerConfigGroup;
};


#endif

