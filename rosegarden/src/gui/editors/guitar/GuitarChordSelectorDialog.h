/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2007
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


#ifndef _RG_GUITARCHORDSELECTORDIALOG_H_
#define _RG_GUITARCHORDSELECTORDIALOG_H_

#include "Chord.h"
#include "ChordMap.h"

#include <kdialogbase.h>
#include <qstring.h>
#include <vector>

class QListBox;
class QListBoxItem;
class QPushButton;

namespace Rosegarden
{

class FingeringBox;

class GuitarChordSelectorDialog : public KDialogBase
{
     Q_OBJECT
    
public:
	GuitarChordSelectorDialog(QWidget *parent=0);

    void init();

    const Guitar::Chord& getChord() const { return m_chord; }

    void setChord(const Guitar::Chord&);

protected slots:
    void slotRootHighlighted(int);
    void slotChordExtHighlighted(int);
    void slotFingeringHighlighted(QListBoxItem*);
    
    void slotNewFingering();
    void slotDeleteFingering();
    void slotEditFingering();

    virtual void slotOk();

protected:

    void parseChordFiles(const std::vector<QString>& chordFiles);
    void parseChordFile(const QString& chordFileName);
    void populateFingerings(const Guitar::ChordMap::chordarray&);
    void populateExtensions(const QStringList& extList);

    void populate();
    void clear();
    void refresh();
    
    bool saveUserChordMap();
    
    QPixmap getFingeringPixmap(const Guitar::Fingering& fingering) const;
         
    //! Find all chord list files on the system
    std::vector<QString> getAvailableChordFiles();

    //! List of Chords
    Guitar::ChordMap m_chordMap;

    /// selected chord
    Guitar::Chord m_chord;
    
    // Chord data
    QListBox* m_rootNotesList;
    QListBox* m_chordExtList;
    QListBox* m_fingeringsList;
    FingeringBox* m_fingeringBox;

    QPushButton* m_newFingeringButton;
    QPushButton* m_deleteFingeringButton;
    QPushButton* m_editFingeringButton;    

    static const unsigned int FINGERING_PIXMAP_HEIGHT = 75;
    static const unsigned int FINGERING_PIXMAP_WIDTH = 75;
    static const unsigned int FINGERING_PIXMAP_H_MARGIN = 5;
    static const unsigned int FINGERING_PIXMAP_W_MARGIN = 5;
    
};

}

#endif /*_RG_GUITARCHORDSELECTORDIALOG_H_*/
