// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    Additions by
        Michael McIntyre    <dmmcintyr@users.sourceforge.net>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#include <klocale.h> // i18n
#include <kconfig.h> // KConfig
#include <kapp.h>
#include <kmessagebox.h>

#include <qstring.h>
#include <qregexp.h> // QT3.0 replace()
#include <qtextcodec.h>

#include "lilypondio.h"
#include "config.h"
#include "rosestrings.h" // strtoqstr
#include "notationproperties.h"

#include "Composition.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using Rosegarden::BaseProperties;
using Rosegarden::Bool;
using Rosegarden::Clef;
using Rosegarden::Composition;
using Rosegarden::Indication;
using Rosegarden::Int;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Segment;
using Rosegarden::SegmentNotationHelper;
using Rosegarden::String;
using Rosegarden::timeT;
using Rosegarden::TimeSignature;
using Rosegarden::Accidental;
using Rosegarden::Accidentals;
using Rosegarden::Text;
using Rosegarden::PropertyName;
using Rosegarden::Marks;
using Rosegarden::Configuration;

LilypondExporter::LilypondExporter(QObject *parent,
                                   Composition *composition,
                                   std::string fileName) :
                                   ProgressReporter(parent, "lilypondExporter"),
                                   m_composition(composition),
                                   m_fileName(fileName) {
    // nothing
}

LilypondExporter::~LilypondExporter() {
    // nothing
}

void
LilypondExporter::handleStartingEvents(eventstartlist &eventsToStart,
        std::ofstream &str) {
    for (eventstartlist::iterator m = eventsToStart.begin();
         m != eventsToStart.end();
         ++m) {
        if ((*m)->isa(Indication::EventType)) {
            std::string which((*m)->get<String>(Indication::IndicationTypePropertyName));
            if (which == Indication::Slur) {
                str << "( ";
            } else if (which == Indication::Crescendo) {
                str << "\\< ";
            } else if (which == Indication::Decrescendo) {
                str << "\\> ";
            }
        } else {
            // Not an indication
        }
        
        // Incomplete: erase during iteration not guaranteed
        // This is bad, but can't find docs on return value at end
        // i.e. I want to increment m with m = events...erase(m), but
        // what is returned when erase(eventsToStart.end())?
        eventsToStart.erase(m);
    }
    // DMM - addTie removed because it was doing the wrong thing here.
}

void
LilypondExporter::handleEndingEvents(eventendlist &eventsInProgress, Segment::iterator &j,
        std::ofstream &str) {
    for (eventendlist::iterator k = eventsInProgress.begin();
         k != eventsInProgress.end();
         ++k) {
        // Handle and remove all the relevant events in progress
        // This assumes all deferred events are indications
        long indicationDuration = 0;
        (*k)->get<Int>(Indication::IndicationDurationPropertyName, indicationDuration);
        if ((*k)->getAbsoluteTime() + indicationDuration <=
            (*j)->getAbsoluteTime() + (*j)->getDuration()) {
            if ((*k)->isa(Indication::EventType)) {
                std::string whichIndication((*k)->get<String>
                        (Indication::IndicationTypePropertyName));
        
                if (whichIndication == Indication::Slur) {
                    str << ") ";
                } else if (whichIndication == Indication::Crescendo ||
                           whichIndication == Indication::Decrescendo) {
                    str << "\\! "; 
                }
                eventsInProgress.erase(k);
            } else {
                std::cerr << "\nLilypondExporter::handleEndingEvents - unhandled deferred ending event, type: " << (*k)->getType();
            }
        }
    }
}

