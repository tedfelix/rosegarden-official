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

#include <vector.h>

#include <kdialogbase.h>

#include <qhbox.h>
#include <qdial.h>

#include "Instrument.h"

#ifndef _AUDIOPLUGINDIALOG_H_
#define _AUDIOPLUGINDIALOG_H_


// Attempt to dynamically create plugin dialogs for the plugins
// that the sequencer has discovered for us and returned into the
// AudioPluginManager.  Based on plugin port descriptions we 
// attempt to represent a meaningful plugin layout.
//
//

class RosegardenComboBox;

namespace Rosegarden
{

class AudioPluginManager;
class PluginPort;

class PluginControl : public QHBox
{
public:

    typedef enum
    {
        Rotary,
        Slider,
        NumericSlider
    } ControlType;

    PluginControl(QWidget *parent,
                  ControlType type,
                  PluginPort *port);

protected:

    ControlType  m_type;
    PluginPort  *m_port;

    int          m_multiplier;
    QDial       *m_dial;

};

typedef std::vector<PluginControl*>::iterator ControlIterator;

class AudioPluginDialog : public KDialogBase
{
    Q_OBJECT

public:
    AudioPluginDialog(QWidget *parent,
                      AudioPluginManager *aPM,
                      Instrument *instrument);

public slots:
    void slotPluginSelected(int);

signals:

protected:

    AudioPluginManager  *m_pluginManager;
    Instrument          *m_instrument;

    RosegardenComboBox  *m_pluginList;

    std::vector<PluginControl*> m_pluginWidgets;

    int                  m_headHeight;


};

}

#endif // _AUDIOPLUGINDIALOG_H_
