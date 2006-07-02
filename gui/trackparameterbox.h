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

class RosegardenGUIDoc;
class KComboBox;
class QLabel;

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
    QLabel              *m_trackLabel;
    
    typedef std::vector<Rosegarden::DeviceId> IdsVector;
    
    IdsVector           m_playDeviceIds;    
    IdsVector           m_recDeviceIds;

    std::map<Rosegarden::DeviceId, IdsVector>   m_instrumentIds;
    std::map<Rosegarden::DeviceId, QStringList> m_instrumentNames;
    
    Rosegarden::TrackId m_selectedTrackId;
};

#endif /*TRACKPARAMETERBOX_H_*/
