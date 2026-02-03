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

#ifndef RG_VUMETER_H
#define RG_VUMETER_H

#include <QColor>
#include <QLabel>


class QWidget;
class QTimer;
class QElapsedTimer;
class QPaintEvent;
class QPainter;


namespace Rosegarden
{


class VelocityColour;


/// Base class for the various VU meter classes.
/**
 * This derives from QLabel because it can become a label when it isn't
 * displaying a level.  This is used by TrackVUMeter to display the track
 * number.
 */
class VUMeter : public QLabel
{
    Q_OBJECT

public:

    typedef enum
    {
        //Plain,  // ??? unused
        PeakHold,  // TrackButtons
        AudioPeakHoldShort,  // AudioVUMeter
        AudioPeakHoldLong,  // ??? unused
        AudioPeakHoldIEC,  // ??? unused
        AudioPeakHoldIECLong,  // AudioStrip
        FixedHeightVisiblePeakHold  // MidiMixerWindow
    } VUMeterType;

    /// For audio meters, level is in dB.  For others it is normalized 0-1.
    void setLevel(double level);
    /// For audio meters, level is in dB.  For others it is normalized 0-1.
    void setLevel(double leftLevel, double rightLevel);

    /// For audio meters, level is in dB.  For others it is normalized 0-1.
    void setRecordLevel(double level);
    /// For audio meters, level is in dB.  For others it is normalized 0-1.
    void setRecordLevel(double leftLevel, double rightLevel);

    void paintEvent(QPaintEvent *) override;

protected:

    typedef enum
    {
        Horizontal,  // TrackVUMeter
        Vertical  // MidiMixerVUMeter, AudioVUMeter
    } VUAlignment;

    VUMeter(QWidget *parent,
            VUMeterType type,
            bool stereo,
            bool hasRecord,
            int width,
            int height,
            VUAlignment alignment);
    ~VUMeter() override;

    /// Used by TrackVUMeter to hide the text and show the meter.
    virtual void meterStart()  { }
    /// Used by TrackVUMeter to show the text and hide the meter.
    virtual void meterStop()  { }
    /// Used by TrackVUMeter to turn the meter on and off.
    bool m_active{true};

    /// Height passed in via ctor.
    const int m_originalHeight;

private slots:

    /// Connected to m_decayTimerLeft.
    void slotDecayLeft();
    /// Connected to m_peakTimerLeft.
    void slotStopShowingPeakLeft();

    /// Connected to m_decayTimerRight.
    void slotDecayRight();
    /// Connected to m_peakTimerRight.
    void slotStopShowingPeakRight();

private:

    // Hide copy ctor and op= due to non-trivial dtor.
    VUMeter(const VUMeter &) = delete;
    VUMeter &operator=(const VUMeter &) = delete;

    const VUMeterType m_type;
    const VUAlignment m_alignment;
    const bool m_stereo;
    const bool m_hasRecord;

    /// Meter background color.
    QColor m_background;

    // Note: All numeric variables use "pixels" for their units unless
    //       otherwise specified.

    // The size of the meter in pixels.
    int m_maxLevelPixels;
    /// pixels per second, 1 pixel per 50msecs
    static constexpr double m_decayRate{1/.05};

    bool m_showPeakLevel;

    // LEFT
    double m_levelLeft{0};
    double m_recordLevelLeft{0};
    QTimer *m_decayTimerLeft{nullptr};
    QElapsedTimer *m_timeDecayLeft{nullptr};
    int m_peakLevelLeft{0};
    QTimer *m_peakTimerLeft{nullptr};

    // RIGHT
    double m_levelRight{0};
    double m_recordLevelRight{0};
    QTimer *m_decayTimerRight{nullptr};
    QElapsedTimer *m_timeDecayRight{nullptr};
    int m_peakLevelRight{0};
    QTimer *m_peakTimerRight{nullptr};

    void setLevel(double leftLeveldB, double rightLeveldB, bool record);

    /// Converts level to color.
    VelocityColour *m_velocityColour;


    /// Called by drawMeterLevel() to do the actual drawing.
    void drawColouredBar(QPainter *paint, int channel,
                         int x, int y, int w, int h);
    /// Called by paintEvent() to draw the meter.
    void drawMeterLevel(QPainter *paint);

};


}

#endif
