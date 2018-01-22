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

#ifndef RG_AUDIOPLUGINDIALOG_H
#define RG_AUDIOPLUGINDIALOG_H

#include "base/Instrument.h"
#include "base/MidiProgram.h"

#include <QDialog>
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
class QShortcut;
class QComboBox;


namespace Rosegarden
{

class PluginControl;
class PluginContainer;
class AudioPluginOSCGUIManager;
class AudioPluginManager;
class AudioPluginInstance;


class AudioPluginDialog : public QDialog
{
    Q_OBJECT

public:
    AudioPluginDialog(QWidget *parent,
                      AudioPluginManager *aPM,
                      AudioPluginOSCGUIManager *aGM,
                      PluginContainer *instrument,
                      int index);
    virtual ~AudioPluginDialog();

    PluginContainer* getPluginContainer() const { return m_pluginContainer; }

    QShortcut* getShortcuts() { return m_shortcuts; }

    bool isSynth() { return m_index == int(Instrument::SYNTH_PLUGIN_POSITION); }

    void updatePlugin(int number);
    void updatePluginPortControl(int port);
    void updatePluginProgramControl();
    void updatePluginProgramList();
    void guiExited() { m_guiShown = false; }

public slots:
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

    void windowActivated();

protected slots:
    // Unused
    //virtual void slotClose();

protected:
    virtual void windowActivationChange(bool);

    void makePluginParamsBox(QWidget*);
    QStringList getProgramsForInstance(AudioPluginInstance *inst, int &current);

    //--------------- Data members ---------------------------------

    AudioPluginManager  *m_pluginManager;
    AudioPluginOSCGUIManager *m_pluginGUIManager;
    PluginContainer     *m_pluginContainer;
    InstrumentId         m_containerId;

    QGroupBox           *m_pluginParamsBox;
    QWidget             *m_pluginCategoryBox;
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
    QPushButton         *m_editorButton;
    
    QLabel              *m_programLabel;
    QComboBox           *m_programCombo;
    std::vector<PluginControl*> m_pluginWidgets;
    QGridLayout         *m_pluginParamsBoxLayout;

    int                  m_index;

    bool                 m_generating;
    bool                 m_guiShown;

    QShortcut              *m_shortcuts;

    void                 populatePluginCategoryList();
    void                 populatePluginList();
};


} // end of namespace



#endif
