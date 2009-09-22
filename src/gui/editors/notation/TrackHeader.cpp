/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

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

#include "TrackHeader.h"
#include "HeadersGroup.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/StaffExportTypes.h"
#include "base/Colour.h"
#include "base/ColourMap.h"
#include "base/Track.h"
#include "gui/general/GUIPalette.h"
#include "document/RosegardenDocument.h"
#include "misc/Strings.h"
#include "NotePixmapFactory.h"
#include "NotationScene.h"
#include "NotationStaff.h"

#include <map>
#include <set>
#include <string>
#include <utility>

#include <QApplication>
#include <QSize>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QString>
#include <QGraphicsPixmapItem>
#include <QBitmap>
#include <QEvent>

#include <QPainter>


namespace Rosegarden
{


// Status bits
const int StaffHeader::SEGMENT_HERE                = 1 << 0;
const int StaffHeader::SUPERIMPOSED_SEGMENTS       = 1 << 1;
const int StaffHeader::INCONSISTENT_CLEFS          = 1 << 2;
const int StaffHeader::INCONSISTENT_KEYS           = 1 << 3;
const int StaffHeader::INCONSISTENT_LABELS         = 1 << 4;
const int StaffHeader::INCONSISTENT_TRANSPOSITIONS = 1 << 5;
const int StaffHeader::INCONSISTENT_COLOURS        = 1 << 6;
const int StaffHeader::BEFORE_FIRST_SEGMENT        = 1 << 7;


StaffHeader::StaffHeader(HeadersGroup *group,
                         TrackId trackId, int height, int ypos) :
        QWidget(0),
        m_headersGroup(group),
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
        m_current(false),
        m_clefItem(0),
        m_keyItem(0),
        m_foreGround(Qt::white),
        m_backGround(Qt::black),
        m_toolTipText(QString("")),
        m_colourIndex(0)

{
    // localStyle (search key)
    //
    // If/when we have the option to override the stylesheet, we should tweak
    // m_foreGround and m_backGround, which are currently hard coded to assume
    // the stylesheet is in effect.  This may or may not make it into the first
    // release.

    m_scene = m_headersGroup->getScene();
    setCurrent(false); 


    //
    // Tooltip text creation

    Composition *comp = m_headersGroup->getComposition();
    Track *track = comp->getTrackById(m_track);
    int trackPos = comp->getTrackPositionById(m_track);

    QString toolTipText = QString(tr("Track %1 : \"%2\"")
                             .arg(trackPos + 1)
                             .arg(strtoqstr(track->getLabel())));

    QString preset = strtoqstr(track->getPresetLabel());
    if (preset != QString(""))
        toolTipText += QString(tr("<br>Notate for: %1").arg(preset));

    QString notationSize = tr("normal");
    switch (track->getStaffSize()) {
        case StaffTypes::Small:
            notationSize = tr("small");
            break;
        case StaffTypes::Tiny:
            notationSize = tr("tiny");
            break;
    }

    QString bracketText = "--";
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

    toolTipText += QString(tr("<br>Size: %1,  Bracket: %2 "))
                            .arg(notationSize)
                            .arg(bracketText);

    // Sort segments by position on the track
    SortedSegments segments;
    std::vector<NotationStaff *> *staffs = m_scene->getStaffs();
    for (size_t i = 0; i < staffs->size(); i++) {

        NotationStaff *notationStaff = (*staffs)[i];
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
            toolTipText += QString(tr("<br>bars [%1-%2] in %3 (tr=%4) : \"%5\""))
                                    .arg(barStart)
                                    .arg(barEnd)
                                    .arg(transposeName)
                                    .arg(transpose)
                                    .arg(strtoqstr((*i)->getLabel()));
        } else {
            toolTipText += QString(tr("<br>bars [%1-%2] (tr=%3) : \"%4\""))
                                    .arg(barStart)
                                    .arg(barEnd)
                                    .arg(transpose)
                                    .arg(strtoqstr((*i)->getLabel()));
        }
    }

    // not translated to spare the translators the pointless effort of copying
    // and pasting this tag for every language we support
    m_toolTipText = "<qt><p>" + toolTipText + "</p></qt>";

    // With Qt3  "this->setToolTip(m_toolTipText);"  would have been
    // called here.
    // This is no more well working with the new Qt4 RG implementation because
    // the StaffHeader widget is now embedded in a QGraphicsScene :
    //   - The tool tip would be clipped by the QGraphicsView
    //   - The tool tip would be zoomed whith the scene.
    //
    // Now the showToolTip signal, carrying the tool tip text, is emitted
    // from the staff header main event handler when a tool tip event occurs.
    // This signal is connected to NotationWidget::slotShowHeaderToolTip()
    // from the headers group. The notation widget displays the tool tip,
    // without clipping nor resizing it, when it receives this signal.

