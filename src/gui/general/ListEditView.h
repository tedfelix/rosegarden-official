/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_LISTEDITVIEW_H
#define RG_LISTEDITVIEW_H

#include "EditViewBase.h"


namespace Rosegarden
{


class Segment;


/// Base class for EventView and TempoView.
/**
 * ??? Get rid of this.
 */
class ListEditView : public EditViewBase
{
    Q_OBJECT

public:

    ListEditView(const std::vector<Segment *> &segments);

protected:

    virtual void refreshList() = 0;

private slots:

    /// Connected to RosegardenDocument::documentModified().
    /**
     * Push down to derivers.  It's only a few lines of code.
     */
    void slotDocumentModified(bool modified);

};


}


#endif
