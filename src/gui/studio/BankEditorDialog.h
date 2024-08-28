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
#include "gui/general/ActionFileClient.h"

#include <QMainWindow>

#include <set>
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


/// Manage MIDI Banks and Programs dialog
class BankEditorDialog : public QMainWindow, public ActionFileClient,
                         public StudioObserver, public DeviceObserver
{
    Q_OBJECT

public:

    BankEditorDialog(QWidget *parent,
                     RosegardenDocument *doc,
                     DeviceId defaultDevice);
    ~BankEditorDialog() override;

    ModifyDeviceCommand *makeCommand(const QString &name);
    // ??? Trivial wrapper.  Inline into each caller.
    void addCommandToHistory(Command *command);

    void setCurrentDevice(DeviceId device);

    // StudioObserver interface
    virtual void deviceAdded(Device* device) override;
    virtual void deviceRemoved(Device* device) override;

    // DeviceObserver interface
    virtual void deviceModified(Device* device) override;

public slots:

    /// Librarian Edit button.  Called by NameSetEditor.
    void slotEditLibrarian();
    /// Close button or when the document is about to change.
    void slotFileClose();

signals:

    /// Called by closeEvent().  Connected to RMW::slotBankEditorClosed().
    void closing();

    /**
     * RosegardenMainWindow::slotEditBanks() connects this to:
     *   - RosegardenMainViewWidget::slotSynchroniseWithComposition()
     *   - DeviceManagerDialog::slotResyncDevicesReceived()
     *   - TrackParameterBox::devicesChanged()
     * This makes sure all the names on the UI are correct.
     */
    void deviceNamesChanged();

private slots:

    /// Tree item double-click to edit name.
    void slotEdit(QTreeWidgetItem *item, int);
    /// Handles name changes in the tree.
    void slotModifyDeviceOrBankName(QTreeWidgetItem *, int);

    // ??? General Comment: "Key Mapping" is too verbose.  Can we change that
    //     to "Key Map" everywhere?

    /// Show and update the program editor or the key mapping editor.
    /**
     * Used to make sure the right portion of the dialog shows the proper
     * editor and contents when a different item is selected in the tree.
     *
     * See updateEditor().
     */
    void slotUpdateEditor(QTreeWidgetItem *, QTreeWidgetItem *);

    // Button Handlers
    void slotAddBank();
    void slotAddKeyMapping();
    void slotDelete();
    void slotDeleteAll();
    void slotImport();
    void slotExport();
    void slotCopy();
    void slotPaste();

    /// "Show Variation list based on" check box handler.
    void slotVariationToggled();
    /// "Show Variation list based on" combo box handler.
    void slotVariationChanged(int);

    /// Help > Help
    void slotHelpRequested();
    /// Help > About Rosegarden
    void slotHelpAbout();

protected:

    /// QWidget override.  Emits the closing() signal.
    void closeEvent(QCloseEvent *) override;

private:

    RosegardenDocument *m_doc;
    Studio *m_studio;

    // Widgets

    QTreeWidget *m_treeWidget;
    void clearItemChildren(QTreeWidgetItem *item);
    /// Add Banks and Key Mappings to the tree for a MidiDevice.
    void populateDeviceItem(QTreeWidgetItem *deviceItem,
                            MidiDevice *midiDevice);
    /// Checks type of item and calls item->parent().
    MidiDeviceTreeWidgetItem *getParentDeviceItem(QTreeWidgetItem *item);
    /// Select a device in the tree.  Used after adding or importing banks.
    void selectDeviceItem(MidiDevice *device);

    QPushButton *m_addBank;
    QPushButton *m_addKeyMapping;
    QPushButton *m_delete;
    QPushButton *m_deleteAll;
    // ??? rename: m_import
    QPushButton *m_importBanks;
    // ??? rename: m_export
    QPushButton *m_exportBanks;
    // ??? rename: m_copy
    QPushButton *m_copyPrograms;
    // ??? rename: m_paste
    QPushButton *m_pastePrograms;

    QFrame *m_rightSide;

    MidiProgramsEditor *m_programEditor;
    MidiKeyMappingEditor *m_keyMappingEditor;

    // Options
    QGroupBox *m_optionBox;
    // Show Variation list based on
    // ??? rename: m_variationCheckBox
    QCheckBox *m_variationToggle;
    // Show Variation list based on
    QComboBox *m_variationCombo;

    QPushButton *m_closeButton;
    // ??? Apply?  I've never seen this.
    //QPushButton *m_applyButton;

    /// Show and update the program editor or the key mapping editor.
    /**
     * One or the other appear on the right side of the dialog.
     */
    void updateEditor(QTreeWidgetItem *item);

    /// Init the tree and the bank/key mapping editor.
    void initDialog();
    /// Create actions and menus.
    void setupActions();

    void updateDialog();

    // Clipboard
    enum class ItemType {NONE, DEVICE, BANK, KEYMAP};
    struct Clipboard
    {
        ItemType itemType{ItemType::NONE};
        DeviceId deviceId{Device::NO_DEVICE};
        int bank{-1};
        QString keymapName;
    };
    Clipboard m_clipboard;

    /// Get first free bank to avoid conflicts.
    /**
     * ??? DO NOT use std::pair.  It is *obtuse*.  For this particular case, use
     *     two more reference parameters: MidiByte &msb, MidiByte &lsb
     */
    std::pair<int /* MSB */, int /* LSB */> getFirstFreeBank(QTreeWidgetItem *item);

    /// Handle bank name conflicts by adding "_1".
    QString makeUniqueBankName(const QString& name,
                               const BankList& banks);
    /// Handle key map name conflicts by adding "_1".
    QString makeUniqueKeymapName(const QString& name,
                                 const KeyMappingList& keymaps);

    /// Identify Tracks using a bank to avoid deleting banks that are in use.
    bool tracksUsingBank(const MidiBank& bank, const MidiDevice& device);

    // ??? HERE

    std::set<Device *> m_observedDevices;
    void observeDevice(Device *device);
    void unobserveDevice(Device *device);

    QString m_selectionName;
    bool m_observingStudio;
};


}

#endif
