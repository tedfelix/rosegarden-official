// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2003
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    Numerous additions and bug fixes by
        Michael McIntyre    <dmmcintyr@users.sourceforge.net>

    Some restructuring by Chris Cannam.

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
#include "notationview.h"
#include "widgets.h"

#include "Composition.h"
#include "BaseProperties.h"
#include "SegmentNotationHelper.h"
#include "NotationTypes.h"
#include "Sets.h"

#include "rosedebug.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

using namespace Rosegarden::BaseProperties;
using Rosegarden::Bool;
using Rosegarden::Clef;
using Rosegarden::Composition;
using Rosegarden::Indication;
using Rosegarden::Int;
using Rosegarden::Key;
using Rosegarden::Note;
using Rosegarden::Segment;
using Rosegarden::String;
using Rosegarden::timeT;
using Rosegarden::TimeSignature;
using Rosegarden::Accidental;
using Rosegarden::Accidentals;
using Rosegarden::Text;
using Rosegarden::PropertyName;
using Rosegarden::Mark;
using Rosegarden::Marks;
using Rosegarden::Configuration;

const Rosegarden::PropertyName LilypondExporter::SKIP_PROPERTY
    = "LilypondExportSkipThisEvent";

LilypondExporter::LilypondExporter(QObject *parent,
                                   Composition *composition,
                                   std::string fileName) :
                                   ProgressReporter(parent, "lilypondExporter"),
                                   m_composition(composition),
                                   m_fileName(fileName)
{
    // nothing
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
		str << "( ";
	    } else if (i.getIndicationType() == Indication::Crescendo) {
		str << "\\< ";
	    } else if (i.getIndicationType() == Indication::Decrescendo) {
		str << "\\> ";
	    }

        } catch (Rosegarden::Event::BadType) {
            // Not an indication
        } catch (Rosegarden::Event::NoData e) {
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
		(i.getIndicationType() == Indication::Slur &&
		 indicationEnd == eventEnd)) {

                if (i.getIndicationType() == Indication::Slur) {
                    str << ") ";
                } else if (i.getIndicationType() == Indication::Crescendo ||
                           i.getIndicationType() == Indication::Decrescendo) {
                    str << "\\! "; 
                }
		
                eventsInProgress.erase(k);
	    }

        } catch (Rosegarden::Event::BadType) {
	    // not an indication

	} catch (Rosegarden::Event::NoData e) {
	    std::cerr << "Bad indication: " << e.getMessage() << std::endl;
	}

	k = l;
    }
}