// processes input to produce a Lilypond-format note written correctly for all
// keys and out-of-key accidental combinations.
std::string
LilypondExporter::convertPitchToLilyNote (long pitch, bool isFlatKeySignature,
                        int accidentalCount, Accidental accidental) {
    std::string lilyNote = "";
    int pitchNote, c;
    
    // get raw semitone number
    pitchNote = (pitch % 12);

    // no accidental, or notes with Natural property
    switch (pitchNote) {
        case 0:  lilyNote = "c";
                 break;
        case 2:  lilyNote = "d";
                 break;
        case 4:  lilyNote = "e";
                 break;
        case 5:  lilyNote = "f";
                 break;
        case 7:  lilyNote = "g";
                 break;
        case 9:  lilyNote = "a";
                 break;
        case 11: lilyNote = "b";
                 break;
        // failsafe to deal with annoying fact that imported/recorded notes don't have
        // persistent accidental properties
        case 1:  lilyNote = (isFlatKeySignature) ? "des" : "cis";
                 break;
        case 3:  lilyNote = (isFlatKeySignature) ? "ees" : "dis";
                 break;
        case 6:  lilyNote = (isFlatKeySignature) ? "ges" : "fis";
                 break;
        case 8:  lilyNote = (isFlatKeySignature) ? "aes" : "gis";
                 break;
        case 10: lilyNote = (isFlatKeySignature) ? "bes" : "ais";
                 break;
    }
        
    // assign out-of-key accidentals first, by BaseProperty::ACCIDENTAL
    if (accidental != "") {
        if (accidental == Accidentals::Sharp) {
            switch (pitchNote) {
                case  5: lilyNote = "eis"; // 5 + Sharp = E#
                         break;
                case  0: lilyNote = "bis"; // 0 + Sharp = B#
                         break;
                case  1: lilyNote = "cis";
                         break;
                case  3: lilyNote = "dis";
                         break;
                case  6: lilyNote = "fis";
                         break;
                case  8: lilyNote = "gis";
                         break;
                case 10: lilyNote = "ais";
            }
        } else if (accidental == Accidentals::Flat) {
            switch (pitchNote) {
                case 11: lilyNote = "ces"; // 11 + Flat = Cb
                         break;
                case  4: lilyNote = "fes"; //  4 + Flat = Fb
                         break;
                case  1: lilyNote = "des";
                         break;
                case  3: lilyNote = "ees";
                         break;
                case  6: lilyNote = "ges";
                         break;
                case  8: lilyNote = "aes";
                         break;
                case 10: lilyNote = "bes";
            }
        } else if (accidental == Accidentals::DoubleSharp) {
            switch (pitchNote) {
                case  1: lilyNote = "bisis"; // 1 + ## = B##
                         break;
                case  2: lilyNote = "cisis"; // 2 + ## = C##
                         break;
                case  4: lilyNote = "disis"; // 4 + ## = D##
                         break;
                case  6: lilyNote = "eisis"; // 6 + ## = E##
                         break;
                case  7: lilyNote = "fisis"; // 7 + ## = F##
                         break;
                case  9: lilyNote = "gisis"; // 9 + ## = G##
                         break;
                case 11: lilyNote = "aisis"; //11 + ## = A##
                         break;
            }
        } else if (accidental == Accidentals::DoubleFlat) {
            switch (pitchNote) {
                case 10: lilyNote = "ceses"; //10 + bb = Cbb
                         break;
                case  0: lilyNote = "deses"; // 0 + bb = Dbb
                         break;
                case  2: lilyNote = "eeses"; // 2 + bb = Ebb
                         break;
                case  3: lilyNote = "feses"; // 3 + bb = Fbb
                         break;
                case  5: lilyNote = "geses"; // 5 + bb = Gbb
                         break;
                case  7: lilyNote = "aeses"; // 7 + bb = Abb
                         break;
                case  9: lilyNote = "beses"; // 9 + bb = Bbb
                         break;
            }
        } else if (accidental == Accidentals::Natural) {
            // do we have anything explicit left to do in this
            // case?  probably not, but I'll leave this placeholder for now
            //
            // eg. note is B + Natural in key Cb, but since it has Natural
            // it winds up here, instead of getting the Cb from the key below.
            // since it will be called "b" from the first switch statement in
            // the entry to this complex logic block, and nothing here changes it,
            // the implicit handling to this point should resolve the case
            // without further effort.
        }
    } else {  // no explicit accidental; note must be in-key
        for (c = 0; c <= accidentalCount; c++) {
            if (isFlatKeySignature) {                              // Flat Keys:
                switch (c) {
                    case 7: if (pitchNote == 11) lilyNote = "ces"; // Cb 
                    case 6: if (pitchNote ==  4) lilyNote = "fes"; // Fb 
                    case 5: if (pitchNote ==  6) lilyNote = "ges"; // Gb 
                    case 4: if (pitchNote ==  1) lilyNote = "des"; // Db 
                    case 3: if (pitchNote ==  3) lilyNote = "ees"; // Eb 
                    case 2: if (pitchNote ==  8) lilyNote = "aes"; // Ab 
                    case 1: if (pitchNote == 10) lilyNote = "bes"; // Bb 
                }
            } else {                                               // Sharp Keys:
                switch (c) {                                       
                    case 7: if (pitchNote ==  0) lilyNote = "bis"; // C# 
                    case 6: if (pitchNote ==  5) lilyNote = "eis"; // F# 
                    case 5: if (pitchNote == 10) lilyNote = "ais"; // B  
                    case 3: if (pitchNote ==  8) lilyNote = "gis"; // A  
                    case 4: if (pitchNote ==  3) lilyNote = "dis"; // D  
                    case 2: if (pitchNote ==  1) lilyNote = "cis"; // D  
                    case 1: if (pitchNote ==  6) lilyNote = "fis"; // G  
                }
           }
       } 
    }

    // leave this in to see if there are any _other_ problems that are going
    // to break this method...
    if (lilyNote == "") {
        std::cerr << "LilypondExporter::convertPitchToLilyNote() -  WARNING: cannot resolve note"
                  << std::endl << "pitch = " << pitchNote << "\tkey sig. = "
                  << ((isFlatKeySignature) ? "flat" : "sharp") << "\tno. of accidentals = "
                  << accidentalCount << "\textra accidental = \"" << accidental << "\""
                  << std::endl;
        
    }

    return lilyNote;
}

