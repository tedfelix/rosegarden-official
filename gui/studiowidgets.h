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

#ifndef _STUDIOWIDGETS_H_
#define _STUDIOWIDGETS_H_

#include <qwidget.h>
#include <qpushbutton.h>
#include <qpixmap.h>
#include <qsignalmapper.h>

#include "widgets.h"
#include "vumeter.h"

#include "AudioLevel.h"
#include "Instrument.h"

namespace Rosegarden {
    class Studio;
}

class QGridLayout;


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
};


// AudioVUMeter - a vertical audio meter.  Default is stereo.
//
class AudioVUMeter : public QWidget
{
public:
    AudioVUMeter(QWidget *parent = 0,
                 VUMeter::VUMeterType type = VUMeter::AudioPeakHoldShort,
                 bool stereo = true,
                 int width = 12,
                 int height = 140,
                 const char *name = 0);

    void setLevel(double dB) {
	m_meter->setLevel(dB);
    }
    void setLevel(double dBleft, double dBright) {
	m_meter->setLevel(dBleft, dBright);
    }

    virtual void paintEvent(QPaintEvent*);

protected:
    class AudioVUMeterImpl : public VUMeter
    {
    public:
	AudioVUMeterImpl(QWidget *parent,
			 VUMeterType type,
			 bool stereo,
			 int width,
			 int height,
			 const char *name);
    protected:
	virtual void meterStart() { }
	virtual void meterStop() { }
    };
	
    AudioVUMeterImpl *m_meter;
    bool m_stereo;
    int m_yoff;
    int m_xoff;
};


// A push button that emits wheel events.  Used by AudioRouteMenu.
//
class WheelyButton : public QPushButton
{
    Q_OBJECT

public:
    WheelyButton(QWidget *w) : QPushButton(w) { }
    virtual ~WheelyButton() { }

signals:
    void wheel(bool up);

protected:
    void wheelEvent(QWheelEvent *e) {
	emit wheel(e->delta() > 0);
    }
};



// A specialised menu for selecting audio inputs or outputs, that
// queries the studio and instrument to find out what it should show.
// Available in a "compact" size, which is a push button with a popup
// menu attached, or a regular size which is a combobox.
//
class AudioRouteMenu : public QObject
{
    Q_OBJECT

public:
    enum Direction { In, Out };
    enum Format { Compact, Regular };

    AudioRouteMenu(QWidget *parent,
		   Direction direction,
		   Format format,
		   Rosegarden::Studio *studio = 0,
		   Rosegarden::Instrument *instrument = 0);

    QWidget *getWidget();

public slots:
    void slotRepopulate();
    void slotSetInstrument(Rosegarden::Studio *, Rosegarden::Instrument *);
    
protected slots:
    void slotWheel(bool up);
    void slotShowMenu();
    void slotEntrySelected(int);

signals:
    // The menu writes changes directly to the instrument, but it
    // also emits this to let you know something has changed
    void changed();

private:
    Rosegarden::Studio *m_studio;
    Rosegarden::Instrument *m_instrument;
    Direction m_direction;
    Format m_format;

    WheelyButton *m_button;
    KComboBox *m_combo;

    int getNumEntries();
    int getCurrentEntry(); // for instrument
    QString getEntryText(int n);
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


class AudioFaderBox : public QFrame
{
    Q_OBJECT

public:
    AudioFaderBox(QWidget *parent,
		  QString id = "",
		  bool haveInOut = true,
		  const char *name = 0);

    void setAudioChannels(int);

    void setIsSynth(bool);

    bool owns(const QObject *object);

    QPushButton               *m_synthButton;
    std::vector<QPushButton*>  m_plugins;

    AudioVUMeter              *m_vuMeter;

    RosegardenFader           *m_fader;
    RosegardenFader           *m_recordFader;
    RosegardenRotary          *m_pan;

    QPixmap                    m_monoPixmap;
    QPixmap                    m_stereoPixmap;

    QSignalMapper             *m_signalMapper;

    QLabel                    *m_inputLabel;
    QLabel                    *m_outputLabel;

    AudioRouteMenu            *m_audioInput; 
    AudioRouteMenu            *m_audioOutput; 

    QString                    m_id;

    bool                       isStereo() const { return m_isStereo; }

signals:
    void audioChannelsChanged(int);

public slots:
    void slotSetInstrument(Rosegarden::Studio *studio,
			   Rosegarden::Instrument *instrument);

protected slots:
    void slotChannelStateChanged();

protected:
    QPushButton               *m_stereoButton;
    bool                       m_isStereo;
};


#endif // _STUDIOWIDGETS_H_
