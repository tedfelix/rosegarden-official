/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "EventQuantizeCommand.h"

#include <klocale.h>
#include "base/NotationTypes.h"
#include "base/Profiler.h"
#include "base/Quantizer.h"
#include "base/BasicQuantizer.h"
#include "base/LegatoQuantizer.h"
#include "base/NotationQuantizer.h"
#include "base/Segment.h"
#include "base/SegmentNotationHelper.h"
#include "base/Selection.h"
#include "document/BasicCommand.h"
#include <QSettings>
#include <misc/Strings.h>
#include <QString>
#include "base/BaseProperties.h"
#include "gui/application/RosegardenApplication.h"
#include <kapplication.h>


namespace Rosegarden
{

using namespace BaseProperties;

EventQuantizeCommand::EventQuantizeCommand(Segment &segment,
        timeT startTime,
        timeT endTime,
        Quantizer *quantizer):
        BasicCommand(getGlobalName(quantizer), segment, startTime, endTime,
                     true),  // bruteForceRedo
        m_quantizer(quantizer),
        m_selection(0)
{
    // nothing else
}

EventQuantizeCommand::EventQuantizeCommand(EventSelection &selection,
        Quantizer *quantizer):
        BasicCommand(getGlobalName(quantizer),
                     selection.getSegment(),
                     selection.getStartTime(),
                     selection.getEndTime(),
                     true),  // bruteForceRedo
        m_quantizer(quantizer),
        m_selection(&selection)
{
    // nothing else
}

EventQuantizeCommand::EventQuantizeCommand(Segment &segment,
        timeT startTime,
        timeT endTime,
        QString configGroup,
        bool notation):
        BasicCommand(getGlobalName(makeQuantizer(configGroup, notation)),
                     segment, startTime, endTime,
                     true),  // bruteForceRedo
        m_selection(0),
        m_configGroup(configGroup)
{
    // nothing else -- m_quantizer set by makeQuantizer
}

EventQuantizeCommand::EventQuantizeCommand(EventSelection &selection,
        QString configGroup,
        bool notation):
        BasicCommand(getGlobalName(makeQuantizer(configGroup, notation)),
                     selection.getSegment(),
                     selection.getStartTime(),
                     selection.getEndTime(),
                     true),  // bruteForceRedo
        m_selection(&selection),
        m_configGroup(configGroup)
{
    // nothing else -- m_quantizer set by makeQuantizer
}

EventQuantizeCommand::~EventQuantizeCommand()
{
    delete m_quantizer;
}

QString
EventQuantizeCommand::getGlobalName(Quantizer *quantizer)
{
    if (quantizer) {
        if (dynamic_cast<NotationQuantizer *>(quantizer)) {
            return i18n("Heuristic Notation &Quantize");
        } else {
            return i18n("Grid &Quantize");
        }
    }

    return i18n("&Quantize...");
}

void
EventQuantizeCommand::modifySegment()
{
    Profiler profiler("EventQuantizeCommand::modifySegment", true);

    Segment &segment = getSegment();
    SegmentNotationHelper helper(segment);

    bool rebeam = false;
    bool makeviable = false;
    bool decounterpoint = false;

    if (!m_configGroup.isEmpty()) {
        //!!! need way to decide whether to do these even if no config group (i.e. through args to the command)
        QSettings config;
        config.beginGroup( m_configGroup );
        // 
        // FIX-manually-(GW), add:
        // config.endGroup();		// corresponding to: config.beginGroup( m_configGroup );
        //  


        rebeam = qStrToBool( config.value("quantizerebeam", "true" ) ) ;
        makeviable = qStrToBool( config.value("quantizemakeviable", "false" ) ) ;
        decounterpoint = qStrToBool( config.value("quantizedecounterpoint", "false" ) ) ;
    }

    if (m_selection) {
        m_quantizer->quantize(m_selection);

    } else {
        m_quantizer->quantize(&segment,
                              segment.findTime(getStartTime()),
                              segment.findTime(getEndTime()));
    }

    if (m_progressTotal > 0) {
        if (rebeam || makeviable || decounterpoint) {
            emit incrementProgress(m_progressTotal / 2);
            rgapp->refreshGUI(50);
        } else {
            emit incrementProgress(m_progressTotal);
            rgapp->refreshGUI(50);
        }
    }

    if (m_selection) {
        EventSelection::RangeTimeList ranges(m_selection->getRangeTimes());
        for (EventSelection::RangeTimeList::iterator i = ranges.begin();
                i != ranges.end(); ++i) {
            if (makeviable) {
                helper.makeNotesViable(i->first, i->second, true);
            }
            if (decounterpoint) {
                helper.deCounterpoint(i->first, i->second);
            }
            if (rebeam) {
                helper.autoBeam(i->first, i->second, GROUP_TYPE_BEAMED);
                helper.autoSlur(i->first, i->second, true);
            }
        }
    } else {
        if (makeviable) {
            helper.makeNotesViable(getStartTime(), getEndTime(), true);
        }
        if (decounterpoint) {
            helper.deCounterpoint(getStartTime(), getEndTime());
        }
        if (rebeam) {
            helper.autoBeam(getStartTime(), getEndTime(), GROUP_TYPE_BEAMED);
            helper.autoSlur(getStartTime(), getEndTime(), true);
        }
    }

    if (m_progressTotal > 0) {
        if (rebeam || makeviable || decounterpoint) {
            emit incrementProgress(m_progressTotal / 2);
            rgapp->refreshGUI(50);
        }
    }
}

Quantizer *
EventQuantizeCommand::makeQuantizer(QString configGroup,
                                    bool notationDefault)
{
    //!!! Excessive duplication with
    // QuantizeParameters::getQuantizer in widgets.cpp

    QSettings config;
    config.beginGroup( configGroup );
    // 
    // FIX-manually-(GW), add:
    // config.endGroup();		// corresponding to: config.beginGroup( configGroup );
    //  


    timeT defaultUnit =
        Note(Note::Demisemiquaver).getDuration();

    int type = config.value("quantizetype", notationDefault ? 2 : 0).toInt() ;
	timeT unit = config.value("quantizeunit",  static_cast<uint>(defaultUnit) ).toInt() ;
    bool notateOnly = qStrToBool( config.value("quantizenotationonly", "notationDefault" ) ) ;
    bool durations = qStrToBool( config.value("quantizedurations", "false" ) ) ;
    int simplicity = config.value("quantizesimplicity", 13).toInt() ;
    int maxTuplet = config.value("quantizemaxtuplet", 3).toInt() ;
    bool counterpoint = config.value("quantizecounterpoint", false).toInt() ;
    bool articulate = qStrToBool( config.value("quantizearticulate", "true" ) ) ;
    int swing = config.value("quantizeswing", 0).toInt() ;
    int iterate = config.value("quantizeiterate", 100).toInt() ;

    m_quantizer = 0;

    if (type == 0) {
        if (notateOnly) {
            m_quantizer = new BasicQuantizer
                          (Quantizer::RawEventData,
                           Quantizer::NotationPrefix,
                           unit, durations, swing, iterate);
        } else {
            m_quantizer = new BasicQuantizer
                          (Quantizer::RawEventData,
                           Quantizer::RawEventData,
                           unit, durations, swing, iterate);
        }
    } else if (type == 1) {
        if (notateOnly) {
            m_quantizer = new LegatoQuantizer
                          (Quantizer::RawEventData,
                           Quantizer::NotationPrefix, unit);
        } else {
            m_quantizer = new LegatoQuantizer
                          (Quantizer::RawEventData,
                           Quantizer::RawEventData, unit);
        }
    } else {

        NotationQuantizer *nq;

        if (notateOnly) {
            nq = new NotationQuantizer();
        } else {
            nq = new NotationQuantizer
                 (Quantizer::RawEventData,
                  Quantizer::RawEventData);
        }

        nq->setUnit(unit);
        nq->setSimplicityFactor(simplicity);
        nq->setMaxTuplet(maxTuplet);
        nq->setContrapuntal(counterpoint);
        nq->setArticulate(articulate);

        m_quantizer = nq;
    }

    return m_quantizer;
}

}
#include "EventQuantizeCommand.moc"