    m_firstSeg = *segments.begin();
    m_firstSegStartTime = m_firstSeg->getStartTime();

    /// This may not work if two segments are superimposed
    /// at the beginning of the track (inconsistencies are
    /// not detected).
    ///   TODO : Look for the first segment(s) in
    ///          lookAtStaff() and not here.

}

StaffHeader::~StaffHeader()
{
    delete m_clefItem;
    delete m_keyItem;
}

void
StaffHeader::paintEvent(QPaintEvent *)
{
    // avoid common random crash by brute force
    if (!m_clefItem) {
        std::cerr << "StaffHeader::paintEvent() - m_clefItem was NULL.  Skipping this paintEvent." << std::endl
                  << "                            This is a BUG, but it seems fairly harmless to just avoid crashing here and return." << std::endl
                  << std::endl
                  << "                            See you next iteration!"  << std::endl;

        return;
    }
        
    QPainter paint(this);
    paint.fillRect(0, 0, width(), height(), m_backGround);  /// ???

    int wHeight = height();
    int wWidth = width();

    wHeight -= 4;    // Make room to the label frame :
                     // 4 = 2 * (margin + lineWidth)

    int lw = m_lineSpacing;
    int h;
    QColor colour;
    int maxDelta = m_maxDelta;

    // Staff Y position inside the whole header
    int offset = (wHeight - 10 * lw -1) / 2;

    // Draw staff lines
    paint.setPen(QPen(QColor(m_foreGround), m_staffLineThickness));
    for (h = 0; h <= 8; h += 2) {
        int y = (lw * 3) + ((8 - h) * lw) / 2;
        paint.drawLine(maxDelta/2, y + offset, wWidth - maxDelta/2, y + offset);
    }

    if (isAClefToDraw()) {

        // Draw clef
        QPixmap clefPixmap = m_clefItem->pixmap();

        h = m_clef.getAxisHeight();
        int y = (lw * 3) + ((8 - h) * lw) / 2;
        paint.drawPixmap(maxDelta, y + m_clefItem->offset().y() + offset, clefPixmap);

        // Draw key
        QPixmap keyPixmap = m_keyItem->pixmap();
        y = lw;   /// Why ???
        paint.drawPixmap((maxDelta * 3) / 2+ clefPixmap.width(),
                         y + m_keyItem->offset().y() + offset, keyPixmap);

    }


    NotePixmapFactory * npf = m_scene->getNotePixmapFactory();
    paint.setFont(npf->getTrackHeaderFont());

    QString text;
    QString textLine;

    int charHeight = npf->getTrackHeaderFontMetrics().height();
    int charWidth = npf->getTrackHeaderFontMetrics().maxWidth();

    const QString transposeText = getTransposeText();
    QRect bounds = npf->getTrackHeaderBoldFontMetrics()
                                   .boundingRect(transposeText);
    int transposeWidth = bounds.width();


    // Write upper text (track name and track label)

    int numberOfTextLines = getNumberOfTextLines();

//    if (isTransposeInconsistent()) {
        int upperTextY = charHeight
                         + numberOfTextLines * npf->getTrackHeaderTextLineSpacing();
//        QColor warningBackGround = m_foreGround == Qt::white ?
//                                                       Qt::black : Qt::white;
//        paint.fillRect(0, 0, width(), upperTextY, warningBackGround);

        // an experiment: fill the upper and lower text areas with 80% opaque
        // black regardless of other color concerns        
        paint.fillRect(0, 0, width(), upperTextY, QColor(0, 0, 0, 200));
//    }

//    paint.setPen(QColor(m_foreGround));
    // always use white as default foreground for text areas    
    QColor textForeground = Qt::white;
    paint.setPen(textForeground);
    text = getUpperText();

    for (int l=1; l<=numberOfTextLines; l++) {
        int upperTextY = charHeight
                         + (l - 1) * npf->getTrackHeaderTextLineSpacing();
        if (l == numberOfTextLines) {
            int transposeSpace = transposeWidth ? transposeWidth + charWidth / 4 : 0;
            textLine = npf->getOneLine(text, m_lastWidth - transposeSpace - charWidth / 2);
            if (!text.isEmpty()) {
                // String too long : cut it and replace last character with dots
                int len = textLine.length();
                if (len > 1) textLine.replace(len - 1, 1, tr("..."));
            }
        } else {
            textLine = npf->getOneLine(text, m_lastWidth - charWidth / 2);
        }
        if (textLine.isEmpty()) break;
        paint.drawText(charWidth / 4, upperTextY, textLine);
    }


    // Write transposition text

    // TODO : use colours from GUIPalette
//    colour = isTransposeInconsistent() ? QColor(Qt::red) : QColor(m_foreGround);
    colour = isTransposeInconsistent() ? QColor(Qt::red) : QColor(textForeground);
    paint.setFont(npf->getTrackHeaderBoldFont());
     // m_p->maskPainter().setFont(m_trackHeaderBoldFont);
    paint.setPen(colour);

    paint.drawText(m_lastWidth - transposeWidth - charWidth / 4,
                   charHeight + (numberOfTextLines - 1)
                                      * npf->getTrackHeaderTextLineSpacing(),
                   transposeText);


     // Write lower text (segment label)

//    if (isLabelInconsistent()) {
        int lowerTextY = wHeight - 4            // -4 : adjust
            - numberOfTextLines * npf->getTrackHeaderTextLineSpacing();
//        QColor warningBackGround = m_foreGround == Qt::white ? Qt::black : Qt::white;
//        paint.fillRect(0, lowerTextY, width(), height(), warningBackGround);

        // experiment        
        paint.fillRect(0, lowerTextY, width(), height(), QColor(0, 0, 0, 200));
//    }

    // TODO : use colours from GUIPalette
//    colour = isLabelInconsistent() ? QColor(Qt::red) : QColor(m_foreGround);
    colour = isLabelInconsistent() ? QColor(Qt::red) : QColor(textForeground);
    paint.setFont(npf->getTrackHeaderFont());

    paint.setPen(colour);
    text = getLowerText();

    for (int l=1; l<=numberOfTextLines; l++) {
        int lowerTextY = wHeight - 4            // -4 : adjust
            - (numberOfTextLines - l) * npf->getTrackHeaderTextLineSpacing();

        QString textLine = npf->getOneLine(text, m_lastWidth - charWidth / 2);
        if (textLine.isEmpty()) break;

        if ((l == numberOfTextLines)  && !text.isEmpty()) {
                // String too long : cut it and replace last character by dots
                int len = textLine.length();
                if (len > 1) textLine.replace(len - 1, 1, tr("..."));
        }

        paint.drawText(charWidth / 4, lowerTextY, textLine);
    }

}


