/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

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

#include "StaffHeader.h"
#include "HeadersGroup.h"
#include "base/Composition.h"
#include "base/NotationTypes.h"
#include "base/StaffExportTypes.h"
#include "base/Colour.h"
#include "base/ColourMap.h"
#include "base/Track.h"
#include "base/Overlaps.h"
#include "gui/application/RosegardenMainWindow.h"
#include "gui/general/GUIPalette.h"
#include "document/RosegardenDocument.h"
#include "misc/Strings.h"
#include "misc/Debug.h"
#include "NotePixmapFactory.h"
#include "NotationScene.h"
#include "NotationStaff.h"

#include "Inconsistencies.h"

#include <vector>
#include <map>
#include <set>
#include <string>
#include <utility>

#include <QApplication>
#include <QSize>
#include <QWidget>
#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QFrame>
#include <QString>
#include <QGraphicsPixmapItem>
#include <QBitmap>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QTextEdit>


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
        m_lastStatusPart(0),
        m_lastWidth(0),
        m_clef(Clef()),
        m_key(Rosegarden::Key()),
        m_label(QString("")),
        m_transpose(0),
        m_status(0),
        m_trackIsCurrent(false),
        m_segmentIsCurrent(false),
        m_upperText(QString("")),
        m_transposeText(QString("")),
        m_numberOfTextLines(0),
        m_firstSeg(0),
        m_firstSegStartTime(0),
        m_clefItem(0),
        m_keyItem(0),
        m_lineSpacing(0),
        m_maxDelta(0),
        m_staffLineThickness(0),
        m_foreground(Qt::white),
        m_background(Qt::black),
        m_toolTipText(QString("")),
        m_warningToolTipText(QString("")),
        m_cursorPos(QPoint()),
        m_toolTipTimer(0),
        m_toolTipCount(0),
        m_colourIndex(0),
        m_lastColourIndex(0),
        m_clefOrKeyInconsistency(0),
        m_transposeOverlaps(0),
        m_clefOverlaps(0),
        m_keyOverlaps(0),
        m_clefOrKeyIsInconsistent(false),
        m_clefOrKeyWasInconsistent(false),
        m_transposeIsInconsistent(false),
        m_transposeWasInconsistent(false),
        m_noSegment(false)
{
    // localStyle (search key)
    //
    // If/when we have the option to override the stylesheet, we should tweak
    // m_foreground and m_background, which are currently hard coded to assume
    // the stylesheet is in effect.  This may or may not make it into the first
    // release.

    m_scene = m_headersGroup->getScene();

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
    m_segments.clear();
    std::vector<NotationStaff *> *staffs = m_scene->getStaffs();
    for (size_t i = 0; i < staffs->size(); i++) {

        NotationStaff *notationStaff = (*staffs)[i];
        Segment &segment = notationStaff->getSegment();
        TrackId trackId = segment.getTrack();

        if (trackId  == m_track) {
            m_segments.insert(&segment);
        }
    }

    for (SortedSegments::iterator i=m_segments.begin();
                                     i!=m_segments.end(); ++i) {
        timeT segStart = (*i)->getStartTime();
        timeT segEnd = (*i)->getEndMarkerTime();
        int barStart = comp->getBarNumber(segStart) + 1;
        int barEnd = comp->getBarNumber(segEnd) + 1;

        int transpose = (*i)->getTranspose();
        if (transpose) {
            QString transposeName = transposeValueToName(transpose);
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

    m_firstSeg = *m_segments.begin();
    if (m_firstSeg == *m_segments.end()) {
        RG_WARNING << "No segments on this track";
        m_noSegment = true;
        return;
    } else {
        m_noSegment = false;
    }
    m_firstSegStartTime = m_firstSeg->getStartTime();

    /// This may not work if two segments are superimposed
    /// at the beginning of the track (inconsistencies are
    /// not detected).
    ///   TODO : Look for the first segment(s) in
    ///          lookAtStaff() and not here.


    // Create buttons to warn/inform on inconsistencies in overlapping segments
    int size = 4 * m_scene->getNotePixmapFactory()->getSize();
    m_clefOrKeyInconsistency = new QToolButton;
    m_clefOrKeyInconsistency->setIcon(QIcon(":/pixmaps/misc/inconsistency.png"));
    m_clefOrKeyInconsistency->setIconSize(QSize(size, size));
    // Don't call setToolTip directly but use the local tooltip implementation
    m_warningToolTipText = tr("<qt><p>Notation is not consistent</p>"
                              "<p>Click to get more information</p></qt>");
    // Needed to see mouseMoveEvent inside this QToolButton widget from
    // the current StaffHeader widget and to know when to show the tooltip
    m_clefOrKeyInconsistency->setMouseTracking(true);

    // Add a layout where to place the warning icon
    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(m_clefOrKeyInconsistency);
    setLayout(hbox);

    // Icon is hidden
    m_clefOrKeyInconsistency->hide();

    // Connect the warning icon to a show popup slot
    connect(m_clefOrKeyInconsistency, SIGNAL(clicked()),
            this, SLOT(slotShowInconsistencies()));


    // Implement a ToolTip event replacement (see enterEvent(), leaveEvent and
    // mouseMoveEvent()).
    m_toolTipTimer = new QTimer(this);
    connect(m_toolTipTimer, SIGNAL(timeout()), this, SLOT(slotToolTip()));
    m_toolTipTimer->setSingleShot(false);
    m_toolTipTimer->setInterval(200);  // 0.2 s
    setMouseTracking(true);

    connect(m_headersGroup, SIGNAL(currentSegmentChanged()),
            this, SLOT(slotSetCurrent()));

            
    // Create three objects where to find possible inconsistencies
    // when segments overlap

    // Create a vector to pass all the segments of a track to the Overlaps ctors
    // and, in the same time, set the header as observer of these segments.
    std::vector<Segment *> segVec;
    for (SortedSegments::iterator i=m_segments.begin();
                                      i!=m_segments.end(); ++i) {
        segVec.push_back(*i);
        (*i)->addObserver(this);
    }

    m_clefOverlaps = new Inconsistencies<Clef>(segVec);
    m_keyOverlaps = new Inconsistencies<Key>(segVec);
    m_transposeOverlaps = new Inconsistencies<int>(segVec);

    // Look for the initial current track
/// TODO : LOOK FOR CURRENT SEGMENT
    m_trackIsCurrent = m_headersGroup->getCurrentTrackId() == m_track;
}

StaffHeader::~StaffHeader()
{
    if (m_noSegment) return;   /// OK here ?

    delete m_toolTipTimer;
    delete m_clefItem;
    delete m_keyItem;

    delete m_clefOverlaps;
    delete m_keyOverlaps;
    delete m_transposeOverlaps;
    
    for (SortedSegments::iterator i=m_segments.begin();
                                      i!=m_segments.end(); ++i) {
        (*i)->removeObserver(this);
    }
}

void
StaffHeader::paintEvent(QPaintEvent *)
{
    if (m_noSegment) return;

    // avoid common random crash by brute force
    if (!m_clefItem) {
        RG_WARNING << "StaffHeader::paintEvent() - m_clefItem was NULL.\n"
                   << "  Skipping this paintEvent to avoid a crash.\n"
                   << "  This is a BUG which should no longer occur. (rev 11137)";
        return;
    }
        
    QPainter paint(this);
    paint.fillRect(0, 0, width(), height(), m_background);  /// ???

    int wHeight = height();
    int wWidth = width();

    wHeight -= 4;    // Make room to the label frame :
                     // 4 = 2 * (margin + lineWidth)
wWidth -= 4; /// ???

    int lw = m_lineSpacing;
    int h;
    QColor colour;
    int maxDelta = m_maxDelta;

    // Staff Y position inside the whole header
    int offset = (wHeight - 10 * lw -1) / 2;

    // Draw staff lines
    paint.setPen(QPen(QColor(m_foreground), m_staffLineThickness));
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


    // Limit between upper text and central part
    // int upperTextY = charHeight
    //               + numberOfTextLines * npf->getTrackHeaderTextLineSpacing();

    paint.setPen(m_foreground);
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
    colour = QColor(m_foreground);;
    paint.setFont(npf->getTrackHeaderBoldFont());
    paint.setPen(colour);

    paint.drawText(m_lastWidth - transposeWidth - charWidth / 4,
                   charHeight + (numberOfTextLines - 1)
                                      * npf->getTrackHeaderTextLineSpacing(),
                   transposeText);


    // Write lower text (segment label)

    // Limit between upper text and central part
    // int lowerTextY = wHeight - 4            // -4 : adjust
    //               - numberOfTextLines * npf->getTrackHeaderTextLineSpacing();

    // Draw top and bottom separation line
    paint.setPen(QColor(m_foreground));
    paint.drawLine(0, 0, width(), 0);
    paint.drawLine(0, height(), width(), height());

    // TODO : use colours from GUIPalette
    colour = QColor(m_foreground);
    paint.setFont(npf->getTrackHeaderFont());
    paint.setPen(colour);
    text = isLabelInconsistent() ? QString("???????") : getLowerText();

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

    // Draw a blue rectangle around the header if staff is the current one
    if (m_trackIsCurrent) {
        QPen pen;

        // Select a visible color depending of the background intensity
        if (m_foreground == Qt::black) {
            pen.setColor(QColor(0, 0, 255));       // Blue
        } else {
            pen.setColor(QColor(170, 170, 255));   // Lighter blue
        }

        // Draw the rectangle
        // TODO : Use strokePath() rather than drawLine()
        pen.setWidth(4);
        paint.setPen(pen);
        paint.drawLine(1, 1, wWidth - 1, 1);
        paint.drawLine(wWidth - 1, 1, wWidth - 1, height() - 1);
        paint.drawLine(wWidth - 1, height() - 1, 1, height() - 1);
        paint.drawLine(1, height() - 1, 1, 1);
    }
}

void
StaffHeader::slotSetCurrent()
{
    m_trackIsCurrent = m_headersGroup->getCurrentTrackId() == m_track;
    if (m_trackIsCurrent && setCurrentSegmentVisible()) {
        m_neverUpdated = true;  /// Hack to force updateHeader() working
        updateHeader(m_lastWidth);   // Show the selected segment and track
    } else {
        update();                    // Show or hide the blue rectangle
    }
}

bool
StaffHeader::setCurrentSegmentVisible()
{
    if (!m_trackIsCurrent) return false;

    if (m_status & BEFORE_FIRST_SEGMENT) {
        m_segmentIsCurrent = m_headersGroup
                             ->timeIsInCurrentSegment(m_firstSegStartTime);
    } else {
        m_segmentIsCurrent = m_headersGroup
                             ->timeIsInCurrentSegment(m_headersGroup
                                                      ->getStartOfViewTime());
    }

    if (m_segmentIsCurrent) {
        Segment *s = m_headersGroup->getCurrentSegment();
        m_label = strtoqstr(s->getLabel());
        m_transpose = s->getTranspose();
        m_colourIndex = s->getColourIndex();
        return true;
    }

    return false;
}

QString
StaffHeader::transposeValueToName(int transpose)
{

    /// TODO : Should be rewritten using methods from Pitch class

    int noteIndex = transpose % 12;
    if (noteIndex < 0) noteIndex += 12;

    switch(noteIndex) {
        case  0 : return tr("C",  "note name");
        case  1 : return tr("C#", "note name");
        case  2 : return tr("D",  "note name");
        case  3 : return tr("Eb", "note name");
        case  4 : return tr("E",  "note name");
        case  5 : return tr("F",  "note name");
        case  6 : return tr("F#", "note name");
        case  7 : return tr("G",  "note name");
        case  8 : return tr("G#", "note name");
        case  9 : return tr("A",  "note name");
        case 10 : return tr("Bb", "note name");
        case 11 : return tr("B",  "note name");
    }

    return QString("???");   // Only here to remove compiler warning
}

int
StaffHeader::lookAtStaff(double x, int maxWidth)
{
    // Read Clef and Key on scene at (x, m_ypos + m_height / 2)
    // then guess the header needed width and return it

    if (m_noSegment) return 0;

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

    // size_t staff;

    Composition *comp = m_headersGroup->getComposition();
    Track *track = comp->getTrackById(m_track);
    int trackPos = comp->getTrackPositionById(m_track);
    std::vector<NotationStaff *> *staffs = m_scene->getStaffs();

    int status = 0;
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
                break;
            }
            timeT segStart = segment.getStartTime();
            timeT segEnd = segment.getEndMarkerTime();

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

                // staff = i;

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
    m_status = status;
    m_clef = clef;
    m_key = (status & INCONSISTENT_KEYS) ? key1 : key;
    if (!setCurrentSegmentVisible()) {
        m_label = (status & INCONSISTENT_LABELS) ? label1 : label;
        m_transpose = (status & INCONSISTENT_TRANSPOSITIONS) ? transpose1 : transpose;
        m_colourIndex = colourIndex;
    }

/// Try to show current segment label rather than ??? when segment is current.
/// May be not a so good idea...
    QString noteName = isTransposeInconsistent() && !m_segmentIsCurrent ?
                           QString("???") : transposeValueToName(m_transpose);

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

    // Is clef or key inconsistent somewhere in the current view ?
    timeT start = m_headersGroup->getStartOfViewTime();
    timeT end = m_headersGroup->getEndOfViewTime();
    m_clefOrKeyIsInconsistent = !m_clefOverlaps->isConsistent(start, end)
        || !m_keyOverlaps->isConsistent(start, end);

    // Is transposition inconsistent somewhere in the current view ?
    m_transposeIsInconsistent = !m_transposeOverlaps->isConsistent(start, end);

    return width;
}


void
StaffHeader::updateHeader(int width)
{
    // Update the header (using given width) if necessary

    if (m_noSegment) return;

    // updateHeader() must be executed the first time it is called
    // (ie when m_neverUpdated is true) as it initialized some data
    // later used by paintEvent()).
    // But to execute it when the headers are not visible is useless.
    if (!m_headersGroup->isVisible() && !m_neverUpdated) return;

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
         || (m_transpose != m_lastTranspose)
         || (m_colourIndex != m_lastColourIndex)
         || (m_clefOrKeyIsInconsistent != m_clefOrKeyWasInconsistent)
         || (m_transposeIsInconsistent != m_transposeWasInconsistent)) {

        m_neverUpdated = false;
        m_lastStatusPart = statusPart;
        m_lastKey = m_key;
        m_lastClef = m_clef;
        m_lastLabel = m_label;
        m_lastTranspose = m_transpose;
        m_lastColourIndex = m_colourIndex;
        m_lastUpperText = m_upperText;
        m_clefOrKeyWasInconsistent = m_clefOrKeyIsInconsistent;
        m_transposeWasInconsistent = m_transposeIsInconsistent;
        

        NotePixmapFactory * npf = m_scene->getNotePixmapFactory();

        // Get background colour from colour index
        m_background = GUIPalette::convertColour(m_headersGroup->getComposition()
                           ->getSegmentColourMap()
                               .getColourByIndex(m_colourIndex));

        // Select foreground colour (black or white) to get the better
        // visibility
        int intensity = qGray(m_background.rgb());
        if (intensity > 127) {
            m_foreground = Qt::black;
            m_foregroundType = NotePixmapFactory::PlainColour;
        } else {
            m_foreground = Qt::white;
            m_foregroundType = NotePixmapFactory::PlainColourLight;
        }
        
        // Fix staff header variant of bug #2997311 (Part 1)
        bool selectedMode = npf->isSelected();
        npf->setSelected(false);
        bool shadedMode = npf->isShaded();
        npf->setShaded(false);

        delete m_clefItem;
        m_clefItem = npf->makeClef(m_clef, m_foregroundType);

        delete m_keyItem;
        m_keyItem = npf->makeKey(m_key, m_clef,
                                 Rosegarden::Key("C major"), m_foregroundType);

        // Fix staff header variant of bug #2997311 (Part 2)
        npf->setSelected(selectedMode);
        npf->setShaded(shadedMode);

        m_lineSpacing = npf->getLineSpacing();
        m_maxDelta = npf->getAccidentalWidth(Accidentals::Sharp);
        m_staffLineThickness = npf->getStaffLineThickness();

        setFixedWidth(width);
        setFixedHeight(m_height);

        // Forced width may differ from localy computed width
        m_lastWidth = width;

        // Show or hide the warning icons
        if (m_clefOrKeyIsInconsistent || m_transposeIsInconsistent) {
            m_clefOrKeyInconsistency->show();
        } else { 
            m_clefOrKeyInconsistency->hide();
        }
    }

    update();
}


