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

#ifndef RG_AUDIOLISTITEM_H
#define RG_AUDIOLISTITEM_H

#include <sound/AudioFile.h>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace Rosegarden
{

class Segment;

// Add an Id to a QTreeWidgetItem
//
class AudioListItem : public QTreeWidgetItem
{

public:
    

    
    //    AudioListItem(QTreeWidget *parent):QTreeWidgetItem(), m_segment(0) {;}
    AudioListItem( const QStringList &strings, int type = Type ) :QTreeWidgetItem(strings, type), m_segment(0) {  }
    
    // without m_id
    AudioListItem( QTreeWidget *parent, const QStringList &strings, int type=Type ) :QTreeWidgetItem(parent, strings, type), m_segment(0) {  }
    
    AudioListItem( AudioListItem *parent, const QStringList &strings, int type=Type ) :QTreeWidgetItem(parent, strings, type), m_segment(0) {  }
    
    
    // with m_id
    AudioListItem( QTreeWidget *parent, const QStringList &strings, AudioFileId id, int type=Type ) :QTreeWidgetItem(parent, strings, type ), m_id(id), m_segment(0) {;}
    
    AudioListItem( AudioListItem *parent, const QStringList &strings, AudioFileId id, int type=Type ) :QTreeWidgetItem(parent, strings, type ), m_id(id), m_segment(0) {;}
    
    
    
    
/*
    AudioListItem(QTreeWidget *parent,
                  QString label,
                  AudioFileId id):
                      QTreeWidgetItem(parent,
                                    label,
                                    "", "", "", "", "", "", ""),
                                    m_id(id),
                                    m_segment(0) {;}

    AudioListItem(QTreeWidgetItem *parent, 
                  QString label,
                  AudioFileId id):
                      QTreeWidgetItem(parent,
                                    label,
                                    "", "", "", "", "", "", ""),
                                    m_id(id),
                                    m_segment(0) {;}

*/
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
