/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include "PitchGraphWidget.h"
#include "misc/ConfigGroups.h"
#include "gui/configuration/PitchTrackerConfigurationPage.h"
#include "sound/PitchDetector.h"

// Access to the currently used tuning class
#include "sound/Tuning.h"

#include <QVBoxLayout>
#include <QPainter>
#include <QLabel>
#include <QSettings>
#include <QPoint>

// maybe move this out of this file?
#include <math.h>

//using namespace Rosegarden;
namespace Rosegarden {

PitchGraphWidget::PitchGraphWidget(PitchHistory &history, QWidget *parent) :
        QWidget(parent),
        m_tuning(NULL),
	m_history(history)
{
    setMinimumHeight(100);
    setMinimumWidth(100);

    // Read this widget's current settings
    QSettings settings;
    settings.beginGroup(PitchTrackerConfigGroup);
    m_graphHeight =
        settings.value("graphheight",
                       PitchTrackerConfigurationPage::defaultGraphHeight).
                       toInt();
    m_graphWidth = 
        settings.value("graphwidth",
                       PitchTrackerConfigurationPage::defaultGraphWidth).
                       toInt();
                       
    m_ignoreOctave =
        settings.value("ignoreoctave",
                       PitchTrackerConfigurationPage::defaultIgnore8ve).
                       toBool();

    settings.endGroup();
    qDebug("******************** end pitchgraphwidget ctor");
}

PitchGraphWidget::~PitchGraphWidget()
{
}

void
PitchGraphWidget::setTuning(Accidentals::Tuning* tuning)
{
    m_tuning = tuning;
    // If the tuning gets changed via a settings update,
    // we'll need to redraw the graph too.
}

void
PitchGraphWidget::paintEvent(QPaintEvent */* event */)
{
    const QColor defaultColor(Qt::white); // for axes, text etc.
    const QColor noNoteColor(Qt::gray);
    const QColor noInputColor(Qt::red);
    const QColor normalPlotColor(Qt::green);
    const QColor noteBoundaryColor(Qt::blue);
    
    QPainter painter(this);
    QString labelStg;
    QColor labelColor;
    
    double targetFreq = 0;
    bool targetValid = false;
    double freq_err_cents = 0;
    double freq_actual = 0;
    if (m_history.m_targetFreqs.size() > 0) {
        targetFreq = m_history.m_targetFreqs.last();
    }
    if (!m_history.m_detectErrorsCents.isEmpty()) {
        targetValid = m_history.m_detectErrorsValid.last();
        freq_err_cents = m_history.m_detectErrorsCents.last();
        freq_actual = m_history.m_detectFreqs.last();
    }

    // Draw the target frequency in green if it's defined,
    // or grey if we're playing a rest right now.
    if (freq_actual != PitchDetector::NONE) {
        labelStg = QString::number(targetFreq, 'f', 3);
        labelColor = normalPlotColor;
    } else {
        labelStg =  tr("None (Rest)",
                       "No target frequency because no note is playing");
        labelColor = noNoteColor;
    }
    painter.setPen(defaultColor);
    painter.drawText(0, 20, "Target freq:");
    painter.setPen(labelColor);
    painter.drawText(100, 20, labelStg);
    
    // Same for the actual frequency, but must take the "error" conditions
    // into account. If actual freq is -ve, could be one of NOSIGNAL
    // or NONE (playing a rest) as defined in PitchDetector.
    if (targetValid) {
        labelStg = QString::number(freq_actual, 'f', 3);
        labelColor = normalPlotColor;
    } else {
        labelStg = tr("Undefined");
        if (freq_actual == PitchDetector::NONE) {
            labelColor = noNoteColor;
        }
        else if (freq_actual == PitchDetector::NOSIGNAL) {
            labelColor = noInputColor;
        }
    }
    painter.setPen(defaultColor);
    painter.drawText(0, 40, tr("Tuning System:"));
    painter.drawText(0, 70, tr("Actual freq:"));
    painter.drawText(0, 90, tr("Error (cents):"));
    painter.setPen(labelColor);
    // If a valid tuning's defined, use its name. Otherwise behave well.
    QString tuningName = m_tuning ? QString(m_tuning->getName().c_str()) :
                                    tr("None available (check preferences)");    
    painter.drawText(100, 40, tuningName);
    painter.drawText(100, 70, labelStg);
    if (targetValid) {
        if (m_ignoreOctave) {
            int octaves(static_cast<int>(freq_err_cents)/1200);
            labelStg = QString::number(fmod(freq_err_cents, 1200.0), 'f', 3)
                       + QString(" ");
            if (octaves >= 0) labelStg += QString("+");
            labelStg += QString::number(octaves) + QString("oct");
        } else {
            labelStg = QString::number(freq_err_cents, 'f', 3);
        }
    }
    painter.drawText(100, 90, labelStg);
    painter.setPen(defaultColor);
    
    RealTime nextBoundary = RealTime::zeroTime;
    QList<RealTime> note_stack = m_history.m_targetChangeTimes;
    if (!note_stack.isEmpty()) {
        // We're going to draw backwards from now, so the first
        // boundary we're going to consider is the most recent one.
        nextBoundary = note_stack.takeLast();
    }

    const int midY = height() / 2;
    // central axis
    painter.drawLine(0, midY, width(), midY);

    const RealTime lastPointTime = m_history.m_detectRealTimes.isEmpty() ?
                                   RealTime::zeroTime :
                                   m_history.m_detectRealTimes.last();
    // Record the state for drawing. If the ErrorsValid QVector is empty,
    // or the first pitch error is invalid (e.g. there was no signal),
    // start off in the first_err state. Otherwise begin in the first state.
    enum {first, subsequent, first_err, subseq_err} drawState =
        (m_history.m_detectErrorsValid.isEmpty() || 
	 !m_history.m_detectErrorsValid[0]) ?
          first_err : first;
    
    QPoint firstPoint;
    // Start plotting at the most recent point and work backwards
    bool lastPoint =  false;
    for (int i=m_history.m_detectErrorsCents.size() - 1;
         i >= 0 && !lastPoint;
         --i) {
        const RealTime pointTime = m_history.m_detectRealTimes[i];
        // Work out how far (in milliseconds) to the right of the graph
        const double timeToLastPoint_msec =
            1000.0 * ((lastPointTime-pointTime)/RealTime(1, 0));
        const int x = width() * (1.0 -  timeToLastPoint_msec/m_graphWidth);
        // if this is past the left end of the graph, it's the last
        // line segment we'll bother with
        lastPoint = (timeToLastPoint_msec > m_graphWidth);
        // draw note boundaries
        painter.setPen(noteBoundaryColor);
        // nextBoundary < zeroTime implies all boundarys have been delt with
        if (nextBoundary >= RealTime::zeroTime && pointTime <= nextBoundary) {
            painter.drawLine(x, 0, x, height());
            if (!note_stack.isEmpty()) {
                nextBoundary = note_stack.takeLast();
            } else {
                nextBoundary = RealTime(-1, 0);
            }
        }


        // computer graphics: lower numbers -> higher on screen
        // Trace should move in range +/- graphHeight
        double cents_err = m_history.m_detectErrorsCents[i];
        if (m_ignoreOctave) cents_err = fmod(cents_err, 1200.0);
        const int y = -height() * (0.5*cents_err/m_graphHeight);
        const bool valid = m_history.m_detectErrorsValid[i];
        QPoint here;
        // draw pitches
        switch (drawState) {
        // start of a valid line
        case first:  
            firstPoint = QPoint(x, midY+y);
            drawState = valid ? subsequent : first_err;
            break;
        // continuation of a valid line
        case subsequent:
            here = QPoint(x, midY+y);
            if (valid) {
                painter.setPen(normalPlotColor);
                painter.drawLine(firstPoint, here);
            } else {
                drawState = first_err;
            }
            firstPoint = here;
            break;
        // start of an invalid region
        case first_err:
            firstPoint = QPoint(x, midY);
            drawState = valid ? first : subseq_err;
            break;
        // continuation of an invalid region
        case subseq_err:
            here = QPoint(x, midY);
            if (!valid) {
                if (m_history.m_detectErrorsCents[i]
		        == PitchDetector::NOSIGNAL) {
                    painter.setPen(noInputColor);
                } else {
                    painter.setPen(noNoteColor);
                }
                painter.drawLine(firstPoint, here);
            } else {
                drawState = first;
            }
            firstPoint = here;
            break;
        }
    }
}

}

