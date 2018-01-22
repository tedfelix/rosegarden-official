/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2018 the Rosegarden development team.
 
    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.
 
    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/


#include "EventQuantizeCommand.h"

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
#include "document/CommandRegistry.h"
#include "misc/Strings.h"
#include "base/BaseProperties.h"
#include "gui/application/RosegardenApplication.h"

#include <QApplication>
#include <QProgressDialog>
#include <QSettings>
#include <QString>


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
                                           QString settingsGroup,
                                           QuantizeScope scope):
    BasicCommand(getGlobalName(makeQuantizer(settingsGroup, scope)),
                 segment, startTime, endTime,
                 true),  // bruteForceRedo
    m_selection(0),
    m_settingsGroup(settingsGroup)
{
    // nothing else -- m_quantizer set by makeQuantizer
}

EventQuantizeCommand::EventQuantizeCommand(EventSelection &selection,
                                           QString settingsGroup,
                                           QuantizeScope scope) :
    BasicCommand(getGlobalName(makeQuantizer(settingsGroup, scope)),
                 selection.getSegment(),
                 selection.getStartTime(),
                 selection.getEndTime(),
                 true),  // bruteForceRedo
    m_selection(&selection),
    m_settingsGroup(settingsGroup)
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
            return tr("Heuristic Notation &Quantize");
        } else {
            return tr("Grid &Quantize");
        }
    }

    return tr("&Quantize...");
}

void
EventQuantizeCommand::modifySegment()
{
    Profiler profiler("EventQuantizeCommand::modifySegment", true);

    // Kick the event loop.
    qApp->processEvents();

    Segment &segment = getSegment();
    SegmentNotationHelper helper(segment);

    bool rebeam = false;
    bool makeviable = false;
    bool decounterpoint = false;

    if (!m_settingsGroup.isEmpty()) {
        //!!! need way to decide whether to do these even if no settings group (i.e. through args to the command)
        QSettings settings;
        settings.beginGroup( m_settingsGroup );

        rebeam = qStrToBool( settings.value("quantizerebeam", "true" ) ) ;
        makeviable = qStrToBool( settings.value("quantizemakeviable", "false" ) ) ;
        decounterpoint = qStrToBool( settings.value("quantizedecounterpoint", "false" ) ) ;
        settings.endGroup();
    }

    timeT endTime = segment.getEndTime();

    if (m_selection) {
        m_quantizer->quantize(m_selection);

    } else {
        m_quantizer->quantize(&segment,
                              segment.findTime(getStartTime()),
                              segment.findTime(getEndTime()));
    }

    // Kick the event loop.
    qApp->processEvents();

    if (segment.getEndTime() < endTime) {
        segment.setEndTime(endTime);
    }

    if (m_progressTotal > 0) {
        if (rebeam || makeviable || decounterpoint) {
            if (m_progressDialog)
                m_progressDialog->setValue(
                        m_progressTotal + m_progressPerCall / 2);
        } else {
            if (m_progressDialog)
                m_progressDialog->setValue(m_progressTotal + m_progressPerCall);
        }
    }

    if (m_selection) {
        EventSelection::RangeTimeList ranges(m_selection->getRangeTimes());
        for (EventSelection::RangeTimeList::iterator i = ranges.begin();
                i != ranges.end(); ++i) {
            if (makeviable) {
                helper.makeNotesViable(i->first, i->second, true);
            }
            // Kick the event loop.
            qApp->processEvents();
            if (decounterpoint) {
                helper.deCounterpoint(i->first, i->second);
            }
            // Kick the event loop.
            qApp->processEvents();
            if (rebeam) {
                helper.autoBeam(i->first, i->second, GROUP_TYPE_BEAMED);
                helper.autoSlur(i->first, i->second, true);
            }
            // Kick the event loop.
            qApp->processEvents();
        }
    } else {
        if (makeviable) {
            helper.makeNotesViable(getStartTime(), getEndTime(), true);
        }
        // Kick the event loop.
        qApp->processEvents();
        if (decounterpoint) {
            helper.deCounterpoint(getStartTime(), getEndTime());
        }
        // Kick the event loop.
        qApp->processEvents();
        if (rebeam) {
            helper.autoBeam(getStartTime(), getEndTime(), GROUP_TYPE_BEAMED);
            helper.autoSlur(getStartTime(), getEndTime(), true);
        }
        // Kick the event loop.
        qApp->processEvents();
    }

    if (m_progressTotal > 0) {
        if (rebeam || makeviable || decounterpoint) {
            if (m_progressDialog)
                m_progressDialog->setValue(
                        m_progressTotal  + m_progressPerCall / 2);
        }
    }

    if (m_progressDialog  &&  m_progressDialog->wasCanceled())
        throw CommandCancelled();
}

Quantizer *
EventQuantizeCommand::makeQuantizer(QString settingsGroup,
                                    QuantizeScope scope)
{
    //!!! Excessive duplication with
    // QuantizeParameters::getQuantizer in widgets.cpp

    QSettings settings;
    settings.beginGroup(settingsGroup);

    timeT defaultUnit =
        Note(Note::Demisemiquaver).getDuration();

    bool notationDefault =
        (scope == QUANTIZE_NOTATION_ONLY ||
         scope == QUANTIZE_NOTATION_DEFAULT);

    int type = settings.value("quantizetype", notationDefault ? 2 : 0).toInt();
    timeT unit = settings.value("quantizeunit", (int)defaultUnit).toInt();

    bool notateOnly;
    if (scope == QUANTIZE_NOTATION_ONLY) {
        notateOnly = true;
    } else {
        notateOnly = qStrToBool(settings.value("quantizenotationonly", notationDefault));
    }

    bool durations = qStrToBool(settings.value("quantizedurations", false));
    int simplicity = settings.value("quantizesimplicity", 13).toInt();
    int maxTuplet = settings.value("quantizemaxtuplet", 3).toInt();
    bool counterpoint = qStrToBool(settings.value("quantizecounterpoint", false));
    bool articulate = qStrToBool(settings.value("quantizearticulate", true));
    int swing = settings.value("quantizeswing", 0).toInt();
    int iterate = settings.value("quantizeiterate", 100).toInt();

    settings.endGroup();

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