// processes input to produce a Lilypond-format note written correctly for all
// keys and out-of-key accidental combinations.
std::string
LilypondExporter::convertPitchToLilyNote(int pitch, Accidental accidental,
					 const Rosegarden::Key &key) 
{
    bool isFlatKeySignature = !key.isSharp();
    int accidentalCount = key.getAccidentalCount();

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
                    case 7: if (pitchNote ==  4) lilyNote = "fes"; // Fb 
                    case 6: if (pitchNote == 11) lilyNote = "ces"; // Cb 
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

    //!!! Alternative implementation in test:

    Rosegarden::Pitch p(pitch, accidental);
    std::string origLilyNote = lilyNote;
    lilyNote = "";

    lilyNote += (char)tolower(p.getNoteName(key));
    Accidental acc = p.getAccidental(key.isSharp());
    if      (acc == Accidentals::DoubleFlat)  lilyNote += "eses";
    else if (acc == Accidentals::Flat)        lilyNote += "es";
    else if (acc == Accidentals::Sharp)       lilyNote += "is";
    else if (acc == Accidentals::DoubleSharp) lilyNote += "isis";
    
    if (lilyNote != origLilyNote) {
	std::cerr << "WARNING: LilypondExporter::convertPitchToLilyNote: " << lilyNote << " != " << origLilyNote << std::endl;
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
LilypondExporter::write()
{
    QString tmp_fileName = strtoqstr(m_fileName);
    
    bool illegalFilename = (tmp_fileName.contains(' ') || tmp_fileName.contains("\\"));            
    tmp_fileName.replace(QRegExp(" "), "");
    tmp_fileName.replace(QRegExp("\\\\"), "");
    tmp_fileName.replace(QRegExp("'"), "");
    tmp_fileName.replace(QRegExp("\""), "");
            
    if (illegalFilename) {
        CurrentProgressDialog::freeze();
        int reply = KMessageBox::warningContinueCancel(
            0, i18n("Lilypond does not allow spaces or backslashes in filenames.  "
                     "Would you like to use\n\n %1\n\n instead?").arg(tmp_fileName)); 
        if (reply != KMessageBox::Continue) return false;
    }

    std::ofstream str(qstrtostr(tmp_fileName).c_str(), std::ios::out);
    if (!str) {
        std::cerr << "LilypondExporter::write() - can't write file " << tmp_fileName << std::endl;
        return false;
    }

    // grab config info
    KConfig *cfg = kapp->config();
    cfg->setGroup(NotationView::ConfigGroup);

    unsigned int paperSize = cfg->readUnsignedNumEntry("lilypapersize", 1);
    unsigned int fontSize = cfg->readUnsignedNumEntry("lilyfontsize", 4);
    bool exportLyrics = cfg->readBoolEntry("lilyexportlyrics", true);
    bool exportHeaders = cfg->readBoolEntry("lilyexportheaders", true);
    bool exportMidi = cfg->readBoolEntry("lilyexportmidi", false);
    bool exportUnmuted = cfg->readBoolEntry("lilyexportunmuted", false);
    bool exportPointAndClick = cfg->readBoolEntry("lilyexportpointandclick", false);
    bool exportBarChecks = cfg->readBoolEntry("lilyexportbarchecks", false);
    bool oldStyle = (cfg->readUnsignedNumEntry("lilylanguage", 1) == 0);

    // enable "point and click" debugging via xdvi to make finding the
    // unfortunately inevitable errors easier
    if (exportPointAndClick) {
        str << "% point and click debugging:" << std::endl;
        str << "#(set-point-and-click! 'line)" << std::endl;
    }

    // Lilypond \header block
    if (oldStyle) {
	str << "\\version \"1.6.0\"" << std::endl;
    } else {
	str << "\\version \"2.0\"" << std::endl;
    }

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
    if (oldStyle) {
	str << indent(++col) << "\\notes <" << std::endl;  // indent+
    } else {
	str << indent(++col) << "\\notes <<" << std::endl;  // indent+
    }	

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
/*
    if (exportLyrics) {
	str << indent(col) << "\\addlyrics" << std::endl;
    }
*/
    int lastTrackIndex = -1;
    int voiceCounter = 0;
    int trackNo = 0;

    // Write out all segments for each Track
    for (Composition::iterator i = m_composition->begin();
         i != m_composition->end(); ++i) {

        emit setProgress(int(double(trackNo++)/
                             double(m_composition->getNbTracks()) * 100.0));
        kapp->processEvents(50);

        // do nothing if track is muted...  this provides a crude but easily implemented
        // method for users to selectively export tracks...
        Rosegarden::Track *track = m_composition->getTrackById((*i)->getTrack());
        
        if (!exportUnmuted || (!track->isMuted())) {
            if ((int) (*i)->getTrack() != lastTrackIndex) {
                if (lastTrackIndex != -1) {
                    // close the old track (Staff context)
		    if (oldStyle) {
			str << std::endl << indent(--col) << "> % Staff" << std::endl;  // indent-
		    } else {
			str << std::endl << indent(--col) << ">> % Staff" << std::endl;  // indent-
		    }
                }
                lastTrackIndex = (*i)->getTrack();

                // avoid problem with <untitled> tracks yielding a .ly file that
                // jumbles all notes together on a single staff...
                // every Staff context has to have a unique name, even if the
                // Staff.instrument property is the same for multiple staffs...
                std::ostringstream staffName;
                staffName << protectIllegalChars(m_composition->
                        getTrackById(lastTrackIndex)->getLabel());

                if (staffName.str() == "") {
                    staffName << "track";
                }
                
                str << std::endl
		    << indent(col) << "\\context Staff = \"" << staffName.str()
                    << " " << (voiceCounter +1) << "\" ";
		if (oldStyle) {
		    str << "< " << std::endl;
		} else {
		    str << "<< " << std::endl;
		}

                str << indent(++col)<< "\\property Staff.instrument = \""  // indent+
                    << staffName.str() <<"\"" << std::endl;
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
            std::ostringstream voiceNumber, lyricNumber;
            voiceNumber << "voice " << voiceCounter;
            lyricNumber << "lyric " << voiceCounter++;
            
            str << indent(col++) << "\\context Voice = \"" << voiceNumber.str()
                << "\" {"; // indent+

	    Rosegarden::SegmentNotationHelper helper(**i);
	    helper.setNotationProperties();
            
	    int firstBar = m_composition->getBarNumber((*i)->getStartTime());

	    if (firstBar > 0) {
		// Add a skip for the duration until the start of the first
		// bar in the segment.  If the segment doesn't start on a bar
		// line, an additional skip will be written (in the form of
		// a series of rests) at the start of writeBar, below.
		//!!! This doesn't cope correctly yet with time signature changes
		// during this skipped section.
		writeSkip(timeSignature, 0, m_composition->getBarStart(firstBar),
			  false, str);
	    }

            std::string lilyText = "";      // text events
            std::string lilyLyrics = "";    // lyric events
            std::string prevStyle = "";     // track note styles 

	    Rosegarden::Key key;

	    for (int barNo = m_composition->getBarNumber((*i)->getStartTime());
		 barNo <= m_composition->getBarNumber((*i)->getEndMarkerTime());
		 ++barNo) {

		str << std::endl;

		writeBar(*i, barNo, col, key,
			 lilyText, lilyLyrics,
			 prevStyle, eventsInProgress, str);

		if (exportBarChecks) {
		    str << " |";
		}
	    }

            // close Voice context
            str << std::endl << indent(--col) << "} % Voice" << std::endl;  // indent-  
            
            // write accumulated lyric events to the Lyric context, if user
            // desires
            if (exportLyrics && (lilyLyrics != "")) {
                str << indent(col) << "\\context Lyrics = \"" << lyricNumber.str()
                    << "\" \\lyrics  { " << std::endl;
                str << indent(++col) << lilyLyrics << " " << std::endl;
                str << std::endl << indent(--col) << "} % Lyrics" << std::endl; // close Lyric context
            }
        }
    }
    
    // close the last track (Staff context)
    if (voiceCounter > 0) {
	if (oldStyle) {
	    str << std::endl << indent(--col) << "> % Staff (final)";  // indent-
	} else {
	    str << std::endl << indent(--col) << ">> % Staff (final)";  // indent-
	}
    } else {
        str << indent(--col) << "% (All staffs were muted.)" << std::endl;
    }
    
    // close \notes section
    if (oldStyle) {
	str << std::endl << indent(--col) << "> % notes" << std::endl;; // indent-
    } else {
	str << std::endl << indent(--col) << ">> % notes" << std::endl;; // indent-
    }

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

timeT
LilypondExporter::calculateDuration(Rosegarden::Segment *s,
				    const Rosegarden::Segment::iterator &i,
				    Rosegarden::timeT barEnd,
				    Rosegarden::timeT &soundingDuration,
				    const std::pair<int, int> &tupletRatio,
				    bool &overlong)
{
    timeT duration = (*i)->getNotationDuration();
    timeT absTime  = (*i)->getNotationAbsoluteTime();

    RG_DEBUG << "LilypondExporter::calculateDuration: first duration, absTime: "
	      << duration << ", " << absTime << endl;

    if ((*i)->isa(Note::EventType) || (*i)->isa(Note::EventRestType)) {
	try {
	    // tuplet compensation, etc
	    Note::Type type = (*i)->get<Int>(NOTE_TYPE);
	    int dots = (*i)->get<Int>(NOTE_DOTS);
	    duration = Note(type, dots).getDuration();
	} catch (Rosegarden::Exception e) { // no properties
	}
    }

    RG_DEBUG << "LilypondExporter::calculateDuration: now duration is "
	      << duration << endl;

    soundingDuration = duration * tupletRatio.first / tupletRatio.second;

    timeT toNext = barEnd - absTime;
    if (soundingDuration > toNext) {
	soundingDuration = toNext;
	duration = soundingDuration * tupletRatio.second / tupletRatio.first;
	overlong = true;
    }

    RG_DEBUG << "LilypondExporter::calculateDuration: first toNext is "
	      << toNext << endl;

    // Examine the following event, and truncate our duration
    // if we overlap it.
    Segment::iterator nextElt = s->end();
    toNext = soundingDuration;

    if ((*i)->isa(Note::EventType)) {
	
	Rosegarden::Chord chord(*s, i, m_composition->getNotationQuantizer());
	Segment::iterator nextElt = chord.getFinalElement();
	++nextElt;

	if (s->isBeforeEndMarker(nextElt)) {
	    // The quantizer sometimes sticks a rest at the same time
	    // as this note -- don't use that one here, and mark it as
	    // not to be exported -- it's just a heavy-handed way of
	    // rendering counterpoint in RG
	    if ((*nextElt)->isa(Note::EventRestType) &&
		(*nextElt)->getNotationAbsoluteTime() == absTime) {
		(*nextElt)->set<Bool>(SKIP_PROPERTY, true);
		++nextElt;
	    }
	}

    } else {

	nextElt = i;
	++nextElt;
    }
	
    if (s->isBeforeEndMarker(nextElt)) {
	toNext = (*nextElt)->getNotationAbsoluteTime() - absTime;
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
LilypondExporter::writeBar(Rosegarden::Segment *s,
			   int barNo, int col,
			   Rosegarden::Key &key,
			   std::string &lilyText,
			   std::string &lilyLyrics,
			   std::string &prevStyle,
			   eventendlist &eventsInProgress,
			   std::ofstream &str)
{
    KConfig *cfg = kapp->config();
    cfg->setGroup(NotationView::ConfigGroup);
    bool exportBeams = cfg->readBoolEntry("lilyexportbeamings", false);
    bool oldStyle = (cfg->readUnsignedNumEntry("lilylanguage", 1) == 0);
    int lastStem = 0; // 0 => unset, -1 => down, 1 => up

    timeT barStart = m_composition->getBarStart(barNo);
    timeT barEnd = m_composition->getBarEnd(barNo);

    Segment::iterator i = s->findTime(barStart);
    if (!s->isBeforeEndMarker(i)) return;

    if ((barNo+1) % 5 == 0) {
	str << "%% " << barNo+1 << std::endl << indent(col);
    } else {
	str << indent(col);
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

    while (s->isBeforeEndMarker(i)) {

	if ((*i)->getNotationAbsoluteTime() >= barEnd) break;

	// First test whether we're entering or leaving a group,
	// before we consider how to write the event itself (at least
	// for pre-2.0 Lilypond output)

	if ((*i)->isa(Note::EventType) || (*i)->isa(Note::EventRestType)) {
	    
	    long newGroupId = -1;
	    if ((*i)->get<Int>(BEAMED_GROUP_ID, newGroupId)) {
		
		if (newGroupId != groupId) {
		    // entering a new beamed group
		    
		    if (groupId != -1) {
			// and leaving an old one
			if (groupType == GROUP_TYPE_TUPLED ||
			    groupType == GROUP_TYPE_GRACE) {
			    str << "} ";
			} else if (groupType == GROUP_TYPE_BEAMED) {
			    if (exportBeams) str << "] ";
			}
		    }
		    
		    groupId = newGroupId;
		    groupType = "";
		    (void)(*i)->get<String>(BEAMED_GROUP_TYPE, groupType);
		    
		    if (groupType == GROUP_TYPE_TUPLED) {
			long numerator = 0;
			long denominator = 0;
			(*i)->get<Int>(BEAMED_GROUP_TUPLED_COUNT, numerator);
			(*i)->get<Int>(BEAMED_GROUP_UNTUPLED_COUNT, denominator);
			if (numerator == 0 || denominator == 0) {
			    std::cerr << "WARNING: LilypondExporter::writeBar: "
				      << "tupled event without tupled/untupled counts"
				      << std::endl;
			    groupId = -1;
			    groupType = "";
			} else {
			    str << "\\times " << numerator << "/" << denominator << " { ";
			    tupletRatio = std::pair<int, int>(numerator, denominator);
			}
		    } else if (groupType == GROUP_TYPE_BEAMED) {
			if (exportBeams) str << "[ "; //!!! wrong for 2.0

		    } else if (groupType == GROUP_TYPE_GRACE) {
			str << "\\grace { ";
		    }
		}
		
	    } else {
		
		if (groupId != -1) {
		    // leaving a beamed group
		    if (groupType == GROUP_TYPE_TUPLED ||
			groupType == GROUP_TYPE_GRACE) {
			str << "} ";
			tupletRatio = std::pair<int, int>(1, 1);
		    } else if (groupType == GROUP_TYPE_BEAMED) {
			if (exportBeams) str << "] ";
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

	    Rosegarden::Chord chord(*s, i, m_composition->getNotationQuantizer());
	    Rosegarden::Event *e = *chord.getInitialNote();
	    bool tiedForward = false;

	    // Examine the following event, and truncate our duration
	    // if we overlap it.

	    if (e->has(STEM_UP) && e->isPersistent<Bool>(STEM_UP)) {
		if (e->get<Bool>(STEM_UP)) {
		    if (lastStem != 1) {
			str << "\\stemUp ";
			lastStem = 1;
		    }
		} else {
		    if (lastStem != -1) {
			str << "\\stemDown ";
			lastStem = -1;
		    }
		}
	    } else {
		if (lastStem != 0) {
		    str << "\\stemBoth ";
		    lastStem = 0;
		}
	    }

	    if (chord.size() > 1) str << "< ";
	    if (oldStyle) handleEndingEvents(eventsInProgress, i, str);

	    for (i = chord.getInitialElement(); s->isBeforeEndMarker(i); ++i) {

		if ((*i)->isa(Text::EventType)) {

		    if (oldStyle) { //!!! work this out for lily 2.x
			handleText(*i, lilyText, lilyLyrics);
		    }

		} else if ((*i)->isa(Note::EventType)) {

		    writeStyle(*i, prevStyle, col, str);
		    writePitch(*i, key, str);

		    if (oldStyle) { // Lily 1.x: durations in simultaneous section
			if (duration != prevDuration) {
			    writeDuration(duration, str);
			    prevDuration = duration;
			}
			if (lilyText != "") {
			    str << lilyText;
			    lilyText = "";
			}
			writeSlashes(*i, str);
		    }

		    bool noteTiedForward = false;
		    (*i)->get<Bool>(TIED_FORWARD, noteTiedForward);
		    if (noteTiedForward) tiedForward = true;

		    str << " ";

		} else if ((*i)->isa(Indication::EventType)) {
		    eventsToStart.insert(*i);
		    eventsInProgress.insert(*i);
		}
		
		if (i == chord.getFinalElement()) break;
	    }

	    if (!oldStyle) {
		if (chord.size() > 1) str << "> ";
		if (duration != prevDuration) {
		    writeDuration(duration, str);
		    str << " ";
		    prevDuration = duration;
		}
		if (lilyText != "") {
		    str << lilyText;
		    lilyText = "";
		}
		writeSlashes(*i, str);
	    }
	    writtenDuration += soundingDuration;

	    std::vector<Mark> marks(chord.getMarksForChord());
	    // problem here: this will never be set if the note has never been notated
	    bool stemUp = true;
	    e->get<Bool>(STEM_UP, stemUp);
	    for (std::vector<Mark>::iterator j = marks.begin(); j != marks.end(); ++j) {
		str << composeLilyMark(*j, stemUp);
	    }
	    if (marks.size() > 0) str << " ";

	    if (!oldStyle) handleEndingEvents(eventsInProgress, i, str);
	    handleStartingEvents(eventsToStart, str);
	    if (oldStyle) {
		if (chord.size() > 1) str << "> ";
	    }

	    if (tiedForward) str << "~ ";

	} else if ((*i)->isa(Note::EventRestType)) {

	    if (oldStyle) handleEndingEvents(eventsInProgress, i, str);

	    str << "r";
	    if (duration != prevDuration) {
		writeDuration(duration, str);
		prevDuration = duration;
	    }
	    writtenDuration += soundingDuration;

	    if (lilyText != "") {
		str << lilyText;
		lilyText = "";
	    }

	    str << " ";
	    if (!oldStyle) handleEndingEvents(eventsInProgress, i, str);
	    handleStartingEvents(eventsToStart, str);

	} else if ((*i)->isa(Clef::EventType)) {
	    
	    try {
		// Incomplete: Set which note the clef should center on  (DMM - why?)
		str << "\\clef ";

		Rosegarden::Clef clef(**i);
	    
		if (clef.getClefType() == Clef::Treble) {
		    str << "treble";
		} else if (clef.getClefType() == Clef::Tenor) {
		    str << "tenor";
		} else if (clef.getClefType() == Clef::Alto) {
		    str << "alto";
		} else if (clef.getClefType() == Clef::Bass) {
		    str << "bass";
		}
	    
		str << std::endl << indent(col);

	    } catch (Rosegarden::Exception e) {
		std::cerr << "Bad clef: " << e.getMessage() << std::endl;
	    }

	} else if ((*i)->isa(Rosegarden::Key::EventType)) {

	    try {
		str << "\\key ";
		key = Rosegarden::Key(**i);
	    
		str << convertPitchToLilyNote(key.getTonicPitch(), "", key);
	    
		if (key.isMinor()) {
		    str << " \\minor";
		} else {
		    str << " \\major";
		}
		str << std::endl << indent(col);

	    } catch (Rosegarden::Exception e) {
		std::cerr << "Bad key: " << e.getMessage() << std::endl;
	    }

	} else if ((*i)->isa(Text::EventType)) {
	    
	    if (oldStyle) { //!!! work this out for lily 2.x
		handleText(*i, lilyText, lilyLyrics);
	    }
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
	    str << "} ";
	    tupletRatio = std::pair<int, int>(1, 1);
	} else if (groupType == GROUP_TYPE_BEAMED) {
	    if (exportBeams) str << "] ";
	}
    }

    if (lastStem != 0) {
	str << "\\stemBoth ";
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
}

void
LilypondExporter::writeSkip(const Rosegarden::TimeSignature &timeSig,
			    timeT offset,
			    timeT duration,
			    bool useRests,
			    std::ofstream &str)
{
    Rosegarden::DurationList dlist;
    timeSig.getDurationListForInterval(dlist, duration, offset);

    int t = 0, count = 0;

    for (Rosegarden::DurationList::iterator i = dlist.begin(); ; ++i) {
	
	if (i == dlist.end() || (*i) != t) {

	    if (count > 0) {

		if (useRests) str << "r";
		else str << "\\skip ";

		writeDuration(t, str);

		if (count > 1) str << "*" << count;
	    }

	    if (i != dlist.end()) {
		t = *i;
		count = 1;
	    }

	} else {
	    ++count;
	}

	str << " ";
	if (i == dlist.end()) break;
    }
}

void
LilypondExporter::handleText(const Rosegarden::Event *textEvent,
			     std::string &lilyText, std::string &lilyLyrics)
{
    try {
	Rosegarden::Text text(*textEvent);
	std::string s = protectIllegalChars(text.getText());
	
	if (text.getTextType() == Text::Tempo) {
	    
	    // print above staff, bold, large
	    lilyText += "^#'((bold Large)\"" + s + "\")";

	} else if (text.getTextType() == Text::LocalTempo) {

	    // print above staff, bold, small
	    lilyText += "^#'(bold \"" + s + "\")";

	} else if (text.getTextType() == Text::Lyric) {

	    lilyLyrics += s + " ";

	} else if (text.getTextType() == Text::Dynamic) {

	    // pass through only supported types
	    if (s == "ppp" || s == "pp"  || s == "p"  ||
		s == "mp"  || s == "mf"  || s == "f"  ||
		s == "ff"  || s == "fff" || s == "rfz" ||
		s == "sf") {
	    
		lilyText = "-\\" + s;

	    } else {
		std::cerr << "LilypondExporter::write() - illegal Lilypond dynamic: "
			  << s << std::endl;
	    }

	} else if (text.getTextType() == Text::Direction) {

	    lilyText = " \\mark \"" + s + "\"";

	} else if (text.getTextType() == Text::LocalDirection) {

	    // print below staff, bold italics, small
	    lilyText = "_#'((bold italic) \"" + s + "\")";

	} else {

	    textEvent->get<String>(Text::TextTypePropertyName, s);
	    std::cerr << "LilypondExporter::write() - unhandled text type: "
		      << s << std::endl;
	}
    } catch (Rosegarden::Exception e) {
	std::cerr << "Bad text: " << e.getMessage() << std::endl;
    }
}

void
LilypondExporter::writePitch(const Rosegarden::Event *note,
			     const Rosegarden::Key &key,
			     std::ofstream &str)
{
    // Note pitch (need name as well as octave)
    // It is also possible to have "relative" pitches,
    // but for simplicity we always use absolute pitch
    // 60 is middle C, one unit is a half-step
    
    long pitch = 60;
    note->get<Int>(PITCH, pitch);

    Rosegarden::Accidental accidental = Rosegarden::Accidentals::NoAccidental;
    note->get<String>(ACCIDENTAL, accidental);

    // format of Lilypond note is:
    // name + (duration) + octave + text markup
    
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
	for (; octave < 4; octave++) octaveMarks += ",";
    } else {
	for (; octave > 4; octave--) octaveMarks += "\'";
    }
    
    str << octaveMarks;
}

void
LilypondExporter::writeStyle(const Rosegarden::Event *note, std::string &prevStyle,
			     int col, std::ofstream &str)
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
    note->get<String>(NotationProperties::NOTE_STYLE, style);

    if (style != prevStyle) {

	if (style == styleClassical && prevStyle == "") return;

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

	str << std::endl << indent(col)
	    << "\\property Voice.NoteHead \\set #'style = #'"
	    << style << std::endl << indent(col);
    }
}    

void
LilypondExporter::writeDuration(Rosegarden::timeT duration,
				std::ofstream &str)
{
    Note note(Note::getNearestNote(duration, MAX_DOTS));
    
    switch (note.getNoteType()) {

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
    
    for (int numDots = 0; numDots < note.getDots(); numDots++) {
	str << ".";
    }
}

void
LilypondExporter::writeSlashes(const Rosegarden::Event *note, std::ofstream &str)
{
    // write slashes after text
    // / = 8 // = 16 /// = 32, etc.
    long slashes = 0;
    note->get<Int>(NotationProperties::SLASHES, slashes);
    if (slashes > 0) {
	str << ":";
	int length = 4;
	for (int c = 1; c <= slashes; c++) {
	    length *= 2;
	}
	str << length;
    }
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
