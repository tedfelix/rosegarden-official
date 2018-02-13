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

#ifndef RG_MIDIKEYMAPPINGEDITOR_H
#define RG_MIDIKEYMAPPINGEDITOR_H

#include "base/MidiProgram.h"
#include "NameSetEditor.h"
#include <string>


class QWidget;
class QString;
class QTreeWidgetItem;


namespace Rosegarden
{

class MidiDevice;
class BankEditorDialog;


class MidiKeyMappingEditor : public NameSetEditor
{
    Q_OBJECT

public:
    MidiKeyMappingEditor(BankEditorDialog *bankEditor,
                         QWidget *parent);

    void clearAll();
    void populate(QTreeWidgetItem *);
    MidiKeyMapping &getMapping() { return m_mapping; }
    void reset();

public slots:
    virtual void slotNameChanged(const QString &);
    virtual void slotKeyMapButtonPressed();

protected:
    virtual QWidget *makeAdditionalWidget(QWidget *parent);
    void blockAllSignals(bool block);

    //--------------- Data members ---------------------------------

    MidiDevice *m_device;
    std::string m_mappingName;
    MidiKeyMapping m_mapping;
};


}

#endif
