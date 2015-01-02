
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_PASTETOTRIGGERSEGMENTCOMMAND_H
#define RG_PASTETOTRIGGERSEGMENTCOMMAND_H

#include "base/TriggerSegment.h"
#include "document/Command.h"
#include <QString>

#include <QCoreApplication>


namespace Rosegarden
{

class Composition;
class Clipboard;
class Segment;
class EventSelection;

 class PasteToTriggerSegmentWorker
{
public:
    /// If basePitch is -1, the first pitch in the selection will be used
    PasteToTriggerSegmentWorker
      (Composition *composition,
       // This takes ownership of clipboard.
       // OK to pass NULL clipboard
       Clipboard *clipboard,
       // The label given to the ornament.
       QString label,
       int basePitch = -1,
       int baseVelocity = -1);

    PasteToTriggerSegmentWorker
      (Composition *composition,
       const EventSelection * selection,
       // The label given to the ornament.
       QString label,
       int basePitch = -1,
       int baseVelocity = -1);
    
    ~PasteToTriggerSegmentWorker();

    void execute();
    void unexecute();

    // Since TriggerSegmentId is unsigned, we can't "x<0" it to check
    // if it's valid.  So we resort to a hack: since m_segment gets a
    // non-null value at the same time, we check for that.
    bool hasTriggerSegmentId(void)
    { return m_segment != 0; }
    TriggerSegmentId getTriggerSegmentId(void)
    { return m_id; }
    int getBasePitch(void)
    { return m_basePitch;}
    int getBaseVelocity(void)
    { return m_baseVelocity;}
    
protected:
    Composition *m_composition;
    Clipboard *m_clipboard;
    QString m_label;
    int m_basePitch;
    int m_baseVelocity;
    Segment *m_segment;
    TriggerSegmentId m_id;
    bool m_detached;
};

class PasteToTriggerSegmentCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::PasteToTriggerSegmentCommand)

public:
    /// If basePitch is -1, the first pitch in the selection will be used
    PasteToTriggerSegmentCommand(Composition *composition,
                                 Clipboard *clipboard,
                                 QString label,
                                 int basePitch = -1,
                                 int baseVelocity = -1);
    virtual ~PasteToTriggerSegmentCommand();

    virtual void execute();
    virtual void unexecute();

protected:
    PasteToTriggerSegmentWorker m_worker;
};



}

#endif
