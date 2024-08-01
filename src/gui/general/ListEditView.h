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

class QFrame;
class QWidget;
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
    /**
     * Create an EditViewBase for the segments \a segments from document \a doc.
     *
     * \arg cols : number of columns, main column is always rightmost (really?)
     *
     */
    ListEditView(const std::vector<Segment *> &segments,
                 unsigned int cols);
    ~ListEditView() override;

protected:

    QFrame *getCentralWidget()  { return m_centralFrame; }

    /// Create actions for menus and toolbars.
    void setupActions(const QString &rcFileName, bool haveClipboard);

    QString getRCFileName() const  { return m_rcFileName; }

    QGridLayout *getGridLayout()  { return m_grid; }

    /**
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
    virtual void refreshSegment(Segment *segment,
                                timeT startTime = 0,
                                timeT endTime = 0) = 0;

    virtual void addCommandToHistory(Command *);

private:

    QGridLayout *m_grid;

    bool isCompositionModified();
    void setCompositionModified(bool modified);

    // ??? Try to get rid of this per explanation above.
    void paintEvent(QPaintEvent *e) override;

    //void setRCFileName(const QString &s)  { m_rcFileName = s; }

    QString m_rcFileName;

    static std::set<int> m_viewNumberPool;
    std::string makeViewLocalPropertyPrefix();
    int m_viewNumber;
    std::string m_viewLocalPropertyPrefix;

    std::vector<unsigned int> m_segmentsRefreshStatusIds;
    void initSegmentRefreshStatusIds();

    QFrame      *m_centralFrame;

    unsigned int m_mainCol;
    unsigned int m_compositionRefreshStatusId;
    bool         m_needUpdate;

    QPaintEvent *m_pendingPaintEvent;
    bool         m_havePendingPaintEvent;
    static bool  m_inPaintEvent; // true if _any_ edit view is in a paint event

    bool         m_inCtor;

    EditViewTimeSigNotifier *m_timeSigNotifier;
};


}


#endif
