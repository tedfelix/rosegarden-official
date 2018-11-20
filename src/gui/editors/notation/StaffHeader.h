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

#ifndef RG_STAFFHEADER_H
#define RG_STAFFHEADER_H

#include "base/NotationTypes.h"
#include "base/Track.h"
#include "base/Overlaps.h"
#include "base/Segment.h"
#include "NotePixmapFactory.h"

#include <QSize>
#include <QWidget>
#include <QLabel>

#include <set>

class QLabel;
class QPixmap;
class QGraphicsPixmapItem;
class QString;
class QToolButton;
class QTimer;
class QMouseEvent;


namespace Rosegarden
{

class HeadersGroup;
class NotationScene;
class ColourMap;
class Segment;
class Clef;
class Key;

template<class T> class Inconsistencies;


// Class rename from TrackHeader to StaffHeader since paintEvent() method
// has been added : Qt disliked to have two different StaffHeader::paintEvent()
// method.

class StaffHeader : public QWidget, public SegmentObserver
{
    Q_OBJECT
public:
    /**
     * Create a new track header for track of id trackId.
     * *parent is the parent widget, height the height of staff and
     * ypos is the staff y position on canvas.
     */
    StaffHeader(HeadersGroup *group, TrackId trackId, int height, int ypos);

    ~StaffHeader();

    /**
     * Examine staff at x position and gather data needed to draw
     * the track header.
     * Return the minimum width required to display the track header.
     * maxWidth is the maximum width allowed to show text. Return width
     * may be greater than maxWidth if needed to show clef and key signature.
     * (Header have always to show complete clef and key signature).
     */
    int lookAtStaff(double x, int maxWidth);

    /**
     * (Re)draw the header on the notation view using the data gathered
     * by lookAtStaff() last call and the specified width.
     */
    void updateHeader(int width);

    /**
     * Return the Id of the associated track.
     */
    TrackId getId()
    { return m_track;
    }

    /**
     * Return how many text lines may be written in the header (above
     * the clef and under the clef).
     * This data is coming from the last call of lookAtStaff().
     */
    int getNumberOfTextLines() { return m_numberOfTextLines; }

    /**
     * Return the Clef to draw in the header
     */
    Clef & getClef() { return m_clef; }

    /**
     * Get which key signature should be drawn in the header
     * from the last call of lookAtStaff().
     */
    Rosegarden::Key & getKey() { return m_key; }

    /**
     * Return true if a Clef (and a key signature) have to be drawn in the header
     */
    bool isAClefToDraw()
    {
        return (m_status & SEGMENT_HERE) || (m_status & BEFORE_FIRST_SEGMENT);
    }

    /**
     * Return the text to write in the header top
     */
    QString getUpperText() { return m_upperText; }

    /**
     * Return the transposition text
     * (to be written at the end of the upper text)
     */
    QString getTransposeText() { return m_transposeText; }

    /**
     * Return the text to write in the header bottom
     */
    QString getLowerText() { return m_label; }

    /**
     * Return true if two segments or more are superimposed and are
     * not using the same clef
     */
    bool isClefInconsistent() { return m_status & INCONSISTENT_CLEFS; }

    /**
     * Return true if two segments or more are superimposed and are
     * not using the same key signature
     */
    bool isKeyInconsistent() { return m_status & INCONSISTENT_KEYS; }

    /**
     * Return true if two segments or more are superimposed and are
     * not using the same label
     */
    bool isLabelInconsistent() { return m_status & INCONSISTENT_LABELS; }

    /**
     * Return true if two segments or more are superimposed and are
     * not using the same transposition
     */
    bool isTransposeInconsistent() 
    {
        return m_status & INCONSISTENT_TRANSPOSITIONS;
    }

    // Method made static to reuse it from notation/Inconsistencies.h
    // TODO : Should be move in a better place (may be something
    //        like gui/general/Translation.cpp)
    /**
     * Convert the transpose value to the instrument tune and
     * return it in a printable string.
     */
    static QString transposeValueToName(int transpose);


