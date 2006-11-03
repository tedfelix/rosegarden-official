// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.
 
    This program is Copyright 2000-2005
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>
 
    The moral right of the authors to claim authorship of this work
    has been asserted.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


// This class was not actually used in the previous RG tree
#ifdef NOT_DEFINED

#ifndef GUITAR_TAB_EDITOR_H_
#define GUITAR_TAB_EDITOR_H_

#include "Fingering.h"
#include "ChordMap.h"
//#include "GuitarTabBase.h"
#include <kmainwindow.h>
#include <kstatusbar.h>
#include <qvbox.h>
#include <kdialog.h>

namespace Rosegarden
{
    class Event;

class GuitarTabEditorWindow : public KDialog
{
    Q_OBJECT

public:

    //! Constructor
    GuitarTabEditorWindow(QWidget *parent);

    //! Destructor
    ~GuitarTabEditorWindow();

    void init (void);

    Guitar::Fingering getArrangement (void) const;

    void setArrangement (Guitar::Fingering& chord);

signals:

    //! Display a given chord fingering
    void displayChord (Guitar::Fingering*);

    //! Alert other objects to this window closing
    void closing();

    //! Display user message
    void statusInfo (QString const&);

public slots:

    //! Begining displaying the given chord to the QPainter object
    void slotDisplayChord ();

    //! Update Modifier list with interval values from the given scale
    void slotSetupModifierList (int index);

    //! Update Suffix list with distance values from the given interval
    void slotSetupSuffixList (int index);

    //! Update Version list with version values from the given distance
    void slotSetupVersionList (int index);

    //! Save chord map to user chord file
    void slotSaveChords ();

    //! Load chord map from user chord file
    void slotLoadChords ();

private:

    //! Create editor components and display them via the QPainter object
    void populate (void);

    //! Base data for editor
    GuitarTabBase* m_data;

    //! Fingering constructor object
    Guitar::FingeringConstructor* m_fingering;
    QTabWidget* m_chord;

};

}

#endif /* GUITAR_TAB_EDITOR_H_ */

#endif
