/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


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

#include <set>

namespace Rosegarden
{

AudioPluginDialog::AudioPluginDialog(QWidget *parent,
                                     AudioPluginManager *aPM,
#ifdef HAVE_LIBLO
                                     AudioPluginOSCGUIManager *aGM,
#endif
                                     PluginContainer *pluginContainer,
                                     int index):
    QDialog(parent),
    m_pluginManager(aPM),
#ifdef HAVE_LIBLO
    m_pluginGUIManager(aGM),
#endif
    m_pluginContainer(pluginContainer),
    m_containerId(pluginContainer->getId()),
    m_programLabel(0),
    m_index(index),
    m_generating(true),
    m_guiShown(false)
{
    //setHelp("studio-plugins");
//     setSizePolicy(QSizePolicy(QSizePolicy::Preferred,
//                               QSizePolicy::Fixed));
    
    setModal(false);
    setWindowTitle(tr("Audio Plugin"));

    //@@@ NOTE:  It is apparently new Qt4 idiom to use something like:
    //
    // QWidget *foo = new QWidget(optional parent);
    // QGridLayout *bar = new QGridLayout;
    // foo->addLayout(bar);
    //
    // Layouts don't have parents or construction parameters in this idiom.
    //
    // This code had been rewritten that way, but I used Designer to help sort
    // out what belonged to what why, and Designer-generated code set the layout
    // of a widget through parenting.  I rewrote the code that way, copying the
    // Designer model, and only later in my digging did I discover that I
    // probably shouldn't have.
    //
    // I can't see where this hurts anything, but it's not consistent with the
    // rest of our code.  I'm going to leave well enough alone, since layout
    // inheritance is no longer why this dialog isn't fully functional, but it
    // might be worth putting it back the other way eventually, on a rainy day.
    //
    QGridLayout *metagrid = new QGridLayout(this);

    QWidget *vbox = new QWidget(this);
    QVBoxLayout *vboxLayout = new QVBoxLayout(vbox);
    metagrid->addWidget(vbox, 0, 0);
  
    QGroupBox *pluginSelectionBox = new QGroupBox(tr("Plugin"), vbox);
    QVBoxLayout *pluginSelectionBoxLayout = new QVBoxLayout(pluginSelectionBox);
    vboxLayout->addWidget(pluginSelectionBox);

    makePluginParamsBox(vbox, 0, 10); // creates a GroupBox
    vboxLayout->addWidget(m_pluginParamsBox);

    // the Category label/combo
    m_pluginCategoryBox = new QWidget(pluginSelectionBox);
    QHBoxLayout *pluginCategoryBoxLayout = new QHBoxLayout(m_pluginCategoryBox);
    pluginSelectionBoxLayout->addWidget(m_pluginCategoryBox);

    pluginCategoryBoxLayout->addWidget(new QLabel(tr("Category:"), m_pluginCategoryBox));

    m_pluginCategoryList = new QComboBox(m_pluginCategoryBox);
    pluginCategoryBoxLayout->addWidget(m_pluginCategoryList);
    m_pluginCategoryList->setMaxVisibleItems(20);

    // the Plugin label/combo 
    QWidget *pluginPluginBox = new QWidget(pluginSelectionBox);
    QHBoxLayout *pluginPluginBoxLayout = new QHBoxLayout(pluginPluginBox);
    pluginSelectionBoxLayout->addWidget(pluginPluginBox);

    pluginPluginBoxLayout->addWidget(new QLabel(tr("Plugin:"), pluginPluginBox));

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
    pluginSelectionBoxLayout->addWidget(pluginDonglesBox);

    m_bypass = new QCheckBox(tr("Bypass"), pluginDonglesBox);
    pluginDonglesBoxLayout->addWidget(m_bypass);
    m_bypass->setToolTip(tr("Bypass this plugin"));

    connect(m_bypass, SIGNAL(toggled(bool)),
            this, SLOT(slotBypassChanged(bool)));


    m_insOuts = new QLabel(tr("<ports>"), pluginDonglesBox);
    pluginDonglesBoxLayout->addWidget(m_insOuts, Qt::AlignRight);
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
    pluginSelectionBoxLayout->addWidget(pluginButtonsBox);

    m_copyButton = new QPushButton(tr("Copy"), pluginButtonsBox);
    pluginButtonsBoxLayout->addWidget(m_copyButton);

    connect(m_copyButton, SIGNAL(clicked()),
            this, SLOT(slotCopy()));
    m_copyButton->setToolTip(tr("Copy plugin parameters"));

    m_pasteButton = new QPushButton(tr("Paste"), pluginButtonsBox);
    pluginButtonsBoxLayout->addWidget(m_pasteButton);

    connect(m_pasteButton, SIGNAL(clicked()),
            this, SLOT(slotPaste()));
    m_pasteButton->setToolTip(tr("Paste plugin parameters"));

    m_defaultButton = new QPushButton(tr("Reset"), pluginButtonsBox);
    pluginButtonsBoxLayout->addWidget(m_defaultButton);

    connect(m_defaultButton, SIGNAL(clicked()),
            this, SLOT(slotDefault()));
    m_defaultButton->setToolTip(tr("Reset plugin controls to factory defaults"));

    populatePluginCategoryList();
    populatePluginList();

    m_generating = false;

    m_shortcuts = new QShortcut(this);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
                 QDialogButtonBox::Close | QDialogButtonBox::Help);
#ifdef HAVE_LIBLO
    QPushButton *detailsButton = new QPushButton(tr("Editor"));
    buttonBox->addButton(detailsButton, QDialogButtonBox::ActionRole);
    connect(detailsButton, SIGNAL(clicked(bool)), this, SLOT(slotDetails()));
#endif
    metagrid->addWidget(buttonBox, 1, 0);
    metagrid->setRowStretch(0, 10);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}



