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

#define RG_MODULE_STRING "[AudioPluginDialog]"

#include "AudioPluginDialog.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/AudioPluginInstance.h"
#include "base/Instrument.h"
#include "base/MidiProgram.h"
#include "gui/studio/AudioPluginClipboard.h"
#include "gui/studio/AudioPlugin.h"
#include "gui/studio/AudioPluginManager.h"
#include "gui/studio/AudioPluginOSCGUIManager.h"
#include "gui/studio/StudioControl.h"
#include "gui/widgets/PluginControl.h"
#include "sound/MappedStudio.h"
#include "sound/PluginIdentifier.h"

#include <QLayout>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QShortcut>
#include <QCheckBox>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QStringList>
#include <QToolTip>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QUrl>
#include <QDesktopServices>

#include <set>

namespace Rosegarden
{

AudioPluginDialog::AudioPluginDialog(QWidget *parent,
                                     QSharedPointer<AudioPluginManager> aPM,
                                     AudioPluginOSCGUIManager *aGM,
                                     PluginContainer *pluginContainer,
                                     int index):
    QDialog(parent),
    m_pluginManager(aPM),
    m_pluginGUIManager(aGM),
    m_pluginContainer(pluginContainer),
    m_containerId(pluginContainer->getId()),
    m_programLabel(nullptr),
    m_index(index),
    m_generating(true),
    m_guiShown(false)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

    setModal(false);
    setWindowTitle(tr("Audio Plugin"));

    // This will get rid of the dialog and the object on close.
    setAttribute(Qt::WA_DeleteOnClose);

    QGridLayout *metagrid = new QGridLayout(this);
//    metagrid->setMargin(0);

    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout(vbox);
    vboxLayout->setMargin(0);
    metagrid->addWidget(vbox, 0, 0);
  
    // Plugin group box
    QGroupBox *pluginSelectionBox = new QGroupBox(tr("Plugin"), vbox);
    QVBoxLayout *pluginSelectionBoxLayout = new QVBoxLayout(pluginSelectionBox);
//    pluginSelectionBoxLayout->setMargin(0);
    vboxLayout->addWidget(pluginSelectionBox);

    // Plugin parameters group box (m_pluginParamsBox).
    makePluginParamsBox(vbox);
    vboxLayout->addWidget(m_pluginParamsBox);

    // the Category label/combo
    m_pluginCategoryBox = new QWidget(pluginSelectionBox);
    QHBoxLayout *pluginCategoryBoxLayout = new QHBoxLayout(m_pluginCategoryBox);
    pluginCategoryBoxLayout->setMargin(0);
    pluginSelectionBoxLayout->addWidget(m_pluginCategoryBox);

    pluginCategoryBoxLayout->addWidget(new QLabel(tr("Category:"), m_pluginCategoryBox));

    m_pluginCategoryList = new QComboBox(m_pluginCategoryBox);
    pluginCategoryBoxLayout->addWidget(m_pluginCategoryList);
    m_pluginCategoryList->setMaxVisibleItems(20);

    // the Plugin label/combo 
    QWidget *pluginPluginBox = new QWidget(pluginSelectionBox);
    QHBoxLayout *pluginPluginBoxLayout = new QHBoxLayout(pluginPluginBox);
    pluginPluginBoxLayout->setMargin(0);
    pluginSelectionBoxLayout->addWidget(pluginPluginBox);

    // Plugin label
    pluginPluginBoxLayout->addWidget(new QLabel(tr("Plugin:"), pluginPluginBox));

    // Plugin combo box
    m_pluginList = new QComboBox(pluginPluginBox);
    pluginPluginBoxLayout->addWidget(m_pluginList);
    m_pluginList->setMaxVisibleItems(20);

    // Let's set the minimumContentsLength() to the longest plugin descriptor
    // string I could find, which should be plenty long enough:
    QString metric("Mvchpf-1   Digital implementation of the VC HP filter invented by R.A. Moog");
    m_pluginList->setMinimumContentsLength ((metric.size() / 2));
    m_pluginList->setToolTip(tr("Select a plugin from this list"));

