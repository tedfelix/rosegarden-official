/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

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
#include "base/MidiProgram.h"
#include "gui/general/ActionFileClient.h"

#include <QMainWindow>

#include <map>
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
class Studio;
class RosegardenDocument;
class MidiProgramsEditor;
class MidiKeyMappingEditor;
class MidiDeviceTreeWidgetItem;
class MidiDevice;


class BankEditorDialog : public QMainWindow, public ActionFileClient
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

    void addCommandToHistory(Command *command);

    void setCurrentDevice(DeviceId device);

    // Get a MidiDevice from an index number
    //
    MidiDevice* getMidiDevice(DeviceId);
    MidiDevice* getMidiDevice(QTreeWidgetItem*);
    MidiDevice* getCurrentMidiDevice();
    BankList&   getBankList()     { return m_bankList; }
    ProgramList&getProgramList()  { return m_programList; }

    Studio *getStudio() { return m_studio; }

    // Set the listview to select a certain device - used after adding
    // or deleting banks.
    //
    void selectDeviceItem(MidiDevice *device);

    // Select a device/bank combination
    //
    void selectDeviceBankItem(DeviceId device, int bank);

public slots:
    void slotPopulateDeviceEditors(QTreeWidgetItem*, QTreeWidgetItem*);//int column);

    void slotApply();
    void slotReset();

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

    void resetProgramList();
    void setProgramList(MidiDevice *device);

    void updateDialog();

    void populateDeviceItem(QTreeWidgetItem* deviceItem,
                            MidiDevice* midiDevice);

    void updateDeviceItem(MidiDeviceTreeWidgetItem* deviceItem);

    bool deviceItemHasBank(MidiDeviceTreeWidgetItem* deviceItem, int bankNb);

    void clearItemChildren(QTreeWidgetItem* deviceItem);

    MidiDeviceTreeWidgetItem* getParentDeviceItem(QTreeWidgetItem*);
    void keepBankListForNextPopulate() { m_keepBankList = true; }

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
    QPushButton             *m_resetButton;
    QPushButton             *m_applyButton;

    QPushButton             *m_addBank;
    QPushButton             *m_addKeyMapping;
    QPushButton             *m_delete;
    QPushButton             *m_deleteAll;

    QPushButton             *m_importBanks;
    QPushButton             *m_exportBanks;

    QPushButton             *m_copyPrograms;
    QPushButton             *m_pastePrograms;
    std::pair<DeviceId, int> m_copyBank;

    std::map<DeviceId,
             std::string>    m_deviceNameMap;
    BankList                 m_bankList;
    ProgramList              m_programList;
    ProgramList              m_oldProgramList;

    bool                     m_keepBankList;
    bool                     m_deleteAllReally;

    DeviceId                 m_lastDevice;
    MidiBank                 m_lastBank;

    bool                     m_updateDeviceList;

    QFrame                  *m_rightSide;
};

// ----------------------- RemapInstrumentDialog ------------------------
//
//


}

#endif
