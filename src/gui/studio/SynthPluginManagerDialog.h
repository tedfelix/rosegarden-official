
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_SYNTHPLUGINMANAGERDIALOG_H
#define RG_SYNTHPLUGINMANAGERDIALOG_H

#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "gui/general/ActionFileClient.h"

#include <QMainWindow>
#include <QSharedPointer>

#include <vector>


class QWidget;
class QPushButton;
class QCloseEvent;
class QComboBox;
class QDialogButtonBox;
class QVBoxLayout;
class QGridLayout;
class QGroupBox;
class QScrollArea;

namespace Rosegarden
{

class Studio;
class RosegardenDocument;
class AudioPluginGUIManager;
class AudioPluginManager;


class SynthPluginManagerDialog : public QMainWindow, public ActionFileClient
{
    Q_OBJECT

public:
    SynthPluginManagerDialog(QWidget *parent,
                             RosegardenDocument *doc,
			     AudioPluginGUIManager *guiManager);

    ~SynthPluginManagerDialog() override;

    void updatePlugin(InstrumentId id, int plugin);

    void setupGuiMain();
    void setupGuiCreatePluginList();

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

    void slotHelpRequested();

protected:
    void closeEvent(QCloseEvent *) override;

protected:
    RosegardenDocument *m_document;
    Studio *m_studio;
    QSharedPointer<AudioPluginManager> m_pluginManager;
    std::vector<int> m_synthPlugins;
    std::vector<QComboBox *> m_synthCombos;
    std::vector<QPushButton *> m_controlsButtons;
    std::vector<QPushButton *> m_guiButtons;


    QWidget     *m_centralWidget;
    QVBoxLayout *m_mainLayout;
    QGroupBox   *m_groupBoxPluginList;
    QVBoxLayout *m_verticalLayout_2;
    QScrollArea *m_scrollArea;
    QWidget     *m_scrollWidget;

    QGridLayout *m_scrollWidgetLayout;
//     QMenuBar    *m_menubar;
//     QStatusBar  *m_statusbar;

    QDialogButtonBox* m_dialogButtonBox;

    AudioPluginGUIManager *m_guiManager;
};



}

#endif