// composes and returns a string to be appended to a note in order to
// represent various Marks (tenuto, accents, etc.)
//
// warning:  text markup features are slated to be re-written
// and the syntax/semantics completely altered, so this will probably
// need extensive reworking eventually, possibly with version awareness
// 
std::string
LilypondExporter::composeLilyMark(std::string eventMark, bool stemUp) {

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
            inStr = "#'(italic \"" + inStr + "\")";
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

// return a string of tabs, used to make it easier to change the horizontal
// layout of export if sections need to be added/moved
std::string
LilypondExporter::indent(const int &column) {
    std::string outStr = "";
    for (int c = 1; c <= column; c++) {
        outStr += "    ";
    }
    return outStr;
}

// close a chord bracket if necessary, and/or add a tie if necessary (tie must
// be outside of chord bracket or Lilypond complains and does silly things...
// this was originally a part of handleStartingEvents, but I split it out
// into this because handleStartingEvents was doing indescribably terrible
// things to slurs/hairpins involving chords
void
LilypondExporter::closeChordWriteTie(bool &addTie, bool &currentlyWritingChord,
                                     std::ofstream &str) {
    // end chord bracket
    if (currentlyWritingChord) {
        currentlyWritingChord = false;
        str << "> ";
    }
    // and/or add a tie (to end of chord, or between plain notes)
    if (addTie) {
        addTie = false;
        str << "~ ";
    }
}    

// find/protect illegal chars in user-supplied strings
//
// (lots of testing probably needed here...  many of these chars can't be
// protected...  lilypond is OK with \{ , but TeX barfs...  none of that is very robust
// at all, so I've changed to just completely dump the most questionable
// things [ { ( ) } ]
std::string
LilypondExporter::protectIllegalChars(std::string inStr) {

    QString tmpStr = strtoqstr(inStr);

    tmpStr.replace(QRegExp("_"), " ");
    tmpStr.replace(QRegExp("&"), "\\&");
    tmpStr.replace(QRegExp("\\^"), "\\^");
    tmpStr.replace(QRegExp("#"), "\\#");
    tmpStr.replace(QRegExp("%"), "\\%");
    tmpStr.replace(QRegExp("<"), "\\<");
    tmpStr.replace(QRegExp(">"), "\\>");
    tmpStr.replace(QRegExp("\\["), "");
    tmpStr.replace(QRegExp("\\]"), "");
    tmpStr.replace(QRegExp("\\{"), "");
    tmpStr.replace(QRegExp("\\}"), "");
// () seem OK
//    tmpStr.replace(QRegExp("\\("), "");
//    tmpStr.replace(QRegExp("\\)"), "");
    tmpStr.replace(QRegExp("\""), "");
    tmpStr.replace(QRegExp("'"), "");
    tmpStr.replace(QRegExp("-"), "\\-");

    // [cc] Convert to latin1, which is what Lilypond expects.
    // I have no idea whether, or how, non-latin1 characters can be
    // handled by Lilypond; I just know if you feed it utf8 you get
    // garbage.

    static QTextCodec *codec(QTextCodec::codecForName("ISO8859-1"));
    return codec->fromUnicode(tmpStr).data();
}

bool
LilypondExporter::write() {
    QString tmp_fileName = strtoqstr(m_fileName);
    // The correct behaviour is to show an error box warning the user -
    // but for now, just remove the illegal chars automatically.
//     if (tmp_fileName.contains(' ') || tmp_fileName.contains('\\')) {
//         // Error: "Lilypond does not allow spaces or backslashes in
//         // the name of the .ly file.  Would you like RG to remove them
//         // automatically or go back and change the filename yourself?"
//         // [Remove] [Cancel Save]
//         return false;
//     }
    tmp_fileName.replace(QRegExp(" "), "");
    tmp_fileName.replace(QRegExp("\\\\"), "");
    tmp_fileName.replace(QRegExp("'"), "");
    tmp_fileName.replace(QRegExp("\""), "");

    std::ofstream str(qstrtostr(tmp_fileName).c_str(), std::ios::out);
    if (!str) {
        std::cerr << i18n("LilypondExporter::write() - can't write file %1").arg(tmp_fileName) << std::endl;
        return false;
    }

    // define some Lilypond headers as const std::strings because == doesn't
    // work between std::string and string literals...  putting this right here isn't
    // very tidy, but it isn't really needed anywhere else, and this seems the
    // least complicated way to get from hither to thither...
    const std::string headerTitle = "title";
    const std::string headerSubtitle = "subtitle";
    const std::string headerPoet = "poet";
    const std::string headerComposer = "composer";
    const std::string headerMeter = "meter";
    const std::string headerArranger = "arranger";
    const std::string headerInstrument =  "instrument";
    const std::string headerDedication = "dedication";
    const std::string headerPiece =  "piece";
    const std::string headerHead = "head";
    const std::string headerCopyright = "copyright";
    const std::string headerFooter = "footer";
    const std::string headerTagline = "tagline";

    // grab config info
    KConfig *cfg = kapp->config();
    cfg->setGroup("Notation Options");

    unsigned int paperSize = cfg->readUnsignedNumEntry("lilypapersize");
    unsigned int fontSize = cfg->readUnsignedNumEntry("lilyfontsize");
//!!!    unsigned int restType = cfg->readUnsignedNumEntry("lilyresttype");
    bool exportLyrics = cfg->readBoolEntry("lilyexportlyrics");
    bool exportHeaders = cfg->readBoolEntry("lilyexportheaders");
    bool exportMidi = cfg->readBoolEntry("lilyexportmidi");
    bool exportUnmuted = cfg->readBoolEntry("lilyexportunmuted");
    bool exportPointAndClick = cfg->readBoolEntry("lilyexportpointandclick");

    // enable "point and click" debugging via xdvi to make finding the
    // unfortunately inevitable errors easier
    if (exportPointAndClick) {
        str << "% point and click debugging:" << std::endl;
        str << "#(set-point-and-click! 'line)" << std::endl;
    }

    // Lilypond \header block
    str << "\\version \"1.6.0\"" << std::endl;

    // set indention level to make future changes to horizontal layout less
    // tedious, ++col to indent a new level, --col to de-indent
    int col = 0;

    // grab user headers from metadata
    Configuration metadata = m_composition->getMetadata();
    std::vector<std::string> propertyNames = metadata.getPropertyNames();
    
    // open \header section if there's metadata to grab, and if the user
    // wishes it
    if (!propertyNames.empty() && exportHeaders) {
        str << "\\header {" << std::endl;
        col++;  // indent+

        bool userTagline = false, userFooter = false;

        for (unsigned int index = 0; index < propertyNames.size(); ++index) {
            std::string property = propertyNames [index];
            if (property == headerTitle || property == headerSubtitle ||
                property == headerPoet  || property == headerComposer ||
                property == headerMeter || property == headerArranger ||
                property == headerInstrument || property == headerDedication ||
                property == headerPiece || property == headerHead ||
                property == headerCopyright || property == headerFooter ||
                property == headerTagline) {
                std::string header = protectIllegalChars(metadata.get<String>(property));
                if (header != "") {
                    str << indent(col) << property << " = \"" << header << "\"" << std::endl;
                    // let users override defaults, but allow for providing
                    // defaults if they don't:
                    if (property == headerTagline) userTagline = true;
                    if (property == headerFooter) userFooter = true;
                }
            }
        }

        // default tagline/footer
        if (!userTagline) {
            str << indent(col) << "tagline = \"" << "Exported by Rosegarden " << VERSION 
                << "\"" << std::endl;
        }

        if (!userFooter) {
            str << indent(col) << "footer = \"" << "Rosegarden " << VERSION
                << "\"" << std::endl;
        }
                
        // close \header
        str << indent(--col) << "}" << std::endl;
    }

    // Lilypond music data!   Mapping:
    // Lilypond Voice = Rosegarden Segment
    // Lilypond Staff = Rosegarden Track
    // (not the cleanest output but maybe the most reliable)
    // Incomplete: add an option to cram it all into one grand staff
    
    // include appropriate paper file to make the specified paper/font sizes
    // work...
    int font = 20; // default, if config problem
    switch (fontSize) {
        case 0 : font = 11; break;
        case 1 : font = 13; break;
        case 2 : font = 16; break;
        case 3 : font = 19; break;
        case 4 : font = 20; break;
        case 5 : font = 23; break;
        case 6 : font = 26; break;
    }
    str << indent(col) << "\\include \"paper" << font << ".ly\"" << std::endl;
   
    // open \score section
    str << "\\score {" << std::endl;
    str << indent(++col) << "\\notes <" << std::endl;  // indent+

    // Make chords offset colliding notes by default
    str << indent(++col) << "% force offset of colliding notes in chords:" << std::endl;
    str << indent(col) << "\\property Score.NoteColumn \\override #\'force-hshift = #1.0"
        << std::endl;
    
    // set initial time signature
    TimeSignature timeSignature = m_composition->
            getTimeSignatureAt(m_composition->getStartMarker());

    str << indent(col) << "\\time "
        << timeSignature.getNumerator() << "/"
        << timeSignature.getDenominator() << "" << std::endl;

    bool isFlatKeySignature = false;
    int lastTrackIndex = -1;
    int voiceCounter = 0;

    // Lilypond remembers the duration of the last note or
    // rest and reuses it unless explicitly changed.
    Note::Type lastType = Note::QuarterNote;
    int lastNumDots = 0;

    int trackNo = 0;

    // Write out all segments for each Track
    for (Composition::iterator i = m_composition->begin();
         i != m_composition->end(); ++i) {

        emit setProgress(int(double(trackNo++)/
                             double(m_composition->getNbTracks()) * 100.0));
        kapp->processEvents(50);

        timeT lastChordTime = m_composition->getStartMarker() - 1;
        bool currentlyWritingChord = false;
      
        // We may need to wait before adding a tie if we are currently in a
        // chord
        bool addTie = false;

        // do nothing if track is muted...  this provides a crude but easily implemented
        // method for users to selectively export tracks...
        Rosegarden::Track *track = m_composition->getTrackByIndex((*i)->getTrack());
        
        if (exportUnmuted && (!track->isMuted())) {
            if ((int) (*i)->getTrack() != lastTrackIndex) {
                if (lastTrackIndex != -1) {
                    // close the old track (Staff context)
                    str << std::endl << indent(--col) << "> % Staff" << std::endl;  // indent-
                }
                lastTrackIndex = (*i)->getTrack();

                // avoid problem with <untitled> tracks yielding a .ly file that
                // jumbles all notes together on a single staff...
                // every Staff context has to have a unique name, even if the
                // Staff.instrument property is the same for multiple staffs...
                std::ostringstream staffName;
                staffName << protectIllegalChars(m_composition->
                        getTrackByIndex(lastTrackIndex)->getLabel());

                if (staffName.str() == "") {
                    staffName << "track";
                }
                
                str << indent(col) << "\\context Staff = \"" << staffName.str()
                    << " " << (voiceCounter +1) << "\" < " << std::endl;

                str << indent(++col)<< "\\property Staff.instrument = \""  // indent+
                    << staffName.str() <<"\"" << std::endl;;
            
            }
            SegmentNotationHelper tmpHelper(**i);

            // Temporary storage for non-atomic events (!BOOM)
            // ex. Lilypond expects signals when a decrescendo starts 
            // as well as when it ends
            eventendlist eventsInProgress;
            eventstartlist eventsToStart;
          
            // If the segment doesn't start at 0, add a "skip" to the start
            // No worries about overlapping segments, because Voices can overlap
            // voiceCounter is a hack because Lilypond does not by default make 
            // them unique
            std::ostringstream voiceNumber, lyricNumber;
            voiceNumber << "voice " << voiceCounter;
            lyricNumber << "lyric " << voiceCounter++;
            
            str << indent(col++) << "\\context Voice = \"" << voiceNumber.str()
                << "\" {"; // indent+
            
            timeT segmentStart = (*i)->getStartTime(); // getFirstEventTime
            
            if (segmentStart > 0) {
                long curNote = long(Note(Note::WholeNote).getDuration());
                long wholeNoteDuration = curNote;
                // Incomplete: Make this a constant!
                // This is the smallest unit on which a Segment may begin
                long MIN_NOTE_SKIP_DURATION = long(Note(Note::ThirtySecondNote).getDuration());
                
                while (curNote >= MIN_NOTE_SKIP_DURATION) {
                    int numCurNotes = ((int)(segmentStart / curNote));
                    if (numCurNotes > 0) {
                        str << std::endl << indent(col) << "\\skip "
                            << (wholeNoteDuration / curNote)
                            << "*" << numCurNotes << std::endl;
                        segmentStart = segmentStart - numCurNotes*curNote;
                    }
                    curNote /= 2;
                }
            }

            timeT prevTime = 0;
            int curTupletNotesRemaining = 0;
            int accidentalCount = 0; 
    // WIP       int timeSignatureIterator = 0;

            // declare these outside the scope of the coming for loop
            std::string lilyText = "";      // text events
            std::ostringstream lilyLyrics;  // stream to collect/hold lyric events
            
            // Write out all events for this Segment
            for (Segment::iterator j = (*i)->begin(); j != (*i)->end(); ++j) {

                // We need to deal with absolute time if we don't have 
                // "complete" lines of music and/or nonoverlapping events
                // i.e. chords etc.
                timeT absoluteTime = (*j)->getAbsoluteTime();

                // new bar
                bool multipleTimeSignatures = ((m_composition->getTimeSignatureCount ()) > 1);
                
    /*  WIP       long timeSignatureCount = m_composition->getTimeSignatureCount();
                bool multipleTimeSignatures = (timeSignatureCount > 1);

                if (multipleTimeSignatures) {
                    timeSignatureIterator++;
                    timeSignature = m_composition->getTimeSignatureChange(timeSignatureIterator);
                    */
                
                if (j == (*i)->begin() ||
                    (prevTime < m_composition->getBarStartForTime(absoluteTime))) {

                    // bar check, for debugging measures that don't count out
                    // make this a toggle...
/*                    if (prevTime <= m_composition->getBarStartForTime(absoluteTime)) {
                        str << " | ";
                    }
good idea, but why are measures in 4/4 coming out with 5-6 beats?  this is no good...
*/

                    
                    // end the line for the current measure
                    str << std::endl << indent(col);
                    
                    // DMM - this is really pretty much an ugly hack, but it works...
                    // check time signature against previous one; if different,
                    // write new one here at bar line...
                    // FIXME - rewrite this to getTimeSignatureChange for time
                    // sig. 1, look at "now" to see if it's time to write it, and
                    // when we get to that point, ++ the time sig iterator to grab
                    // the next change and be looking for that, etc. until out of
                    // events.  Probably much less ugly and worth doing, but the
                    // existing implementation works well enough to put off doing
                    // that until some day when I run out of more exciting
                    // things to do...
                    //
                    // (WIP)
                    TimeSignature prevTimeSignature = timeSignature;
                    if (multipleTimeSignatures) {
                        timeSignature = m_composition->getTimeSignatureAt(absoluteTime);
                        if (
                            (timeSignature.getNumerator() != prevTimeSignature.getNumerator())
                            || 
                            (timeSignature.getDenominator() != prevTimeSignature.getDenominator())
                            &&
                            !(timeSignature.isHidden())
                           ) {
                            str << "\\time "
                                << timeSignature.getNumerator() << "/"
                                << timeSignature.getDenominator() << std::endl << indent(col);
                        }
                    }
                }
                
                prevTime = absoluteTime;

                timeT duration = (*j)->getNotationDuration();

                // handle text events...  unhandled text types:
                // UnspecifiedType, StaffName, ChordName, KeyName, Annotation
                //
                // text must be bound to a note, and it needs to be bound to the
                // note immediately after receiving the text event, so we'll
                // process it, then continue out of the loop, and then
                // write it out the next time we have a Note event.
                //
                // Incomplete?  There may be cases where binding it to the next
                // note arbitrarily is the wrong thing to do...
                if ((*j)->isa(Text::EventType)) {
                    
                    std::string text = "";
                    (*j)->get<String>(Text::TextPropertyName, text);
                    text = protectIllegalChars(text);

                    // Incomplete - these interpretations aren't that great, but
                    // this is about all we can do to represent what the notation
                    // editor displays for these text types without getting into highly
                    // arcane, complicated Lilypond twiddling...

                    if (Text::isTextOfType(*j, Text::Tempo)) {
                        // print above staff, bold, large
                        lilyText = "^#'((bold Large)\"" + text + "\")";
                    } else if (Text::isTextOfType(*j, Text::LocalTempo)) {
                        // print above staff, bold, small
                        lilyText = "^#'(bold \"" + text + "\")";
                    } else if (Text::isTextOfType(*j, Text::Lyric)) {
                        lilyLyrics << text << " ";
                    } else if (Text::isTextOfType(*j, Text::Dynamic)) {
                        // pass through only supported types
                        if (text == "ppp" || text == "pp"  || text == "p"  ||
                            text == "mp"  || text == "mf"  || text == "f"  ||
                            text == "ff"  || text == "fff" || text == "rfz" ||
                            text == "sf") {
                            
                            lilyText = "-\\" + text;
                        } else {
                            std::cerr << "LilypondExporter::write() - illegal Lilypond dynamic: "
                                      << text << std::endl;
                        }                         
                    } else if (Text::isTextOfType(*j, Text::Direction)) {
                        // print above staff, large
//                        lilyText = "^#'(Large\"" + text + "\")";
                        lilyText = " \\mark \"" + text + "\"";
                    } else if (Text::isTextOfType(*j, Text::LocalDirection)) {
                        // print below staff, bold italics, small
                        lilyText = "_#'((bold italic) \"" + text + "\")";
                    } else {
                        (*j)->get<String>(Text::TextTypePropertyName, text);
                        std::cerr << "LilypondExporter::write() - unhandled text type: "
                                  << text << std::endl;
                    }
                    
                    // jump out of this for loop so that lilyText can be attached
                    // to the next note we come to...  this might actually be
                    // redundant, but it shouldn't hurt anything just in case
                    continue;

                } else if ((*j)->isa(Note::EventType) ||
                            (*j)->isa(Note::EventRestType)) {
                    // Tuplet code from notationhlayout.cpp
                    int tcount = 0;
                    int ucount = 0;
                    if ((*j)->has(BaseProperties::BEAMED_GROUP_TUPLET_BASE)) {
                        tcount = (*j)->get<Int>(BaseProperties::BEAMED_GROUP_TUPLED_COUNT);
                        ucount = (*j)->get<Int>(BaseProperties::BEAMED_GROUP_UNTUPLED_COUNT);
                        assert(tcount != 0);

                        duration = ((*j)->getNotationDuration() / tcount) * ucount;
                        if (curTupletNotesRemaining == 0) {
                            // +1 is a hack so we can close the tuplet bracket
                            // at the right time
                            curTupletNotesRemaining = ucount + 1;
                        }
                    }
                    
                    Note tmpNote = Note::getNearestNote(duration, MAX_DOTS);
                    std::string lilyMark = "";

                    if ((*j)->isa(Note::EventType)) {
                        // Algorithm for writing chords:
                        // 1) Close the old chord
                        //   - if there is one
                        //   - if the next note is not part of the old chord
                        // 2) Open the new chord
                        //   - if the next note is in a chord
                        //   - and we're not writing one now
                        bool nextNoteIsInChord = tmpHelper.noteIsInChord(*j);
                        if (currentlyWritingChord &&
                            (!nextNoteIsInChord ||
                             (nextNoteIsInChord && lastChordTime != absoluteTime))) {
                            closeChordWriteTie(addTie, currentlyWritingChord, str);
                        }
                        if (curTupletNotesRemaining > 0) {
                            curTupletNotesRemaining--;
                            if (curTupletNotesRemaining == 0) {
                                str << "} ";
                            }
                        }
                        if (ucount == curTupletNotesRemaining &&
                            ucount != 0) {
                            str << "\\times " << tcount << "/" << ucount << " { ";
                        }
                        if (nextNoteIsInChord && !currentlyWritingChord) {
                            currentlyWritingChord = true;
                            str << "< ";
                            lastChordTime = absoluteTime;
                        }

                        // Incomplete: velocity?
                        long velocity = 127;
                        (*j)->get<Int>(BaseProperties::VELOCITY, velocity);

                        // close out any pending slurs/hairpins
                        handleEndingEvents(eventsInProgress, j, str);

                        // Note pitch (need name as well as octave)
                        // It is also possible to have "relative" pitches,
                        // but for simplicity we always use absolute pitch
                        // 60 is middle C, one unit is a half-step
                        long pitch = 0;

                        // format of Lilypond note is:
                        // name + (duration) + octave + text markup

                        // calculate note name and write note
                        Accidental accidental;
                        std::string lilyNote;

                        (*j)->get<Int>(BaseProperties::PITCH, pitch);
                        
                        (*j)->get<String>(BaseProperties::ACCIDENTAL, accidental);

                        lilyNote = convertPitchToLilyNote(pitch, isFlatKeySignature,
                                                      accidentalCount, accidental);

                        str << lilyNote;

                        // generate and write octave marks
                        std::string octaveMarks = "";
                        int octave = (int)(pitch / 12);

                        // tweak the octave break for B# / Cb
                        if ((lilyNote == "bisis")||(lilyNote == "bis")) {
                            octave--;
                        } else if ((lilyNote == "ceses")||(lilyNote == "ces")) {
                            octave++;
                        }

                        if (octave < 4) {
                            for (; octave < 4; octave++) octaveMarks += ",";
                        } else {
                            for (; octave > 4; octave--) octaveMarks += "\'";
                        }

                        str << octaveMarks;

                        // string together Marks, to be written outside this if
                        // block we're in, after the duration is written
                        long markCount = 0;
                        bool stemUp;
                        std::string eventMark = "",
                                    stem = "";
                        
                        (*j)->get<Int>(BaseProperties::MARK_COUNT, markCount);
                        (*j)->get<Bool>(BaseProperties::STEM_UP, stemUp);

                        if (markCount > 0) {
                            for (int c = 0; c < markCount; c++) {
                                (*j)->get<String>(BaseProperties::getMarkPropertyName(c),
                                    eventMark);
                                std::string mark = composeLilyMark(eventMark, stemUp);
                                lilyMark += mark;
                            }
                        }

                    } else { // it's a rest

                        if (curTupletNotesRemaining > 0) {
                            curTupletNotesRemaining--;
                            if (curTupletNotesRemaining == 0) {
                                str << "} ";
                            }
                        }
                        if (ucount == curTupletNotesRemaining &&
                            ucount != 0) {
                            str << "\\times " << tcount << "/" << ucount << " { ";
                        }
                        handleEndingEvents(eventsInProgress, j, str);
                        str << "r";
                    }
                    
                    // Note/rest length (type), including dots
                    if (tmpNote.getNoteType() != lastType ||
                        tmpNote.getDots() != lastNumDots) {
                        switch (tmpNote.getNoteType()) {
                        case Note::SixtyFourthNote:
                            str << "64"; break;
                        case Note::ThirtySecondNote:
                            str << "32"; break;
                        case Note::SixteenthNote:
                            str << "16"; break;
                        case Note::EighthNote:
                            str << "8"; break;
                        case Note::QuarterNote:
                            str << "4"; break;
                        case Note::HalfNote:
                            str << "2"; break;
                        case Note::WholeNote:
                            str << "1"; break;
                       case Note::DoubleWholeNote:
                           str << "\\breve"; break;
                        }
                        lastType = tmpNote.getNoteType();
                        for (int numDots = 0; numDots < tmpNote.getDots(); numDots++) {
                            str << ".";
                        }
                        lastNumDots = tmpNote.getDots();
                    } // end isa Note

                    // write string of marks after duration
                    if (lilyMark != "") {
                        str << lilyMark;
                    }

                    // write text carried forward from previous iteration of the
                    // for loop we're in
                    if (lilyText != "") {
                        str << lilyText;
                        lilyText = "";  // purge now that text has been bound to note
                    }

                    // write slashes after text
                    // / = 8 // = 16 /// = 32, etc.
                    long slashes = 0;
                    (*j)->get<Int>(NotationProperties::SLASHES, slashes);
                    if (slashes > 0) {
                        str << ":";
                        int length = 4;
                        for (int c = 1; c <= slashes; c++) {
                            length *= 2;
                        }
                        str << length;
                    }
                    
                    // decide whether or not we need a tie, to be handled later...
                    // this complication is necessary because Lilypond does
                    // bizarre things when ties are inside chord brackets, and
                    // this allows us to ensure that ties always come after
                    // chords have been closed out
                    bool tiedForward = false;
                    (*j)->get<Bool>(BaseProperties::TIED_FORWARD, tiedForward);
                    if (tiedForward) {
                        addTie = true;
                    }
                    
                    // Add a space before the next note/event
                    str << " ";

                    // handle start of slurs/hairpins
                    handleStartingEvents(eventsToStart, str);

                    // catch ties not in chords...  this is an unabashed
                    // kludge...  this should use closeChordWriteTie, but that
                    // function won't do the right job here.  If we're *not*
                    // writing a chord, we need to write a tie here to avoid
                    // deferring it to the wrong spot; otherwise
                    // closeChordWriteTie will handle the tie when it's called
                    // in various other places.
                    if (!currentlyWritingChord && addTie) {
                        str << "~ ";
                        addTie = false;
                    }

                } else if ((*j)->isa(Clef::EventType)) {
                    
                    closeChordWriteTie(addTie, currentlyWritingChord, str);

                    // Incomplete: Set which note the clef should center on  (DMM - why?)
                    str << "\\clef ";
                    
                    std::string whichClef((*j)->get<String>(Clef::ClefPropertyName));
                    if (whichClef == Clef::Treble) {
                        str << "treble" << std::endl;
                    } else if (whichClef == Clef::Tenor) {
                        str << "tenor" << std::endl;
                    } else if (whichClef == Clef::Alto) {
                        str << "alto" << std::endl;
                    } else if (whichClef == Clef::Bass) {
                        str << "bass" << std::endl;
                    }
                    
                    str << indent(col);

                } else if ((*j)->isa(Rosegarden::Key::EventType)) {

                    closeChordWriteTie(addTie, currentlyWritingChord, str);

                    str << "\\key ";
                    Rosegarden::Key whichKey(**j);
                    isFlatKeySignature = !whichKey.isSharp();
                    accidentalCount = whichKey.getAccidentalCount();
                    
                    str << convertPitchToLilyNote(whichKey.getTonicPitch(), isFlatKeySignature,
                                                  accidentalCount, "");
                    
                    if (whichKey.isMinor()) {
                        str << " \\minor";
                    } else {
                        str << " \\major";
                    }
                    str << std::endl << indent(col);
                            
                } else if ((*j)->isa(Indication::EventType)) {
                    // Handle the end of these events when it's time
                    //
                    // If we get an indication, add the event to
                    // eventsToStart, keep track of on-going events with
                    // eventsInProgress, which are both std::multiset
                    eventsToStart.insert(*j);
                    eventsInProgress.insert(*j);
                } else {
                    std::cerr << std::endl << "LilypondExporter::write() - unhandled event type: "
                              << (*j)->getType();
                }
            }

            closeChordWriteTie(addTie, currentlyWritingChord, str);
            
            // close Voice context
            str << std::endl << indent(--col) << "} % Voice" << std::endl;  // indent-  
            
            // write accumulated lyric events to the Lyric context, if user
            // desires
            if (exportLyrics) {
                str << indent(col) << "\\context Lyrics = \"" << lyricNumber.str()
                    << "\" \\lyrics  { " << std::endl;
                str << indent(++col) << lilyLyrics.str() << " " << std::endl;
                str << std::endl << indent(--col) << "} % Lyrics"; // close Lyric context
            }
            // cheap hack...  sometimes things combine so there's no \n after
            // the Lyric block ends...  this sometimes puts in an extra \n,
            // but one too many is ugly, while one too few causes Lilypond to
            // barf, and the user has to correct the file by hand to get it
            // to render...
            str << std::endl;
        }
    }
    
    // close the last track (Staff context)
    if (voiceCounter > 0) {
        str << std::endl << indent(--col) << "> % Staff (final)";  // indent-
    } else {
        str << indent(--col) << "% (All staffs were muted.)" << std::endl;
/*        KMessageBox::sorry(0, i18n(
            "All tracks were muted, so there were no notes to export.\n\n"
            "For better results, either un-mute some tracks or toggle off the "
            "\"Do not export muted tracks\" option on the Lilypond tab of the "
            "Notation configuration page, then try again.")); */
    }
    
    // close \notes section
    str << std::endl << indent(--col) << "> % notes" << std::endl;; // indent-

    // write user-specified paper type in \paper block
    std::string paper = "papersize = \"";
    switch (paperSize) {
        case 0 : paper += "letter\""; break;
        case 1 : paper += "a4\"";     break;
        case 2 : paper += "legal\"";  break;
        case 3 : paper = "";          break; // "do not specify"
    }
    str << indent(col) << "\\paper { " << paper <<" }" << std::endl;

    // write initial tempo in Midi block, if user wishes (added per user request...
    // makes debugging the .ly file easier because fewer "noisy" errors are
    // produced during the process of rendering MIDI...)
    if (exportMidi) {
        double tempo = m_composition->getTempoAt(m_composition->getStartMarker());
        // Incomplete?  Can I get away without converting tempo relative to the time
        // signature for this purpose?  we'll see...
        str << indent(col++) << "\\midi {" << std::endl;
            str << indent(col) << "\\tempo 4 = " << tempo << std::endl;
        str << indent(--col) << "} " << std::endl;
    }    

    // close \score section and close out the file
    str << "} % score" << std::endl;
    str.close();
    return true;
}



// DMM
// moved out of the way for enhanced readability when working out the indent()
// stuff...
//
// Doing something with all of this is pretty non-trivial.  Initial
// tempo goes into the \midi section, but subsequent tempo changes need to be
// written along with the notes, and should probably only be written in one
// staff context.  Dealing with all of that is certainly possible, but it's
// a lot more trouble than I think it's worth, and I'm not going to do
// anything with this unless pressed by user request.
//
//     int tempoCount = m_composition->getTempoChangeCount();

//     if (tempoCount > 0) {

//      str << "\nt ";

//      for (int i = 0; i < tempoCount - 1; ++i) {

//          std::pair<Rosegarden::timeT, long> tempoChange = 
//              m_composition->getRawTempoChange(i);

//          timeT myTime = tempoChange.first;
//          timeT nextTime = myTime;
//          if (i < m_composition->getTempoChangeCount()-1) {
//              nextTime = m_composition->getRawTempoChange(i+1).first;
//          }
            
//          int tempo = tempoChange.second / 60;

//          str << convertTime(  myTime) << " " << tempo << " "
//              << convertTime(nextTime) << " " << tempo << " ";
//      }

//      str << convertTime(m_composition->getRawTempoChange(tempoCount-1).first)
//          << " "
//          << m_composition->getRawTempoChange(tempoCount-1).second/60
//          << std::endl;
//     }
