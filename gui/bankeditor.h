
/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
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

#include <kcompletion.h>
#include <kdialogbase.h>
#include <qlistview.h>
#include <qvgroupbox.h>

#include "Instrument.h"

class RosegardenComboBox;
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
    MidiDeviceListViewItem(int deviceNb,
                           QListView* parent, QString name);

    MidiDeviceListViewItem(int deviceNb,
                           QListViewItem* parent, QString name,
                           QString msb, QString lsb);

    int getDevice()   { return m_deviceNb; }

protected:

    //--------------- Data members ---------------------------------
    int m_deviceNb;
};

class MidiBankListViewItem : public MidiDeviceListViewItem
{
public:
    MidiBankListViewItem(int deviceNb,
                         int bankNb,
                         QListViewItem* parent, QString name,
                         QString msb, QString lsb);

    int getBank()     { return m_bankNb; }

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
    bool banklistContains(int msb, int lsb);

    MidiProgramContainer
        getBankSubset(Rosegarden::MidiByte msb, Rosegarden::MidiByte lsb);

    Rosegarden::MidiBank* getCurrentBank();

    /// Set the currently loaded programs to new MSB and LSB
    void modifyCurrentPrograms(int oldMSB, int oldLSB,
                               int msb, int lsb);

    // Get a program
    //
    Rosegarden::MidiProgram* getProgram(int msb, int lsb, int program);

    void setBankName(const QString& s);

    void clearAll();

    void populateBank(QListViewItem*);

public slots:

    // Check that any new MSB/LSB combination is unique for this device
    //
    void slotNewMSB(int value);
    void slotNewLSB(int value);

    void slotProgramChanged(const QString&);

protected:

    void blockAllSignals(bool block);

    //--------------- Data members ---------------------------------
    BankEditorDialog*        m_bankEditor;

    KCompletion              m_completion;
    std::vector<KLineEdit*>  m_programNames;

    QFrame                   *m_mainFrame;

    QLabel                   *m_bankName;
    QSpinBox                 *m_msb;
    QSpinBox                 *m_lsb;

    Rosegarden::MidiBank     *m_currentBank;
    MidiBankContainer        &m_bankList;
    MidiProgramContainer     &m_programList;
};

class BankEditorDialog : public KDialogBase
{
    Q_OBJECT

public:
    BankEditorDialog(QWidget *parent,
                     RosegardenGUIDoc *doc);

    // Initialise the devices/banks and programs - the whole lot
    //
    void initDialog();

    std::pair<int, int> getFirstFreeBank(QListViewItem*);

    void addCommandToHistory(KCommand *command);
    MultiViewCommandHistory* getCommandHistory();

    Rosegarden::MidiBank* getCurrentBank() { return m_programEditor->getCurrentBank(); }

    // Get a MidiDevice from an index number
    //
    Rosegarden::MidiDevice* getMidiDevice(int);
    Rosegarden::MidiDevice* getMidiDevice(QListViewItem*);
    Rosegarden::MidiDevice* getCurrentMidiDevice();
    MidiProgramsEditor::MidiBankContainer&      getBankList()     { return m_bankList; }
    MidiProgramsEditor::MidiProgramContainer&   getProgramList()  { return m_programList; }

    void setModified(bool value);

    void checkModified();

public slots:
    void slotPopulateDevice(QListViewItem*);

    void slotOk();
    void slotApply();
    void slotClose();

    void slotAddBank();
    void slotDeleteBank();
    void slotDeleteAllBanks();

    void slotImport();
    void slotExport();

    void slotModifyDeviceOrBankName(QListViewItem*, const QString&,int);

    void slotCopy();
    void slotPaste();

protected:
    MidiDeviceListViewItem* getParentDeviceItem(QListViewItem*);
    void keepBankListForNextPopulate() { m_keepBankList = true; }

    //--------------- Data members ---------------------------------
    Rosegarden::Studio      *m_studio;
    RosegardenGUIDoc        *m_doc;

    
    MidiProgramsEditor      *m_programEditor;
    KListView               *m_listView;

    QPushButton             *m_addBank;
    QPushButton             *m_deleteBank;
    QPushButton             *m_deleteAllBanks;

    QPushButton             *m_importBanks;
    QPushButton             *m_exportBanks;

    QPushButton             *m_copyPrograms;
    QPushButton             *m_pastePrograms;
    std::pair<int, int>      m_copyBank;

    std::vector<std::string>                     m_deviceList;
    MidiProgramsEditor::MidiBankContainer        m_bankList;
    MidiProgramsEditor::MidiProgramContainer     m_programList;

    bool                     m_modified;
    bool                     m_keepBankList;

    int                      m_lastDevice;
    int                      m_lastMSB;
    int                      m_lastLSB;
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
    RosegardenComboBox  *m_fromCombo;
    RosegardenComboBox  *m_toCombo;
};

// --------------------- ImportDeviceDialog --------------------------
//
//

class ImportDeviceDialog : public KDialogBase
{
    Q_OBJECT
public:
    ImportDeviceDialog(QWidget *parent,
                       std::vector<QString> devices);

public slots:
    void slotOk();
    void slotCancel();

protected:
    RosegardenComboBox *m_deviceCombo;

    QButtonGroup       *m_buttonGroup;
    QRadioButton       *m_mergeBanks;
    QRadioButton       *m_overwriteBanks;

};
#endif // _BANKEDITOR_H_

