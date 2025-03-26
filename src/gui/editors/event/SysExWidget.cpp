/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2025 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[SysExWidget]"
#define RG_NO_DEBUG_PRINT

#include "SysExWidget.h"

#include "EditEvent.h"

#include "base/Event.h"
#include "base/MidiTypes.h"  // For SystemExclusive::EventType...
#include "gui/widgets/FileDialog.h"
#include "gui/widgets/LineEdit.h"
#include "misc/ConfigGroups.h"  // For LastUsedPathsConfigGroup
#include "misc/Strings.h"  // For qstrtostr()...
#include "sound/Midi.h"  // For MIDI_SYSTEM_EXCLUSIVE...

#include <QDir>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>


namespace Rosegarden
{


SysExWidget::SysExWidget(EditEvent *parent, const Event &event) :
    EventWidget(parent)
{
    if (event.getType() != SystemExclusive::EventType)
        return;

    // Main layout.
    // This is a "fake" layout that is needed to make sure there is a
    // layout at each parent/child level.  If we remove this layout, the
    // resizing from the parent down to the widgets becomes a mess.
    // Using QGridLayout because it is handy.  Any layout would do here.
    QGridLayout *mainLayout = new QGridLayout(this);
    // Get rid of any extra margins introduced by the layout.
    mainLayout->setContentsMargins(0,0,0,0);

    // SysEx Properties group box

    QGroupBox *propertiesGroup = new QGroupBox(tr("SysEx Properties"), this);
    propertiesGroup->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(propertiesGroup);

    QGridLayout *propertiesLayout = new QGridLayout(propertiesGroup);
    propertiesLayout->setSpacing(5);

    int row{0};

    // Data
    QLabel *dataLabel = new QLabel(tr("Data:"), propertiesGroup);
    propertiesLayout->addWidget(dataLabel, row, 0);

    m_dataEdit = new LineEdit;
    std::string datablock;
    if (event.has(SystemExclusive::DATABLOCK))
        datablock = event.get<String>(SystemExclusive::DATABLOCK);
    m_dataEdit->setText(strtoqstr(datablock));
    propertiesLayout->addWidget(m_dataEdit, row, 1);

    m_loadDataButton = new QPushButton(tr("Load data"), propertiesGroup);
    connect(m_loadDataButton, &QPushButton::clicked,
            this, &SysExWidget::slotLoadData);
    propertiesLayout->addWidget(m_loadDataButton, row, 2);

    ++row;

    m_saveDataButton = new QPushButton(tr("Save data"), propertiesGroup);
    connect(m_saveDataButton, &QPushButton::clicked,
            this, &SysExWidget::slotSaveData);
    propertiesLayout->addWidget(m_saveDataButton, row, 2);

    ++row;

}

EventWidget::PropertyNameSet
SysExWidget::getPropertyFilter() const
{
    return PropertyNameSet{SystemExclusive::DATABLOCK};
}

void
SysExWidget::slotLoadData()
{
    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    const QString pathKey = "load_sysex";
    QString directory = settings.value(pathKey, QDir::homePath()).toString();

    QString name = FileDialog::getOpenFileName(
            this,  // parent
            tr("Load System Exclusive data in File"),  // caption
            directory,  // dir
            tr("System exclusive files") + " (*.syx *.SYX)" + ";;" +
                tr("All files") + " (*)");  // filter
    if (name.isNull())
        return;

    QFile file(name);
    file.open(QIODevice::ReadOnly);
    std::string s;
    char c;

    // Discard leading bytes up to and including the first SysEx Start (F0).
    while (file.getChar(&c)) {
        if (c == static_cast<char>(MIDI_SYSTEM_EXCLUSIVE))
            break;
    }
    // Copy up to but not including SysEx End (F7).
    while (file.getChar(&c)) {
        if (c == static_cast<char>(MIDI_END_OF_EXCLUSIVE))
            break;
        s += c;
    }

    file.close();

    if (s.empty())
        QMessageBox::critical(this, tr("Rosegarden"), tr("Could not load SysEx file."));

    m_dataEdit->setText(strtoqstr(SystemExclusive::toHex(s)));

    // Write the directory to the settings
    QDir d = QFileInfo(name).dir();
    directory = d.canonicalPath();
    settings.setValue(pathKey, directory);
    settings.endGroup();
}

void
SysExWidget::slotSaveData()
{
    QSettings settings;
    settings.beginGroup(LastUsedPathsConfigGroup);
    const QString pathKey = "save_sysex";
    QString directory = settings.value(pathKey, QDir::homePath()).toString();

    QString name = FileDialog::getSaveFileName(
            this,  // parent
            tr("Save System Exclusive data to..."),  // caption
            directory,  // dir
            "",  // defaultName
            tr("System exclusive files") + " (*.syx *.SYX)" + ";;" +
                tr("All files") + " (*)");  // filter
    if (name.isNull())
        return;

    static QRegularExpression extensionRegEx("\\..{1,4}$");
    // If the file name has no extension, add ".syx".
    if (!extensionRegEx.match(name).hasMatch())
        name += ".syx";

    QFile file(name);
    file.open(QIODevice::WriteOnly);

    std::string datablock = qstrtostr(m_dataEdit->text());
    // Add SysEx and End SysEx status bytes.
    datablock = "F0 " + datablock + " F7";
    datablock = SystemExclusive::toRaw(datablock);
    file.write(datablock.c_str(),
               static_cast<qint64>(datablock.length()));

    file.close();

    // Write the directory to the settings
    QDir d = QFileInfo(name).dir();
    directory = d.canonicalPath();
    settings.setValue(pathKey, directory);
    settings.endGroup();
}

void SysExWidget::updateEvent(Event &event) const
{
    event.set<String>(SystemExclusive::DATABLOCK,
            qstrtostr(m_dataEdit->text()));
}


}
