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

#ifndef _RG_AUDIOLISTITEM_H_
#define _RG_AUDIOLISTITEM_H_

#include <sound/AudioFile.h>
#include <QListView>

namespace Rosegarden
{

class Segment;

// Add an Id to a QListViewItem
//
class AudioListItem : public QListViewItem
{

public:

    AudioListItem(QListView *parent):QListViewItem(parent),
                                     m_segment(0) {;}

    AudioListItem(QListViewItem *parent):QListViewItem(parent),
                                         m_segment(0) {;}

    AudioListItem(QListView *parent,
                  QString label,
                  AudioFileId id):
                      QListViewItem(parent,
                                    label,
                                    "", "", "", "", "", "", ""),
                                    m_id(id),
                                    m_segment(0) {;}

    AudioListItem(QListViewItem *parent, 
                  QString label,
                  AudioFileId id):
                      QListViewItem(parent,
                                    label,
                                    "", "", "", "", "", "", ""),
                                    m_id(id),
                                    m_segment(0) {;}


    AudioFileId getId() { return m_id; }

    void setStartTime(const RealTime &time)
        { m_startTime = time; }
    RealTime getStartTime() { return m_startTime; }

    void setDuration(const RealTime &time)
        { m_duration = time; }
    RealTime getDuration() { return m_duration; }

    void setSegment(Segment *segment)
        { m_segment = segment; }
    Segment *getSegment() { return m_segment; }

protected:
    AudioFileId m_id;

    // for audio segments
    RealTime m_startTime;
    RealTime m_duration;

    // pointer to a segment
    Segment *m_segment;

};

}


#endif /*AUDIOLISTITEM_H_*/
