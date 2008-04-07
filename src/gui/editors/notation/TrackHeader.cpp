/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2008
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    This file is Copyright 2007-2008
        Yves Guillemot      <yc.guillemot@wanadoo.fr> 

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/



#include "TrackHeader.h"
#include "HeadersGroup.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/StaffExportTypes.h"
#include "base/Colour.h"
#include "base/ColourMap.h"
#include "base/Track.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/LinedStaff.h"
#include "document/RosegardenGUIDoc.h"
#include "misc/Strings.h"
#include "NotePixmapFactory.h"
#include "NotationView.h"
#include "NotationStaff.h"

#include <map>
#include <set>
#include <string>
#include <utility>

#include <kapplication.h>
#include <klocale.h>
#include <qsize.h>
#include <qwidget.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qframe.h>
#include <qstring.h>
#include <qtooltip.h>


namespace Rosegarden
{


// Status bits
const int TrackHeader::SEGMENT_HERE                = 1 << 0;
const int TrackHeader::SUPERIMPOSED_SEGMENTS       = 1 << 1;
const int TrackHeader::INCONSISTENT_CLEFS          = 1 << 2;
const int TrackHeader::INCONSISTENT_KEYS           = 1 << 3;
const int TrackHeader::INCONSISTENT_LABELS         = 1 << 4;
const int TrackHeader::INCONSISTENT_TRANSPOSITIONS = 1 << 5;
const int TrackHeader::BEFORE_FIRST_SEGMENT        = 1 << 6;


TrackHeader::TrackHeader(QWidget *parent, TrackId trackId, int height, int ypos) :
        QLabel(parent),
        m_track(trackId),
        m_height(height),
        m_ypos(ypos),
        m_lastClef(Clef()),
        m_lastKey(Rosegarden::Key()),
        m_lastLabel(QString("")),
        m_lastTranspose(0),
        m_lastUpperText(QString("")),
        m_neverUpdated(true),
        m_isCurrent(true),
        m_lastStatusPart(0),
        m_lastWidth(0),
        m_key(Rosegarden::Key()),
        m_label(QString("")),
        m_transpose(0),
        m_status(0),
        m_current(false)
{

    m_notationView = static_cast<HeadersGroup *>(parent)->getNotationView();

    setFrameStyle(QFrame::Box | QFrame::Plain);
    setCurrent(false);


    //
    // Tooltip text creation

    Composition *comp = 
        static_cast<HeadersGroup *>(parent)->getComposition();
    Track *track = comp->getTrackById(m_track);
    int trackPos = comp->getTrackPositionById(m_track);

    QString toolTipText = QString(i18n("Track %1 : \"%2\"")
                            .arg(trackPos + 1)
                            .arg(strtoqstr(track->getLabel())));

    QString preset = track->getPresetLabel();
    if (preset != QString(""))
        toolTipText += QString(i18n("\nNotate for: %1").arg(preset));

    QString notationSize = i18n("normal");
    switch (track->getStaffSize()) {
        case StaffTypes::Small:
            notationSize = i18n("small");
            break;
        case StaffTypes::Tiny:
            notationSize = i18n("tiny");
            break;
    }

    QString bracketText = i18n("--");
    switch (track->getStaffBracket()) {
        case Brackets::SquareOn:
            bracketText = "[-";
            break;
        case Brackets::SquareOff:
            bracketText = "-]";
            break;
        case Brackets::SquareOnOff:
            bracketText = "[-]";
            break;
        case Brackets::CurlyOn:
            bracketText = "{-";
            break;
        case Brackets::CurlyOff:
            bracketText = "-}";
            break;
        case Brackets::CurlySquareOn:
            bracketText = "{[-";
            break;
        case Brackets::CurlySquareOff:
            bracketText = "-]}";
            break;
    }

    toolTipText += QString(i18n("\nSize: %1,  Bracket: %2 "))
                            .arg(notationSize)
                            .arg(bracketText);

    // Sort segments by position on the track
    SortedSegments segments;
    for (int i=0; i<m_notationView->getStaffCount(); i++) {

        NotationStaff * notationStaff = m_notationView->getNotationStaff(i);
        Segment &segment = notationStaff->getSegment();
        TrackId trackId = segment.getTrack();

        if (trackId  == m_track) {
            segments.insert(&segment);
        }
    }

    for (SortedSegments::iterator i=segments.begin(); i!=segments.end(); ++i) {
        timeT segStart = (*i)->getStartTime();
        timeT segEnd = (*i)->getEndMarkerTime();
        int barStart = comp->getBarNumber(segStart) + 1;
        int barEnd = comp->getBarNumber(segEnd) + 1;

        int transpose = (*i)->getTranspose();
        if (transpose) {
            QString transposeName;
            transposeValueToName(transpose, transposeName);
            toolTipText += QString(i18n("\nbars [%1-%2] in %3 (tr=%4) : \"%5\""))
                                    .arg(barStart)
                                    .arg(barEnd)
                                    .arg(transposeName)
                                    .arg(transpose)
                                    .arg(strtoqstr((*i)->getLabel()));
        } else {
            toolTipText += QString(i18n("\nbars [%1-%2] (tr=%3) : \"%4\""))
                                    .arg(barStart)
                                    .arg(barEnd)
                                    .arg(transpose)
                                    .arg(strtoqstr((*i)->getLabel()));
        }
    }

    QToolTip::add(this, toolTipText);

    m_firstSeg = *segments.begin();
    m_firstSegStartTime = m_firstSeg->getStartTime();

    /// This may not work if two segments are superimposed
    /// at the beginning of the track (inconsistencies are
    /// not detected).
    ///   TODO : Look for the first segment(s) in
    ///          lookAtStaff() and not here.
}

void
TrackHeader::setCurrent(bool current)
{
    /// TODO : use colours from GUIPalette

    if (current != m_isCurrent) {
        m_isCurrent = current;
        if (current) {
            setLineWidth(2);
            setMargin(0);
            setPaletteForegroundColor(QColor(0, 0, 255));
        } else {
            setLineWidth(1);
            setMargin(1);
            setPaletteForegroundColor(QColor(0, 0, 0));
        }
    }
}

void
TrackHeader::transposeValueToName(int transpose, QString &transposeName)
{

    /// TODO : Should be rewrited using methods from Pitch class

    int noteIndex = transpose % 12;
    if (noteIndex < 0) noteIndex += 12;

    switch(noteIndex) {
        case  0 : transposeName = i18n("C");  break;
        case  1 : transposeName = i18n("C#"); break;
        case  2 : transposeName = i18n("D");  break;
        case  3 : transposeName = i18n("Eb"); break;
        case  4 : transposeName = i18n("E");  break;
        case  5 : transposeName = i18n("F");  break;
        case  6 : transposeName = i18n("F#"); break;
        case  7 : transposeName = i18n("G");  break;
        case  8 : transposeName = i18n("G#"); break;
        case  9 : transposeName = i18n("A");  break;
        case 10 : transposeName = i18n("Bb"); break;
        case 11 : transposeName = i18n("B");  break;
    }
}

int
TrackHeader::lookAtStaff(double x, int maxWidth)
{
    // Read Clef and Key on canvas at (x, m_ypos + m_height / 2)
    // then guess the header needed width and return it

    // When walking through the segments :
    //    clef, key, label and transpose are current values
    //    clef0, key0, label0 and transpose0 are preceding values used to look
    //                                       for inconsistencies
    //    key1, label1 and transpose1 are "visible" (opposed at invisible as are
    //                                key=<C major>, label="" or transpose=0)
    //                                preceding or current values which may be
    //                                displayed with a red colour if some
    //                                inconsistency occurs.
    Clef clef, clef0;
    Rosegarden::Key key, key0, key1 = Rosegarden::Key("C major");
    QString label = QString(""), label0, label1 = QString("");
    int transpose = 0, transpose0, transpose1 = 0;

    int staff;

    Composition *comp = 
        static_cast<HeadersGroup *>(parent())->getComposition();
    Track *track = comp->getTrackById(m_track);
    int trackPos = comp->getTrackPositionById(m_track);

    int status = 0;
    bool current = false;
    for (int i=0; i<m_notationView->getStaffCount(); i++) {
        NotationStaff * notationStaff = m_notationView->getNotationStaff(i);
        Segment &segment = notationStaff->getSegment();
        TrackId trackId = segment.getTrack();
        if (trackId  == m_track) {

            /// TODO : What if a segment has been moved ???
            timeT xTime = notationStaff->getTimeAtCanvasCoords(x, m_ypos);
            if (xTime < m_firstSegStartTime) {
                status |= BEFORE_FIRST_SEGMENT;
                /// TODO : What if superimposed segments ???
                m_firstSeg->getFirstClefAndKey(clef, key);
                label = strtoqstr(m_firstSeg->getLabel());
                transpose = m_firstSeg->getTranspose();
                current = current || m_notationView->isCurrentStaff(i);
                break;
            }
            timeT segStart = segment.getStartTime();
            timeT segEnd = segment.getEndMarkerTime();
            current = current || m_notationView->isCurrentStaff(i);

            if ((xTime >= segStart) && (xTime < segEnd)) {

                notationStaff->getClefAndKeyAtCanvasCoords(x,
                                            m_ypos + m_height / 2, clef, key);
                label = strtoqstr(segment.getLabel());
                transpose = segment.getTranspose();

                if (status & SEGMENT_HERE) {
                    status |= SUPERIMPOSED_SEGMENTS;
                    if (clef != clef0)
                        status |= INCONSISTENT_CLEFS;
                    if (key != key0)
                        status |= INCONSISTENT_KEYS;
                    if (label != label0)
                        status |= INCONSISTENT_LABELS;
                    if (transpose != transpose0)
                        status |= INCONSISTENT_TRANSPOSITIONS;
                } else {
                    status |= SEGMENT_HERE;
                }

                staff = i;

                // If current value is visible, remember it
                if (key.getAccidentalCount()) key1 = key;
                if (label.stripWhiteSpace().length()) label1 = label;
                if (transpose) transpose1 = transpose;

                // Current values become last values
                clef0 = clef;
                key0 = key;
                label0 = label;
                transpose0 = transpose;
            }                                                // if(xTime...)
        }                                                // if(trackId...)
    }

    // Remember current data (but only visible data if inconsistency)
    m_clef = clef;
    m_key = (status & INCONSISTENT_KEYS) ? key1 : key;
    m_label = (status & INCONSISTENT_LABELS) ? label1 : label;
    m_transpose = (status & INCONSISTENT_TRANSPOSITIONS) ? transpose1 : transpose;
    m_current = current;
    m_status = status;

    QString noteName;
    transposeValueToName(m_transpose, noteName);

    m_upperText = QString(i18n("%1: %2")
                                .arg(trackPos + 1)
                                .arg(strtoqstr(track->getLabel())));
    if (m_transpose) m_transposeText = i18n(" in %1").arg(noteName);
    else             m_transposeText = QString("");

    NotePixmapFactory * npf = m_notationView->getNotePixmapFactory();
    int clefAndKeyWidth = npf->getClefAndKeyWidth(m_key, m_clef);

    // How many text lines may be written above or under the clef
    // in track header ?
    m_numberOfTextLines = npf->getTrackHeaderNTL(m_height);

    int trackLabelWidth =
            npf->getTrackHeaderTextWidth(m_upperText + m_transposeText)
                                                        / m_numberOfTextLines;
    int segmentNameWidth =
            npf->getTrackHeaderTextWidth(m_label) / m_numberOfTextLines;

    // Get the max. width from upper text and lower text
    int width = (segmentNameWidth > trackLabelWidth)
                            ? segmentNameWidth : trackLabelWidth;

    // Text width is limited by max header Width
    if (width > maxWidth) width = maxWidth;

    // But clef and key width may override max header width
    if (width < clefAndKeyWidth) width = clefAndKeyWidth;

    return width;
}



void
TrackHeader::updateHeader(int width)
{

    // Update the header (using given width) if necessary

    // Filter out bits whose display doesn't depend from
    int statusPart = m_status & ~(SUPERIMPOSED_SEGMENTS);

    // Header should be updated only if necessary
    if (    m_neverUpdated
         || (width != m_lastWidth)
         || (statusPart != m_lastStatusPart)
         || (m_key != m_lastKey)
         || (m_clef != m_lastClef)
         || (m_label != m_lastLabel)
         || (m_upperText != m_lastUpperText)
         || (m_transpose != m_lastTranspose)) {

        m_neverUpdated = false;
        m_lastStatusPart = statusPart;
        m_lastKey = m_key;
        m_lastClef = m_clef;
        m_lastLabel = m_label;
        m_lastTranspose = m_transpose;
        m_lastUpperText = m_upperText;

        bool drawClef = true;
        QColor clefColour;
        if (m_status & (SEGMENT_HERE | BEFORE_FIRST_SEGMENT)) {
            if (m_status & (INCONSISTENT_CLEFS | INCONSISTENT_KEYS))
                clefColour = Qt::red;
            else
                clefColour = Qt::black;
        } else {
            drawClef = false;
        }

        NotePixmapFactory * npf = m_notationView->getNotePixmapFactory();
        QPixmap pmap = NotePixmapFactory::toQPixmap(
                  npf->makeTrackHeaderPixmap(width, m_height, this));

        setPixmap(pmap);
        setFixedWidth(width);

        // Forced width may differ from localy computed width
        m_lastWidth = width;
    }

    // Highlight header if track is the current one
    setCurrent(m_current);
}

bool
TrackHeader::SegmentCmp::operator()(const Segment * s1, const Segment * s2) const
{
    // Sort segments by start time, then by end time
    if (s1->getStartTime() < s2->getStartTime()) return true;
    if (s1->getStartTime() > s2->getStartTime()) return false;
    if (s1->getEndMarkerTime() < s2->getEndMarkerTime()) return true;
    return false;
}

}
#include "TrackHeader.moc"
