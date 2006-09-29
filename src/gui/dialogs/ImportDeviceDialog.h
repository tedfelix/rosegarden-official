
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_IMPORTDEVICEDIALOG_H_
#define _RG_IMPORTDEVICEDIALOG_H_

#include "base/MidiDevice.h"
#include <string>
#include <kdialogbase.h>
#include <qstring.h>
#include <vector>
#include <kurl.h>


class QWidget;
class QRadioButton;
class QLabel;
class QCheckBox;
class QButtonGroup;
class ProgramList;
class KeyMappingList;
class KComboBox;
class ControlList;
class BankList;


namespace Rosegarden
{

class RosegardenGUIDoc;


class ImportDeviceDialog : public KDialogBase
{
    Q_OBJECT

public:
    ImportDeviceDialog(QWidget *parent, KURL url);
    virtual ~ImportDeviceDialog();

    bool doImport();

    bool shouldImportBanks() const;
    bool shouldImportKeyMappings() const;
    bool shouldImportControllers() const;
    bool shouldOverwriteBanks() const; 
    bool shouldRename() const;

    std::string getDeviceName() const;
    const BankList &getBanks() const;
    const ProgramList &getPrograms() const;
    const KeyMappingList &getKeyMappings() const;
    const ControlList &getControllers() const;
    std::string getLibrarianName() const;
    std::string getLibrarianEmail() const;
    MidiDevice::VariationType getVariationType() const;

public slots:
    void slotOk();
    void slotCancel();

protected:
    bool importFromRG(QString fileName);
    bool importFromSF2(QString fileName);

    KURL               m_url;

    KComboBox          *m_deviceCombo;
    QLabel             *m_deviceLabel;

    QCheckBox          *m_importBanks;
    QCheckBox          *m_importKeyMappings;
    QCheckBox          *m_importControllers;
    QCheckBox          *m_rename;

    QButtonGroup       *m_buttonGroup;
    QRadioButton       *m_mergeBanks;
    QRadioButton       *m_overwriteBanks;

    RosegardenGUIDoc   *m_fileDoc;
    std::vector<MidiDevice *> m_devices;
    MidiDevice *m_device;
};


}

#endif
