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

#include <vector>

#include <klocale.h>

#include "basiccommand.h"

#include "Instrument.h"
#include "Device.h"
#include "MidiDevice.h"
#include "ControlParameter.h"

class RosegardenGUIDoc;
namespace Rosegarden { class Studio; }

// Use the overwrite flag to overwrite banks and programs rather
// than merging (default behaviour).
//
class ModifyDeviceCommand : public KNamedCommand
{
public:
    // Any of the arguments passed by pointer may be null (except for
    // the Studio) -- in which case they will not be changed in the device.
    ModifyDeviceCommand(Rosegarden::Studio *studio,
                        Rosegarden::DeviceId device,
                        const std::string &name,
                        const std::string &librarianName,
                        const std::string &librarianEmail,
			Rosegarden::MidiDevice::VariationType *variationType,
                        const Rosegarden::BankList *bankList,
                        const Rosegarden::ProgramList *programList,
			const Rosegarden::ControlList *controlList,
                        bool overwrite,
			bool rename);

    static QString getGlobalName() { return i18n("Modify &MIDI Bank"); }

    virtual void execute();
    virtual void unexecute();

protected:

    Rosegarden::Studio                    *m_studio;
    int                                    m_device;
    std::string                            m_name;
    std::string                            m_librarianName;
    std::string                            m_librarianEmail;
    Rosegarden::MidiDevice::VariationType  m_variationType;
    Rosegarden::BankList                   m_bankList;
    Rosegarden::ProgramList                m_programList;
    Rosegarden::ControlList                m_controlList;

    std::string                            m_oldName;
    std::string                            m_oldLibrarianName;
    std::string                            m_oldLibrarianEmail;
    Rosegarden::MidiDevice::VariationType  m_oldVariationType;
    Rosegarden::BankList                   m_oldBankList;
    Rosegarden::ProgramList                m_oldProgramList;
    Rosegarden::ControlList                m_oldControlList;

    bool                                   m_overwrite;
    bool                                   m_rename;
    bool                                   m_changeVariation;
    bool                                   m_changeBanks;
    bool                                   m_changePrograms;
    bool                                   m_changeControls;
};

class ModifyDeviceMappingCommand : public KNamedCommand
{
public:
    ModifyDeviceMappingCommand(RosegardenGUIDoc *doc,
                               Rosegarden::DeviceId fromDevice,
                               Rosegarden::DeviceId toDevice);

    static QString getGlobalName() { return i18n("Modify &Device Mapping"); }

    virtual void execute();
    virtual void unexecute();
protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::Studio      *m_studio;
    Rosegarden::DeviceId     m_fromDevice;
    Rosegarden::DeviceId     m_toDevice;

    std::vector<std::pair<Rosegarden::TrackId, Rosegarden::InstrumentId> >
                             m_mapping;
};

class ModifyInstrumentMappingCommand : public KNamedCommand
{
public:
    ModifyInstrumentMappingCommand(RosegardenGUIDoc *doc,
                                   Rosegarden::InstrumentId fromInstrument,
                                   Rosegarden::InstrumentId toInstrument);

