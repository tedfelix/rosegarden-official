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

#ifndef _MIXER_H_
#define _MIXER_H_

#include <kmainwindow.h>
#include "studiowidgets.h"
#include "Instrument.h"

class RosegardenGUIDoc;
namespace Rosegarden { class Studio; }
class SequencerMapper;

class MixerWindow : public KMainWindow
{
    Q_OBJECT

public:
    MixerWindow(QWidget *parent, RosegardenGUIDoc *document);
    ~MixerWindow();

    void updateMeters(SequencerMapper *mapper);

signals:
    void closing();
    void selectPlugin(QWidget *, Rosegarden::InstrumentId id, int index);

protected slots:
    void slotRoutingButtonPressed();
    void slotRouteChanged(int);
    void slotFaderLevelChanged(float level);
    void slotPanChanged(float value);
    void slotSelectPlugin();
    
protected:
    virtual void closeEvent(QCloseEvent *);

private:
    RosegardenGUIDoc *m_document;
    Rosegarden::Studio *m_studio;

    // Not practical to use a single widget for this, as we want to
    // manage the various bits of it in horizontal/vertical slices
    // with other faders:

    struct FaderRec {

	FaderRec() :
	    m_input(0), m_output(0), m_pan(0), m_fader(0), m_meter(0),
	    m_muteButton(0), m_soloButton(0), m_recordButton(0),
	    m_stereoButton(0), m_pluginBox(0)
	{ }

	QPushButton *m_input;
	QPushButton *m_output;

	RosegardenRotary *m_pan;
	RosegardenFader *m_fader;
	AudioVUMeter *m_meter;

	QPushButton *m_muteButton;
	QPushButton *m_soloButton;
	QPushButton *m_recordButton;
	QPushButton *m_stereoButton;
	bool m_stereoness;

	QVBox *m_pluginBox;
	std::vector<QPushButton *> m_plugins;
    };

    typedef std::map<Rosegarden::InstrumentId, FaderRec> FaderMap;
    FaderMap m_faders;

    typedef std::vector<FaderRec> FaderVector;
    FaderVector m_submasters;
    FaderRec m_monitor;
    FaderRec m_master;

    void updateFader(int id); // instrument id if large enough, master/sub otherwise
    void updateRouteButtons(int id);

    QPixmap m_monoPixmap;
    QPixmap m_stereoPixmap;

    Rosegarden::InstrumentId m_currentId;
};

#endif
