/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2022 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_AUDIOCONFIGURATIONPAGE_H
#define RG_AUDIOCONFIGURATIONPAGE_H

#include "TabbedConfigurationPage.h"
#include "gui/widgets/LineEdit.h"

#include <QString>
#include <QSettings>


class QWidget;
class QSpinBox;
class QSlider;
class QPushButton;
class QLabel;
class QComboBox;
class QCheckBox;
class QComboBox;
class LineEdit;


namespace Rosegarden
{

class RosegardenDocument;


class AudioConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT
public:
    AudioConfigurationPage(RosegardenDocument *doc,
                               QWidget *parent = nullptr);

    void apply() override;

    static QString iconLabel() { return tr("Audio"); }
    static QString title()     { return tr("Audio Settings"); }
    static QString iconName()  { return "configure-audio"; }

#ifdef HAVE_LIBJACK
    //QString getJackPath() { return m_jackPath->text(); }
#endif // HAVE_LIBJACK

    static QString getBestAvailableAudioEditor();

private slots:
    void slotFileDialog();

private:
    QString getExternalAudioEditor() { return m_externalAudioEditorPath->text(); }

    // Widgets...

    QComboBox *m_previewStyle;

#ifdef HAVE_LIBJACK
    QComboBox *m_audioRecFormat;
#endif

    QCheckBox *m_showAudioLocation;
    QComboBox *m_defaultAudioLocation;
    LineEdit *m_customAudioLocation;

    LineEdit *m_externalAudioEditorPath;

#ifdef HAVE_LIBJACK
    // Number of JACK input ports our RG client creates - 
    // this decides how many audio input destinations
    // we have.
    QCheckBox *m_createFaderOuts;
    QCheckBox *m_createSubmasterOuts;

    QCheckBox *m_connectDefaultAudioOutputs;
    QCheckBox *m_connectDefaultAudioInputs;

    QCheckBox *m_autoStartJackServer;
    QCheckBox *m_outOfProcessorPower;

    //QCheckBox *m_startJack;
    //LineEdit  *m_jackPath;
#endif // HAVE_LIBJACK


};
 


}

#endif