void
StaffHeader::setCurrent(bool current)
{
    /// TODO : use colours from GUIPalette

//     if (current != m_isCurrent) {
//         m_isCurrent = current;
//         if (current) {
//             setLineWidth(2);
//             setMargin(0);
//             setPaletteForegroundColor(QColor(0, 0, 255));
//         } else {
//             setLineWidth(1);
//             setMargin(1);
//             setPaletteForegroundColor(QColor(0, 0, 0));
//         }
//     }
}

void
StaffHeader::transposeValueToName(int transpose, QString &transposeName)
{

    /// TODO : Should be rewrited using methods from Pitch class

    int noteIndex = transpose % 12;
    if (noteIndex < 0) noteIndex += 12;

    switch(noteIndex) {
        case  0 : transposeName = tr("C");  break;
        case  1 : transposeName = tr("C#"); break;
        case  2 : transposeName = tr("D");  break;
        case  3 : transposeName = tr("Eb"); break;
        case  4 : transposeName = tr("E");  break;
        case  5 : transposeName = tr("F");  break;
        case  6 : transposeName = tr("F#"); break;
        case  7 : transposeName = tr("G");  break;
        case  8 : transposeName = tr("G#"); break;
        case  9 : transposeName = tr("A");  break;
        case 10 : transposeName = tr("Bb"); break;
        case 11 : transposeName = tr("B");  break;
    }
}

