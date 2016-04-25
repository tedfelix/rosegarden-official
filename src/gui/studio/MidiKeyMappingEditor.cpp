/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2016 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[MidiKeyMappingEditor]"

#include "MidiKeyMappingEditor.h"
#include "NameSetEditor.h"
#include "BankEditorDialog.h"
#include "MidiKeyMapTreeWidgetItem.h"

#include "misc/Debug.h"
#include "misc/Strings.h"
#include "base/MidiDevice.h"
#include "base/MidiProgram.h"
#include "base/NotationTypes.h"
#include "gui/widgets/LineEdit.h"

#include <QObject>
#include <QFrame>
#include <QLayout>
#include <QLabel>
#include <QList>
#include <QObjectList>
#include <QPushButton>
#include <QRegExp>
#include <QString>
#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>


namespace Rosegarden
{

MidiKeyMappingEditor::MidiKeyMappingEditor(BankEditorDialog* bankEditor,
        QWidget* parent,
        const char* name)
        : NameSetEditor(bankEditor,
                        tr("Key Mapping details"),
                        parent, name, tr("Pitches"), false),
        m_device(0)
{
    QWidget *additionalWidget = makeAdditionalWidget(m_mainFrame);
    if (additionalWidget) {
        m_mainLayout->addWidget(additionalWidget, 0, 0, 2- 0+1, 2- 0+1);
    }
}

QWidget *
MidiKeyMappingEditor::makeAdditionalWidget(QWidget */* parent */)
{
    return 0;
}

void
MidiKeyMappingEditor::clearAll()
{
    blockAllSignals(true);

    for (size_t i = 0; i < m_names.size(); ++i)
        m_names[i]->clear();

    setTitle(tr("Key Mapping details"));

    m_librarian->clear();
    m_librarianEmail->clear();
    setEnabled(false);

    blockAllSignals(false);
    
}

void
MidiKeyMappingEditor::populate(QTreeWidgetItem* item)
{
    RG_DEBUG << "MidiKeyMappingEditor::populate\n";

    MidiKeyMapTreeWidgetItem *keyItem =
        dynamic_cast<MidiKeyMapTreeWidgetItem *>(item);
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
        RG_DEBUG << "WARNING: MidiKeyMappingEditor::reset: No such mapping as " << m_mappingName;
        return;
    }

    m_mapping = *m;

    
    blockAllSignals(true);

    // Librarian details
    //
    m_librarian->setText(strtoqstr(m_device->getLibrarianName()));
    m_librarianEmail->setText(strtoqstr(m_device->getLibrarianEmail()));

    // Clear each LineEdit and perform a more secure scan of mappings.
    for (unsigned int i = 0; i < (unsigned int)m_names.size(); i++) {
        m_names[i]->clear();

        for (MidiKeyMapping::KeyNameMap::const_iterator it =
                    m_mapping.getMap().begin();
                it != m_mapping.getMap().end(); ++it) {

            int index = it->first;

            if ( (int)i == index) {
                QString name = strtoqstr(it->second);
                m_completions << name;
                m_names[i]->setText(name);
                m_names[i]->setCursorPosition(0);
            }
        }
    }

    blockAllSignals(false);
}

void
MidiKeyMappingEditor::slotNameChanged(const QString& name)
{
    const LineEdit* lineEdit = dynamic_cast<const LineEdit*>(sender());
    if (!lineEdit) {
        RG_DEBUG << "MidiKeyMappingEditor::slotNameChanged() : %%% ERROR - signal sender is not a Rosegarden::LineEdit\n";
        return ;
    }

    QString senderName = sender()->objectName();

    // Adjust value back to zero rated
    //
    unsigned int pitch = senderName.toUInt() - 1;

    RG_DEBUG << "MidiKeyMappingEditor::slotNameChanged("
    << name << ") : pitch = " << pitch << endl;

    if (qstrtostr(name) != m_mapping.getMap()[pitch]) {
        m_mapping.getMap()[pitch] = qstrtostr(name);
        m_bankEditor->slotApply();
    }
}

void
MidiKeyMappingEditor::slotKeyMapButtonPressed()
{}

void MidiKeyMappingEditor::blockAllSignals(bool block)
{
    QList<LineEdit *> allChildren = findChildren<LineEdit*>((QRegExp)"[0-9]+");
    QList<LineEdit *>::iterator it;

    for (it = allChildren.begin(); it != allChildren.end(); ++it) {
        (*it)->blockSignals(block);
    }
}

}