    // the Bypass <ports> <id>
    QWidget *pluginDonglesBox = new QWidget(pluginSelectionBox);
    QHBoxLayout *pluginDonglesBoxLayout = new QHBoxLayout(pluginDonglesBox);
    pluginDonglesBoxLayout->setMargin(0);
    pluginSelectionBoxLayout->addWidget(pluginDonglesBox);

    m_bypass = new QCheckBox(tr("Bypass"), pluginDonglesBox);
    pluginDonglesBoxLayout->addWidget(m_bypass);
    pluginDonglesBoxLayout->addStretch(20); // spread these out some so they don't clump together
    m_bypass->setToolTip(tr("Bypass this plugin"));

    connect(m_bypass, &QAbstractButton::toggled,
            this, &AudioPluginDialog::slotBypassChanged);


    m_insOuts = new QLabel(tr("<ports>"), pluginDonglesBox);
    pluginDonglesBoxLayout->addWidget(m_insOuts, Qt::AlignRight);
    pluginDonglesBoxLayout->addStretch(20); // spread these out some so they don't clump together
    m_insOuts->setToolTip(tr("<qt><p>Tells you if the plugin is <b>mono</b>, <b>stereo</b>, or has some other combination of input and output ports, such as <b>2 in, 1 out</b>, which would take a stereo input and output mono</p></qt>"));

    m_pluginId = new QLabel(tr("<id>", "'id' is short for 'identification'"), pluginDonglesBox);
    pluginDonglesBoxLayout->addWidget(m_pluginId, Qt::AlignRight);
    m_pluginId->setToolTip(tr("Unique ID of plugin"));

    connect(m_pluginList, SIGNAL(activated(int)),
            this, SLOT(slotPluginSelected(int)));

    connect(m_pluginCategoryList, SIGNAL(activated(int)),
            this, SLOT(slotCategorySelected(int)));

    // the Copy/Paste/Default buttons
    QWidget *pluginButtonsBox = new QWidget(pluginSelectionBox);
    QHBoxLayout *pluginButtonsBoxLayout = new QHBoxLayout(pluginButtonsBox);
    pluginButtonsBoxLayout->setMargin(0);
    pluginSelectionBoxLayout->addWidget(pluginButtonsBox);

    m_copyButton = new QPushButton(tr("Copy"), pluginButtonsBox);
    pluginButtonsBoxLayout->addWidget(m_copyButton);

    connect(m_copyButton, &QAbstractButton::clicked,
            this, &AudioPluginDialog::slotCopy);
    m_copyButton->setToolTip(tr("Copy plugin parameters"));

    m_pasteButton = new QPushButton(tr("Paste"), pluginButtonsBox);
    pluginButtonsBoxLayout->addWidget(m_pasteButton);

    connect(m_pasteButton, &QAbstractButton::clicked,
            this, &AudioPluginDialog::slotPaste);
    m_pasteButton->setToolTip(tr("Paste plugin parameters"));

    m_defaultButton = new QPushButton(tr("Reset"), pluginButtonsBox);
    pluginButtonsBoxLayout->addWidget(m_defaultButton);

    connect(m_defaultButton, &QAbstractButton::clicked,
            this, &AudioPluginDialog::slotDefault);
    m_defaultButton->setToolTip(tr("Reset plugin controls to factory defaults"));
    
    m_generating = false;

    m_shortcuts = new QShortcut(this);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
                 QDialogButtonBox::Close | QDialogButtonBox::Help);

    m_editorButton = new QPushButton(tr("Editor"));
    //RG_DEBUG << "ctor - created Editor button";
    buttonBox->addButton(m_editorButton, QDialogButtonBox::ActionRole);
    connect(m_editorButton, &QAbstractButton::clicked, this, &AudioPluginDialog::slotEditor);
    m_editorButton->setEnabled(false);

    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);

    // "Close" button has the RejectRole
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    // "Help" button has the HelpRole
    connect(buttonBox, &QDialogButtonBox::helpRequested, this, &AudioPluginDialog::slotHelpRequested);

    //RG_DEBUG << "ctor: populating plugin category list...";
    populatePluginCategoryList();
    //RG_DEBUG << "ctor: populating plugin list...";
    populatePluginList();

    //RG_DEBUG << "ctor finished.";
}


AudioPluginDialog::~AudioPluginDialog()
{
    //RG_DEBUG << "dtor...";
    emit destroyed(m_containerId, m_index);
}


