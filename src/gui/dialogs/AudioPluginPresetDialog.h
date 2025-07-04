/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOPLUGINPRESETDIALOG_H
#define RG_AUDIOPLUGINPRESETDIALOG_H

#include <QDialog>

#include <vector>

#include "base/AudioPluginInstance.h"

class QComboBox;

namespace Rosegarden
{

class AudioPluginGUIManager;

/// The "Audio Plugin Presets" dialog.
class AudioPluginPresetDialog : public QDialog
{
    Q_OBJECT

 public:
    AudioPluginPresetDialog(QWidget *parent,
                            InstrumentId instrument,
                            int position);

 public slots:
    void slotSetPreset();
    void slotLoadPreset();
    void slotSavePreset();

 private:
    InstrumentId m_instrument;
    int m_position;
    AudioPluginGUIManager *m_pluginGUIManager;
    QComboBox* m_presetCombo;
    AudioPluginInstance::PluginPresetList m_presets;
};

}

#endif
