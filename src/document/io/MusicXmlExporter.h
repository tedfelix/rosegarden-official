
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2017 the Rosegarden development team.

    This file is Copyright 2002
        Hans Kieserman      <hkieserman@mail.com>
    with heavy lifting from csoundio as it was on 13/5/2002.

    More or less complete rewrite (Aug 2011)
        Niek van den Berg   <niekjvandenberg@gmail.com>

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MUSICXMLEXPORTER_H
#define RG_MUSICXMLEXPORTER_H

#include "MusicXmlExportHelper.h"

#include "document/RosegardenDocument.h"

#include <QPointer>

class QProgressDialog;

namespace Rosegarden
{

class RosegardenDocument;
class Key;
class Clef;
class AccidentalTable;
class Instrument;

/**
 *   The global structure of this exporter (MusicXmlExporter::write()) is
 *
 *   o   Write MusicXML header. This header is written by writeHeader().
 *
 *   o   Scan all tracks. During this scan an MusicXmlExportHelper is created
 *       for each track or trackgroup which will be exported. Any
 *       MusicXmlExportHelper results in a single MusicXML part. Tracks maybe
 *       grouped by the staff export options to form a multi staff part.
 *       Tracks are grouped into one single MusicXmlExportHelper if:
 *         - Consecutive track are "grouped" by curly brackets,
 *         - Tracks are assigned to the same instrument.
 *         - The setting "mxmlmultistave" is not "none"
 *
 *       Every MusicXmlExportHelper also contains a list of segments belonging
 *       the track(s) which are part of the MusicXmlExportHelper. These
 *       segments are grouped into voices (layers). Any voice consists of non
 *       overlapping segments. It is assumed the segments of the first
 *       voice/layers of each staff are joining. When there is a gap between
 *       two successive a new temporary segments is created. This segment
 *       contains rests.
 *
 *   o   Next the parts itself are written. This exporter supports both
 *       part-wise and time-wise. The structure is
 *         - part-wise:
 *               header/identification
 *               part-list
 *               part 1
 *                   measure 1
 *                   measure 2
 *                      .
 *                      .
 *               part 2
 *                   measure 1
 *                   measure 2
 *                      .
 *                      .
 *
 *         - time-wise:
 *               header/identification
 *               part-list
 *               measure 1
 *                   part 1
 *                   part 2
 *                      .
 *                      .
 *               measure 2
 *                   part 1
 *                   part 2
 *                      .
 *                      .
 *
 *       To write the measures, write() will iterate over all parts and bars
 *       for a part-wise file (or over all bars and parts for a time-wise file)
 *       and calls the MusicXmlExportHelper member writeEvents().
 *       The member iterates over all voices of the part and handles all events
 *       of the segments.
 *
 *       Some known problems:
 *       1)     When exporting multi staff part, problems arise when segments
 *              at the same time will have different transpositions. It is
 *              assumed all tracks/segments will have the same transpostion.
 *              However, this is not checked!
 */

class MusicXmlExporter
{
public:

    /**
     * MusicXmlExportHelper represents a MusicXML part.
     */
//     class MusicXmlExportHelper;
    typedef std::vector<MusicXmlExportHelper *> PartsVector;

    /**
     * A simple structure to store some MIDI information required to include
     * the MIDI support in MusicXML file.
     */
    struct MidiInstrument
    {
      MidiInstrument(void) {};
      MidiInstrument(Instrument * instrument, int pitch);
      int         channel;
      int         program;
      int         unpitched;
    };
    typedef std::map<std::string, MidiInstrument> InstrumentMap;

    /**
     * Constructs a MusicXmlExporter object
     *
     * @param parent the parent object.
     * @param doc the Rosegarden document.
     * @param filename name of the outfile MusicXML file.
     */
    MusicXmlExporter(RosegardenMainWindow *parent, RosegardenDocument *doc, std::string fileName);

    /**
     * Destroys a MusicXmlExporter object
     *
     * @param parent the parent object.
     * @param doc the Rosegarden document.
     * @param filename name of the outfile MusicXML file.
     */
    ~MusicXmlExporter();

    bool write();

    void setProgressDialog(QPointer<QProgressDialog> progressDialog)
            { m_progressDialog = progressDialog; }

protected:
    unsigned int m_exportSelection;
    static const unsigned int EXPORT_ALL_TRACKS = 0;
    static const unsigned int EXPORT_NONMUTED_TRACKS = 1;
    static const unsigned int EXPORT_SELECTED_TRACK = 2;
    static const unsigned int EXPORT_SELECTED_SEGMENTS = 3;

    unsigned int m_MusicXmlVersion;
    static const int MUSICXML_VERSION_1_1  = 0;
    static const int MUSICXML_VERSION_2_0  = 1;

    unsigned int m_mxmlDTDType;
    static const unsigned int DTD_PARTWISE = 0;
    static const unsigned int DTD_TIMEWISE = 1;

    bool         m_exportStaffBracket;

    static const unsigned int DONT_EXPORT_PERCUSSION = 0;
    static const unsigned int EXPORT_PERCUSSION_AS_NOTES = 1;
    static const unsigned int EXPORT_PERCUSSION_AS_PERCUSSION = 2;
    unsigned int m_exportPercussion;

    bool         m_mxmlUseOctaveShift;

    unsigned int m_multiStave;
    static const unsigned int MULTI_STAVE_NONE = 0;
    static const unsigned int MULTI_STAVE_CURLY_SQUARE = 1;
    static const unsigned int MULTI_STAVE_CURLY = 2;

    AccidentalTable::OctaveType m_octaveType;
    AccidentalTable::BarResetType m_barResetType;

    // The exporter doesn't try to find a reasonable divisions number but
    // simply uses the samen number which is used internally.
    static const int divisions = 960;

    Composition *m_composition;
    RosegardenDocument *m_doc;
    std::string m_fileName;
    RosegardenMainViewWidget *m_view;

    void readConfigVariables(void);
    bool isPercussionTrack(Track *track);
    bool exportTrack(Track *track);
    void writeHeader(std::ostream &str);
    MusicXmlExportHelper* initalisePart(timeT compositionEndTime, int curTrackPos,
                            bool &exporting, bool &inMultiStaffGroup);
    PartsVector writeScorePart(timeT compositionEndTime, std::ostream &str);

private:
    QPointer<QProgressDialog> m_progressDialog;
};

}

#endif
