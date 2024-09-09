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
#include "base/MidiDevice.h"
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
class QStackedLayout;


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

    ModifyDeviceCommand *makeCommand(const QString &commandName);

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
    void slotEdit(QTreeWidgetItem *item, int column);
    /// Handles name changes in the tree.
    void slotItemChanged(QTreeWidgetItem *item, int column);

    /// Show and update the program editor or the key map editor.
    /**
     * Used to make sure the right portion of the dialog shows the proper
     * editor and contents when a different item is selected in the tree.
     *
     * See updateEditor().
     */
    void slotUpdateEditor(
            QTreeWidgetItem *currentItem, QTreeWidgetItem *previousItem);

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
    void slotVariationChanged(int index);

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
    /// Add Banks and Key Maps to the tree for a MidiDevice.
    void populateDeviceItem(QTreeWidgetItem *deviceItem,
                            MidiDevice *midiDevice);
    /// Checks type of item and calls item->parent().
    MidiDeviceTreeWidgetItem *getParentDeviceItem(QTreeWidgetItem *item);
    /// Select a device in the tree.  Used after adding or importing banks.
    void selectDeviceItem(MidiDevice *device);
    /// Select a bank or key map item by name under deviceItem.
    void selectItem(MidiDeviceTreeWidgetItem *deviceItem, const QString &name);

    QWidget *m_rightSide;
    QStackedLayout *m_rightSideLayout;

    MidiProgramsEditor *m_programEditor;
    MidiKeyMappingEditor *m_keyMappingEditor;

    // Options
    QGroupBox *m_optionBox;
    // Show Variation list based on
    QCheckBox *m_variationCheckBox;
    // Show Variation list based on
    QComboBox *m_variationCombo;
    // Cache to detect changes.
    MidiDevice::VariationType m_variationType;

    QPushButton *m_closeButton;

    /// Show and update the program editor or the key map editor.
    /**
     * One or the other appear on the right side of the dialog.
     */
    void updateEditor(QTreeWidgetItem *item);

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
    static void getFirstFreeBank(MidiDevice *device, MidiByte &msb, MidiByte &lsb);

    /// Handle bank name conflicts by adding "_1".
    static QString makeUniqueBankName(const QString &name,
                               const BankList &banks);
    /// Handle key map name conflicts by adding "_1".
    static QString makeUniqueKeymapName(const QString &name,
                                 const KeyMappingList &keymaps);

    /// Identify Tracks using a bank to avoid deleting banks that are in use.
    bool tracksUsingBank(const MidiBank &bank, const MidiDevice &device);

    // Device Observer Management
    std::set<Device *> m_observedDevices;
    void observeDevice(Device *device);
    void unobserveDevice(Device *device);

    // Studio Observer Management
    bool m_observingStudio;

    /// Tells updateDialog() which item to select.
    /**
     * Used by slotModifyDeviceOrBankName() to let updateDialog() know which
     * item to select.  We want the item that was just renamed to be selected.
     */
    QString m_selectionName;
};


}

#endif
