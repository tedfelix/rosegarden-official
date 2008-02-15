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
#include "base/Track.h"
#include "base/Colour.h"
#include "base/ColourMap.h"
#include "base/Track.h"
#include "gui/general/GUIPalette.h"
#include "gui/general/LinedStaff.h"
#include "document/RosegardenGUIDoc.h"
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

TrackHeader::TrackHeader(QWidget *parent, TrackId trackId, int height, int ypos) :
        QLabel(parent),                  // QWidget(parent)
        m_track(trackId),
        m_height(height),
        m_ypos(ypos),
        m_lastClef(Clef()),
        m_lastKey(Rosegarden::Key()),
        m_lastLabel(QString("")),
        m_lastTranspose(0),
        m_neverUpdated(true),
        m_isCurrent(true),
        m_lastStatusPart(0)
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
                            .arg(track->getLabel()));

    QString preset = track->getPresetLabel();
    if (preset != QString(""))
        toolTipText += QString(i18n("\nPreset : %1").arg(preset));

    for (int i=0; i<m_notationView->getStaffCount(); i++) {

        NotationStaff * notationStaff = m_notationView->getNotationStaff(i);
        Segment &segment = notationStaff->getSegment();
        TrackId trackId = segment.getTrack();

        if (trackId  == m_track) {
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

	    toolTipText += QString(i18n("\n  size: %1  bracket: %2 "))
				    .arg(notationSize)
				    .arg(bracketText);
	                               
            timeT segStart = segment.getStartTime();
            timeT segEnd = segment.getEndMarkerTime();
            int barStart = comp->getBarNumber(segStart) + 1;
            int barEnd = comp->getBarNumber(segEnd) + 1;
            int transpose = segment.getTranspose();
            if (transpose) {
                QString transposeName;
                transposeValueToName(transpose, transposeName);
                toolTipText += QString(i18n("\n  bars [%1-%2] in %3 (tr=%4) : \"%5\""))
                                        .arg(barStart)
                                        .arg(barEnd)
                                        .arg(transposeName)
                                        .arg(transpose)
                                        .arg(segment.getLabel());
            } else {
                toolTipText += QString(i18n("\n  bars [%1-%2] (tr=%3) : \"%4\""))
                                        .arg(barStart)
                                        .arg(barEnd)
                                        .arg(transpose)
                                        .arg(segment.getLabel());
            }
        }
    }

    QToolTip::add(this, toolTipText);
}





void
TrackHeader::updateHeader(double x)
{

    // Read Clef and Key on canvas at (x, m_ypos + m_height / 2)
    // Compare them to last Clef and Key then update if necessary

    // Status bits
    const int SEGMENT_HERE                = 1 << 0;
    const int SUPERIMPOSED_SEGMENTS       = 1 << 1;
    const int INCONSISTENT_CLEFS          = 1 << 2;
    const int INCONSISTENT_KEYS           = 1 << 3;
    const int INCONSISTENT_LABELS         = 1 << 4;
    const int INCONSISTENT_TRANSPOSITIONS = 1 << 5;

    Clef clef, clef0;
    Rosegarden::Key key, key0;
    QString label, label0;
    int transpose = 0, transpose0;
    int staff;

    Composition *comp = 
        static_cast<HeadersGroup *>(parent())->getComposition();
    Track *track = comp->getTrackById(m_track);
    int trackPos = comp->getTrackPositionById(m_track);

    QString trackName = QString("");
    int status = 0;
    bool current = false;
    for (int i=0; i<m_notationView->getStaffCount(); i++) {
        NotationStaff * notationStaff = m_notationView->getNotationStaff(i);
        Segment &segment = notationStaff->getSegment();
        TrackId trackId = segment.getTrack();
        if (trackId  == m_track) {

            timeT segStart = segment.getStartTime();
            timeT segEnd = segment.getEndMarkerTime();
            timeT xTime = notationStaff->getTimeAtCanvasCoords(x, m_ypos);

            current = m_notationView->isCurrentStaff(i);


            if ((xTime >= segStart) && (xTime < segEnd)) {

                notationStaff->getClefAndKeyAtCanvasCoords(x,
                                                           m_ypos + m_height / 2,
                                                           clef, key);
                label = QString(segment.getLabel());
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
                clef0 = clef;
                key0 = key;
                label0 = label;
                transpose0 = transpose;
            }                                                // if(xTime...)
        }                                                // if(trackId...)
    }


    // Filter out bits whose display doesn't depend from
    int statusPart = status & ~(SUPERIMPOSED_SEGMENTS);

    // Header should be updated only if necessary
    if (    m_neverUpdated
         || (statusPart != m_lastStatusPart)
         || (key != m_lastKey)
         || (clef != m_lastClef)
         || (label != m_lastLabel)
         || (transpose != m_lastTranspose)) {

        m_neverUpdated = false;
        m_lastStatusPart = statusPart;
        m_lastKey = key;
        m_lastClef = clef;
        m_lastLabel = label;
        m_lastTranspose = transpose;

        QString noteName;
        transposeValueToName(transpose, noteName);

        QString upperText = QString(i18n("%1: %2")
                                         .arg(trackPos + 1)
                                         .arg(track->getLabel()));
        if (transpose) upperText += i18n(" in %1").arg(noteName);

        bool drawClef = true;
        QColor clefColour;
        if (status & SEGMENT_HERE) {
            if (status & (INCONSISTENT_CLEFS | INCONSISTENT_KEYS))
                clefColour = Qt::red;
            else
                clefColour = Qt::black;
        } else {
            drawClef = false;
        }

        /// TODO : use colours from GUIPalette

        QColor upperTextColour = (status & INCONSISTENT_TRANSPOSITIONS) ?
                                                     Qt::red : Qt::black;
        QColor lowerTextColour = (status & INCONSISTENT_LABELS) ?
                                                     Qt::red : Qt::black;

        NotePixmapFactory * npf = m_notationView->getNotePixmapFactory();
        QPixmap pmap = NotePixmapFactory::toQPixmap(
                  npf->makeTrackHeaderPixmap(m_height,
                                             key, clef, clefColour, drawClef,
                                             upperText, upperTextColour,
                                             label, lowerTextColour));

        setPixmap(pmap);

    }

    setCurrent(current);
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




}
#include "TrackHeader.moc"
