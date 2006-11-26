
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_SYNTHPLUGINMANAGERDIALOG_H_
#define _RG_SYNTHPLUGINMANAGERDIALOG_H_

#include "base/MidiProgram.h"
#include <kmainwindow.h>
#include <vector>
#include "document/ConfigGroups.h"


class QWidget;
class QPushButton;
class QCloseEvent;
class KComboBox;


namespace Rosegarden
{

class Studio;
class RosegardenGUIDoc;
class AudioPluginOSCGUIManager;
class AudioPluginManager;


class SynthPluginManagerDialog : public KMainWindow
{
    Q_OBJECT

public:
    SynthPluginManagerDialog(QWidget *parent,
                             RosegardenGUIDoc *doc
#ifdef HAVE_LIBLO
                             , AudioPluginOSCGUIManager *guiManager
#endif
        );

    virtual ~SynthPluginManagerDialog();

    void updatePlugin(InstrumentId id, int plugin);

signals:
    void closing();
    void pluginSelected(InstrumentId, int pluginIndex, int plugin);
    void showPluginDialog(QWidget *, InstrumentId, int pluginIndex);
    void showPluginGUI(InstrumentId, int pluginIndex);

protected slots:
    void slotClose();
    void slotPluginChanged(int index);
    void slotControlsButtonClicked();
    void slotGUIButtonClicked();

protected:
    virtual void closeEvent(QCloseEvent *);

protected:
    RosegardenGUIDoc *m_document;
    Studio *m_studio;
    AudioPluginManager *m_pluginManager;
    std::vector<int> m_synthPlugins;
    std::vector<KComboBox *> m_synthCombos;
    std::vector<QPushButton *> m_controlsButtons;
    std::vector<QPushButton *> m_guiButtons;

#ifdef HAVE_LIBLO
    AudioPluginOSCGUIManager *m_guiManager;
#endif

    static const char* const SynthPluginManagerConfigGroup;
};



}

#endif
