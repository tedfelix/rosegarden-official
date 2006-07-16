// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2006
        Pedro Lopez-Cabanillas <plcl@users.sourceforge.net>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef TRACKPARAMETERBOX_H_
#define TRACKPARAMETERBOX_H_

#include "widgets.h"
#include "colours.h"

class RosegardenGUIDoc;
class Rosegarden::Track;
class KComboBox;
class QLabel;
class RosegardenColourTable;

class TrackParameterBox : public RosegardenParameterBox
{
Q_OBJECT
        
public:
    TrackParameterBox( RosegardenGUIDoc *doc,
                       QWidget *parent=0);
    ~TrackParameterBox();
    
    void setDocument( RosegardenGUIDoc *doc );
    void populateDeviceLists();

public slots:
    void slotSelectedTrackChanged();
    void slotSelectedTrackNameChanged();
    void slotPlaybackDeviceChanged(int index);
    void slotInstrumentChanged(int index);
    void slotUpdateControls(int);
    void slotInstrumentLabelChanged(Rosegarden::InstrumentId id, QString label);

    void slotClefChanged(int clef);
    void slotTransposeChanged(int transpose);
    void slotDocColoursChanged();
    void slotColorChanged(int index);

signals:
    void instrumentSelected(Rosegarden::TrackId, int);

protected:
    void populatePlaybackDeviceList();
    void populateRecordingDeviceList();

private:
    RosegardenGUIDoc    *m_doc;

    KComboBox           *m_playDevice;
    KComboBox           *m_instrument;
    KComboBox           *m_recDevice;
    KComboBox           *m_recChannel;

    // will launch a new dialog to load presets
    ///////////////////////////////////
    QPushButton		*m_presetButton;
    ///////////////////////////////////

    KComboBox		*m_defClef;

    //!!! using a spin box for this one to avoid the complications of a combo box; I guess the SPB
    // uses a combo box for visual uniformity, but forget that for now.
    QSpinBox		*m_defTranspose;

    KComboBox		*m_defColor;

    // need to work out widgets for the pitch pickers and stuff for
    // highest/lowest playable; store actual value as int MIDI pitch, but
    // display in user friendly way if practical, and pick pitches as on event
    // filter dialog with picker widget
    //
    QLabel   		*m_lowestPlayable;
    QLabel   		*m_highestPlayable;
    /////////////////////////////////////////////////////////////////

    int			m_addColourPos;
    RosegardenColourTable::ColourList  m_colourList;

    
    QLabel              *m_trackLabel;
    
    typedef std::vector<Rosegarden::DeviceId> IdsVector;
    
    IdsVector           m_playDeviceIds;    
    IdsVector           m_recDeviceIds;

    std::map<Rosegarden::DeviceId, IdsVector>   m_instrumentIds;
    std::map<Rosegarden::DeviceId, QStringList> m_instrumentNames;
    
    Rosegarden::TrackId m_selectedTrackId;
};

#endif /*TRACKPARAMETERBOX_H_*/
