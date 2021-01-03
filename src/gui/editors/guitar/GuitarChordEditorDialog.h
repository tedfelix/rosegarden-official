/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2021 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_GUITARCHORDEDITORDIALOG_H
#define RG_GUITARCHORDEDITORDIALOG_H

#include <QDialog>

#include "Chord.h"
#include "ChordMap.h"

class QComboBox;
class QSpinBox;

namespace Rosegarden
{

class FingeringBox;


class GuitarChordEditorDialog : public QDialog
{
    Q_OBJECT
    
public:
	GuitarChordEditorDialog(Guitar::Chord&, const Guitar::ChordMap& chordMap, QWidget *parent=nullptr);

protected slots:
    void slotStartFretChanged(int);
    void accept() override;
    
protected:

    void populateExtensions(const QStringList&);

    FingeringBox* m_fingeringBox;
    QComboBox* m_rootNotesList;
    QSpinBox* m_startFret;
    QComboBox* m_ext;
    Guitar::Chord& m_chord;
    const Guitar::ChordMap& m_chordMap;    
};

}

#endif /*RG_GUITARCHORDEDITORDIALOG_H*/
