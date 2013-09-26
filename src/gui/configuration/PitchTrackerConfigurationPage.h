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

#ifndef RG_PITCHTRACKERCONFIGURATIONPAGE_H
#define RG_PITCHTRACKERCONFIGURATIONPAGE_H

#include <string>
#include <vector>
#include "TabbedConfigurationPage.h"
#include <QString>
#include <QStringList>
#include <QErrorMessage>

#include "sound/Tuning.h"

class QWidget;
class QPushButton;
class QLabel;
class QComboBox;
class QCheckBox;
class QSpinBox;

namespace Rosegarden
{

/**
 * \addtogroup Codicil
 * \@{
 * \brief Pitch Tracker Configuration dialogue
 */
class PitchTrackerConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
    static const int defaultGraphWidth;     /**< Width of graph in ms */
    static const int defaultGraphHeight;    /**< Max excursion in cents */
    static const bool defaultIgnore8ve;     /**< Ignore octave errors */

    PitchTrackerConfigurationPage(QWidget *parent = 0);

    virtual void apply();

    static QString iconLabel() {
        return tr("Pitch Tracker");
    }
    static QString title()     {
        return tr("Pitch Tracker");
    }
    static QString iconName()  {
        return "configure-pitchtracker";
    }

protected slots:
    void slotPopulateTuningCombo(bool rescan);

protected:
    //--------------- Data members ---------------------------------

    QComboBox    *m_tuningMode;      /**< Index of current tuning mode */
    unsigned int  m_rootPitch;       /**< 12ET pitch to anchor tuning */
    unsigned int  m_refPitch;        /**< pitch equivalent to anchor */
    double        m_refFreq;         /**< Frequency in Hz of m_refPitch */
    QLabel       *m_rootPitchLabel;
    QLabel       *m_refPitchLabel;
    QLabel       *m_refFreqLabel;
    QComboBox    *m_method;          /**< Which pitch detector to use */
    QSpinBox     *m_frameSize;       /**< Size of analysis frame */
    QSpinBox     *m_stepSize;        /**< Increment between frames */
    QCheckBox    *m_ignore8ve;       /**< Ignore 8ve in pitch comparison? */
    QSpinBox     *m_graphWidth;      /**< Width of graph in milliseconds */
    QSpinBox     *m_graphHeight;     /**< Height of graph in cents */
    std::vector<Accidentals::Tuning *> *m_tunings; /**< Available tunings */
    
    QErrorMessage m_noTuningsAlert;  /**< Alert missing tunings file */
    
};


}

/**\@}*/
#endif
