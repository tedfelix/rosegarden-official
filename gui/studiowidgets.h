// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
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

#ifndef _STUDIOWIDGETS_H_
#define _STUDIOWIDGETS_H_

#include <qwidget.h>
#include <qpushbutton.h>
#include <qpixmap.h>
#include <qsignalmapper.h>

#include "widgets.h"
#include "trackvumeter.h"

/*
class AudioVUMeter : public VUMeter 
{
public:
    AudioVUMeter(QWidget *parent);

    virtual void meterStart();
    virtual void meterStop();

protected:

};
*/

// We need one of these because the QSlider is stupid and won't
// let us have the maximum value of the slider at the top.  Or
// just I can't find a way of doing it.  Anyway, this is a 
// vertically aligned volume/MIDI fader.
//
class RosegardenFader : public QSlider
{
    Q_OBJECT
public:
    RosegardenFader(QWidget *parent);

public slots:
    void slotValueChanged(int);

    // Use this in preference to setValue - horrible hack but it's
    // quicker than fiddling about with the insides of QSlider.
    //
    virtual void setFader(int);

    int faderLevel() const { return maxValue() - value(); }

    void slotFloatTimeout();

    // Prependable text for tooltip
    //
    void setPrependText(const QString &text) { m_prependText = text; }

signals:
    void faderChanged(int);

protected slots:
    void slotShowFloatText();

protected:

    RosegardenTextFloat *m_float;
    QTimer              *m_floatTimer;

    QString              m_prependText;
};


class AudioFaderWidget : public QWidget
{
    Q_OBJECT

public:
    AudioFaderWidget(QWidget *parent,
                     const char *name=0,
                     bool vertical=true);

    void setAudioChannels(int);

    std::vector<QPushButton*>  m_plugins;
    AudioVUMeter              *m_vuMeter;
    RosegardenFader           *m_fader;
    QPushButton               *m_muteButton;
    QPushButton               *m_soloButton;
    QPushButton               *m_recordButton;
    RosegardenRotary          *m_pan;

    QPixmap                    m_monoPixmap;
    QPixmap                    m_stereoPixmap;

    QSignalMapper             *m_signalMapper;

    KComboBox                 *m_audioInput; 
    //KComboBox                 *m_audioOutput; 

signals:
    void audioChannelsChanged(int);

public slots:

protected slots:
    void slotChannelStateChanged();

protected:
    QPushButton               *m_stereoButton;
    bool                       m_isStereo;



};


#endif // _STUDIOWIDGETS_H_
