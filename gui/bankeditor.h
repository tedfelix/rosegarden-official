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


#ifndef _BANKEDITOR_H_
#define _BANKEDITOR_H_

#include <vector>
#include <map>

#include <qlistview.h>
#include <qvgroupbox.h>

#include <kcompletion.h>
#include <kdialogbase.h>
#include <kmainwindow.h>

#include "Device.h"
#include "Instrument.h"
#include "MidiProgram.h"

class KComboBox;
class QButtonGroup;
class QPushButton;
class QFrame;
class QSpinBox;
class QCheckBox;
class QRadioButton;
class RosegardenGUIDoc;
class MultiViewCommandHistory;
class KCommand;
class BankEditorDialog;
class KListView;
class KLineEdit;

namespace Rosegarden { class Studio; class MidiDevice; }

class MidiDeviceListViewItem : public QListViewItem
{
public:
    MidiDeviceListViewItem(Rosegarden::DeviceId id,
                           QListView* parent, QString name);

    MidiDeviceListViewItem(Rosegarden::DeviceId id,
                           QListViewItem* parent, QString name,
			   bool percussion,
                           int msb, int lsb);

    Rosegarden::DeviceId getDeviceId() const { return m_deviceId; }

    virtual int compare(QListViewItem *i, int col, bool ascending) const;

protected:

    //--------------- Data members ---------------------------------
    Rosegarden::DeviceId m_deviceId;
};

class MidiBankListViewItem : public MidiDeviceListViewItem
{
public:
    MidiBankListViewItem(Rosegarden::DeviceId deviceId,
                         int bankNb,
                         QListViewItem* parent, QString name,
			 bool percussion,
                         int msb, int lsb);

    int getBank()     { return m_bankNb; }

    void setPercussion(bool percussion);
    void setMSB(int msb);
    void setLSB(int msb);

    virtual int compare(QListViewItem *i, int col, bool ascending) const;
    
protected:

    //--------------- Data members ---------------------------------
    int    m_bankNb;
};

class MidiProgramsEditor : public QVGroupBox
{
    Q_OBJECT
public:
    MidiProgramsEditor(BankEditorDialog *bankEditor,
                       QWidget *parent,
                       const char *name = 0);

    typedef std::vector<Rosegarden::MidiProgram> MidiProgramContainer;
    typedef std::vector<Rosegarden::MidiBank>    MidiBankContainer;
    
    int ensureUniqueMSB(int msb, bool ascending);
    int ensureUniqueLSB(int lsb, bool ascending);

    // Does the banklist contain this combination already?
    //
    bool banklistContains(const Rosegarden::MidiBank &);

    MidiProgramContainer getBankSubset(const Rosegarden::MidiBank &);

    Rosegarden::MidiBank* getCurrentBank();

    /// Set the currently loaded programs to new MSB and LSB
    void modifyCurrentPrograms(const Rosegarden::MidiBank &oldBank,
			       const Rosegarden::MidiBank &newBank);
    
    // Get a program (pointer into program list) for modification
    //
    Rosegarden::MidiProgram* getProgram(const Rosegarden::MidiBank &bank, int program);

    void setBankName(const QString& s);

    void clearAll();

    void populateBank(QListViewItem*);

    void resetMSBLSB();

public slots:

    // Check that any new MSB/LSB combination is unique for this device
    //
    void slotNewMSB(int value);
    void slotNewLSB(int value);
    void slotNewPercussion(); // gets value from checkbox

    void slotProgramChanged(const QString&);

protected:

    void blockAllSignals(bool block);

    //--------------- Data members ---------------------------------
    BankEditorDialog*        m_bankEditor;

    KCompletion              m_completion;
    std::vector<KLineEdit*>  m_programNames;

    QFrame                   *m_mainFrame;

    QLabel                   *m_bankName;
    QCheckBox                *m_percussion;
    QSpinBox                 *m_msb;
    QSpinBox                 *m_lsb;

    QLabel                   *m_librarian;
    QLabel                   *m_librarianEmail;

    Rosegarden::MidiBank     *m_currentBank;
    MidiBankContainer        &m_bankList;
    MidiProgramContainer     &m_programList;

    Rosegarden::MidiBank      m_oldBank;
};

class BankEditorDialog : public KMainWindow
{
    Q_OBJECT

public:
    BankEditorDialog(QWidget *parent,
                     RosegardenGUIDoc *doc);

    ~BankEditorDialog();

    // Initialise the devices/banks and programs - the whole lot
    //
    void initDialog();

    std::pair<int, int> getFirstFreeBank(QListViewItem*);

    void addCommandToHistory(KCommand *command);
    MultiViewCommandHistory* getCommandHistory();

    Rosegarden::MidiBank* getCurrentBank() { return m_programEditor->getCurrentBank(); }