void
AudioPluginDialog::slotEditor()
{
    slotShowGUI();
}


void
AudioPluginDialog::slotShowGUI()
{
    emit showPluginGUI(m_containerId, m_index);
    m_guiShown = true;

    //!!! need to get notification of when a plugin gui exits from the
    //gui manager
}

void
AudioPluginDialog::populatePluginCategoryList()
{
    //RG_DEBUG << "populatePluginCategoryList()";
    AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);
    std::set
        <QString> categories;
    QString currentCategory;

    for (PluginIterator i = m_pluginManager->begin();
         i != m_pluginManager->end(); ++i) {

        if (( isSynth() && (*i)->isSynth()) ||
            (!isSynth() && (*i)->isEffect())) {

            if ((*i)->getCategory() != "") {
                categories.insert((*i)->getCategory());
            }

            if (inst && inst->isAssigned() &&
                ((*i)->getIdentifier() == inst->getIdentifier().c_str())) {
                currentCategory = (*i)->getCategory();
            }
        }
    }

    //if (inst)
    //    RG_DEBUG << "populatePluginCategoryList(): inst id " << inst->getIdentifier() << ", cat " << currentCategory;

    if (categories.empty()) {
        m_pluginCategoryBox->hide();
        //&&& Still needs disabling after all the other fixes worked.  No
        // obvious side effects of just not hiding this though.
        //m_pluginLabel->hide();
    }


    m_pluginCategoryList->clear();
    m_pluginCategoryList->addItem(tr("(any)"));
    m_pluginCategoryList->addItem(tr("(unclassified)"));
    m_pluginCategoryList->setCurrentIndex(0);

    for (std::set
             <QString>::iterator i = categories.begin();
         i != categories.end(); ++i) {

        m_pluginCategoryList->addItem(*i);

        if (*i == currentCategory) {
            m_pluginCategoryList->setCurrentIndex(m_pluginCategoryList->count() - 1);
        }
    }
}

void
AudioPluginDialog::populatePluginList()
{
    //RG_DEBUG << "populatePluginList()";

    m_pluginList->clear();
    m_pluginsInList.clear();

    m_pluginList->addItem(tr("(none)"));
    m_pluginsInList.push_back(0);

    QString category;
    bool needCategory = false;

    // If a category other than the first is selected in the combobox
    if (m_pluginCategoryList->currentIndex() > 0) {
        needCategory = true;
        if (m_pluginCategoryList->currentIndex() == 1) {
            category = "";
        } else {
            category = m_pluginCategoryList->currentText();
        }
    }

    // Use this temporary map to ensure that the plugins are sorted
    // by name when they go into the combobox.
    // The count is the AudioPluginManager index.
    typedef std::pair<int /* count */, AudioPlugin *> PluginPair;
    typedef std::map<QString, PluginPair> PluginMap;
    PluginMap pluginsSorted;
    int count = 0;

    // For each plugin in the AudioPluginManager
    for (PluginIterator i = m_pluginManager->begin();
         i != m_pluginManager->end(); ++i) {

        AudioPlugin *plugin = (*i);

        ++count;

        // If this is the kind of plugin we are displaying
        if ((isSynth()  &&  plugin->isSynth())  ||
            (!isSynth()  &&  plugin->isEffect())) {

            if (needCategory) {
                QString cat = "";
                if (!plugin->getCategory().isEmpty())
                    cat = plugin->getCategory();
                if (cat != category)
                    continue;
            }

            QString name = plugin->getName();
            bool store = true;

            if (pluginsSorted.find(name) != pluginsSorted.end()) {
                // We already have a plugin of this name.  If it's a
                // LADSPA plugin, replace it (this one might be
                // something better); otherwise leave it alone.
                QString id = pluginsSorted[name].second->getIdentifier();
                QString type, soname, label;
                PluginIdentifier::parseIdentifier(id, type, soname, label);
                if (type != "ladspa") {
                    store = false;
                }
            }

            if (store) {
                pluginsSorted[plugin->getName()] = PluginPair(count, plugin);
            }
        }
    }

    // Check for plugin and setup as required
    AudioPluginInstance *plugin = m_pluginContainer->getPlugin(m_index);
    if (plugin)
        m_bypass->setChecked(plugin->isBypassed());

    QString currentId;

    if (plugin  &&  plugin->isAssigned()) {
        //RG_DEBUG << "  plugin->getIdentifier(): " << plugin->getIdentifier();
        currentId = plugin->getIdentifier().c_str();
    }

    // For each plugin in the sorted map
    for (PluginMap::iterator i = pluginsSorted.begin();
         i != pluginsSorted.end();
         ++i) {

        QString name = i->first;
        if (name.endsWith(" VST"))
            name = name.left(name.length() - 4);

        m_pluginList->addItem(name);
        m_pluginsInList.push_back(i->second.first);

        // If this is the plugin that is selected, select it in the combobox.
        if (currentId == i->second.second->getIdentifier()) {
            m_pluginList->setCurrentIndex(m_pluginList->count() - 1);
        }
    }

    slotPluginSelected(m_pluginList->currentIndex());
}

