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

#include "AudioLevel.h"


class RosegardenFader : public QWidget
{
    Q_OBJECT

public:
    /**
     * Construct a dB fader.  The fader calculates its orientation
     * based on the given dimensions.
     */
    RosegardenFader(Rosegarden::AudioLevel::FaderType,
		    int width, int height, QWidget *parent);

    /**
     * Construct a fader on an integral scale.  The fader calculates
     * its orientation based on the given dimensions.
     */
    RosegardenFader(int min, int max, int deflt,
		    int width, int height, QWidget *parent);

    /**
     * Construct a fader on an integral scale, with a 1:1 ratio of
     * pixel positions and values.
     */
    RosegardenFader(int min, int max, int deflt,
		    bool vertical, QWidget *parent);

    virtual ~RosegardenFader();

    float getFaderLevel() const;

public slots:
    void setFader(float value);
    void slotFloatTimeout();

signals:
    void faderChanged(float);

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void wheelEvent(QWheelEvent *);

    float position_to_value(int);
    int value_to_position(float);

    void calculateGroovePixmap();
    void calculateButtonPixmap();
    void showFloatText();

    bool m_integral;
    bool m_vertical;

    int m_sliderMin;
    int m_sliderMax;
    float m_value;

    int m_min;
    int m_max;
    Rosegarden::AudioLevel::FaderType m_type;

    int m_clickMousePos;
    int m_clickButtonPos;

    RosegardenTextFloat *m_float;
    QTimer              *m_floatTimer;

    QPixmap *groovePixmap();
    QPixmap *buttonPixmap();

    typedef std::pair<int, int> SizeRec;
    typedef std::pair<QPixmap *, QPixmap *> PixmapRec;
    typedef std::map<SizeRec, PixmapRec> PixmapCache;
    static PixmapCache m_pixmapCache;

    //!!! we should really get all sophisticated and cache these
    //shared among faders of a given dimension
//    QPixmap *m_groovePixmap;
//    QPixmap *m_buttonPixmap;
};
    

class MidiFaderWidget : public QFrame
{
    Q_OBJECT

public:
    MidiFaderWidget(QWidget *parent,
		    QString id = "");
    
    AudioVUMeter              *m_vuMeter;

    RosegardenFader           *m_fader;

    QPushButton               *m_muteButton;
    QPushButton               *m_soloButton;
    QPushButton               *m_recordButton;
    RosegardenRotary          *m_pan;

    KComboBox                 *m_output; 

    QString                    m_id;
};


class AudioFaderWidget : public QFrame
{
    Q_OBJECT

public:
    enum LayoutType {
	FaderStrip,
	FaderBox
    };
    
    AudioFaderWidget(QWidget *parent,
                     LayoutType type,
		     QString id = "",
		     bool haveInOut = true,
                     const char *name = 0);

    void setAudioChannels(int);

    std::vector<QPushButton*>  m_plugins;

    AudioVUMeter              *m_vuMeter;

    RosegardenFader           *m_fader;
    RosegardenFader           *m_recordFader;

    QPushButton               *m_muteButton;
    QPushButton               *m_soloButton;
    QPushButton               *m_recordButton;
    RosegardenRotary          *m_pan;

    QPixmap                    m_monoPixmap;
    QPixmap                    m_stereoPixmap;

    QSignalMapper             *m_signalMapper;

    KComboBox                 *m_audioInput; 
    KComboBox                 *m_audioOutput; 

    QString                    m_id;

    bool                       isStereo() const { return m_isStereo; }

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