bool
StaffHeader::SegmentCmp::operator()(const Segment * s1, const Segment * s2) const
{
    // Sort segments by start time, then by end time, then by address.
    // The last comparison garantees two segments will never be equals and
    // allows to remove easily one of them from the m_segments multiset.
    // (Now, a set may replace the multiset.) 
    if (s1->getStartTime() < s2->getStartTime()) return true;
    if (s1->getStartTime() > s2->getStartTime()) return false;
    if (s1->getEndMarkerTime() < s2->getEndMarkerTime()) return true;
    if (s1->getEndMarkerTime() > s2->getEndMarkerTime()) return false;
    return (long) s1 < (long) s2;
}



// bool
// StaffHeader::event(QEvent *event)
// {
//     if (event->type() == QEvent::ToolTip) {
//         emit(showToolTip(m_toolTipText));
//         return true;
//     }
// 
//     return QWidget::event(event);
// }
//
// For some reason ToolTip event is not received after a change of font size
// (ie after staff headers have been deleted then recreated) or when the first
// staff headers show() is called late (ie is called after some other action
// occured, but I was unable to determine what this action is).
// The 4 following methods and m_toolTipTimer are used to replace that
// ToolTip event.
// Moreover, QToolTip::showText() called from NotationWidget when the
// showToolTip() signal is emitted displays the tooltip for a short duration
// only. That's why m_toolTipCount is used with a multipleshot m_toolTipTimer
// to get a reasonable tooltip display duration (10 s).