    /// YG: Only for debug
    void dumpSegs();


/** SegmentObserver methods **/
    void eventAdded(const Segment *, Event *) override;

    void eventRemoved(const Segment *, Event *) override;

    void appearanceChanged(const Segment *) override;

    void startChanged(const Segment *, timeT) override;

    void endMarkerTimeChanged(const Segment *, bool /*shorten*/) override;

    void transposeChanged(const Segment *, int) override;

    void segmentDeleted(const Segment *) override;




signals :
    void showToolTip(QString toolTipText);
    void staffModified();

protected :
    void paintEvent(QPaintEvent *) override;
//    virtual bool event(QEvent *event);
    void enterEvent(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
//    virtual void mousePressEvent(QMouseEvent *event);

    /**
     * Look if the current segment should be shown in the header.
     * If it does, set the related members accordingly and return true,
     * else do nothing and return false.
     */
    bool setCurrentSegmentVisible();

protected slots :
    void slotToolTip();

    /**
     * Look if the staff is the current one and consequently draw or remove
     * a blue line around the header.
     * (intended to highlight the "current" track).
     */
    void slotSetCurrent();

    void slotShowInconsistencies();

private :
    // Status bits
    static const int SEGMENT_HERE;
    static const int SUPERIMPOSED_SEGMENTS;
    static const int INCONSISTENT_CLEFS;
    static const int INCONSISTENT_KEYS;
    static const int INCONSISTENT_LABELS;
    static const int INCONSISTENT_TRANSPOSITIONS;
    static const int INCONSISTENT_COLOURS;
    static const int BEFORE_FIRST_SEGMENT;

    HeadersGroup *m_headersGroup;
    TrackId m_track;
    int m_height;
    int m_ypos;
    NotationScene *m_scene;

    Clef m_lastClef;
    Rosegarden::Key m_lastKey;
    QString m_lastLabel;
    int m_lastTranspose;
    QString m_lastUpperText;
    bool m_neverUpdated;
    int m_lastStatusPart;
    int m_lastWidth;

    Clef m_clef;
    Rosegarden::Key m_key;
    QString m_label;
    int m_transpose;
    int m_status;
    bool m_trackIsCurrent;
    bool m_segmentIsCurrent;

    QString m_upperText;
    QString m_transposeText;
    int m_numberOfTextLines;

    // Used to sort the segments listed in the header toolTipText
    struct SegmentCmp {
        bool operator()(const Segment *s1, const Segment *s2) const;
    };
    typedef std::multiset<Segment *, StaffHeader::SegmentCmp> SortedSegments;
    
    SortedSegments m_segments;

    // First segment on the track.
    Segment *m_firstSeg;
    timeT m_firstSegStartTime;

    QGraphicsPixmapItem *m_clefItem;
    QGraphicsPixmapItem *m_keyItem;
    int m_lineSpacing;
    int m_maxDelta;
    int m_staffLineThickness;

    QColor m_foreground;
    QColor m_background;
    NotePixmapFactory::ColourType m_foregroundType;

    QString m_toolTipText;
    QString m_warningToolTipText;
    QPoint m_cursorPos;
    QTimer *m_toolTipTimer;
    int m_toolTipCount;

    unsigned int m_colourIndex;
    unsigned int m_lastColourIndex;

    QToolButton *m_clefOrKeyInconsistency;

    Inconsistencies<int> *m_transposeOverlaps;
    Inconsistencies<Clef> *m_clefOverlaps;
    Inconsistencies<Key> *m_keyOverlaps;

    bool m_clefOrKeyIsInconsistent;
    bool m_clefOrKeyWasInconsistent;
    bool m_transposeIsInconsistent;
    bool m_transposeWasInconsistent;

    bool m_noSegment; // Sometimes, when segments are moved, we can have
                      // a staff without any segment.
};

}

#endif