void
AudioPluginDialog::makePluginParamsBox(QWidget *parent)
{
    //RG_DEBUG << "makePluginParamsBox()";
    //@@@ //!!!
    // There really isn't anything to do here now.  It used to calculate how
    // many rows and columns to use to create a QGridLayout, but in Qt4 there is
    // no ctor like this at all.  You have to create an empty QGridLayout and
    // let it expand to fit what you stuff into it.  So we have to manage that
    // on the stuffing into it end now.
    
    m_pluginParamsBox = new QGroupBox(parent);

    m_pluginParamsBox->setContentsMargins(5, 5, 5, 5);
    m_pluginParamsBoxLayout = new QGridLayout(m_pluginParamsBox);
    m_pluginParamsBoxLayout->setVerticalSpacing(0);
}

void
AudioPluginDialog::slotCategorySelected(int)
{
    //RG_DEBUG << "slotCategorySelected()";
    populatePluginList();
}

void
AudioPluginDialog::slotPluginSelected(int i)
{
    bool guiWasShown = m_guiShown;

    if (m_guiShown) {
        emit stopPluginGUI(m_containerId, m_index);
        m_guiShown = false;
    }

    int number = m_pluginsInList[i];

    //RG_DEBUG << "slotPluginSelected(): setting up plugin from position " << number << " at menu item " << i;

    QString caption =
        strtoqstr(m_pluginContainer->getName()) +
        QString(" [ %1 ] - ").arg(m_index + 1);

    if (number == 0) {
        setWindowTitle(caption + tr("<no plugin>"));
        m_insOuts->setText(tr("<ports>"));
        m_pluginId->setText(tr("<id>"));

        m_pluginList->setToolTip( tr("Select a plugin from this list") );
    }

    AudioPlugin *plugin = m_pluginManager->getPlugin(number - 1);

    // Destroy old param widgets
    //
    QWidget* parent = dynamic_cast<QWidget*>(m_pluginParamsBox->parent());

    delete m_pluginParamsBox;
    //RG_DEBUG << "slotPluginSelected(): deleted m_pluginParamsBox";
    m_pluginWidgets.clear(); // The widgets are deleted with the parameter box
    m_programCombo = nullptr;

    // count up how many controller inputs the plugin has
    int portCount = 0;
    if (plugin) {
        for (AudioPlugin::PortIterator it = plugin->begin(); it != plugin->end(); ++it) {
            if (((*it)->getType() & PluginPort::Control) &&
                ((*it)->getType() & PluginPort::Input))
                ++portCount;
        }
    }

    int tooManyPorts = 96;
    makePluginParamsBox(parent);
    parent->layout()->addWidget(m_pluginParamsBox);
    bool showBounds = (portCount <= 48);
    bool hidden = false;

    // FluidSynth-DSSI just doesn't have any controls, rather than too many, and
    // the lack of warning struck me as inconsistent.
    int insRow = (m_programCombo && m_programCombo->isVisible() ? 2 : 1);

    if (portCount > tooManyPorts) {

        m_pluginParamsBoxLayout->addWidget(
            new QLabel(tr("<qt><p>This plugin has too many controls to edit here.</p><p>Use the external editor, if available.</p></qt>"),
                            m_pluginParamsBox), insRow, 0, 1, 2, Qt::AlignCenter);
            hidden = true;
    } else if (portCount == 0 && plugin) {
        m_pluginParamsBoxLayout->addWidget(
            new QLabel(tr("This plugin does not have any controls that can be edited here."),
                           m_pluginParamsBox), insRow, 0, 1, 2, Qt::AlignCenter);
    }

    AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);
    if (!inst)
        return ;

    if (plugin) {
        setWindowTitle(caption + plugin->getName());
        m_pluginId->setText(tr("Id: %1").arg(plugin->getUniqueId()));

        QString pluginInfo = plugin->getAuthor() + QString("\n") +
            plugin->getCopyright();

        m_pluginList->setToolTip(pluginInfo);

        std::string identifier = qstrtostr(plugin->getIdentifier());

        // Only clear ports &c if this method is accessed by user
        // action (after the constructor)
        //
        if (m_generating == false) {
///////
//          inst->clearPorts();
///////
            if (inst->getIdentifier() != identifier) {
                inst->clearConfiguration();
            }
        }

        inst->setIdentifier(identifier);

        AudioPlugin::PortIterator it = plugin->begin();
        int count = 0;
        int ins = 0, outs = 0;

        for (; it != plugin->end(); ++it) {
            if (((*it)->getType() & PluginPort::Control) &&
                ((*it)->getType() & PluginPort::Input)) {
                // Check for port existence and create with default value
                // if it doesn't exist.  Modification occurs through the
                // slotPluginPortChanged signal.
                //
                if (inst->getPort(count) == nullptr) {
                    inst->addPort(count, (float)(*it)->getDefaultValue());
//                    std::cerr << "Plugin port name " << (*it)->getName() << ", default: " << (*it)->getDefaultValue() << std::endl;
                }

            } else if ((*it)->getType() & PluginPort::Audio) {
                if ((*it)->getType() & PluginPort::Input)
                    ++ins;
                else if ((*it)->getType() & PluginPort::Output)
                    ++outs;
            }

            ++count;
        }

        if (ins == 1 && outs == 1)
            m_insOuts->setText(tr("mono"));
        else if (ins == 2 && outs == 2)
            m_insOuts->setText(tr("stereo"));
        else
            m_insOuts->setText(tr("%1 in, %2 out").arg(ins).arg(outs));

        QString shortName(plugin->getName());
        int parenIdx = shortName.indexOf(" (");
        if (parenIdx > 0) {
            shortName = shortName.left(parenIdx);
            if (shortName == "Null")
                shortName = "Plugin";
        }
    }

    adjustSize();
    
    // tell the sequencer
    emit pluginSelected(m_containerId, m_index, number - 1);

    if (plugin) {

        int current = -1;
        QStringList programs = getProgramsForInstance(inst, current);

        if (programs.count() > 0) {

            m_programLabel = new QLabel(tr("Program:  "), m_pluginParamsBox);

            m_programCombo = new QComboBox(m_pluginParamsBox);
            m_programCombo->setMaxVisibleItems(20);
            m_programCombo->addItem(tr("<none selected>"));
            m_pluginParamsBoxLayout->addWidget(m_programLabel, 0, 0, Qt::AlignRight);
            m_pluginParamsBoxLayout->addWidget(m_programCombo, 0, 1, Qt::AlignLeft);
            connect(m_programCombo, SIGNAL(activated(const QString &)),
                    this, SLOT(slotPluginProgramChanged(const QString &)));

            m_programCombo->clear();
            m_programCombo->addItem(tr("<none selected>"));
            m_programCombo->addItems(programs);
            m_programCombo->setCurrentIndex(current + 1);
            m_programCombo->adjustSize();

            m_programLabel->show();
            m_programCombo->show();
        }

        AudioPlugin::PortIterator it = plugin->begin();
        int count = 0;

        int row = insRow + 1;
        int col = 0;

        int wrap = 3;
        int portCalc = portCount;

        // if it isn't even, make it so
        if (portCount % 2 != 0) {
            portCalc++;
            //std::cout << "was: " << portCount << " now: " << portCalc << std::endl;
        }

        // balance by the smallest number of columns that suits
        for (int c = 2; c < 7; c++) {
            if ((portCalc % c) && portCalc > c + 1) wrap = c;
        }

        // stick in some overrides for known cases where the above chooses
        // poorly...  I'm sure it's all pure crap, but unless I write another
        // gigantic switch statement to spell out how to resolve all 95 cases
        // exactly the way I want, this is probably as close as I'm going to
        // get.  I'm just not much good at manipulating numbers this way.
        if (portCalc == 4) wrap = 2;
        if (portCalc == 6) wrap = 3;
        if (portCalc == 8) wrap = 4;
        if (portCalc == 16) wrap = 4;
        if (portCalc == 10) wrap = 5;
        if (portCalc == 11) wrap = 4;
        if (portCalc == 12) wrap = 4;

//        std::cout << "final wrap was " << wrap << " for " << portCount << " plugins." << std::endl;
//        std::cout << "this equals " << (portCount / wrap) << " rows by " << wrap << " columns." << std::endl;


        // Create the plugin controls, but only if they're going to be visible.
        // (Else what's the point of creating them at all?)
        for (; it != plugin->end(); ++it) {
            if (((*it)->getType() & PluginPort::Control) &&
                ((*it)->getType() & PluginPort::Input) &&
                !hidden) {
                PluginControl *control =
                    new PluginControl(m_pluginParamsBox,
                                      PluginControl::Rotary,
                                      *it,
                                      m_pluginManager,
                                      count,
                                      inst->getPort(count)->value,
                                      showBounds);
                m_pluginParamsBoxLayout->addWidget(control, row, col);
//                m_pluginParamsBoxLayout->setColumnStretch(col, 20);

//                RG_DEBUG << "Added PluginControl: showBounds: "
//                         << (showBounds ? "true" : "false")
//                         << " hidden: "
//                         << (hidden ? "true" : "false")
//                         << endl;
                
                if (++col >= wrap) {
                    row++;
                    col = 0;
                }

                connect(control, &PluginControl::valueChanged,
                        this, &AudioPluginDialog::slotPluginPortChanged);

                m_pluginWidgets.push_back(control);
            }

            ++count;
        }

        m_pluginParamsBox->show();
    }

    if (guiWasShown) {
        emit showPluginGUI(m_containerId, m_index);
        m_guiShown = true;
    }

    // unused
    //bool gui = false;
    m_pluginGUIManager->hasGUI(m_containerId, m_index);
