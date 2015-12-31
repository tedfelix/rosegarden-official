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


#ifndef RG_GUITARCHORDSELECTORDIALOG_H
#define RG_GUITARCHORDSELECTORDIALOG_H

#include "Chord.h"
#include "ChordMap.h"

#include <QDialog>
#include <QString>
#include <vector>

class QListWidget;
class QListWidgetItem;
class QComboBox;
class QPushButton;

namespace Rosegarden
{

class FingeringBox;

class GuitarChordSelectorDialog : public QDialog
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

    /** Overloaded function to allow activation both through int-based and
     * QListWidgetItem*-based signals.  This version takes an int in the form of
     * a row coordinate, and obtains the correct QListWidgetItem* to pass into
     * the version below, which does the real work.
     */
    void slotFingeringHighlighted(int);

    /** Overloaded function to allow activation both through int-based and
     * QListWidgetItem*-based signals.  This version takes a QListWidgetItem*
     * and does real work accordingly.
     */
    void slotFingeringHighlighted(QListWidgetItem*);

    void slotComplexityChanged(int);
    
    void slotNewFingering();
    void slotDeleteFingering();
    void slotEditFingering();

    virtual void accept();

protected:

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
         
    /// Unbundle chords.xml then return a path to its location in userspace.
    //
    // User edits will be saved to the user's local, writeable copy of
    // chords.xml, rather than to a separate user_chords.xml as was the case in
    // Rosegarden Classic.  This simplifies troubleshooting at the expense of
    // losing the ability to drop random .xml files into the chord dictionary
    // path and have them picked up.  (Nobody ever did that as far as I know,
    // but if people complain, we can work something out.  I think it will be
    // transparent to everyone, and if not, we'll hear about it!)
    QString getChordFile();

    Guitar::ChordMap m_chordMap;

    /// current selected chord
    Guitar::Chord m_chord;
    
    // Chord data
    QListWidget* m_rootNotesList;
    QListWidget* m_chordExtList;
    QListWidget* m_fingeringsList;
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

#endif /*RG_GUITARCHORDSELECTORDIALOG_H*/
