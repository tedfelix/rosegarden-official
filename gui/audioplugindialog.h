/*
    Rosegarden-4 v0.2
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <kdialogbase.h>

#include "Instrument.h"

#ifndef _AUDIOPLUGINDIALOG_H_
#define _AUDIOPLUGINDIALOG_H_


namespace Rosegarden
{

class AudioPluginManager;

class AudioPluginDialog : public KDialogBase
{
    Q_OBJECT

public:
    AudioPluginDialog(QWidget *parent,
                      AudioPluginManager *aPM,
                      InstrumentId id);

signals:

protected:

    AudioPluginManager *m_pluginManager;
    InstrumentId        m_instrumentId;

};

}

#endif // _AUDIOPLUGINDIALOG_H_
