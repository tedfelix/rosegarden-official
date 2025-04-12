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

#ifndef RG_AUDIOPLUGINDIALOG_H
#define RG_AUDIOPLUGINDIALOG_H

#include "base/Instrument.h"
#include "base/MidiProgram.h"

#include <QDialog>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

#include <vector>


class QWidget;
class QPushButton;
class QLabel;
class QGridLayout;
//class QFrame;
class QGroupBox;
class QCheckBox;
class QComboBox;
class QLineEdit;

namespace Rosegarden
{

class PluginControl;
class PluginContainer;
class AudioPluginGUIManager;
class AudioPluginManager;
class AudioPluginInstance;


class AudioPluginDialog : public QDialog
{
    Q_OBJECT

public:
    AudioPluginDialog(QWidget *parent,
                      QSharedPointer<AudioPluginManager> aPM,
                      AudioPluginGUIManager *aGM,
                      PluginContainer *pluginContainer,
                      int index);
    ~AudioPluginDialog() override;

    PluginContainer* getPluginContainer() const { return m_pluginContainer; }

    bool isSynth() { return m_index == int(Instrument::SYNTH_PLUGIN_POSITION); }

    void updatePlugin(int number);
    void updatePluginPortControl(int port);
    void updatePluginProgramControl();
    void updatePluginProgramList();
    void guiExited() { m_guiShown = false; }

public slots:
    void slotSearchTextChanged(const QString& text);
    void slotArchSelected(int);
    void slotCategorySelected(int);
    void slotPluginSelected(int index);
    void slotPluginPortChanged(float value);
    void slotPluginProgramChanged(const QString &value);
    void slotBypassChanged(bool);
    void slotCopy();
    void slotPaste();
    void slotDefault();
    void slotShowGUI();
    void slotHelpRequested();

    void slotParameters();
    void slotPresets();
    virtual void slotEditConnections();
    virtual void slotEditor();

signals:
    void pluginSelected(InstrumentId, int pluginIndex, int plugin);
    void pluginPortChanged(InstrumentId, int pluginIndex, int portIndex);
    void pluginProgramChanged(InstrumentId, int pluginIndex);
    void changePluginConfiguration(InstrumentId, int pluginIndex,
                                   bool global, QString key, QString value);
    void showPluginGUI(InstrumentId, int pluginIndex);
    void stopPluginGUI(InstrumentId, int pluginIndex);

    // is the plugin being bypassed
    void bypassed(InstrumentId, int pluginIndex, bool bp);
    void destroyed(InstrumentId, int index);

protected slots:
    // Unused
    //virtual void slotClose();

protected:
    void makePluginParamsBox(QWidget*);
    QStringList getProgramsForInstance(AudioPluginInstance *inst, int &current);

    //--------------- Data members ---------------------------------

    QSharedPointer<AudioPluginManager> m_pluginManager;
    AudioPluginGUIManager *m_pluginGUIManager;
    PluginContainer     *m_pluginContainer;
    InstrumentId         m_containerId;

    QGroupBox           *m_pluginParamsBox;
    QWidget             *m_pluginSearchBox;
    QLineEdit           *m_pluginSearchText;
    QWidget             *m_pluginCategoryBox;
    QComboBox           *m_pluginArchList;
    QComboBox           *m_pluginCategoryList;
    QLabel              *m_pluginLabel;
    QComboBox           *m_pluginList;
    std::vector<int>     m_pluginsInList;
    QLabel              *m_insOuts;
    QLabel              *m_pluginId;
    QCheckBox           *m_bypass;
    QPushButton         *m_copyButton;
    QPushButton         *m_pasteButton;
    QPushButton         *m_defaultButton;
    QPushButton         *m_guiButton;
    QPushButton         *m_paramsButton;
    QPushButton         *m_presetButton;
    QPushButton         *m_editConnectionsButton;
    QPushButton         *m_editorButton;

    QLabel              *m_programLabel;
    QComboBox           *m_programCombo;
    std::vector<PluginControl*> m_pluginWidgets;
    QGridLayout         *m_pluginParamsBoxLayout;

    int                  m_index;

    bool                 m_generating;
    bool                 m_guiShown;
    int                  m_selectdPluginNumber;

    void                 populatePluginCategoryList();
    void                 populatePluginList();
};


} // end of namespace



#endif
