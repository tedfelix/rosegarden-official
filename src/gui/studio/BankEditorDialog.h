/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_BANKEDITORDIALOG_H
#define RG_BANKEDITORDIALOG_H

#include "base/Device.h"
#include "base/Studio.h"
#include "base/MidiProgram.h"
#include "gui/general/ActionFileClient.h"

#include <QMainWindow>

#include <map>
#include <set>
#include <string>
#include <utility>


class QWidget;
class QString;
class QPushButton;
class QTreeWidgetItem;
class QGroupBox;
class QCloseEvent;
class QCheckBox;
class QTreeWidget;
class QComboBox;
class QFrame;


namespace Rosegarden
{

class Command;
class RosegardenDocument;
class MidiProgramsEditor;
class MidiKeyMappingEditor;
class MidiDeviceTreeWidgetItem;
class MidiDevice;
class ModifyDeviceCommand;

class BankEditorDialog : public QMainWindow, public ActionFileClient,
    public StudioObserver, public DeviceObserver
{
    Q_OBJECT

public:
    BankEditorDialog(QWidget *parent,
                     RosegardenDocument *doc,
                     DeviceId defaultDevice =
                     Device::NO_DEVICE);

    ~BankEditorDialog() override;

    // Initialize the devices/banks and programs - the whole lot
    //
    void initDialog();

    std::pair<int, int> getFirstFreeBank(QTreeWidgetItem*);

    ModifyDeviceCommand* makeCommand(const QString& name);
    void addCommandToHistory(Command *command);

    void setCurrentDevice(DeviceId device);

    BankList&   getBankList()     { return m_bankList; }
    ProgramList&getProgramList()  { return m_programList; }

    Studio *getStudio() { return m_studio; }

    // Set the listview to select a certain device - used after adding
    // or deleting banks.
    //
    void selectDeviceItem(MidiDevice *device);

    // Select a device/bank combination
    //
    void selectDeviceBankItem(DeviceId deviceId, int bank);
    void selectDeviceKeymapItem(DeviceId deviceId, const QString& keymapName);

    QString makeUniqueBankName(const QString& name,
                               const BankList& banks);
    QString makeUniqueKeymapName(const QString& name,
                                 const KeyMappingList& keymaps);

public slots:
    void slotPopulateDeviceEditors(QTreeWidgetItem*, QTreeWidgetItem*);//int column);

    void slotUpdate();

    void slotAddBank();
    void slotAddKeyMapping();
    void slotDelete();
    void slotDeleteAll();

    void slotImport();
    void slotExport();

    void slotModifyDeviceOrBankName(QTreeWidgetItem*, int);

    void slotFileClose();

    void slotEdit(QTreeWidgetItem *item, int);
    void slotEditCopy();
    void slotEditPaste();

    void slotVariationToggled();
    void slotVariationChanged(int);
    void slotHelpRequested();
    void slotHelpAbout();

signals:
    void closing();
    void deviceNamesChanged();

protected:
    void closeEvent(QCloseEvent*) override;

    void setProgramList(MidiDevice *device);

    void updateDialog();

    void populateDeviceItem(QTreeWidgetItem* deviceItem,
                            MidiDevice* midiDevice);

    void updateDeviceItem(MidiDeviceTreeWidgetItem* deviceItem);

    void clearItemChildren(QTreeWidgetItem* item);

    MidiDeviceTreeWidgetItem* getParentDeviceItem(QTreeWidgetItem*);

    void populateDeviceEditors(QTreeWidgetItem*);

    void setupActions();

    //--------------- Data members ---------------------------------
    Studio                 *m_studio;
    RosegardenDocument     *m_doc;

    MidiProgramsEditor      *m_programEditor;
    MidiKeyMappingEditor    *m_keyMappingEditor;
    QTreeWidget             *m_treeWidget;

    QGroupBox               *m_optionBox;
    QCheckBox               *m_variationToggle;
    QComboBox               *m_variationCombo;

    QPushButton             *m_closeButton;
    QPushButton             *m_applyButton;

    QPushButton             *m_addBank;
    QPushButton             *m_addKeyMapping;
    QPushButton             *m_delete;
    QPushButton             *m_deleteAll;

    QPushButton             *m_importBanks;
    QPushButton             *m_exportBanks;

    QPushButton             *m_copyPrograms;
    QPushButton             *m_pastePrograms;

    enum class ItemType {NONE, DEVICE, BANK, KEYMAP};
    struct Clipboard
    {
        ItemType itemType;
        DeviceId deviceId;
        int bank;
        QString keymapName;
    };
    Clipboard m_clipboard;

    BankList                 m_bankList;
    ProgramList              m_programList;
    ProgramList              m_oldProgramList;

    bool                     m_deleteAllReally;

    DeviceId                 m_lastDevice;
    MidiBank                 m_lastBank;

    bool                     m_updateDeviceList;

    QFrame                  *m_rightSide;

 private:
    bool tracksUsingBank(const MidiBank& bank, const MidiDevice& device);

    // studio observer interface
    virtual void deviceAdded(Device* device) override;
    virtual void deviceRemoved(Device* device) override;

    // device observer interface
    virtual void deviceModified(Device* device) override;

    void observeDevice(Device* device);
    void unobserveDevice(Device* device);

    std::set<Device*> m_observedDevices;
};

// ----------------------- RemapInstrumentDialog ------------------------
//
//


}

#endif