    static QString getGlobalName() { return i18n("Modify &Instrument Mapping"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Composition *m_composition;
    Rosegarden::Studio      *m_studio;
    Rosegarden::InstrumentId m_fromInstrument;
    Rosegarden::InstrumentId m_toInstrument;

    std::vector<Rosegarden::TrackId> m_mapping;

};


// because ModifyDeviceCommand is overkill for this

class RenameDeviceCommand : public KNamedCommand
{
public:
    RenameDeviceCommand(Rosegarden::Studio *studio,
			Rosegarden::DeviceId deviceId,
			std::string name) :
	KNamedCommand(getGlobalName()),
	m_studio(studio),
	m_deviceId(deviceId),
	m_name(name) { }

    static QString getGlobalName() { return i18n("Rename Device"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Studio *m_studio;
    Rosegarden::DeviceId m_deviceId;
    std::string m_name;
    std::string m_oldName;
};


class CreateOrDeleteDeviceCommand : public KNamedCommand
{
public:
    // Creation constructor
    CreateOrDeleteDeviceCommand(Rosegarden::Studio *studio,
				std::string name,
				Rosegarden::Device::DeviceType type,
				Rosegarden::MidiDevice::DeviceDirection direction,
				std::string connection) :
	KNamedCommand(getGlobalName(false)),
	m_studio(studio),
	m_name(name),
	m_type(type),
	m_direction(direction),
	m_connection(connection),
	m_deviceId(Rosegarden::Device::NO_DEVICE),
	m_deviceCreated(false) { }

    // Deletion constructor
    CreateOrDeleteDeviceCommand(Rosegarden::Studio *studio,
				Rosegarden::DeviceId deviceId);
    
    static QString getGlobalName(bool deletion) {
	return (deletion ? i18n("Delete Device") : i18n("Create Device")); 
    }
    
    virtual void execute();
    virtual void unexecute() { execute(); }
    
protected:
    Rosegarden::Studio *m_studio;
    std::string m_name;
    Rosegarden::Device::DeviceType m_type;
    Rosegarden::MidiDevice::DeviceDirection m_direction;
    std::string m_connection;
    Rosegarden::DeviceId m_deviceId;
    bool m_deviceCreated;
};


class ReconnectDeviceCommand : public KNamedCommand
{
public:
    ReconnectDeviceCommand(Rosegarden::Studio *studio,
			   Rosegarden::DeviceId deviceId,
			   std::string newConnection) :
	KNamedCommand(getGlobalName()),
	m_studio(studio),
	m_deviceId(deviceId),
	m_newConnection(newConnection) { }

    static QString getGlobalName() { return i18n("Reconnect Device"); }

    virtual void execute();
    virtual void unexecute();

protected:
    Rosegarden::Studio *m_studio;
    Rosegarden::DeviceId m_deviceId;
    std::string m_newConnection;
    std::string m_oldConnection;
};

class AddControlParameterCommand : public KNamedCommand
{
public:
    AddControlParameterCommand(Rosegarden::Studio *studio,
			       Rosegarden::DeviceId device,
                               Rosegarden::ControlParameter control):
        KNamedCommand(getGlobalName()),
        m_studio(studio),
	m_device(device),
        m_control(control),
        m_id(0) { }

    ~AddControlParameterCommand();

    virtual void execute();
    virtual void unexecute();

    static QString getGlobalName() { return i18n("&Add Control Parameter"); }

protected:
    Rosegarden::Studio              *m_studio;
    Rosegarden::DeviceId             m_device;
    Rosegarden::ControlParameter     m_control;
    int                              m_id;
    
};


class RemoveControlParameterCommand : public KNamedCommand
{
public:
    RemoveControlParameterCommand(Rosegarden::Studio *studio,
				  Rosegarden::DeviceId device,
                                  int id):
        KNamedCommand(getGlobalName()),
        m_studio(studio),
	m_device(device),
        m_id(id) { }

    ~RemoveControlParameterCommand();

    virtual void execute();
    virtual void unexecute();

    static QString getGlobalName() { return i18n("&Remove Control Parameter"); }

protected:
    Rosegarden::Studio              *m_studio;
    Rosegarden::DeviceId             m_device;
    int                              m_id;
    Rosegarden::ControlParameter     m_oldControl;

};

class ModifyControlParameterCommand : public KNamedCommand
{
public:
    ModifyControlParameterCommand(Rosegarden::Studio *studio,
				  Rosegarden::DeviceId device,
                                  Rosegarden::ControlParameter control,
                                  int id):
        KNamedCommand(getGlobalName()),
        m_studio(studio),
	m_device(device),
        m_control(control),
        m_id(id) { }
    ~ModifyControlParameterCommand();

    virtual void execute();
    virtual void unexecute();

    static QString getGlobalName() { return i18n("&Modify Control Parameter"); }

protected:
    Rosegarden::Studio            *m_studio;
    Rosegarden::DeviceId           m_device;
    Rosegarden::ControlParameter   m_control;
    int                            m_id;
    Rosegarden::ControlParameter   m_originalControl;

};
