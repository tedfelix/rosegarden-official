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


#ifndef _IMPORT_DEVICE_DIALOG_H_
#define _IMPORT_DEVICE_DIALOG_H_

#include <kdialogbase.h>
#include <kurl.h>

#include "MidiDevice.h"

class KComboBox;
class QButtonGroup;
class QLabel;
class QCheckBox;
class QRadioButton;
class RosegardenGUIDoc;

class ImportDeviceDialog : public KDialogBase
{
    Q_OBJECT

public:
    ImportDeviceDialog(QWidget *parent, KURL url);
    virtual ~ImportDeviceDialog();

    bool shouldImportBanks() const;
    bool shouldImportControllers() const;
    bool shouldOverwriteBanks() const; 
    bool shouldRename() const;

    std::string getDeviceName() const;
    const Rosegarden::BankList &getBanks() const;
    const Rosegarden::ProgramList &getPrograms() const;
    const Rosegarden::ControlList &getControllers() const;
    std::string getLibrarianName() const;
    std::string getLibrarianEmail() const;
    Rosegarden::MidiDevice::VariationType getVariationType() const;

public slots:
    void slotOk();
    void slotCancel();

protected:
    bool importFromRG(QString fileName);
    bool importFromSF2(QString fileName);

    KComboBox          *m_deviceCombo;
    QLabel             *m_deviceLabel;

    QCheckBox          *m_importBanks;
    QCheckBox          *m_importControllers;
    QCheckBox          *m_rename;

    QButtonGroup       *m_buttonGroup;
    QRadioButton       *m_mergeBanks;
    QRadioButton       *m_overwriteBanks;

    RosegardenGUIDoc   *m_fileDoc;
    std::vector<Rosegarden::MidiDevice *> m_devices;
    Rosegarden::MidiDevice *m_device;
};

#endif