//    std::cout << "gui is: " << gui
//              << " container ID: " << m_containerId
//              << " index: " << m_index
//              << std::endl;
//    m_editorButton->setEnabled(gui);    

    //!!!  I can't get to the bottom of this in a reasonable amount of time.
    // m_containerId is always 10013, and m_index is either 0 (LADSPA plugin) or
    // 999 (DSSI plugin) with no variation, and hasGUI() always tests false.
    // This means the editor button is never enabled, and this is unacceptable
    // for plugins that have no controls that can be edited from within
    // Rosegarden.  I'm hacking the button always enabled, even if no GUI is
    // available.  This seems to fail silently until the user randomly switches
    // to a plugin that does have a GUI, in which case its GUI pops up, even
    // though they're five plugins away from the one where the pushed the button
    // originally.  Compared with never making it available, this is tolerable.
    m_editorButton->setEnabled(true);

    adjustSize();
    update(0, 0, width(), height());
}

QStringList
AudioPluginDialog::getProgramsForInstance(AudioPluginInstance *inst, int &current)
{
    QStringList list;
    int mappedId = inst->getMappedId();
    QString currentProgram = strtoqstr(inst->getProgram());

    MappedObjectPropertyList propertyList = StudioControl::getStudioObjectProperty
        (mappedId, MappedPluginSlot::Programs);

    current = -1;

    for (MappedObjectPropertyList::iterator i = propertyList.begin();
         i != propertyList.end(); ++i) {
        if (*i == currentProgram)
            current = list.count();
        list.append(*i);
    }

    return list;
}