#ifdef HAVE_LIBLO
void
AudioPluginDialog::slotDetails()
{
    slotShowGUI();
}
#endif



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

    if (inst) {
        RG_DEBUG << "AudioPluginDialog::populatePluginCategoryList: inst id " << inst->getIdentifier() << ", cat " << currentCategory << endl;
    }

    if (categories.empty()) {
        // there was a crash when clicking on the <synth> button because the
        // label was getting hidden before it existed, apparently
        if (m_pluginCategoryBox) m_pluginCategoryBox->hide();
        if (m_pluginLabel) m_pluginLabel->hide();
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
    m_pluginList->clear();
    m_pluginsInList.clear();

    m_pluginList->addItem(tr("(none)"));
    m_pluginsInList.push_back(0);

    QString category;
    bool needCategory = false;

    if (m_pluginCategoryList->currentIndex() > 0) {
        needCategory = true;
        if (m_pluginCategoryList->currentIndex() == 1) {
            category = "";
        } else {
            category = m_pluginCategoryList->currentText();
        }
    }

    // Check for plugin and setup as required
    AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);
    if (inst)
        m_bypass->setChecked(inst->isBypassed());

    // Use this temporary map to ensure that the plugins are sorted
    // by name when they go into the combobox
    typedef std::pair<int, AudioPlugin *> PluginPair;
    typedef std::map<QString, PluginPair> PluginMap;
    PluginMap plugins;
    int count = 0;

    for (PluginIterator i = m_pluginManager->begin();
         i != m_pluginManager->end(); ++i) {

        ++count;

        if (( isSynth() && (*i)->isSynth()) ||
            (!isSynth() && (*i)->isEffect())) {

            if (needCategory) {
                QString cat = "";
                if (!(*i)->getCategory().isEmpty())
                    cat = (*i)->getCategory();
                if (cat != category)
                    continue;
            }

            QString name = (*i)->getName();
            bool store = true;

            if (plugins.find(name) != plugins.end()) {
                // We already have a plugin of this name.  If it's a
                // LADSPA plugin, replace it (this one might be
                // something better); otherwise leave it alone.
                QString id = plugins[name].second->getIdentifier();
                QString type, soname, label;
                PluginIdentifier::parseIdentifier(id, type, soname, label);
                if (type != "ladspa") {
                    store = false;
                }
            }

            if (store) {
                plugins[(*i)->getName()] = PluginPair(count, *i);
            }
        }
    }

    const char *currentId = 0;
    if (inst && inst->isAssigned())
        currentId = inst->getIdentifier().c_str();

    for (PluginMap::iterator i = plugins.begin(); i != plugins.end(); ++i) {

        QString name = i->first;
        if (name.endsWith(" VST"))
            name = name.left(name.length() - 4);

        m_pluginList->addItem(name);
        m_pluginsInList.push_back(i->second.first);

        if (currentId && currentId == i->second.second->getIdentifier()) {
            m_pluginList->setCurrentIndex(m_pluginList->count() - 1);
        }
    }

    slotPluginSelected(m_pluginList->currentIndex());
}