int
StaffHeader::lookAtStaff(double x, int maxWidth)
{
    // Read Clef and Key on scene at (x, m_ypos + m_height / 2)
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
    int transpose = 0, transpose0 = 0, transpose1 = 0;
    unsigned int colourIndex = 0, colourIndex0 = 0;

    size_t staff;

    Composition *comp = m_headersGroup->getComposition();
    Track *track = comp->getTrackById(m_track);
    int trackPos = comp->getTrackPositionById(m_track);
    std::vector<NotationStaff *> *staffs = m_scene->getStaffs();

    int status = 0;
    bool current = false;
    for (size_t i = 0; i < staffs->size(); i++) {
        NotationStaff *notationStaff = (*staffs)[i];
        Segment &segment = notationStaff->getSegment();
        TrackId trackId = segment.getTrack();
        if (trackId  == m_track) {

            /// TODO : What if a segment has been moved ???
            timeT xTime = notationStaff->getTimeAtSceneCoords(x, m_ypos);
            if (xTime < m_firstSegStartTime) {
                status |= BEFORE_FIRST_SEGMENT;
                /// TODO : What if superimposed segments ???
                m_firstSeg->getFirstClefAndKey(clef, key);
                label = strtoqstr(m_firstSeg->getLabel());
                transpose = m_firstSeg->getTranspose();
                colourIndex = m_firstSeg->getColourIndex();
//!!!  getCurrentStaffNumber() doesn't exist still
//!!!                current = current || (m_scene->getCurrentStaffNumber() == i);
                break;
            }
            timeT segStart = segment.getStartTime();
            timeT segEnd = segment.getEndMarkerTime();
//!!!  getCurrentStaffNumber() doesn't exist still
//!!!            current = current || (m_scene->getCurrentStaffNumber() == i);

            if ((xTime >= segStart) && (xTime < segEnd)) {

                notationStaff->getClefAndKeyAtSceneCoords(x,
                                            m_ypos + m_height / 2, clef, key);
                label = strtoqstr(segment.getLabel());
                transpose = segment.getTranspose();
                colourIndex = segment.getColourIndex();

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
                    if (colourIndex != colourIndex0)
                        status |= INCONSISTENT_COLOURS;
                } else {
                    status |= SEGMENT_HERE;
                }

                staff = i;

                // If current value is visible, remember it
                if (key.getAccidentalCount()) key1 = key;
                if (label.trimmed().length()) label1 = label;
                if (transpose) transpose1 = transpose;

                // Current values become last values
                clef0 = clef;
                key0 = key;
                label0 = label;
                transpose0 = transpose;
                colourIndex0 = colourIndex;
            }                                                // if(xTime...)
        }                                                // if(trackId...)
    }

    // Remember current data (but only visible data if inconsistency)
    m_clef = clef;
    m_key = (status & INCONSISTENT_KEYS) ? key1 : key;
    m_label = (status & INCONSISTENT_LABELS) ? label1 : label;
    m_transpose = (status & INCONSISTENT_TRANSPOSITIONS) ? transpose1 : transpose;
    m_current = current;
    m_colourIndex = colourIndex;
    m_status = status;

    QString noteName;
    transposeValueToName(m_transpose, noteName);

    m_upperText = QString(tr("%1: %2")
                                 .arg(trackPos + 1)
                                 .arg(strtoqstr(track->getLabel())));
    if (m_transpose) m_transposeText = tr(" in %1").arg(noteName);
    else             m_transposeText = QString("");

    NotePixmapFactory * npf = m_scene->getNotePixmapFactory();
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
StaffHeader::updateHeader(int width)
{
    if (!m_headersGroup->isVisible()) return;

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
        //QColor clefColour;
        NotePixmapFactory::ColourType colourType = NotePixmapFactory::PlainColour;

        if (m_status & (SEGMENT_HERE | BEFORE_FIRST_SEGMENT)) {

        } else {
            drawClef = false;   ///!!! drawClef is never used
        }

        NotePixmapFactory * npf = m_scene->getNotePixmapFactory();

        // Get background colour from colour index
        m_backGround = GUIPalette::convertColour(m_headersGroup->getComposition()
                            ->getSegmentColourMap().getColourByIndex(m_colourIndex));

        // Select foreground colour (black or white) to get the better
        // visibility
        int intensity = qGray(m_backGround.rgb());
        if (intensity > 127) {
            m_foreGround = Qt::black;
            m_foreGroundType = NotePixmapFactory::PlainColour;
        } else {
            m_foreGround = Qt::white;
            m_foreGroundType = NotePixmapFactory::PlainColourLight;
        }

        colourType = (m_status & INCONSISTENT_CLEFS) ?
                         NotePixmapFactory::ConflictColour : m_foreGroundType;
        delete m_clefItem;
        m_clefItem = npf->makeClef(m_clef, colourType);

        colourType = (m_status & INCONSISTENT_KEYS) ?
                         NotePixmapFactory::ConflictColour : m_foreGroundType;
        delete m_keyItem;
        m_keyItem = npf->makeKey(m_key, m_clef, Rosegarden::Key("C major"), colourType); 

        m_lineSpacing = npf->getLineSpacing();
        m_maxDelta = npf->getAccidentalWidth(Accidentals::Sharp);
        m_staffLineThickness = npf->getStaffLineThickness();

        setFixedWidth(width);
        setFixedHeight(m_height);

        // Forced width may differ from localy computed width
        m_lastWidth = width;
    }

    // Highlight header if track is the current one
    setCurrent(m_current);

    update();
}

bool
StaffHeader::SegmentCmp::operator()(const Segment * s1, const Segment * s2) const
{
    // Sort segments by start time, then by end time
    if (s1->getStartTime() < s2->getStartTime()) return true;
    if (s1->getStartTime() > s2->getStartTime()) return false;
    if (s1->getEndMarkerTime() < s2->getEndMarkerTime()) return true;
    return false;
}

bool
StaffHeader::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        emit(showToolTip(m_toolTipText));
        return true;
    }
    return QWidget::event(event);
}

}
#include "TrackHeader.moc"

