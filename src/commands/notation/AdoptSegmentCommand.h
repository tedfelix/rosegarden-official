/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2012 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_ADOPTSEGMENTCOMMAND_H
#define RG_ADOPTSEGMENTCOMMAND_H

#include "document/BasicCommand.h"

#include <QObject>

namespace Rosegarden
{

class Segment;
class NotationView;

// Needs to be a QObject so it can get a signal if view is destroyed.
class AdoptSegmentCommand : public QObject, public NamedCommand
{
    Q_OBJECT;

 public:
    AdoptSegmentCommand(QString name,
                        NotationView &view,
                        Segment *segment,
                        bool into = true,
                        bool inComposition = false);

    // Alternative constructor if segment does not exist at creation time
    AdoptSegmentCommand(QString name,
                        NotationView &view,
                        const QString& segmentMarking,
                        Composition* comp,
                        bool into = true,
                        bool inComposition = false);
    ~AdoptSegmentCommand() override;

protected:
    void execute() override;
    void unexecute() override;
    void adopt();
    void unadopt();

protected slots:
    void slotViewdestroyed();
    
 private:

    void requireSegment();

    NotationView &m_view;
    Segment   *m_segment;
    const bool m_into;
    bool       m_detached;
    bool       m_viewDestroyed;
    bool       m_inComposition;
    QString    m_segmentMarking;

    /// the composition
    Composition *m_comp;
};
 
}

#endif /* ifndef RG_ADOPTSEGMENTCOMMAND_H */