    // Get a MidiDevice from an index number
    //
    Rosegarden::MidiDevice* getMidiDevice(Rosegarden::DeviceId);
    Rosegarden::MidiDevice* getMidiDevice(QListViewItem*);
    Rosegarden::MidiDevice* getCurrentMidiDevice();
    MidiProgramsEditor::MidiBankContainer&      getBankList()     { return m_bankList; }
    MidiProgramsEditor::MidiProgramContainer&   getProgramList()  { return m_programList; }

    void setModified(bool value);

    void checkModified();

    // Set the listview to select a certain device - used after adding
    // or deleting banks.
    //
    void selectDeviceItem(Rosegarden::MidiDevice *device);

    // Select a device/bank combination
    //
    void selectDeviceBankItem(Rosegarden::DeviceId device, int bank);

public slots:
    void slotPopulateDevice(QListViewItem*);

    void slotApply();
    void slotReset();

    void slotUpdate();

    void slotAddBank();
    void slotDeleteBank();
    void slotDeleteAllBanks();

    void slotImport();
    void slotExport();

    void slotModifyDeviceOrBankName(QListViewItem*, const QString&,int);

    void slotFileClose();

    void slotEditCopy();
    void slotEditPaste();

    void slotVariationToggled();
    void slotVariationChanged(int);

signals:
    void closing();
    void deviceNamesChanged();

protected:
    virtual void closeEvent(QCloseEvent*);

    void resetProgramList();
    void setProgramList(Rosegarden::MidiDevice *device);

    void updateDialog();

    void populateDeviceItem(QListViewItem* deviceItem,
                            Rosegarden::MidiDevice* midiDevice);

    void updateDeviceItem(MidiDeviceListViewItem* deviceItem);

    bool deviceItemHasBank(MidiDeviceListViewItem* deviceItem, int bankNb);

    void clearItemChildren(QListViewItem* deviceItem);

    void importFromSF2(QString filename);

    MidiDeviceListViewItem* getParentDeviceItem(QListViewItem*);
    void keepBankListForNextPopulate() { m_keepBankList = true; }

    void populateDevice(QListViewItem*);

    void setupActions();

    //--------------- Data members ---------------------------------
    Rosegarden::Studio      *m_studio;
    RosegardenGUIDoc        *m_doc;

    
    MidiProgramsEditor      *m_programEditor;
    KListView               *m_listView;

    QGroupBox               *m_optionBox;
    QCheckBox               *m_variationToggle;
    KComboBox               *m_variationCombo;

    QPushButton             *m_closeButton;
    QPushButton             *m_resetButton;
    QPushButton             *m_applyButton;

    QPushButton             *m_addBank;
    QPushButton             *m_deleteBank;
    QPushButton             *m_deleteAllBanks;

    QPushButton             *m_importBanks;
    QPushButton             *m_exportBanks;

    QPushButton             *m_copyPrograms;
    QPushButton             *m_pastePrograms;
    std::pair<Rosegarden::DeviceId, int> m_copyBank;

    std::map<Rosegarden::DeviceId, std::string>  m_deviceNameMap;
    MidiProgramsEditor::MidiBankContainer        m_bankList;
    MidiProgramsEditor::MidiProgramContainer     m_programList;
    MidiProgramsEditor::MidiProgramContainer     m_oldProgramList;

    bool                     m_modified;
    bool                     m_keepBankList;
    bool                     m_deleteAll;

    Rosegarden::DeviceId     m_lastDevice;
    Rosegarden::MidiBank     m_lastBank;

    static const char* const BankEditorConfigGroup;

    bool                     m_updateDeviceList;
};

// ----------------------- RemapInstrumentDialog ------------------------
//
//

class RemapInstrumentDialog : public KDialogBase
{
    Q_OBJECT
public:
    RemapInstrumentDialog(QWidget *parent,
                          RosegardenGUIDoc *doc);

    void populateCombo(int id);

    void addCommandToHistory(KCommand *command);
    MultiViewCommandHistory* getCommandHistory();

public slots:
    void slotRemapReleased(int id);

    void slotOk();
    void slotApply();

protected:

    RosegardenGUIDoc    *m_doc;

    QRadioButton        *m_deviceButton;
    QRadioButton        *m_instrumentButton;

    QButtonGroup        *m_buttonGroup;
    KComboBox           *m_fromCombo;
    KComboBox           *m_toCombo;
};

// --------------------- ImportDeviceDialog --------------------------
//
//

class ImportDeviceDialog : public KDialogBase
{
    Q_OBJECT
public:
    ImportDeviceDialog(QWidget *parent,
                       std::vector<QString> devices,
		       bool showRenameOption);
 
    int getDeviceIndex() const;
    bool getOverwrite() const; 
    bool getRename() const;

public slots:
    void slotOk();
    void slotCancel();

protected:
    KComboBox          *m_deviceCombo;
    QLabel             *m_label;

    QButtonGroup       *m_buttonGroup;
    QRadioButton       *m_mergeBanks;
    QRadioButton       *m_overwriteBanks;
    QCheckBox          *m_rename;

};

#endif // _BANKEDITOR_H_