void
AudioPluginDialog::slotPluginPortChanged(float value)
{
    //RG_DEBUG << "slotPluginPortChanged(" << value << ")";
    const QObject* object = sender();

    const PluginControl* control = dynamic_cast<const PluginControl*>(object);

    if (!control)
        return ;

    // store the new value
    AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);
    inst->getPort(control->getIndex())->setValue(value);

    emit pluginPortChanged(m_containerId, m_index, control->getIndex());
}

void
AudioPluginDialog::slotPluginProgramChanged(const QString &value)
{
    // store the new value
    AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);

    if (m_programCombo && value == m_programCombo->itemText(0)) { // "<none set>"
        inst->setProgram("");
    } else {
        inst->setProgram(qstrtostr(value));
        emit pluginProgramChanged(m_containerId, m_index);
    }
}

void
AudioPluginDialog::updatePlugin(int number)
{
    for (unsigned int i = 0; i < m_pluginsInList.size(); ++i) {
        if (m_pluginsInList[i] == number + 1) {
            blockSignals(true);
            m_pluginList->setCurrentIndex(i);
            blockSignals(false);
            return ;
        }
    }
}

void
AudioPluginDialog::updatePluginPortControl(int port)
{
    AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);
    if (inst) {
        PluginPortInstance *pti = inst->getPort(port);
        if (pti) {
            for (std::vector<PluginControl *>::iterator i = m_pluginWidgets.begin();
                 i != m_pluginWidgets.end(); ++i) {
                if ((*i)->getIndex() == port) {
                    (*i)->setValue(pti->value, false); // don't emit
                    return ;
                }
            }
        }
    }
}

