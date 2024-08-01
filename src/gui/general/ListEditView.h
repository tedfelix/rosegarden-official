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

#include <QString>

class QFrame;
class QPaintEvent;
class QGridLayout;


namespace Rosegarden
{


class EditViewTimeSigNotifier;


/// Base class for EventView and TempoView.
/**
 * ListEditView is a subclass of EditViewBase that contains some
 * functionality used by "old-style" EditView implementations but not
 * required by the new QGraphicsView-based notation and matrix views.
 *
 * The main thing this code implements is a refresh-on-repaint
 * mechanism through paintEvent.  This mechanism is much too complex
 * to be really justified for the only classes that use it now -- it
 * was original designed for more complex views such as the old
 * notation view -- but it's retained more or less verbatim "for
 * historical reasons" awaiting a possible future refactoring.
 *
 * Apart from that, it also provides some widget layout skeleton code,
 * and a few utility methods.
 *
 * Currently the subclasses of this class are EventView and TempoView,
 * which is why it is named ListEditView -- because it's used by list
 * edit views, not because it provides anything particularly focused
 * on lists.
 */
class ListEditView : public EditViewBase
{
    Q_OBJECT

public:

    ListEditView(const std::vector<Segment *> &segments);
    ~ListEditView() override;

protected:

    QFrame *getFrame()  { return m_frame; }

    /// Create actions for menus and toolbars.
    // ??? This is far too trivial to pull up.  Inline into callers.
    void setupActions(bool haveClipboard);

    QGridLayout *getGridLayout()  { return m_gridLayout; }

    /**
     * ??? This is part of that paintEvent() thing.  REMOVE THIS.
     *     EventView refreshes its tree widget.
     *     TempoView refreshes the entire list.
     *
     * Refresh part of a Segment following a modification made in this
     * or another view.  The startTime and endTime give the extents of
     * the modified region.  This method is called following a
     * modification to any Segment; no attempt has been made to check
     * that the given Segment is actually shown in this view, so take
     * care.
     *
     * If segment is null, refresh all segments.
     * If the startTime and endTime are equal, refresh the whole of
     * the relevant segments.
     */
    virtual void refreshList() = 0;

    /// ??? This is a one-line wrapper.  Inline into all callers.
    static void addCommandToHistory(Command *);

private:

    // ??? This is far too trivial to pull up.  Push down into derivers.
    QFrame *m_frame;
    // ??? This is far too trivial to pull up.  Push down into derivers.
    /// Layout within m_frame.
    QGridLayout *m_gridLayout;

    // *** paintEvent() Related
    // ??? We need to try to get rid of all this.  Or at least re-org so
    //     that it is all handled via document modified handlers.

    /// ??? Inline into only caller.  paintEvent() is going away.
    bool isCompositionModified();
    /// ??? Inline into only caller.  paintEvent() is going away.
    void setCompositionModified(bool modified);
    unsigned int m_compositionRefreshStatusId;
    // ??? Try to get rid of this per explanation above.
    void paintEvent(QPaintEvent *e) override;

    // ??? paintEvent() related.
    std::vector<unsigned int> m_segmentsRefreshStatusIds;

    // ??? paintEvent() related.
    EditViewTimeSigNotifier *m_timeSigNotifier;
};


}


#endif