void
AudioPluginDialog::makePluginParamsBox(QWidget *parent, int portCount,
                                       int tooManyPorts)
{
    //@@@ //!!!
    // There really isn't anything to do here now.  It used to calculate how
    // many rows and columns to use to create a QGridLayout, but in Qt4 there is
    // no ctor like this at all.  You have to create an empty QGridLayout and
    // let it expand to fit what you stuff into it, apparently.
    //
    // This means we're passing around a lot of useless crap unless we edit this
    // to get rid of the parameters.  Let's just sort out how to make the new
    // layout actually work first, and leave that for later.
    
    portCount = tooManyPorts; // to shut up unused variable warnings

    //m_pluginParamsBox = new QGroupBox(parent);
    m_pluginParamsBox = new QGroupBox(parent);

    m_pluginParamsBox->setContentsMargins(5, 5, 5, 5);
    m_pluginParamsBoxLayout = new QGridLayout(m_pluginParamsBox);
    //m_pluginParamsBoxLayout->setColumnStretch(3, 2);
    //m_pluginParamsBoxLayout->setColumnStretch(7, 2);
}

void
AudioPluginDialog::slotCategorySelected(int)
{
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

    RG_DEBUG << "AudioPluginDialog::::slotPluginSelected - "
             << "setting up plugin from position " << number << " at menu item " << i << endl;

    QString caption =
        strtoqstr(m_pluginContainer->getName()) +
        QString(" [ %1 ] - ").arg(m_index + 1);

    if (number == 0) {
        setWindowTitle(caption + tr("<no plugin>"));
        m_insOuts->setText(tr("<ports>"));
        m_pluginId->setText(tr("<id>"));

//        QToolTip::hideText(false);
//        QToolTip::remove(m_pluginList);
//        QToolTip::add(m_pluginList, tr("Select a plugin from this list."));
        m_pluginList->setToolTip( tr("Select a plugin from this list") );
    }

    AudioPlugin *plugin = m_pluginManager->getPlugin(number - 1);

    // Destroy old param widgets
    //
    QWidget* parent = dynamic_cast<QWidget*>(m_pluginParamsBox->parent());

    delete m_pluginParamsBox;
    m_pluginWidgets.clear(); // The widgets are deleted with the parameter box
    m_programCombo = 0;

    int portCount = 0;
    if (plugin) {
        for (AudioPlugin::PortIterator it = plugin->begin(); it != plugin->end(); ++it) {
            if (((*it)->getType() & PluginPort::Control) &&
                ((*it)->getType() & PluginPort::Input))
                ++portCount;
        }
    }

    // Qt4 port : parent should be an old Qt3 QHBox now replaced for a
    //            QWidget with a QHBoxLayout
    int tooManyPorts = 96;
    makePluginParamsBox(parent, portCount, tooManyPorts);
    parent->layout()->addWidget(m_pluginParamsBox);
    bool showBounds = (portCount <= 48);

    if (portCount > tooManyPorts) {

        m_pluginParamsBoxLayout->addWidget(
            new QLabel(tr("This plugin has too many controls to edit here"),
                            m_pluginParamsBox),
                       1, 1, 0, m_pluginParamsBoxLayout->columnCount() - 1, Qt::AlignCenter);
    }

    AudioPluginInstance *inst = m_pluginContainer->getPlugin(m_index);
    if (!inst)
        return ;

    if (plugin) {
        setWindowTitle(caption + plugin->getName());
        m_pluginId->setText(tr("Id: %1").arg(plugin->getUniqueId()));

        QString pluginInfo = plugin->getAuthor() + QString("\n") +
            plugin->getCopyright();

//        QToolTip::setVisible(false);
//        QToolTip::remove(m_pluginList);
//        QToolTip::add(m_pluginList, pluginInfo);
        m_pluginList->setToolTip(pluginInfo);

        std::string identifier = qstrtostr(plugin->getIdentifier());

        // Only clear ports &c if this method is accessed by user
        // action (after the constructor)
        //
        if (m_generating == false) {
            inst->clearPorts();
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
                if (inst->getPort(count) == 0) {
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
        int parenIdx = shortName.find(" (");
        if (parenIdx > 0) {
            shortName = shortName.left(parenIdx);
            if (shortName == "Null")
                shortName = "Plugin";
        }
    }

    adjustSize();
//&&&    setFixedSize(minimumSizeHint());
//    setFixedSize(minimumSizeHint());

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
            m_pluginParamsBoxLayout->addWidget(m_programLabel, 0, 0, 0, 0, Qt::AlignRight);
            m_pluginParamsBoxLayout->addWidget(m_programCombo, 0, 0, 1, m_pluginParamsBoxLayout->columnCount() - 1, Qt::AlignLeft);
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

        int row = 0;
        int col = 0;

        for (; it != plugin->end(); ++it) {
            if (((*it)->getType() & PluginPort::Control) &&
                ((*it)->getType() & PluginPort::Input)) {
                PluginControl *control =
                    new PluginControl(m_pluginParamsBox,
                                      PluginControl::Rotary,
                                      *it,
                                      m_pluginManager,
                                      count,
                                      inst->getPort(count)->value,
                                      showBounds,
                                      portCount > tooManyPorts);
                m_pluginParamsBoxLayout->addWidget(control, row, col);
                
                // I have no idea how all of this used to work before, and I
                // haven't worked out how to put some sane upper limit on the
                // number of rows and columns.  Let's just do this to have
                // something done, and I'll work on it more later.  There was
                // all kinds of business about > and < and tooManyPorts that
                // just doesn't work anymore.  Plus none of this is working for
                // synth plugins at all yet.
                if (col++ >= 5) {
                    row++;
                    col = 0;
                }


                connect(control, SIGNAL(valueChanged(float)),
                        this, SLOT(slotPluginPortChanged(float)));

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

#ifdef HAVE_LIBLO
    bool gui = m_pluginGUIManager->hasGUI(m_containerId, m_index);
//&&&    actionButton(Details)->setEnabled(gui);    
#endif

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

    if (m_programCombo && value == m_programCombo->text(0)) { // "<none set>"
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
            m_pluginParamsBoxLayout->addWidget(m_programLabel, 0, 0, 0, 0, Qt::AlignRight);
            m_pluginParamsBoxLayout->addWidget(m_programCombo, 0, 0, 1, m_pluginParamsBoxLayout->columnCount() - 1, Qt::AlignLeft);

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
AudioPluginDialog::windowActivationChange(bool oldState)
{
    if (isActiveWindow()) {
        emit windowActivated();
    }
}

void
AudioPluginDialog::closeEvent(QCloseEvent *e)
{
    e->accept();
    emit destroyed(m_containerId, m_index);
}

void
AudioPluginDialog::slotClose()
{
    emit destroyed(m_containerId, m_index);
    reject();
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

            if (count < clipboard->m_controlValues.size()) {
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

}
#include "AudioPluginDialog.moc"
