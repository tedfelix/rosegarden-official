/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_IMPORTDEVICEDIALOG_H
#define RG_IMPORTDEVICEDIALOG_H

#include "base/MidiDevice.h"
#include <string>
#include <QDialog>
#include <QString>
#include <vector>
#include <QUrl>


class QWidget;
class QRadioButton;
class QLabel;
class QCheckBox;
class QButtonGroup;
class ProgramList;
class KeyMappingList;
class QComboBox;
class ControlList;
class BankList;


namespace Rosegarden
{

class RosegardenDocument;


class ImportDeviceDialog : public QDialog
{
    Q_OBJECT

public:
    ImportDeviceDialog(QWidget *parent, QUrl url);
    virtual ~ImportDeviceDialog();

    bool doImport();

    bool shouldImportBanks() const;
    bool shouldImportKeyMappings() const;
    bool shouldImportControllers() const;
    bool shouldOverwriteBanks() const; 
    bool shouldRename() const;

    bool haveDevice() const;
    std::string getDeviceName() const;
    const BankList &getBanks() const;
    const ProgramList &getPrograms() const;
    const KeyMappingList &getKeyMappings() const;
    const ControlList &getControllers() const;
    std::string getLibrarianName() const;
    std::string getLibrarianEmail() const;
    MidiDevice::VariationType getVariationType() const;

public slots:
    void accept();
    void slotCancel();

protected:
    bool importFromRG(QString fileName);
    bool importFromSF2(QString fileName);
    bool importFromLSCP(QString filename);

    QUrl               m_url;

    QComboBox          *m_deviceCombo;
    QLabel             *m_deviceLabel;

    QCheckBox          *m_importBanks;
    QCheckBox          *m_importKeyMappings;
    QCheckBox          *m_importControllers;
    QCheckBox          *m_rename;

    QButtonGroup       *m_buttonGroup;
    QRadioButton       *m_mergeBanks;
    QRadioButton       *m_overwriteBanks;

    std::vector<MidiDevice *> m_devices;
    MidiDevice *m_device;
};


}

#endif
