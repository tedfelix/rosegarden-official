/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.

    This file is Copyright 2007-2009
        Yves Guillemot      <yc.guillemot@wanadoo.fr> 

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_HEADERSGROUP_H
#define RG_HEADERSGROUP_H

#include "base/NotationTypes.h"
#include "base/Track.h"

#include <vector>
#include <QSize>
#include <QWidget>
#include <QVBoxLayout>

class QLabel;
class QResizeEvent;


namespace Rosegarden
{


class NotationWidget;
class NotationScene;
class Composition;
class StaffHeader;
class RosegardenDocument;
class Segment;


class HeadersGroup : public QWidget
{
    Q_OBJECT
public:
    /**
     * Create an empty headers group
     */
    HeadersGroup(RosegardenDocument *document);
    
    ~HeadersGroup();

    void removeAllHeaders();

    void addHeader(int trackId, int height, int ypos, double xcur);

    void setTracks(NotationWidget *widget, NotationScene *scene);

    /**
     * Resize a filler at bottom of group to set the headersGroup height
     * to the value specified in parameter.
     * (Used to give to the headers group exactly the same height as the
     * canvas. Necessary to get synchronous vertical scroll.) 
     */
    void completeToHeight(int height);

    NotationWidget * getNotationWidget()
    { return m_widget;
    }

    Composition *getComposition()
    { return &m_composition;
    }

    /**
     * Return the total height of all the headers (without the filler).
     */
    int getUsedHeight()
    { return m_usedHeight;
    }

    /**
     * Highlight as "current" the header of the specified track.
     */
    int getWidth()
    {
        return m_lastWidth;
    }

    typedef enum { ShowNever, ShowWhenNeeded, ShowAlways } ShowHeadersModeType;

    // Used to ensure to have one default value and only one.
    static const ShowHeadersModeType DefaultShowMode = ShowWhenNeeded;

    // Useful in configuration dialog.
    static bool isValidShowMode(int mode)
    {
        return ((mode >= ShowNever) && (mode <= ShowAlways));
    }

    NotationScene *getScene();


    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // Only return valid values after slotUpdateAllHeaders() has been called
    timeT getStartOfViewTime() { return m_startOfView; }
    timeT getEndOfViewTime() { return m_endOfView; }

    bool timeIsInCurrentSegment(timeT t);

    TrackId getCurrentTrackId() { return m_currentTrackId; }

    Segment *getCurrentSegment() { return m_currentSegment; }




public slots :
    /**
     * Called when notation view moves.
     * Arg x is the scene X coord of the left of the view.
     * Setting force to true forces the headers to be redrawn even 
     * if x has not changed since the last call.
     */
    void slotUpdateAllHeaders(int x, bool force = false);

    void slotSetCurrentSegment();

signals :
    void headersResized(int newWidth);
    void currentSegmentChanged();

private :
    Composition &m_composition;
    NotationScene *m_scene;
    NotationWidget *m_widget;

    typedef std::vector<StaffHeader *> TrackHeaderVector;
    TrackHeaderVector m_headers;

    int m_usedHeight;
    QLabel *m_filler;
    int m_lastX;
    int m_lastWidth;

    QVBoxLayout *m_layout;

    timeT m_startOfView;
    timeT m_endOfView;

    Segment *m_currentSegment;
    timeT m_currentSegStartTime;
    timeT m_currentSegEndTime;
    TrackId m_currentTrackId;
};


}

#endif