void
AudioPluginDialog::updatePluginProgramControl()
{
    //RG_DEBUG << "updatePluginProgramControl()";

    AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);
    if (inst) {
        std::string program = inst->getProgram();
        if (m_programCombo) {
            m_programCombo->blockSignals(true);
            m_programCombo->setItemText( m_index, strtoqstr(program) );    //@@@ m_index param correct ? (=index-nr)
            m_programCombo->blockSignals(false);
        }
        for (std::vector<PluginControl *>::iterator i = m_pluginWidgets.begin();
             i != m_pluginWidgets.end(); ++i) {
            PluginPortInstance *pti = inst->getPort((*i)->getIndex());
            if (pti) {
                (*i)->setValue(pti->value, false); // don't emit
            }
        }
    }
}

void
AudioPluginDialog::updatePluginProgramList()
{
    //RG_DEBUG << "updatePluginProgramList()";

    if (!m_programLabel)
        return ;

    AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);
    if (!inst)
        return ;

    if (!m_programCombo) {

        int current = -1;
        QStringList programs = getProgramsForInstance(inst, current);

        if (programs.count() > 0) {

            m_programLabel = new QLabel(tr("Program:  "), m_pluginParamsBox);

            m_programCombo = new QComboBox(m_pluginParamsBox);
            m_programCombo->setMaxVisibleItems(20);
            m_programCombo->addItem(tr("<none selected>"));
            m_pluginParamsBoxLayout->addWidget(m_programLabel, 0, 0, Qt::AlignRight);
            m_pluginParamsBoxLayout->addWidget(m_programCombo, 0, 1, Qt::AlignLeft);

            m_programCombo->clear();
            m_programCombo->addItem(tr("<none selected>"));
            m_programCombo->addItems(programs);
            m_programCombo->setCurrentIndex(current + 1);
            m_programCombo->adjustSize();

            m_programLabel->show();
            m_programCombo->show();

            m_programCombo->blockSignals(true);
            connect(m_programCombo, SIGNAL(activated(const QString &)),
                    this, SLOT(slotPluginProgramChanged(const QString &)));

        } else {
            return ;
        }
    } else {
    }

    while (m_programCombo->count() > 0) {
        m_programCombo->removeItem(0);
    }

    int current = -1;
    QStringList programs = getProgramsForInstance(inst, current);

    if (programs.count() > 0) {
        m_programCombo->show();
        m_programLabel->show();
        m_programCombo->clear();
        m_programCombo->addItem(tr("<none selected>"));
        m_programCombo->addItems(programs);
        m_programCombo->setCurrentIndex(current + 1);
    } else {
        m_programLabel->hide();
        m_programCombo->hide();
    }

    m_programCombo->blockSignals(false);
}

void
AudioPluginDialog::slotBypassChanged(bool bp)
{
    AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);

    if (inst)
        inst->setBypass(bp);

    emit bypassed(m_containerId, m_index, bp);
}

void
AudioPluginDialog::windowActivationChange(bool /*oldState*/)
{
    if (isActiveWindow()) {
        emit windowActivated();
    }
}


