/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "MidiKeyMappingEditor.h"

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "BankEditorDialog.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "MidiKeyMapListViewItem.h"
#include "NameSetEditor.h"
#include <kcompletion.h>
#include <klineedit.h>
#include <qframe.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qobject.h>
#include <qobjectlist.h>
#include <qpushbutton.h>
#include <qstring.h>
#include <qvgroupbox.h>
#include <qwidget.h>


namespace Rosegarden
{

MidiKeyMappingEditor::MidiKeyMappingEditor(BankEditorDialog* bankEditor,
        QWidget* parent,
        const char* name)
        : NameSetEditor(bankEditor,
                        i18n("Key Mapping details"),
                        parent, name, i18n("Pitches"), false),
        m_device(0)
{
    QWidget *additionalWidget = makeAdditionalWidget(m_mainFrame);
    if (additionalWidget) {
        m_mainLayout->addMultiCellWidget(additionalWidget, 0, 2, 0, 2);
    }
}

QWidget *
MidiKeyMappingEditor::makeAdditionalWidget(QWidget *parent)
{
    return 0;
}

void
MidiKeyMappingEditor::clearAll()
{
    blockAllSignals(true);

    for (unsigned int i = 0; i < m_names.size(); ++i)
        m_names[i]->clear();

    setTitle(i18n("Key Mapping details"));

    setEnabled(false);

    blockAllSignals(false);
}

void
MidiKeyMappingEditor::populate(QListViewItem* item)
{
    RG_DEBUG << "MidiKeyMappingEditor::populate\n";

    MidiKeyMapListViewItem *keyItem =
        dynamic_cast<MidiKeyMapListViewItem *>(item);
    if (!keyItem) {
        RG_DEBUG << "MidiKeyMappingEditor::populate : not a key item - returning\n";
        return ;
    }

    MidiDevice* device = m_bankEditor->getCurrentMidiDevice();
    if (!device)
        return ;

    m_device = device;
    m_mappingName = qstrtostr(keyItem->getName());

    setEnabled(true);

    reset();
}

void
MidiKeyMappingEditor::reset()
{
    if (!m_device)
        return ;

    setTitle(strtoqstr(m_mappingName));

    const MidiKeyMapping *m = m_device->getKeyMappingByName(m_mappingName);

    if (!m) {
        RG_DEBUG << "WARNING: MidiKeyMappingEditor::reset: No such mapping as " << m_mappingName << endl;
    }

    m_mapping = *m;

    blockAllSignals(true);

    // Librarian details
    //
    m_librarian->setText(strtoqstr(m_device->getLibrarianName()));
    m_librarianEmail->setText(strtoqstr(m_device->getLibrarianEmail()));

    for (MidiKeyMapping::KeyNameMap::const_iterator it =
                m_mapping.getMap().begin();
            it != m_mapping.getMap().end(); ++it) {

        int i = it->first;
        if (i < 0 || i > 127) {
            RG_DEBUG << "WARNING: MidiKeyMappingEditor::reset: Key " << i
            << " out of range in mapping " << m_mapping.getName()
            << endl;
            continue;
        }

        QString name = strtoqstr(it->second);
        m_completion.addItem(name);
        m_names[i]->setText(name);
        m_names[i]->setCursorPosition(0);
    }

    blockAllSignals(false);
}

void
MidiKeyMappingEditor::slotNameChanged(const QString& name)
{
    const KLineEdit* lineEdit = dynamic_cast<const KLineEdit*>(sender());
    if (!lineEdit) {
        RG_DEBUG << "MidiKeyMappingEditor::slotNameChanged() : %%% ERROR - signal sender is not a KLineEdit\n";
        return ;
    }

    QString senderName = sender()->name();

    // Adjust value back to zero rated
    //
    unsigned int pitch = senderName.toUInt() - 1;

    RG_DEBUG << "MidiKeyMappingEditor::slotNameChanged("
    << name << ") : pitch = " << pitch << endl;

    if (qstrtostr(name) != m_mapping.getMap()[pitch]) {
        m_mapping.getMap()[pitch] = qstrtostr(name);
        m_bankEditor->setModified(true);
    }
}

void
MidiKeyMappingEditor::slotEntryButtonPressed()
{}

void MidiKeyMappingEditor::blockAllSignals(bool block)
{
    const QObjectList* allChildren = queryList("KLineEdit", "[0-9]+");
    QObjectListIt it(*allChildren);
    QObject *obj;

    while ( (obj = it.current()) != 0 ) {
        obj->blockSignals(block);
        ++it;
    }
}

}
#include "MidiKeyMappingEditor.moc"