void
StaffHeader::enterEvent(QEvent */* event */)
{
    // Start timer when mouse enters
    m_toolTipTimer->start();
    m_toolTipCount = 50;   // Set a 10 s tooltip duration if timer is 0.2 s
}

void
StaffHeader::leaveEvent(QEvent */* event */)
{
    // Stop timer when mouse leaves
    m_toolTipTimer->stop();
    m_toolTipCount = 0;
}

void
StaffHeader::mouseMoveEvent(QMouseEvent *event)
{
    // Restart timer while mouse is moving
    m_toolTipTimer->start();

    // and remember current cursor position
    m_cursorPos = event->pos();
}

void
StaffHeader::slotToolTip()
{
    // Timeout occured : show the tool tip while countdown unfinished

    if (m_toolTipCount-- <= 0)  return;

    // First select what tool tip to display then show it
    QRect inconIconRect = m_clefOrKeyInconsistency->frameGeometry();
    if ((m_clefOrKeyIsInconsistent || m_transposeIsInconsistent)
         && inconIconRect.contains(m_cursorPos)) {
        emit(showToolTip(m_warningToolTipText));
    } else {
        emit(showToolTip(m_toolTipText));
    }
}



// void
// StaffHeader::mousePressEvent(QMouseEvent *event)
// {
//     int h = height();
//     int offset = (h - 10 * m_lineSpacing -1) / 2;
//     if (event->y() > (h - offset)) {
/// Here is a place to call a popup menu to select what segment is the current 
/// one when segments are overlapping.
//         std::cerr << "START MENU\n";
//     }
// }


