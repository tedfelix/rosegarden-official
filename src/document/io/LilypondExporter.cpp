/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
 
    This program is Copyright 2000-2007
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>
 
    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.
 
    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    Numerous additions and bug fixes by
        Michael McIntyre    <dmmcintyr@users.sourceforge.net>

    Some restructuring by Chris Cannam.

    Brain surgery to support LilyPond 2.x export by Heikki Junes.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "LilypondExporter.h"

#include <klocale.h>
#include "misc/Debug.h"
#include "misc/Strings.h"
#include "document/ConfigGroups.h"
#include "base/BaseProperties.h"
#include "base/Composition.h"
#include "base/Configuration.h"
#include "base/Event.h"
#include "base/Exception.h"
#include "base/Instrument.h"
#include "base/NotationTypes.h"
#include "base/PropertyName.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Sets.h"
#include "base/Staff.h"
#include "base/Studio.h"
#include "base/Track.h"
#include "base/NotationQuantizer.h"
#include "document/RosegardenGUIDoc.h"
#include "gui/application/RosegardenApplication.h"
#include "gui/application/RosegardenGUIView.h"
#include "gui/editors/notation/NotationProperties.h"
#include "gui/editors/notation/NotationView.h"
#include "gui/editors/guitar/Chord.h"
#include "gui/general/ProgressReporter.h"
#include "gui/widgets/CurrentProgressDialog.h"
#include <kconfig.h>
#include <kmessagebox.h>
#include <qfileinfo.h>
#include <qobject.h>
#include <qregexp.h>
#include <qstring.h>
#include <qtextcodec.h>
#include <kapplication.h>
#include <sstream>

#define LILYPOND_VERSION_2_6	0
#define LILYPOND_VERSION_2_8	1
#define LILYPOND_VERSION_2_10	2
#define LILYPOND_VERSION_2_12	3