void
AudioPluginDialog::slotCopy()
{
    int item = m_pluginList->currentIndex();
    int number = m_pluginsInList[item] - 1;

    if (number >= 0) {
        AudioPluginClipboard *clipboard =
            m_pluginManager->getPluginClipboard();

        clipboard->m_pluginNumber = number;

        AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);
        if (inst) {
            clipboard->m_configuration = inst->getConfiguration();
        } else {
            clipboard->m_configuration.clear();
        }

        std::cout << "AudioPluginDialog::slotCopy - plugin number = " << number
                  << std::endl;

        if (m_programCombo && m_programCombo->currentIndex() > 0) {
            clipboard->m_program = qstrtostr(m_programCombo->currentText());
        } else {
            clipboard->m_program = "";
        }

        clipboard->m_controlValues.clear();
        std::vector<PluginControl*>::iterator it;
        for (it = m_pluginWidgets.begin(); it != m_pluginWidgets.end(); ++it) {
            std::cout << "AudioPluginDialog::slotCopy - "
                      << "value = " << (*it)->getValue() << std::endl;

            clipboard->m_controlValues.push_back((*it)->getValue());
        }
    }
}

void
AudioPluginDialog::slotPaste()
{
    AudioPluginClipboard *clipboard = m_pluginManager->getPluginClipboard();

    std::cout << "AudioPluginDialog::slotPaste - paste plugin id "
              << clipboard->m_pluginNumber << std::endl;

    if (clipboard->m_pluginNumber != -1) {
        int count = 0;
        for (std::vector<int>::iterator it = m_pluginsInList.begin();
             it != m_pluginsInList.end(); ++it) {
            if ((*it) == clipboard->m_pluginNumber + 1)
                break;
            count++;
        }

        if (count >= int(m_pluginsInList.size()))
            return ;

        // now select the plugin
        //
        m_pluginList->setCurrentIndex(count);
        slotPluginSelected(count);

        // set configuration data
        //
        for (std::map<std::string, std::string>::const_iterator i =
                 clipboard->m_configuration.begin();
             i != clipboard->m_configuration.end(); ++i) {
            emit changePluginConfiguration(m_containerId,
                                           m_index,
                                           false,
                                           strtoqstr(i->first),
                                           strtoqstr(i->second));
        }

        // and set the program
        //
        if (m_programCombo && clipboard->m_program != "") {
            m_programCombo->setItemText( count, strtoqstr(clipboard->m_program));
            slotPluginProgramChanged(strtoqstr(clipboard->m_program));
        }

        // and ports
        //
        count = 0;

        for (std::vector<PluginControl *>::iterator i = m_pluginWidgets.begin();
             i != m_pluginWidgets.end(); ++i) {

            if (count < (int)clipboard->m_controlValues.size()) {
                (*i)->setValue(clipboard->m_controlValues[count], true);
            }
            ++count;
        }
    }
}

void
AudioPluginDialog::slotDefault()
{
    AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);
    if (!inst)
        return ;

    int i = m_pluginList->currentIndex();
    int n = m_pluginsInList[i];
    if (n == 0)
        return ;

    AudioPlugin *plugin = m_pluginManager->getPlugin(n - 1);
    if (!plugin)
        return ;

    for (std::vector<PluginControl *>::iterator i = m_pluginWidgets.begin();
         i != m_pluginWidgets.end(); ++i) {

        for (AudioPlugin::PortIterator pi = plugin->begin(); pi != plugin->end(); ++pi) {
            if ((*pi)->getNumber() == (*i)->getIndex()) {
                (*i)->setValue((*pi)->getDefaultValue(), true); // and emit
                break;
            }
        }
    }
}

void
AudioPluginDialog::slotHelpRequested()
{
    // TRANSLATORS: if the manual is translated into your language, you can
    // change the two-letter language code in this URL to point to your language
    // version, eg. "http://rosegardenmusic.com/wiki/doc:audioPluginDialog-es" for the
    // Spanish version. If your language doesn't yet have a translation, feel
    // free to create one.
    QString helpURL = tr("http://rosegardenmusic.com/wiki/doc:audioPluginDialog-en");
    QDesktopServices::openUrl(QUrl(helpURL));
}

}
