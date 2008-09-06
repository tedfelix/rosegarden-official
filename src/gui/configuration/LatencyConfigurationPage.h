
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_LATENCYCONFIGURATIONPAGE_H_
#define _RG_LATENCYCONFIGURATIONPAGE_H_

#include "TabbedConfigurationPage.h"
#include <QString>
#include <klocale.h>
#include <QSlider>


class QWidget;
class QPushButton;
class KConfig;


namespace Rosegarden
{

class RosegardenGUIDoc;


/**
 * Latency Configuration page
 *
 * (application-wide settings)
 */
class LatencyConfigurationPage : public TabbedConfigurationPage
{
    Q_OBJECT

public:
    LatencyConfigurationPage(RosegardenGUIDoc *doc,
                             QSettings cfg,
                             QWidget *parent=0, const char *name=0);

    virtual void apply();

    static QString iconLabel() { return i18n("Latency"); }
    static QString title()     { return i18n("Sequencer Latency"); }

    int getJACKPlaybackValue() { return m_jackPlayback->value(); }
    int getJACKRecordValue() { return m_jackRecord->value(); }

protected slots:
    // Get the latest latency values from the sequencer
    //
    void slotFetchLatencyValues();

protected:

    //--------------- Data members ---------------------------------

    QSlider* m_jackPlayback;
    QSlider* m_jackRecord;

    QPushButton* m_fetchLatencyValues;
};



}

#endif