namespace Rosegarden
{

using namespace BaseProperties;

const PropertyName LilypondExporter::SKIP_PROPERTY
    = "LilypondExportSkipThisEvent";

LilypondExporter::LilypondExporter(RosegardenGUIApp *parent,
                                   RosegardenGUIDoc *doc,
                                   std::string fileName) :
        ProgressReporter((QObject *)parent, "lilypondExporter"),
        m_doc(doc),
        m_fileName(fileName)
{
    m_pitchBorked = false;

    // grab config info
    KConfig *cfg = kapp->config();
    cfg->setGroup(NotationViewConfigGroup);

    m_view = ((RosegardenGUIApp *)parent)->getView();
    m_notationView = NULL;
    m_composition = &m_doc->getComposition();
    m_studio = &m_doc->getStudio();
    m_paperSize = cfg->readUnsignedNumEntry("lilypapersize", 1);
    m_paperLandscape = cfg->readBoolEntry("lilypaperlandscape", false);
    m_fontSize = cfg->readUnsignedNumEntry("lilyfontsize", 4);
    m_raggedBottom = cfg->readBoolEntry("lilyraggedbottom", false);
    m_exportSelection = cfg->readUnsignedNumEntry("lilyexportselection", 1);
    m_exportLyrics = cfg->readBoolEntry("lilyexportlyrics", true);
    m_exportMidi = cfg->readBoolEntry("lilyexportmidi", false);
    m_exportTempoMarks = cfg->readUnsignedNumEntry("lilyexporttempomarks", 0);
    m_exportPointAndClick = cfg->readBoolEntry("lilyexportpointandclick", false);
    m_exportBeams = cfg->readBoolEntry("lilyexportbeamings", false);
    m_exportStaffGroup = cfg->readBoolEntry("lilyexportstaffgroup", false);
    m_exportStaffMerge = cfg->readBoolEntry("lilyexportstaffmerge", false);

    m_languageLevel = cfg->readUnsignedNumEntry("lilylanguage", 0);

}

LilypondExporter::LilypondExporter(NotationView *parent,
                                   RosegardenGUIDoc *doc,
                                   std::string fileName) :
        ProgressReporter((QObject *)parent, "lilypondExporter"),
        m_doc(doc),
        m_fileName(fileName)
{
    m_pitchBorked = false;

    // grab config info
    KConfig *cfg = kapp->config();
    cfg->setGroup(NotationViewConfigGroup);

    m_view = NULL;
    m_notationView = ((NotationView *)parent);
    m_composition = &m_doc->getComposition();
    m_studio = &m_doc->getStudio();
    m_paperSize = cfg->readUnsignedNumEntry("lilypapersize", 1);
    m_paperLandscape = cfg->readBoolEntry("lilypaperlandscape", false);
    m_fontSize = cfg->readUnsignedNumEntry("lilyfontsize", 4);
    m_exportSelection = cfg->readUnsignedNumEntry("lilyexportselection", 1);
    m_exportLyrics = cfg->readBoolEntry("lilyexportlyrics", true);
    m_exportMidi = cfg->readBoolEntry("lilyexportmidi", false);
    m_exportTempoMarks = cfg->readUnsignedNumEntry("lilyexporttempomarks", 0);
    m_exportPointAndClick = cfg->readBoolEntry("lilyexportpointandclick", false);
    m_exportBeams = cfg->readBoolEntry("lilyexportbeamings", false);
    m_exportStaffGroup = cfg->readBoolEntry("lilyexportstaffgroup", false);
    m_exportStaffMerge = cfg->readBoolEntry("lilyexportstaffmerge", false);

    m_languageLevel = cfg->readUnsignedNumEntry("lilylanguage", 0);

}

LilypondExporter::~LilypondExporter()
{
    // nothing
}

void
LilypondExporter::handleStartingEvents(eventstartlist &eventsToStart,
                                       std::ofstream &str)
{
    eventstartlist::iterator m = eventsToStart.begin();

    while (m != eventsToStart.end()) {

        try {
            Indication i(**m);

            if (i.getIndicationType() == Indication::Slur) {
                if ((*m)->get
                        <Bool>(NotationProperties::SLUR_ABOVE))
                    str << "^( ";
                else
                    str << "_( ";
            } else if (i.getIndicationType() == Indication::PhrasingSlur) {
                str << "\\( ";
            } else if (i.getIndicationType() == Indication::Crescendo) {
                str << "\\< ";
            } else if (i.getIndicationType() == Indication::Decrescendo) {
                str << "\\> ";
            }

        } catch (Event::BadType) {
            // Not an indication
        } catch (Event::NoData e) {
            std::cerr << "Bad indication: " << e.getMessage() << std::endl;
        }

        eventstartlist::iterator n(m);
        ++n;
        eventsToStart.erase(m);
        m = n;
    }
}

void
LilypondExporter::handleEndingEvents(eventendlist &eventsInProgress,
                                     const Segment::iterator &j,
                                     std::ofstream &str)
{
    eventendlist::iterator k = eventsInProgress.begin();

    while (k != eventsInProgress.end()) {

        eventendlist::iterator l(k);
        ++l;

        // Handle and remove all the relevant events in progress
        // This assumes all deferred events are indications

        try {
            Indication i(**k);

            timeT indicationEnd =
                (*k)->getNotationAbsoluteTime() + i.getIndicationDuration();
            timeT eventEnd =
                (*j)->getNotationAbsoluteTime() + (*j)->getNotationDuration();

            if (indicationEnd < eventEnd ||
                    ((i.getIndicationType() == Indication::Slur ||
                      i.getIndicationType() == Indication::PhrasingSlur) &&
                     indicationEnd == eventEnd)) {

                if (i.getIndicationType() == Indication::Slur) {
                    str << ") ";
                } else if (i.getIndicationType() == Indication::PhrasingSlur) {
                    str << "\\) ";
                } else if (i.getIndicationType() == Indication::Crescendo ||
                           i.getIndicationType() == Indication::Decrescendo) {
                    str << "\\! ";
                }

                eventsInProgress.erase(k);
            }

        } catch (Event::BadType) {
            // not an indication

        } catch (Event::NoData e) {
            std::cerr << "Bad indication: " << e.getMessage() << std::endl;
        }

        k = l;
    }
}

std::string
LilypondExporter::convertPitchToLilyNote(int pitch, Accidental accidental,
        const Rosegarden::Key &key)
{
    Pitch p(pitch, accidental);
    std::string lilyNote = "";

    lilyNote += (char)tolower(p.getNoteName(key));
    //    std::cout << "lilyNote: " << lilyNote << std::endl;
    Accidental acc = p.getAccidental(key);
    if (acc == Accidentals::DoubleFlat)
        lilyNote += "eses";
    else if (acc == Accidentals::Flat)
        lilyNote += "es";
    else if (acc == Accidentals::Sharp)
        lilyNote += "is";
    else if (acc == Accidentals::DoubleSharp)
        lilyNote += "isis";

    return lilyNote;
}

std::string
LilypondExporter::composeLilyMark(std::string eventMark, bool stemUp)
{

    std::string inStr = "", outStr = "";
    std::string prefix = (stemUp) ? "_" : "^";

    // shoot text mark straight through unless it's sf or rf
    if (Marks::isTextMark(eventMark)) {
        inStr = protectIllegalChars(Marks::getTextFromMark(eventMark));

        if (inStr == "sf") {
            inStr = "\\sf";
        } else if (inStr == "rf") {
            inStr = "\\rfz";
        } else {
            inStr = "\\markup { \\italic " + inStr + " } ";
        }

        outStr = prefix + inStr;

    } else if (Marks::isFingeringMark(eventMark)) {

        // fingering marks: use markup syntax only for non-trivial fingerings

        inStr = protectIllegalChars(Marks::getFingeringFromMark(eventMark));

        if (inStr != "0" && inStr != "1" && inStr != "2" && inStr != "3" && inStr != "4" && inStr != "5" && inStr != "+" ) {
            inStr = "\\markup { \\finger \"" + inStr + "\" } ";
        }

        outStr = prefix + inStr;

    } else {
        outStr = "-";

        // use full \accent format for everything, even though some shortcuts
        // exist, for the sake of consistency
        if (eventMark == Marks::Accent) {
            outStr += "\\accent";
        } else if (eventMark == Marks::Tenuto) {
            outStr += "\\tenuto";
        } else if (eventMark == Marks::Staccato) {
            outStr += "\\staccato";
        } else if (eventMark == Marks::Staccatissimo) {
            outStr += "\\staccatissimo";
        } else if (eventMark == Marks::Marcato) {
            outStr += "\\marcato";
        } else if (eventMark == Marks::Trill) {
            outStr += "\\trill";
        } else if (eventMark == Marks::LongTrill) {
            // span trill up to the next note:
            // tweak the beginning of the next note using an invisible rest having zero length
            outStr += "\\startTrillSpan s4*0 \\stopTrillSpan";
        } else if (eventMark == Marks::Turn) {
            outStr += "\\turn";
        } else if (eventMark == Marks::Pause) {
            outStr += "\\fermata";
        } else if (eventMark == Marks::UpBow) {
            outStr += "\\upbow";
        } else if (eventMark == Marks::DownBow) {
            outStr += "\\downbow";
        } else {
            outStr = "";
            std::cerr << "LilypondExporter::composeLilyMark() - unhandled mark:  "
            << eventMark << std::endl;
        }
    }

    return outStr;
}

std::string
LilypondExporter::indent(const int &column)
{
    std::string outStr = "";
    for (int c = 1; c <= column; c++) {
        outStr += "    ";
    }
    return outStr;
}

std::string
LilypondExporter::protectIllegalChars(std::string inStr)
{

    QString tmpStr = strtoqstr(inStr);

    tmpStr.replace(QRegExp("&"), "\\&");
    tmpStr.replace(QRegExp("\\^"), "\\^");
    tmpStr.replace(QRegExp("%"), "\\%");
    tmpStr.replace(QRegExp("<"), "\\<");
    tmpStr.replace(QRegExp(">"), "\\>");
    tmpStr.replace(QRegExp("\\["), "");
    tmpStr.replace(QRegExp("\\]"), "");
    tmpStr.replace(QRegExp("\\{"), "");
    tmpStr.replace(QRegExp("\\}"), "");

    //
    // LilyPond uses utf8 encoding.
    //
    return tmpStr.utf8().data();
}

bool
LilypondExporter::write()
{
    QString tmpName = strtoqstr(m_fileName);

    // dmm - modified to act upon the filename itself, rather than the whole
    // path; fixes bug #855349

    // split name into parts:
    QFileInfo nfo(tmpName);
    QString dirName = nfo.dirPath();
    QString baseName = nfo.fileName();

    // sed Lilypond-choking chars out of the filename proper
    bool illegalFilename = (baseName.contains(' ') || baseName.contains("\\"));
    baseName.replace(QRegExp(" "), "");
    baseName.replace(QRegExp("\\\\"), "");
    baseName.replace(QRegExp("'"), "");
    baseName.replace(QRegExp("\""), "");

    // cat back together
    tmpName = dirName + '/' + baseName;

    if (illegalFilename) {
        CurrentProgressDialog::freeze();
        int reply = KMessageBox::warningContinueCancel(
                        0, i18n("Lilypond does not allow spaces or backslashes in filenames.\n\n"
                                "Would you like to use\n\n %1\n\n instead?").arg(baseName));
        if (reply != KMessageBox::Continue)
            return false;
    }

    std::ofstream str(qstrtostr(tmpName).c_str(), std::ios::out);
    if (!str) {
        std::cerr << "LilypondExporter::write() - can't write file " << tmpName << std::endl;
        return false;
    }

    str << "% This LilyPond file was generated by Rosegarden " << protectIllegalChars(VERSION) << std::endl;

    switch (m_languageLevel) {

    case LILYPOND_VERSION_2_6:
        str << "\\version \"2.6.0\"" << std::endl;
        break;

    case LILYPOND_VERSION_2_8:
        str << "\\version \"2.8.0\"" << std::endl;
        break;

    case LILYPOND_VERSION_2_10:
        str << "\\version \"2.10.0\"" << std::endl;
        break;

    case LILYPOND_VERSION_2_12:
        str << "\\version \"2.12.0\"" << std::endl;
        break;

    default:
        // force the default version if there was an error
        std::cerr << "ERROR: Unknown language level " << m_languageLevel
	    << ", using \\version \"2.6.0\" instead" << std::endl;
        str << "\\version \"2.6.0\"" << std::endl;
        m_languageLevel = LILYPOND_VERSION_2_6;
    }

    // enable "point and click" debugging via pdf to make finding the
    // unfortunately inevitable errors easier
    if (m_exportPointAndClick) {
        str << "% point and click debugging is enabled" << std::endl;
    } else {
        str << "% point and click debugging is disabled" << std::endl;
        str << "#(ly:set-option 'point-and-click #f)" << std::endl;
    }

    // Lilypond \header block

    // set indention level to make future changes to horizontal layout less
    // tedious, ++col to indent a new level, --col to de-indent
    int col = 0;

    // grab user headers from metadata
    Configuration metadata = m_composition->getMetadata();
    std::vector<std::string> propertyNames = metadata.getPropertyNames();

    // open \header section if there's metadata to grab, and if the user
    // wishes it
    if (!propertyNames.empty()) {
        str << "\\header {" << std::endl;
        col++;  // indent+

        const std::string headerDedication = "dedication";
        const std::string headerTitle = "title";
        const std::string headerSubtitle = "subtitle";
        const std::string headerSubsubtitle = "subsubtitle";
        const std::string headerPoet = "poet";
        const std::string headerComposer = "composer";
        const std::string headerMeter = "meter";
        const std::string headerOpus = "opus";
        const std::string headerArranger = "arranger";
        const std::string headerInstrument = "instrument";
        const std::string headerPiece = "piece";
        const std::string headerCopyright = "copyright";
        const std::string headerTagline = "tagline";

        bool userTagline = false;

        for (unsigned int index = 0; index < propertyNames.size(); ++index) {
            std::string property = propertyNames [index];
            if (property == headerDedication || property == headerTitle ||
                    property == headerSubtitle || property == headerSubsubtitle ||
                    property == headerPoet || property == headerComposer ||
                    property == headerMeter || property == headerOpus ||
                    property == headerArranger || property == headerInstrument ||
                    property == headerPiece || property == headerCopyright ||
                    property == headerTagline) {
                std::string header = protectIllegalChars(metadata.get<String>(property));
                if (header != "") {
                    str << indent(col) << property << " = \"" << header << "\"" << std::endl;
                    // let users override defaults, but allow for providing
                    // defaults if they don't:
                    if (property == headerTagline)
                        userTagline = true;
                }
            }
        }

        // default tagline
        if (!userTagline) {
            str << indent(col) << "tagline = \""
            << "Created using Rosegarden " << protectIllegalChars(VERSION) << " and LilyPond"
            << "\"" << std::endl;
        }

        // close \header
        str << indent(--col) << "}" << std::endl;
    }

    // Lilypond \paper block (optional)
    if (m_raggedBottom) {
        str << indent(col) << "\\paper {" << std::endl;
        str << indent(++col) << "ragged-bottom=##t" << std::endl;
        str << indent(--col) << "}" << std::endl;
    }

    // Lilypond music data!   Mapping:
    // Lilypond Voice = Rosegarden Segment
    // Lilypond Staff = Rosegarden Track
    // (not the cleanest output but maybe the most reliable)

    // paper/font sizes
    int font = 20; // default, if config problem
    switch (m_fontSize) {
    case 0 :
        font = 11;
        break;
    case 1 :
        font = 13;
        break;
    case 2 :
        font = 16;
        break;
    case 3 :
        font = 19;
        break;
    case 4 :
        font = 20;
        break;
    case 5 :
        font = 23;
        break;
    case 6 :
        font = 26;
        break;
    }

    str << indent(col) << "#(set-global-staff-size " << font << ")" << std::endl;

    // write user-specified paper type as default paper size
    std::string paper = "";
    switch (m_paperSize) {
    case 0 :
        paper += "a3";
        break;
    case 1 :
        paper += "a4";
        break;
    case 2 :
        paper += "a5";
        break;
    case 3 :
        paper += "a6";
        break;
    case 4 :
        paper += "legal";
        break;
    case 5 :
        paper += "letter";
        break;
    case 6 :
        paper += "tabloid";
        break;
    case 7 :
        paper = "";
        break; // "do not specify"
    }
    if (paper != "") {
        str << indent(col) << "#(set-default-paper-size \"" << paper << "\"" 
	    << (m_paperLandscape ? " 'landscape" : "") << ")"
	    << std::endl;
    }

    // Find out the printed length of the composition
    Composition::iterator i = m_composition->begin();
    if ((*i) == NULL) {
        str << indent(col) << "\\score {" << std::endl;
        str << indent(++col) << "% no segments found" << std::endl;
        // bind staffs with or without staff group bracket
        str << indent(col) // indent
	    << "<<" << " s4 " << ">>" << std::endl;
        str << indent(col) << "\\layout { }" << std::endl;
        str << indent(--col) << "}" << std::endl;
        return true;
    }
    timeT compositionStartTime = (*i)->getStartTime();
    timeT compositionEndTime = (*i)->getEndMarkerTime();
    for (; i != m_composition->end(); ++i) {
        if (compositionStartTime > (*i)->getStartTime() && (*i)->getTrack() >= 0) {
            compositionStartTime = (*i)->getStartTime();
        }
        if (compositionEndTime < (*i)->getEndMarkerTime()) {
            compositionEndTime = (*i)->getEndMarkerTime();
        }
    }

    // define global context which is common for all staffs
    str << indent(col++) << "global = { " << std::endl;
    TimeSignature timeSignature = m_composition->
                                  getTimeSignatureAt(m_composition->getStartMarker());
    if (m_composition->getBarStart(m_composition->getBarNumber(compositionStartTime)) < compositionStartTime) {
        str << indent(col) << "\\partial ";
        // Arbitrary partial durations are handled by the following way:
        // split the partial duration to 64th notes: instead of "4" write "64*16". (hjj)
        Note partialNote = Note::getNearestNote(1, MAX_DOTS);
        int partialDuration = m_composition->getBarStart(m_composition->getBarNumber(compositionStartTime) + 1) - compositionStartTime;
        writeDuration(1, str);
        str << "*" << ((int)(partialDuration / partialNote.getDuration()))
        << std::endl;
    }
    int leftBar = 0;
    int rightBar = leftBar;
    do {
        bool isNew = false;
        m_composition->getTimeSignatureInBar(rightBar + 1, isNew);

        if (isNew || (m_composition->getBarStart(rightBar + 1) >= compositionEndTime)) {
            //  - set initial time signature; further time signature changes
            //    are defined within the segments, because they may be hidden
            str << indent(col) << (leftBar == 0 ? "" : "% ") << "\\time "
            << timeSignature.getNumerator() << "/"
            << timeSignature.getDenominator() << std::endl;
            //  - place skips upto the end of the composition;
            //    this justifies the printed staffs
            str << indent(col);
            timeT leftTime = m_composition->getBarStart(leftBar);
            timeT rightTime = m_composition->getBarStart(rightBar + 1);
            if (leftTime < compositionStartTime) {
                leftTime = compositionStartTime;
            }
            writeSkip(timeSignature, leftTime, rightTime - leftTime, false, str);
            str << " %% " << (leftBar + 1) << "-" << (rightBar + 1) << std::endl;

            timeSignature = m_composition->getTimeSignatureInBar(rightBar + 1, isNew);
            leftBar = rightBar + 1;
        }
    } while (m_composition->getBarStart(++rightBar) < compositionEndTime);
    str << indent(--col) << "}" << std::endl;

    // time signatures changes are in segments, reset initial value
    timeSignature = m_composition->
                    getTimeSignatureAt(m_composition->getStartMarker());

    // All the tempo changes are included in "globalTempo" context.
    // This context contains only skip notes between the tempo changes.
    // First tempo marking should still be include in \midi{ } block.
    // If tempo marks are printed in future, they should probably be
    // included in this context and the note duration in the tempo
    // mark should be according to the time signature. (hjj)
    int tempoCount = m_composition->getTempoChangeCount();

    if (tempoCount > 0) {

        timeT prevTempoChangeTime = m_composition->getStartMarker();
        int tempo = int(Composition::getTempoQpm(m_composition->getTempoAtTime(prevTempoChangeTime)));
        bool tempoMarksInvisible = false;

        str << indent(col++) << "globalTempo = {" << std::endl;
        if (m_exportTempoMarks == 0 && tempoMarksInvisible == false) {
            str << indent(col) << "\\override Score.MetronomeMark #'transparent = ##t" << std::endl;
            tempoMarksInvisible = true;
        }
        str << indent(col) << "\\tempo 4 = " << tempo << "  ";
        int prevTempo = tempo;

        for (int i = 0; i < tempoCount; ++i) {

            std::pair<timeT, long> tempoChange =
                m_composition->getTempoChange(i);

            timeT tempoChangeTime = tempoChange.first;

            tempo = int(Composition::getTempoQpm(tempoChange.second));

            // First tempo change may be before the first segment.
            // Do not apply it before the first segment appears.
            if (tempoChangeTime < compositionStartTime) {
                tempoChangeTime = compositionStartTime;
            } else if (tempoChangeTime >= compositionEndTime) {
                tempoChangeTime = compositionEndTime;
            }
            if (prevTempoChangeTime < compositionStartTime) {
                prevTempoChangeTime = compositionStartTime;
            } else if (prevTempoChangeTime >= compositionEndTime) {
                prevTempoChangeTime = compositionEndTime;
            }
            writeSkip(m_composition->getTimeSignatureAt(tempoChangeTime),
                      tempoChangeTime, tempoChangeTime - prevTempoChangeTime, false, str);
            // add new \tempo only if tempo was changed
            if (tempo != prevTempo) {
                if (m_exportTempoMarks == 1 && tempoMarksInvisible == false) {
                    str << std::endl << indent(col) << "\\override Score.MetronomeMark #'transparent = ##t";
                    tempoMarksInvisible = true;
                }
                str << std::endl << indent(col) << "\\tempo 4 = " << tempo << "  ";
            }

            prevTempo = tempo;
            prevTempoChangeTime = tempoChangeTime;
            if (prevTempoChangeTime == compositionEndTime)
                break;
        }
        // First tempo change may be before the first segment.
        // Do not apply it before the first segment appears.
        if (prevTempoChangeTime < compositionStartTime) {
            prevTempoChangeTime = compositionStartTime;
        }
        writeSkip(m_composition->getTimeSignatureAt(prevTempoChangeTime),
                  prevTempoChangeTime, compositionEndTime - prevTempoChangeTime, false, str);
        str << std::endl;
        str << indent(--col) << "}" << std::endl;
    }

    // open \score section
    str << "\\score {" << std::endl;

    // bind staffs with or without staff group bracket
    str << indent(++col) // indent+
	<< (m_exportStaffGroup == true ? "\\new StaffGroup " : "")
	<< "<<" << std::endl;

    // Make chords offset colliding notes by default
    str << indent(++col) << "% force offset of colliding notes in chords:" << std::endl;
    str << indent(col)
    << "\\override Score.NoteColumn #\'force-hshift = #1.0" << std::endl;

    int lastTrackIndex = -1;
    int voiceCounter = 0;

    // Write out all segments for each Track, in track order.
    // This involves a hell of a lot of loops through all tracks
    // and segments, but the time spent doing that should still
    // be relatively small in the greater scheme.

    Track *track = 0;

    for (int trackPos = 0;
            (track = m_composition->getTrackByPosition(trackPos)) != 0; ++trackPos) {

        for (Composition::iterator i = m_composition->begin();
                i != m_composition->end(); ++i) {

            if ((*i)->getTrack() != track->getId())
                continue;

            emit setProgress(int(double(trackPos) /
                                 double(m_composition->getNbTracks()) * 100.0));
            rgapp->refreshGUI(50);
            
            bool currentSegmentSelected = false;
            if ((m_exportSelection == 3) && (m_view != NULL) && (m_view->haveSelection())) {
            	//
            	// Check whether the current segment is in the list of selected segments.
            	//
            	SegmentSelection selection = m_view->getSelection();
                for (SegmentSelection::iterator it = selection.begin(); it != selection.end(); it++) {
                    if ((*it) == (*i)) currentSegmentSelected = true;
                }
            } else if ((m_exportSelection == 3) && (m_notationView != NULL)) {
		currentSegmentSelected = m_notationView->hasSegment(*i);
	    }

	    // Check whether the track is a non-midi track.
	    InstrumentId instrumentId = track->getInstrument();
	    bool isMidiTrack = (instrumentId >= MidiInstrumentBase && instrumentId < SoftSynthInstrumentBase);
    
    	    // The meaning of exportSelection meaning is to export:
    	    // 0 -> All tracks
    	    // 1 -> Non-muted tracks
    	    // 2 -> Selected track
    	    // 3 -> Selected segments
            if (isMidiTrack && ( // Skip non-midi tracks.
		(m_exportSelection == 0) || 
                ((m_exportSelection == 1) && (!track->isMuted())) ||
                ((m_exportSelection == 2) && (m_view != NULL) && (track->getId() == m_composition->getSelectedTrack())) ||
                ((m_exportSelection == 2) && (m_notationView != NULL) && (track->getId() == m_notationView->getCurrentSegment()->getTrack())) ||
                ((m_exportSelection == 3) && (currentSegmentSelected)))) {
                if ((int) (*i)->getTrack() != lastTrackIndex) {
                    if (lastTrackIndex != -1) {
                        // close the old track (Staff context)
                        str << indent(--col) << ">> % Staff" << std::endl;  // indent-
                    }
                    lastTrackIndex = (*i)->getTrack();

                    // avoid problem with <untitled> tracks yielding a
                    // .ly file that jumbles all notes together on a
                    // single staff...  every Staff context has to
                    // have a unique name, even if the
                    // Staff.instrument property is the same for
                    // multiple staffs...
                    // Added an option to merge staffs with the same, non-empty
                    // name. This option makes it possible to produce staffs
                    // with polyphonic, and polyrhytmic, music. Polyrhytmic
                    // music in a single staff is typical in piano, or
                    // guitar music. (hjj)
                    // In the case of colliding note heads, user may define
                    //  - DISPLACED_X -- for a note/chord
                    //  - INVISIBLE -- for a rest
                    std::ostringstream staffName;
                    staffName << protectIllegalChars(m_composition->
                                                     getTrackById(lastTrackIndex)->getLabel());

                    if (!m_exportStaffMerge || staffName.str() == "") {
                        str << std::endl << indent(col)
                        << "\\context Staff = \"track "
                        << (trackPos + 1) << "\" ";
                    } else {
                        str << std::endl << indent(col)
                        << "\\context Staff = \"" << staffName.str()
                        << "\" ";
                    }

                    str << "<< " << std::endl;

		    if (m_languageLevel < LILYPOND_VERSION_2_10) {
			str << indent(++col) << "\\set Staff.instrument = \"" << staffName.str() << " \"" << std::endl;
		    } else {
			str << indent(++col) << "\\set Staff.instrumentName = \"" << staffName.str() << " \"" << std::endl;
		    }

                    if (m_exportMidi) {
                        // Set midi instrument for the Staff
                        std::ostringstream staffMidiName;
                        Instrument *instr = m_studio->getInstrumentById(m_composition->getTrackById(lastTrackIndex)->getInstrument());
                        staffMidiName << instr->getProgramName();

                        str << indent(col) << "\\set Staff.midiInstrument = \"" << staffMidiName.str() << "\"" << std::endl;
                    }

		    // multi measure rests are used by default
                    str << indent(col) << "\\set Score.skipBars = ##t" << std::endl;

                    // turn off the stupid accidental cancelling business,
                    // because we don't do that ourselves, and because my 11
                    // year old son pointed out to me that it "Looks really
                    // stupid.  Why is it cancelling out four flats and then
                    // adding five flats back?  That's brain damaged."
                    str << indent(col) << "\\set Staff.printKeyCancellation = ##f" << std::endl;
                    str << indent(col) << "\\new Voice \\global" << std::endl;
                    if (tempoCount > 0) {
                        str << indent(col) << "\\new Voice \\globalTempo" << std::endl;
                    }
                }

                // Temporary storage for non-atomic events (!BOOM)
                // ex. Lilypond expects signals when a decrescendo starts
                // as well as when it ends
                eventendlist eventsInProgress;
                eventstartlist eventsToStart;

                // If the segment doesn't start at 0, add a "skip" to the start
                // No worries about overlapping segments, because Voices can overlap
                // voiceCounter is a hack because Lilypond does not by default make
                // them unique
                std::ostringstream voiceNumber;
                voiceNumber << "voice " << ++voiceCounter;

                str << std::endl << indent(col++) << "\\context Voice = \"" << voiceNumber.str()
                << "\" {"; // indent+

                str << std::endl << indent(col) << "\\override Voice.TextScript #'padding = #2.0";
                str << std::endl << indent(col) << "\\override MultiMeasureRest #'expand-limit = 1" << std::endl;

                SegmentNotationHelper helper(**i);
                helper.setNotationProperties();

                int firstBar = m_composition->getBarNumber((*i)->getStartTime());

                if (firstBar > 0) {
                    // Add a skip for the duration until the start of the first
                    // bar in the segment.  If the segment doesn't start on a bar
                    // line, an additional skip will be written (in the form of
                    // a series of rests) at the start of writeBar, below.
                    //!!! This doesn't cope correctly yet with time signature changes
                    // during this skipped section.
                    str << std::endl << indent(col);
                    writeSkip(timeSignature, compositionStartTime, m_composition->getBarStart(firstBar) - compositionStartTime,
                              false, str);
                }

                std::string lilyText = "";      // text events
                std::string prevStyle = "";     // track note styles

                Rosegarden::Key key;

                bool haveRepeating = false;
                bool haveAlternates = false;

                bool nextBarIsAlt1 = false;
                bool nextBarIsAlt2 = false;
                bool prevBarWasAlt2 = false;

                int MultiMeasureRestCount = 0;

                bool nextBarIsDouble = false;
                bool nextBarIsEnd = false;
                bool nextBarIsDot = false;

                for (int barNo = m_composition->getBarNumber((*i)->getStartTime());
                        barNo <= m_composition->getBarNumber((*i)->getEndMarkerTime());
                        ++barNo) {

                    timeT barStart = m_composition->getBarStart(barNo);
                    timeT barEnd = m_composition->getBarEnd(barNo);
                    if (barStart < compositionStartTime) {
                        barStart = compositionStartTime;
                    }

                    // open \repeat section if this is the first bar in the
                    // repeat
                    if ((*i)->isRepeating() && !haveRepeating) {

                        haveRepeating = true;

                        //!!! calculate the number of times this segment
                        //repeats and make the following variable meaningful
                        int numRepeats = 2;

                        str << std::endl << indent(col++) << "\\repeat volta " << numRepeats << " {";
                    }

                    // open the \alternative section if this bar is alternative ending 1
                    // ending (because there was an "Alt1" flag in the
                    // previous bar to the left of where we are right now)
                    //
                    // Alt1 remains in effect until we run into Alt2, which
                    // runs to the end of the segment
                    if (nextBarIsAlt1 && haveRepeating) {
                        str << std::endl << indent(--col) << "} \% repeat close (before alternatives) ";
                        str << std::endl << indent(col++) << "\\alternative {";
                        str << std::endl << indent(col++) << "{  \% open alternative 1 ";
                        nextBarIsAlt1 = false;
                        haveAlternates = true;
                    } else if (nextBarIsAlt2 && haveRepeating) {
                        if (!prevBarWasAlt2) {
                            col--;
                            str << std::endl << indent(--col) << "} \% close alternative 1 "
                            << std::endl << indent(col++) << "{  \% open alternative 2";
                            col++;
                        }
                        prevBarWasAlt2 = true;
                    }

                    // write out a bar's worth of events
                    writeBar(*i, barNo, barStart, barEnd, col, key,
                             lilyText,
                             prevStyle, eventsInProgress, str,
                             MultiMeasureRestCount, 
                             nextBarIsAlt1, nextBarIsAlt2, nextBarIsDouble, nextBarIsEnd, nextBarIsDot);

                }

                // close \repeat
                if (haveRepeating) {

                    // close \alternative section if present
                    if (haveAlternates) {
                        str << std::endl << indent(--col) << " } \% close alternative 2 ";
                    }

                    // close \repeat section in either case
                    str << std::endl << indent(--col) << " } \% close "
                    << (haveAlternates ? "alternatives" : "repeat");
                }

                // closing bar
                if (((*i)->getEndMarkerTime() == compositionEndTime) && !haveRepeating) {
                    str << std::endl << indent(col) << "\\bar \"|.\"";
                }

                // close Voice context
                str << std::endl << indent(--col) << "} % Voice" << std::endl;  // indent-

		//
                // Write accumulated lyric events to the Lyric context, if desired.
                //
		// Sync the code below with LyricEditDialog::unparse() !!
		//
                if (m_exportLyrics) {
		    for (long currentVerse = 0, lastVerse = 0; 
                         currentVerse <= lastVerse; 
			 currentVerse++) {
		        bool haveLyric = false;
		        bool haveSlur = false;
			timeT slurEnd;
			bool firstNote = true;
		        QString text = "";

		        timeT lastTime = (*i)->getStartTime();
		        int lastBarNo = m_composition->getBarNumber(lastTime);
		        for (Segment::iterator j = (*i)->begin();
		            (*i)->isBeforeEndMarker(j); ++j) {
		
		            bool isNote = (*j)->isa(Note::EventType);
		            bool isLyric = false;
		
		            if (!isNote) {
		                if ((*j)->isa(Text::EventType)) {
		                    std::string textType;
		                    if ((*j)->get
		                            <String>(Text::TextTypePropertyName, textType) &&
		                            textType == Text::Lyric) {
		                        isLyric = true;
		                    }
		                } else {
				    try {
					Indication indic(**j);

					if (indic.getIndicationType() == Indication::Slur) {
					    slurEnd = (*j)->getNotationAbsoluteTime() + indic.getIndicationDuration();
					    haveSlur = true;
					}
				    } catch (Exception e) { }
				}
		            } else {
		                if ((*j)->has(BaseProperties::TIED_BACKWARD) &&
		                        (*j)->get
		                        <Bool>(BaseProperties::TIED_BACKWARD))
		                    continue;
		            }
		
		            if (!isNote && !isLyric) continue;
		
		            timeT myTime = (*j)->getNotationAbsoluteTime();
		            int myBarNo = m_composition->getBarNumber(myTime);
		
			    if (isNote) {
				if ((myTime > lastTime) || firstNote) {
				    if (!haveLyric && !haveSlur)
					text += " _";
				    if (!haveLyric && haveSlur) {
					timeT eventEnd =
					(*j)->getNotationAbsoluteTime() + (*j)->getNotationDuration();
					if (slurEnd < eventEnd)
					    haveSlur = false;
					else
					    text += " _";
				    }
				    lastTime = myTime;
				    haveLyric = false;
				    firstNote = false;
				}
			    }
		
		            if (isLyric) {
			        long verse;
		                (*j)->get<Int>(Text::LyricVersePropertyName, verse);

				if (verse == currentVerse) {
		                    std::string ssyllable;
		                    (*j)->get<String>(Text::TextPropertyName, ssyllable);
				    if (haveSlur) {
					timeT eventEnd =
					(*j)->getNotationAbsoluteTime() + (*j)->getNotationDuration();
					if (slurEnd < eventEnd) {
					    haveSlur = false;
					    text += " ";
					} else {
					    text += "_";
					}
				    } else {
					text += " ";
				    }
			    
		                    QString syllable(strtoqstr(ssyllable));
		                    syllable.replace(QRegExp("\\s+"), "");
		                    text += "\"" + syllable + "\"";
		                    haveLyric = true;
				} else if (verse > lastVerse) {
                                  lastVerse = verse;
				}
			    }
		        }

			text.replace( QRegExp(" _+([^ ])") , " \\1" );
			text.replace( "\"_\"" , " " );
		
		        // Do not create empty context for lyrics.
		        // Does this save some vertical space, as was written
		        // in earlier comment?
		        QRegExp rx( "\"" );
		        if ( rx.search( text ) != -1 ) {
		    
			    str << indent(col) << "\\lyricsto \"" << voiceNumber.str() << "\""
			        << " \\new Lyrics \\lyricmode {" << std::endl;
			    str << indent(++col) << "\\override LyricText #'self-alignment-X = #LEFT" << std::endl;
			    str << indent(col) << text.utf8() << " " << std::endl;
			    str << indent(--col) << "} % Lyrics " << (currentVerse+1) << std::endl;
			    // close the Lyrics context
		        }
		    }
		}
            }
        }
    }

    // close the last track (Staff context)
    if (voiceCounter > 0) {
        str << indent(--col) << ">> % Staff (final)";  // indent-
    } else {
        str << indent(--col) << "% (All staffs were muted.)" << std::endl;
    }

    // close \notes section
    str << std::endl << indent(--col) << ">> % notes" << std::endl << std::endl; // indent-

    // write \layout block
    str << indent(col) << "\\layout { }" << std::endl;

    // write initial tempo in Midi block, if user wishes (added per user request...
    // makes debugging the .ly file easier because fewer "noisy" errors are
    // produced during the process of rendering MIDI...)
    if (m_exportMidi) {
        int tempo = int(Composition::getTempoQpm(m_composition->getTempoAtTime(m_composition->getStartMarker())));
        // Incomplete?  Can I get away without converting tempo relative to the time
        // signature for this purpose?  we'll see...
        str << indent(col++) << "\\midi {" << std::endl;
        str << indent(col) << "\\tempo 4 = " << tempo << std::endl;
        str << indent(--col) << "} " << std::endl;
    }

    // DMM - a friendly userland warning if they happen to tease out any lingering
    // trouble with this stuff;  since users don't usually see the debug messages
    if (m_pitchBorked) {
        CurrentProgressDialog::freeze();
        KMessageBox::sorry(0,
                           i18n("You encountered an enharmonic resolution bug somewhere during this "
                                "operation.  You may wish to save a copy of your current composition "
                                "and email it to dmmcintyr@users.sourceforge.net for analysis.\n\n"
                                "In the meantime, you will probably need to make manual adjustments "
                                "to the file you've just exported."));
    }

    // close \score section and close out the file
    str << "} % score" << std::endl;
    str.close();
    return true;
}

timeT
LilypondExporter::calculateDuration(Segment *s,
                                    const Segment::iterator &i,
                                    timeT barEnd,
                                    timeT &soundingDuration,
                                    const std::pair<int, int> &tupletRatio,
                                    bool &overlong)
{
    timeT duration = (*i)->getNotationDuration();
    timeT absTime = (*i)->getNotationAbsoluteTime();

    RG_DEBUG << "LilypondExporter::calculateDuration: first duration, absTime: "
    << duration << ", " << absTime << endl;

    timeT durationCorrection = 0;

    if ((*i)->isa(Note::EventType) || (*i)->isa(Note::EventRestType)) {
        try {
            // tuplet compensation, etc
            Note::Type type = (*i)->get
                              <Int>(NOTE_TYPE);
            int dots = (*i)->get
                       <Int>(NOTE_DOTS);
            durationCorrection = Note(type, dots).getDuration() - duration;
        } catch (Exception e) { // no properties
        }
    }

    duration += durationCorrection;

    RG_DEBUG << "LilypondExporter::calculateDuration: now duration is "
    << duration << " after correction of " << durationCorrection << endl;

    soundingDuration = duration * tupletRatio.first / tupletRatio.second;

    timeT toNext = barEnd - absTime;
    if (soundingDuration > toNext) {
        soundingDuration = toNext;
        duration = soundingDuration * tupletRatio.second / tupletRatio.first;
        overlong = true;
    }

    RG_DEBUG << "LilypondExporter::calculateDuration: time to barEnd is "
    << toNext << endl;

    // Examine the following event, and truncate our duration
    // if we overlap it.
    Segment::iterator nextElt = s->end();
    toNext = soundingDuration;

    if ((*i)->isa(Note::EventType)) {

        Chord chord(*s, i, m_composition->getNotationQuantizer());
        Segment::iterator nextElt = chord.getFinalElement();
        ++nextElt;

        if (s->isBeforeEndMarker(nextElt)) {
            // The quantizer sometimes sticks a rest at the same time
            // as this note -- don't use that one here, and mark it as
            // not to be exported -- it's just a heavy-handed way of
            // rendering counterpoint in RG
            if ((*nextElt)->isa(Note::EventRestType) &&
                    (*nextElt)->getNotationAbsoluteTime() == absTime) {
                (*nextElt)->set
                <Bool>(SKIP_PROPERTY, true);
                ++nextElt;
            }
        }

    } else {

        nextElt = i;
        ++nextElt;
    }

    if (s->isBeforeEndMarker(nextElt)) {
	RG_DEBUG << "LilypondExporter::calculateDuration: inside conditional " << endl;
        toNext = (*nextElt)->getNotationAbsoluteTime() - absTime;
	// if the note was lengthened, assume it was lengthened to the left
	// when truncating to the beginning of the next note
	if (durationCorrection > 0)
	{
	    toNext += durationCorrection;
	}
        if (soundingDuration > toNext) {
            soundingDuration = toNext;
            duration = soundingDuration * tupletRatio.second / tupletRatio.first;
        }
    }

    RG_DEBUG << "LilypondExporter::calculateDuration: second toNext is "
    << toNext << endl;

    RG_DEBUG << "LilypondExporter::calculateDuration: final duration, soundingDuration: " << duration << ", " << soundingDuration << endl;

    return duration;
}

void
LilypondExporter::writeBar(Segment *s,
                           int barNo, int barStart, int barEnd, int col,
                           Rosegarden::Key &key,
                           std::string &lilyText,
                           std::string &prevStyle,
                           eventendlist &eventsInProgress,
                           std::ofstream &str,
                           int &MultiMeasureRestCount,
                           bool &nextBarIsAlt1, bool &nextBarIsAlt2,
                           bool &nextBarIsDouble, bool &nextBarIsEnd, bool &nextBarIsDot)
{
    int lastStem = 0; // 0 => unset, -1 => down, 1 => up

    Segment::iterator i = s->findTime(barStart);
    if (!s->isBeforeEndMarker(i))
        return ;

    if (MultiMeasureRestCount == 0) {
	str << std::endl;

	if ((barNo + 1) % 5 == 0) {
	    str << "%% " << barNo + 1 << std::endl << indent(col);
	} else {
	    str << indent(col);
	}
    }

    bool isNew = false;
    TimeSignature timeSignature = m_composition->getTimeSignatureInBar(barNo, isNew);
    if (isNew && !timeSignature.isHidden()) {
        str << "\\time "
        << timeSignature.getNumerator() << "/"
        << timeSignature.getDenominator()
        << std::endl << indent(col);
    }

    timeT absTime = (*i)->getNotationAbsoluteTime();
    timeT writtenDuration = 0;

    if (absTime > barStart) {
        writtenDuration = absTime - barStart;
        writeSkip(timeSignature, 0, writtenDuration, true, str);
    }

    timeT prevDuration = -1;
    eventstartlist eventsToStart;

    long groupId = -1;
    std::string groupType = "";
    std::pair<int, int> tupletRatio(1, 1);

    bool overlong = false;
    bool newBeamedGroup = false;
    int notesInBeamedGroup = 0;

    while (s->isBeforeEndMarker(i)) {

        if ((*i)->getNotationAbsoluteTime() >= barEnd)
            break;

        // First test whether we're entering or leaving a group,
        // before we consider how to write the event itself (at least
        // for pre-2.0 Lilypond output)

        if ((*i)->isa(Note::EventType) || (*i)->isa(Note::EventRestType) ||
                (*i)->isa(Clef::EventType) || (*i)->isa(Rosegarden::Key::EventType)) {

            long newGroupId = -1;
            if ((*i)->get
                    <Int>(BEAMED_GROUP_ID, newGroupId)) {

                if (newGroupId != groupId) {
                    // entering a new beamed group

                    if (groupId != -1) {
                        // and leaving an old one
                        if (groupType == GROUP_TYPE_TUPLED ||
                                groupType == GROUP_TYPE_GRACE) {
                            if (m_exportBeams && notesInBeamedGroup > 0)
                                str << "] ";
                            str << "} ";
                        } else if (groupType == GROUP_TYPE_BEAMED) {
                            if (m_exportBeams && notesInBeamedGroup > 0)
                                str << "] ";
                        }
                    }

                    groupId = newGroupId;
                    groupType = "";
                    (void)(*i)->get
                    <String>(BEAMED_GROUP_TYPE, groupType);

                    if (groupType == GROUP_TYPE_TUPLED) {
                        long numerator = 0;
                        long denominator = 0;
                        (*i)->get
                        <Int>(BEAMED_GROUP_TUPLED_COUNT, numerator);
                        (*i)->get
                        <Int>(BEAMED_GROUP_UNTUPLED_COUNT, denominator);
                        if (numerator == 0 || denominator == 0) {
                            std::cerr << "WARNING: LilypondExporter::writeBar: "
                            << "tupled event without tupled/untupled counts"
                            << std::endl;
                            groupId = -1;
                            groupType = "";
                        } else {
                            str << "\\times " << numerator << "/" << denominator << " { ";
                            tupletRatio = std::pair<int, int>(numerator, denominator);
			    // Require explicit beamed groups,
			    // fixes bug #1683205.
			    // HJJ: Why line below was originally present?
                            // newBeamedGroup = true;
                            notesInBeamedGroup = 0;
                        }
                    } else if (groupType == GROUP_TYPE_BEAMED) {
                        newBeamedGroup = true;
                        notesInBeamedGroup = 0;
                    } else if (groupType == GROUP_TYPE_GRACE) {
                        str << "\\grace { ";
			// Require explicit beamed group,
			// fixes bug #1683205.
		        // HJJ: Why line below was originally present?
                        // newBeamedGroup = true;
                        notesInBeamedGroup = 0;
                    }
                }

            }
            else {

                if (groupId != -1) {
                    // leaving a beamed group
                    if (groupType == GROUP_TYPE_TUPLED ||
                            groupType == GROUP_TYPE_GRACE) {
                        if (m_exportBeams && notesInBeamedGroup > 0)
                            str << "] ";
                        str << "} ";
                        tupletRatio = std::pair<int, int>(1, 1);
                    } else if (groupType == GROUP_TYPE_BEAMED) {
                        if (m_exportBeams && notesInBeamedGroup > 0)
                            str << "] ";
                    }
                    groupId = -1;
                    groupType = "";
                }
            }
        }

        timeT soundingDuration = -1;
        timeT duration = calculateDuration
                         (s, i, barEnd, soundingDuration, tupletRatio, overlong);

        if (soundingDuration == -1) {
            soundingDuration = duration * tupletRatio.first / tupletRatio.second;
        }

        if ((*i)->has(SKIP_PROPERTY)) {
            (*i)->unset(SKIP_PROPERTY);
            ++i;
            continue;
        }

        if ((*i)->isa(Note::EventType)) {

            Chord chord(*s, i, m_composition->getNotationQuantizer());
            Event *e = *chord.getInitialNote();
            bool tiedForward = false;

            // Examine the following event, and truncate our duration
            // if we overlap it.

            if (e->has(DISPLACED_X)) {
                double xDisplacement = 1 + ((double) e->get
                                            <Int>(DISPLACED_X)) / 1000;
                str << "\\once \\override NoteColumn #'force-hshift = #"
                << xDisplacement << " ";
            }

            if (e->has(NotationProperties::STEM_UP)) {
                if (e->get
                        <Bool>(NotationProperties::STEM_UP)) {
                    if (lastStem != 1) {
                        str << "\\stemUp ";
                        lastStem = 1;
                    }
                }
                else {
                    if (lastStem != -1) {
                        str << "\\stemDown ";
                        lastStem = -1;
                    }
                }
            } else {
                if (lastStem != 0) {
                    str << "\\stemNeutral ";
                    lastStem = 0;
                }
            }

            if (chord.size() > 1)
                str << "< ";

            Segment::iterator stylei = s->end();

            for (i = chord.getInitialElement(); s->isBeforeEndMarker(i); ++i) {

                if ((*i)->isa(Text::EventType)) {
                    if (!handleDirective(*i, lilyText, nextBarIsAlt1, nextBarIsAlt2,
                                         nextBarIsDouble, nextBarIsEnd, nextBarIsDot)) {

                        handleText(*i, lilyText);
                    }

                } else if ((*i)->isa(Note::EventType)) {

                    if (m_languageLevel >= LILYPOND_VERSION_2_8) {
                        // one \tweak per each chord note
                        if (chord.size() > 1)
                            writeStyle(*i, prevStyle, col, str, true);
                        else
                            writeStyle(*i, prevStyle, col, str, false);
                    } else {
                        // only one override per chord, and that outside the <>
                        stylei = i;
                    }
                    writePitch(*i, key, str);

                    bool noteHasCautionaryAccidental = false;
                    (*i)->get
                    <Bool>(NotationProperties::USE_CAUTIONARY_ACCIDENTAL, noteHasCautionaryAccidental);
                    if (noteHasCautionaryAccidental)
                        str << "?";

                    bool noteTiedForward = false;
                    (*i)->get
                    <Bool>(TIED_FORWARD, noteTiedForward);
                    if (noteTiedForward)
                        tiedForward = true;

                    str << " ";
                } else if ((*i)->isa(Indication::EventType)) {
                    eventsToStart.insert(*i);
                    eventsInProgress.insert(*i);
                }

                if (i == chord.getFinalElement())
                    break;
            }

            if (chord.size() > 1)
                str << "> ";

            if (duration != prevDuration) {
                writeDuration(duration, str);
                str << " ";
                prevDuration = duration;
            }

            if (m_languageLevel == LILYPOND_VERSION_2_6) {
                // only one override per chord, and that outside the <>
                if (stylei != s->end()) {
                    writeStyle(*stylei, prevStyle, col, str, false);
                    stylei = s->end();
                }
            }

            if (lilyText != "") {
                str << lilyText;
                lilyText = "";
            }
            writeSlashes(*i, str);

            writtenDuration += soundingDuration;

            std::vector<Mark> marks(chord.getMarksForChord());
            // problem here: stem direction unavailable (it's a view-local property)
            bool stemUp = true;
            e->get
            <Bool>(NotationProperties::STEM_UP, stemUp);
            for (std::vector<Mark>::iterator j = marks.begin(); j != marks.end(); ++j) {
                str << composeLilyMark(*j, stemUp);
            }
            if (marks.size() > 0)
                str << " ";

            handleEndingEvents(eventsInProgress, i, str);
            handleStartingEvents(eventsToStart, str);

            if (tiedForward)
                str << "~ ";

            if (newBeamedGroup) {
                // This is a workaround for bug #1705430:
                //   Beaming groups erroneous after merging notes
                // There will be fewer "e4. [ ]" errors in LilyPond-compiling.
                // HJJ: This should be fixed in notation engine,
                //      after which the workaround below should be removed.
                Note note(Note::getNearestNote(duration, MAX_DOTS));

                switch (note.getNoteType()) {
                case Note::SixtyFourthNote:
                case Note::ThirtySecondNote:
                case Note::SixteenthNote:
                case Note::EighthNote:
                    notesInBeamedGroup++;
                    break;
                }
            }
            // // Old version before the workaround for bug #1705430:
            // if (newBeamedGroup)
            //    notesInBeamedGroup++;
        } else if ((*i)->isa(Note::EventRestType)) {

            bool hiddenRest = false;
            if ((*i)->has(INVISIBLE)) {
                if ((*i)->get
                        <Bool>(INVISIBLE)) {
                    hiddenRest = true;
                }
            }

	    if (MultiMeasureRestCount == 0) {
		if (hiddenRest) {
		    str << "s";
		} else if (duration == timeSignature.getBarDuration()) {
		    // Look ahead the segment in order to detect
		    // the number of measures in the multi measure rest.
		    Segment::iterator mm_i = i;
		    while (s->isBeforeEndMarker(++mm_i)) {
			if ((*mm_i)->isa(Note::EventRestType) &&
			    (*mm_i)->getNotationDuration() == (*i)->getNotationDuration() &&
			    timeSignature == m_composition->getTimeSignatureAt((*mm_i)->getNotationAbsoluteTime())) {
			    MultiMeasureRestCount++;
			} else {
			    break;
			}
		    }
		    str << "R";
		} else {
		    str << "r";
		}
	    
		if (duration != prevDuration) {
		    writeDuration(duration, str);
		    if (MultiMeasureRestCount > 0) {
			str << "*" << (1 + MultiMeasureRestCount);
		    }
		    prevDuration = duration;
		}

		if (lilyText != "") {
		    str << lilyText;
		    lilyText = "";
		}

		str << " ";
	    
		handleEndingEvents(eventsInProgress, i, str);
		handleStartingEvents(eventsToStart, str);

		if (newBeamedGroup)
		    notesInBeamedGroup++;
	    } else {
		MultiMeasureRestCount--;
	    }
	    writtenDuration += soundingDuration;
        } else if ((*i)->isa(Clef::EventType)) {

            try {
                // Incomplete: Set which note the clef should center on  (DMM - why?)
                // To allow octavation of the clef, enclose the clefname always with quotes.
                str << "\\clef \"";

                Clef clef(**i);

                if (clef.getClefType() == Clef::Treble) {
                    str << "treble";
                } else if (clef.getClefType() == Clef::French) {
                    str << "french";
                } else if (clef.getClefType() == Clef::Soprano) {
                    str << "soprano";
                } else if (clef.getClefType() == Clef::Mezzosoprano) {
                    str << "mezzosoprano";
                } else if (clef.getClefType() == Clef::Alto) {
                    str << "alto";
                } else if (clef.getClefType() == Clef::Tenor) {
                    str << "tenor";
                } else if (clef.getClefType() == Clef::Baritone) {
                    str << "baritone";
                } else if (clef.getClefType() == Clef::Varbaritone) {
                    str << "varbaritone";
                } else if (clef.getClefType() == Clef::Bass) {
                    str << "bass";
                } else if (clef.getClefType() == Clef::Subbass) {
                    str << "subbass";
                }

                // Transpose the clef one or two octaves up or down, if specified.
                int octaveOffset = clef.getOctaveOffset();
                if (octaveOffset > 0) {
                    str << "^" << 8*octaveOffset;
                } else if (octaveOffset < 0) {
                    str << "_" << -8*octaveOffset;
                }

                str << "\"" << std::endl << indent(col);

            } catch (Exception e) {
                std::cerr << "Bad clef: " << e.getMessage() << std::endl;
            }

        } else if ((*i)->isa(Rosegarden::Key::EventType)) {

            try {
                str << "\\key ";
                key = Rosegarden::Key(**i);
                Accidental accidental = Accidentals::NoAccidental;

                std::cout << "key tonic pitch: " << key.getTonicPitch() << std::endl; //REMOVE
                str << convertPitchToLilyNote(key.getTonicPitch(), accidental, key);

                if (key.isMinor()) {
                    str << " \\minor";
                } else {
                    str << " \\major";
                }
                str << std::endl << indent(col);

            } catch (Exception e) {
                std::cerr << "Bad key: " << e.getMessage() << std::endl;
            }

        } else if ((*i)->isa(Text::EventType)) {

            if (!handleDirective(*i, lilyText, nextBarIsAlt1, nextBarIsAlt2,
                                 nextBarIsDouble, nextBarIsEnd, nextBarIsDot)) {
                handleText(*i, lilyText);
            }

        } else if ((*i)->isa(Guitar::Chord::EventType)) {

            try {
                Guitar::Chord chord = Guitar::Chord(**i);
                const Guitar::Fingering& fingering = chord.getFingering();
                
                int firstFret = fingering.getStartFret();
                int barreStart = 0, barreEnd = 0, barreFret = 0;

                // 
                // Check if there is a barre.
                //
                if (fingering.hasBarre()) {
                    Guitar::Fingering::Barre barre = fingering.getBarre();
                    barreStart = barre.start;
                    barreEnd = barre.end;
                    barreFret = barre.fret;
                }

                if (barreStart == 0) {
                    str << " s4*0^\\markup \\fret-diagram #\"";
                } else {
                    str << " s4*0^\\markup \\override #'(barre-type . straight) \\fret-diagram #\"";
                }
                //
                // Check each string individually.
                // Note: LilyPond numbers strings differently.
                //
                for (int stringNum = 6; stringNum >= 1; --stringNum) {
                    if (barreStart == stringNum) {
                        str << "c:" << barreStart << "-" << barreEnd << "-" << barreFret << ";";
                    }

                    if (fingering.getStringStatus( 6-stringNum ) == Guitar::Fingering::MUTED) {
                        str << stringNum << "-x;";
                    } else if (fingering.getStringStatus( 6-stringNum ) == Guitar::Fingering::OPEN) {
                        str << stringNum << "-o;";
                    } else {
                        int stringStatus = fingering.getStringStatus(6-stringNum);
                        if ((stringNum <= barreStart) && (stringNum >= barreEnd)) {
                            str << stringNum << "-" << barreFret << ";";
                        } else {
                            str << stringNum << "-" << stringStatus << ";";
                        }
                    }
                }
                str << "\" ";

            } catch (Exception e) { // GuitarChord ctor failed
                RG_DEBUG << "Bad GuitarChord event in Lilypond export" << endl;
            }
        }

        // LilyPond 2.0 introduces required postfix syntax for beaming
        if (m_exportBeams && newBeamedGroup && notesInBeamedGroup > 0) {
            str << "[ ";
            newBeamedGroup = false;
        }

        if ((*i)->isa(Indication::EventType)) {
            eventsToStart.insert(*i);
            eventsInProgress.insert(*i);
        }

        ++i;
    }

    if (groupId != -1) {
        if (groupType == GROUP_TYPE_TUPLED ||
                groupType == GROUP_TYPE_GRACE) {
            if (m_exportBeams && notesInBeamedGroup > 0)
                str << "] ";
            str << "} ";
            tupletRatio = std::pair<int, int>(1, 1);
        } else if (groupType == GROUP_TYPE_BEAMED) {
            if (m_exportBeams && notesInBeamedGroup > 0)
                str << "] ";
        }
    }

    if (lastStem != 0) {
        str << "\\stemNeutral ";
    }

    if (overlong) {
        str << std::endl << indent(col) <<
        qstrtostr(QString("% %1").
                  arg(i18n("warning: overlong bar truncated here")));
    }
    if (writtenDuration < barEnd - barStart) {
        str << std::endl << indent(col) <<
        qstrtostr(QString("% %1").
                  arg(i18n("warning: bar too short, padding with rests")))
        << std::endl << indent(col);
        writeSkip(timeSignature, writtenDuration,
                  (barEnd - barStart) - writtenDuration, true, str);
    }
    //
    // Export bar checks.
    //
    if (MultiMeasureRestCount == 0 && !nextBarIsDouble && !nextBarIsEnd && !nextBarIsDot) {
        str << " |";
    }
    if (nextBarIsDouble) {
        str << "\\bar \"||\" ";
        nextBarIsDouble = false;
    } else if (nextBarIsEnd) {
        str << "\\bar \"|.\" ";
        nextBarIsEnd = false;
    } else if (nextBarIsDot) {
        str << "\\bar \":\" ";
        nextBarIsDot = false;
    }
}

void
LilypondExporter::writeSkip(const TimeSignature &timeSig,
                            timeT offset,
                            timeT duration,
                            bool useRests,
                            std::ofstream &str)
{
    DurationList dlist;
    timeSig.getDurationListForInterval(dlist, duration, offset);

    int t = 0, count = 0;

    for (DurationList::iterator i = dlist.begin(); ; ++i) {

        if (i == dlist.end() || (*i) != t) {

            if (count > 0) {

                if (!useRests)
                    str << "\\skip ";
                else if (t == timeSig.getBarDuration())
                    str << "R";
                else
                    str << "r";

                writeDuration(t, str);

                if (count > 1)
                    str << "*" << count;
                str << " ";
            }

            if (i != dlist.end()) {
                t = *i;
                count = 1;
            }

        } else {
            ++count;
        }

        if (i == dlist.end())
            break;
    }
}

bool
LilypondExporter::handleDirective(const Event *textEvent,
                                  std::string &lilyText,
                                  bool &nextBarIsAlt1, bool &nextBarIsAlt2,
                                  bool &nextBarIsDouble, bool &nextBarIsEnd, bool &nextBarIsDot)
{
    Text text(*textEvent);

    if (text.getTextType() == Text::LilypondDirective) {
        std::string directive = text.getText();
        if (directive == Text::Segno) {
            lilyText += "^\\markup { \\musicglyph #\"scripts.segno\" } ";
        } else if (directive == Text::Coda) {
            lilyText += "^\\markup { \\musicglyph #\"scripts.coda\" } ";
        } else if (directive == Text::Alternate1) {
            nextBarIsAlt1 = true;
        } else if (directive == Text::Alternate2) {
            nextBarIsAlt1 = false;
            nextBarIsAlt2 = true;
        } else if (directive == Text::BarDouble) {
            nextBarIsDouble = true;
        } else if (directive == Text::BarEnd) {
            nextBarIsEnd = true;
        } else if (directive == Text::BarDot) {
            nextBarIsDot = true;
        } else {
            // pass along less special directives for handling as plain text,
            // so they can be attached to chords and whatlike without
            // redundancy
            return false;
        }
        return true;
    } else {
        return false;
    }
}

void
LilypondExporter::handleText(const Event *textEvent,
                             std::string &lilyText)
{
    try {

        Text text(*textEvent);
        std::string s = text.getText();

        // only protect illegal chars if this is Text, rather than
        // LilypondDirective
        if ((*textEvent).isa(Text::EventType))
            s = protectIllegalChars(s);

        if (text.getTextType() == Text::Tempo) {

            // print above staff, bold, large
            lilyText += "^\\markup { \\bold \\large \"" + s + "\" } ";

        } else if (text.getTextType() == Text::LocalTempo ||
                   text.getTextType() == Text::Chord) {

            // print above staff, bold, small
            lilyText += "^\\markup { \\bold \"" + s + "\" } ";

        } else if (text.getTextType() == Text::Dynamic) {

            // pass through only supported types
            if (s == "ppppp" || s == "pppp" || s == "ppp" ||
                    s == "pp" || s == "p" || s == "mp" ||
                    s == "mf" || s == "f" || s == "ff" ||
                    s == "fff" || s == "ffff" || s == "rfz" ||
                    s == "sf") {

                lilyText += "-\\" + s + " ";

            } else {
                std::cerr << "LilypondExporter::write() - illegal Lilypond dynamic: "
                << s << std::endl;
            }

        } else if (text.getTextType() == Text::Direction) {

            // print above staff, large
            lilyText += "^\\markup { \\large \"" + s + "\" } ";

        } else if (text.getTextType() == Text::LocalDirection) {

            // print below staff, bold italics, small
            lilyText += "_\\markup { \\bold \\italic \"" + s + "\" } ";

            // LilyPond directives that don't require special handling across
            // barlines are handled here along with ordinary text types.  These
            // can be injected wherever they happen to occur, and should get
            // attached to the right bits in due course without extra effort.
            //
        } else if (text.getText() == Text::Gliss) {
            lilyText += "\\glissando ";
        } else if (text.getText() == Text::Arpeggio) {
            lilyText += "\\arpeggio ";
        } else if (text.getText() == Text::Tiny) {
            lilyText += "\\tiny ";
        } else if (text.getText() == Text::Small) {
            lilyText += "\\small ";
        } else if (text.getText() == Text::NormalSize) {
            lilyText += "\\normalsize ";
        } else {
            textEvent->get
            <String>(Text::TextTypePropertyName, s);
            std::cerr << "LilypondExporter::write() - unhandled text type: "
            << s << std::endl;
        }
    } catch (Exception e) {
        std::cerr << "Bad text: " << e.getMessage() << std::endl;
    }
}

void
LilypondExporter::writePitch(const Event *note,
                             const Rosegarden::Key &key,
                             std::ofstream &str)
{
    // Note pitch (need name as well as octave)
    // It is also possible to have "relative" pitches,
    // but for simplicity we always use absolute pitch
    // 60 is middle C, one unit is a half-step

    long pitch = 60;
    note->get
    <Int>(PITCH, pitch);

    Accidental accidental = Accidentals::NoAccidental;
    note->get
    <String>(ACCIDENTAL, accidental);

    // format of Lilypond note is:
    // name + octave + (duration) + text markup

    // calculate note name and write note
    std::string lilyNote;

    lilyNote = convertPitchToLilyNote(pitch, accidental, key);

    str << lilyNote;

    // generate and write octave marks
    std::string octaveMarks = "";
    int octave = (int)(pitch / 12);

    // tweak the octave break for B# / Cb
    if ((lilyNote == "bisis") || (lilyNote == "bis")) {
        octave--;
    } else if ((lilyNote == "ceses") || (lilyNote == "ces")) {
        octave++;
    }

    if (octave < 4) {
        for (; octave < 4; octave++)
            octaveMarks += ",";
    } else {
        for (; octave > 4; octave--)
            octaveMarks += "\'";
    }

    str << octaveMarks;
}

void
LilypondExporter::writeStyle(const Event *note, std::string &prevStyle,
                             int col, std::ofstream &str, bool isInChord)
{
    // some hard-coded styles in order to provide rudimentary style export support
    // note that this is technically bad practice, as style names are not supposed
    // to be fixed but deduced from the style files actually present on the system
    const std::string styleMensural = "Mensural";
    const std::string styleTriangle = "Triangle";
    const std::string styleCross = "Cross";
    const std::string styleClassical = "Classical";

    // handle various note styles before opening any chord
    // brackets
    std::string style = "";
    note->get
    <String>(NotationProperties::NOTE_STYLE, style);

    if (style != prevStyle) {

        if (style == styleClassical && prevStyle == "")
            return ;

        if (!isInChord)
            prevStyle = style;

        if (style == styleMensural) {
            style = "mensural";
        } else if (style == styleTriangle) {
            style = "triangle";
        } else if (style == styleCross) {
            style = "cross";
        } else {
            style = "default"; // failsafe default or explicit
        }

        if (!isInChord) {
            str << std::endl << indent(col) << "\\override Voice.NoteHead #'style = #'" << style << std::endl << indent(col);
        } else {
            str << "\\tweak #'style #'" << style << " ";
        }
    }
}

void
LilypondExporter::writeDuration(timeT duration,
                                std::ofstream &str)
{
    Note note(Note::getNearestNote(duration, MAX_DOTS));

    switch (note.getNoteType()) {

    case Note::SixtyFourthNote:
        str << "64";
        break;

    case Note::ThirtySecondNote:
        str << "32";
        break;

    case Note::SixteenthNote:
        str << "16";
        break;

    case Note::EighthNote:
        str << "8";
        break;

    case Note::QuarterNote:
        str << "4";
        break;

    case Note::HalfNote:
        str << "2";
        break;

    case Note::WholeNote:
        str << "1";
        break;

    case Note::DoubleWholeNote:
        str << "\\breve";
        break;
    }

    for (int numDots = 0; numDots < note.getDots(); numDots++) {
        str << ".";
    }
}

void
LilypondExporter::writeSlashes(const Event *note, std::ofstream &str)
{
    // write slashes after text
    // / = 8 // = 16 /// = 32, etc.
    long slashes = 0;
    note->get
    <Int>(NotationProperties::SLASHES, slashes);
    if (slashes > 0) {
        str << ":";
        int length = 4;
        for (int c = 1; c <= slashes; c++) {
            length *= 2;
        }
        str << length;
    }
}

}