void
StaffHeader::slotShowInconsistencies()
{
    Composition *comp = m_headersGroup->getComposition();
    Track *track = comp->getTrackById(m_track);
    int trackPos = comp->getTrackPositionById(m_track);

    QString str = tr("<h2>Notation Inconsistencies</h2>");
       
    str += tr("<h3>Filename: %1 </h3>")
             .arg(RosegardenMainWindow::self()->getDocument()->getTitle());

    str += tr("<h3>Track %1: \"%2\"</h3>").arg(trackPos + 1)
                                          .arg(strtoqstr(track->getLabel()));

    if (!m_clefOverlaps->isConsistent()) {
        str += QString("<br><b>");
        str += tr("Overlapping segments with inconsistent clefs:");
        str += QString("</b>");
        m_clefOverlaps->display(str, comp, tr("Segment \"%1\": %2 clef"));
    }

    if (!m_keyOverlaps->isConsistent()) {
        str += QString("<br><b>");
        str += tr("Overlapping segments with inconsistent keys:");
        str += QString("</b>");
        m_keyOverlaps->display(str, comp, tr("Segment \"%1\": %2 key"));
    }

    if (!m_transposeOverlaps->isConsistent()) {
        str += QString("<br><b>");
        str += tr("Overlapping segments with inconsistent transpositions:");
        str += QString("</b>");
        m_transposeOverlaps->display(str, comp, tr("Segment \"%1\": %2"));
    }

    QTextEdit *warning = new QTextEdit(str);
    warning->setReadOnly(true);
    warning->setAttribute(Qt::WA_DeleteOnClose);
    warning->setWindowTitle(tr("Rosegarden")); // (in case we ever go cross-platform)
    warning->setWindowFlags(Qt::Dialog); // Get a popup in middle of screen
    warning->setMinimumWidth(500);
    warning->show();
    connect(this, SIGNAL(destroyed()), warning, SLOT(close()));
}

