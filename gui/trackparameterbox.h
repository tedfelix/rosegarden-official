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
#include "Instrument.h"

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
    virtual void showAdditionalControls(bool showThem);

public slots:
    void slotSelectedTrackChanged();
    void slotSelectedTrackNameChanged();
    void slotPlaybackDeviceChanged(int index);
    void slotInstrumentChanged(int index);
    void slotRecordingDeviceChanged(int index);
    void slotRecordingChannelChanged(int index);
    void slotUpdateControls(int);
    void slotInstrumentLabelChanged(Rosegarden::InstrumentId id, QString label);

    void slotClefChanged(int clef);
    void slotTransposeChanged(int transpose);
    void slotDocColoursChanged();
    void slotColorChanged(int index);
    void slotHighestPressed();
    void slotLowestPressed();
    void slotPresetPressed();

signals:
    void instrumentSelected(Rosegarden::TrackId, int);

protected:
    void populatePlaybackDeviceList();
    void populateRecordingDeviceList();
    void updateHighLow();

private:
    RosegardenGUIDoc    *m_doc;

    KComboBox           *m_playDevice;
    KComboBox           *m_instrument;
    KComboBox           *m_recDevice;
    KComboBox           *m_recChannel;

    QPushButton		*m_presetButton;
    QPushButton		*m_highButton;
    QPushButton		*m_lowButton;

    KComboBox		*m_defClef;
    KComboBox		*m_defColor;

    //!!! using a spin box for this one to avoid the complications of a combo box; I guess the SPB
    // uses a combo box for visual uniformity, but forget that for now.
    QSpinBox		*m_defTranspose;

    int			m_addColourPos;
    int			m_highestPlayable;
    int 		m_lowestPlayable;
    RosegardenColourTable::ColourList  m_colourList;
    
    QLabel              *m_trackLabel;
    
    typedef std::vector<Rosegarden::DeviceId> IdsVector;
    
    IdsVector           m_playDeviceIds;    
    IdsVector           m_recDeviceIds;

    std::map<Rosegarden::DeviceId, IdsVector>   m_instrumentIds;
    std::map<Rosegarden::DeviceId, QStringList> m_instrumentNames;
    
    Rosegarden::TrackId m_selectedTrackId;
    
    char                m_lastInstrumentType;
    
    // Additional elements that may be hidden in vertical stacked mode
    QFrame              *m_separator2;
    QLabel              *m_segHeader;
    QLabel              *m_presetLbl;
    QLabel              *m_clefLbl;
    QLabel              *m_transpLbl;
    QLabel              *m_colorLbl;
    QLabel		*m_rangeLbl;
    QLabel              *m_psetLbl;
};

#endif /*TRACKPARAMETERBOX_H_*/
