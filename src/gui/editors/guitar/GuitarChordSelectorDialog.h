/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

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
class QComboBox;
class QPushButton;

namespace Rosegarden
{

class FingeringBox;

class GuitarChordSelectorDialog : public KDialogBase
{
     Q_OBJECT
    
    enum { COMPLEXITY_BEGINNER, COMPLEXITY_COMMON, COMPLEXITY_ALL };
    
public:
	GuitarChordSelectorDialog(QWidget *parent=0);

    void init();

    const Guitar::Chord& getChord() const { return m_chord; }

    void setChord(const Guitar::Chord&);

protected slots:
    void slotRootHighlighted(int);
    void slotChordExtHighlighted(int);
    void slotFingeringHighlighted(QListBoxItem*);
    void slotComplexityChanged(int);
    
    void slotNewFingering();
    void slotDeleteFingering();
    void slotEditFingering();

    virtual void slotOk();

protected:

    void parseChordFiles(const std::vector<QString>& chordFiles);
    void parseChordFile(const QString& chordFileName);
    void populateFingerings(const Guitar::ChordMap::chordarray&, const Guitar::Fingering& refFingering=Guitar::Fingering(0));
    void populateExtensions(const QStringList& extList);

    /// set enabled state of edit/delete buttons
    void setEditionEnabled(bool);
    
    void populate();
    void clear();
    void refresh();
    
    bool saveUserChordMap();
    int evaluateChordComplexity(const QString& ext);
    
    QPixmap getFingeringPixmap(const Guitar::Fingering& fingering) const;
         
    /// Find all chord list files on the system
    std::vector<QString> getAvailableChordFiles();

    Guitar::ChordMap m_chordMap;

    /// current selected chord
    Guitar::Chord m_chord;
    
    // Chord data
    QListBox* m_rootNotesList;
    QListBox* m_chordExtList;
    QListBox* m_fingeringsList;
    FingeringBox* m_fingeringBox;

    QComboBox*   m_chordComplexityCombo;
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