void
StaffHeader::eventAdded(const Segment */* seg */, Event *ev)
{
   if (ev->isa(Key::EventType) || ev->isa(Clef::EventType)) {
        emit(staffModified());
    }
}

void
StaffHeader::eventRemoved(const Segment */* seg */, Event *ev)
{
    if (ev->isa(Key::EventType) || ev->isa(Clef::EventType)) {
        emit(staffModified());
    }
}

void
StaffHeader::appearanceChanged(const Segment */* seg */)
{
        emit(staffModified());
}

void
StaffHeader::startChanged(const Segment */* seg */, timeT)
{
        emit(staffModified());
}

void
StaffHeader::endMarkerTimeChanged(const Segment */* seg */, bool /*shorten*/)
{
        emit(staffModified());
}

void
StaffHeader::transposeChanged(const Segment */* seg */, int)
{
        emit(staffModified());
}

void
StaffHeader::segmentDeleted(const Segment *seg)
{
    Segment *s = const_cast<Segment *>(seg);

    // Remove one segment from m_segments, the right segment and only one
    // segment, even if the comparison operator used in SortedSegments is
    // not as good as it would be wished.
    std::pair<SortedSegments::iterator, SortedSegments::iterator> range;
    range = m_segments.equal_range(s);
    for (SortedSegments::iterator i=m_segments.begin();
                                      i!=m_segments.end(); ++i) {
        if (*i == s) {
            m_segments.erase(i);
            break;
        }
    }

    emit(staffModified());
}


}


