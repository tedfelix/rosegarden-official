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

#include <vector>

#include <kdialogbase.h>

#include <qhbox.h>
#include <qdial.h>
#include <qpushbutton.h>
#include <qstringlist.h>

#include "Instrument.h"

#ifndef _AUDIOPLUGINDIALOG_H_
#define _AUDIOPLUGINDIALOG_H_


// Attempt to dynamically create plugin dialogs for the plugins
// that the sequencer has discovered for us and returned into the
// AudioPluginManager.  Based on plugin port descriptions we 
// attempt to represent a meaningful plugin layout.
//
//

class RosegardenRotary;
class RosegardenGUIDoc;
class KComboBox;
class QGroupBox;
class QCheckBox;
class QLabel;
class QGridLayout;
class QAccel;

namespace Rosegarden
{

class PluginPort;
class AudioPluginManager;
class Studio;

class PluginControl : public QObject
{
    Q_OBJECT
public:

    typedef enum
    {
        Rotary,
        Slider,
        NumericSlider
    } ControlType;

    PluginControl(QWidget *parent,
		  QGridLayout *layout,
                  ControlType type,
                  PluginPort *port,
                  AudioPluginManager *pluginManager,
                  int index,
                  float initialValue);
 
    void setValue(float value);
    float getValue() const;

    int getIndex() const { return m_index; }

public slots:
    void slotValueChanged(float value);

signals:
    void valueChanged(float value);

protected:

    //--------------- Data members ---------------------------------

    QGridLayout         *m_layout;

    ControlType          m_type;
    PluginPort          *m_port;

    RosegardenRotary    *m_dial;
    AudioPluginManager  *m_pluginManager;

    int                  m_index;

};

typedef std::vector<PluginControl*>::iterator ControlIterator;
typedef std::vector<QHBox*>::iterator ControlLineIterator;

class AudioPluginDialog : public KDialogBase
{
    Q_OBJECT

public:
    AudioPluginDialog(QWidget *parent,
                      AudioPluginManager *aPM,
                      Instrument *instrument,
                      int index);

    Instrument* getInstrument() const { return m_instrument; }

    QAccel* getAccelerators() { return m_accelerators; }

    bool isSynth() { return m_index == int(Instrument::SYNTH_PLUGIN_POSITION); }

public slots:
    void slotCategorySelected(int);
    void slotPluginSelected(int index);
    void slotPluginPortChanged(float value);
    void slotPluginProgramChanged(const QString &value);
    void slotBypassChanged(bool);
    void slotCopy();
    void slotPaste();
    void slotDefault();

signals:
    void pluginSelected(Rosegarden::InstrumentId, int pluginIndex, int plugin);
    void pluginPortChanged(Rosegarden::InstrumentId, int pluginIndex, int portIndex, float value);
    void pluginProgramChanged(Rosegarden::InstrumentId, int pluginIndex, QString program);

    // is the plugin being bypassed
    void bypassed(Rosegarden::InstrumentId, int pluginIndex, bool bp);
    void destroyed(Rosegarden::InstrumentId, int index);

protected slots:
    virtual void slotClose();

protected:
    virtual void closeEvent(QCloseEvent *e);

    void makePluginParamsBox(QWidget*);
    QStringList getProgramsForInstance(AudioPluginInstance *inst, int &current);

    //--------------- Data members ---------------------------------

    AudioPluginManager  *m_pluginManager;
    Instrument          *m_instrument;

    QFrame		*m_pluginParamsBox;
    QWidget             *m_pluginCategoryBox;
    KComboBox           *m_pluginCategoryList;
    QLabel              *m_pluginLabel;
    KComboBox           *m_pluginList;
    std::vector<int>     m_pluginsInList;
    QLabel              *m_insOuts;
    QLabel              *m_pluginId;
    QCheckBox		*m_bypass;
    QPushButton         *m_copyButton;
    QPushButton         *m_pasteButton;
    QPushButton         *m_defaultButton;

    KComboBox           *m_programCombo;
    std::vector<PluginControl*> m_pluginWidgets;
    QGridLayout         *m_gridLayout;

    int                  m_index;

    bool                 m_generating;

    QAccel              *m_accelerators;

    void                 populatePluginCategoryList();
    void                 populatePluginList();
};


} // end of namespace


#endif // _AUDIOPLUGINDIALOG_H_
