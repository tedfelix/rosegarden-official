/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

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
                                           // cppcheck-suppress passedByValue
                                           std::shared_ptr<Quantizer> quantizer):
    BasicCommand(getGlobalName(quantizer), segment, startTime, endTime,
                 true),  // bruteForceRedo
    m_quantizer(quantizer)
{
}

EventQuantizeCommand::EventQuantizeCommand(EventSelection &selection,
                                           // cppcheck-suppress passedByValue
                                           std::shared_ptr<Quantizer> quantizer):
    BasicCommand(getGlobalName(quantizer),
                 selection.getSegment(),
                 selection.getStartTime(),
                 selection.getEndTime(),
                 true),  // bruteForceRedo
    m_selection(&selection),
    m_quantizer(quantizer)
{
}

EventQuantizeCommand::EventQuantizeCommand(Segment &segment,
                                           timeT startTime,
                                           timeT endTime,
                                           const QString& settingsGroup,
                                           QuantizeScope scope):
    BasicCommand("Quantize",
                 segment, startTime, endTime,
                 true),  // bruteForceRedo
    m_settingsGroup(settingsGroup)
{
    makeQuantizer(settingsGroup, scope);
}

EventQuantizeCommand::EventQuantizeCommand(EventSelection &selection,
                                           const QString& settingsGroup,
                                           QuantizeScope scope) :
    BasicCommand("Quantize",
                 selection.getSegment(),
                 selection.getStartTime(),
                 selection.getEndTime(),
                 true),  // bruteForceRedo
    m_selection(&selection),
    m_settingsGroup(settingsGroup)
{
    makeQuantizer(settingsGroup, scope);
}

EventQuantizeCommand::~EventQuantizeCommand()
{
}

QString
EventQuantizeCommand::getGlobalName(std::shared_ptr<Quantizer> quantizer)
{
    if (quantizer) {
        if (std::dynamic_pointer_cast<NotationQuantizer>(quantizer)) {
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

    bool rebeam = false;
    bool makeViable = false;
    bool decounterpoint = false;

    if (!m_settingsGroup.isEmpty()) {
        // !!! need way to decide whether to do these even if no settings
        //     group (i.e. through args to the command)
        QSettings settings;
        settings.beginGroup( m_settingsGroup );

        rebeam = qStrToBool( settings.value("quantizerebeam", "true" ) ) ;
        makeViable = qStrToBool( settings.value("quantizemakeviable", "false" ) ) ;
        decounterpoint = qStrToBool( settings.value("quantizedecounterpoint", "false" ) ) ;
        settings.endGroup();
    }

    Segment &segment = getSegment();

    if (m_selection) {
        m_quantizer->quantize(m_selection);

    } else {
        m_quantizer->quantize(&segment,
                              segment.findTime(getStartTime()),
                              segment.findTime(getEndTime()));
    }

    // Kick the event loop.
    qApp->processEvents();

    const timeT endTime = segment.getEndTime();

    if (segment.getEndTime() < endTime)
        segment.setEndTime(endTime);

    if (m_progressTotal > 0) {
        if (rebeam || makeViable || decounterpoint) {
            if (m_progressDialog)
                m_progressDialog->setValue(
                        m_progressTotal + m_progressPerCall / 2);
        } else {
            if (m_progressDialog)
                m_progressDialog->setValue(m_progressTotal + m_progressPerCall);
        }
    }

    SegmentNotationHelper helper(segment);

    if (m_selection) {
        EventSelection::RangeTimeList ranges(m_selection->getRangeTimes());

        // For each time range in the selection
        for (const EventSelection::RangeTimeList::value_type &rRange : ranges) {

            const timeT startTime = rRange.first;
            const timeT endTime = rRange.second;

            if (makeViable)
                helper.makeNotesViable(startTime, endTime, true);

            // Kick the event loop.
            qApp->processEvents();

            if (decounterpoint)
                helper.deCounterpoint(startTime, endTime);

            // Kick the event loop.
            qApp->processEvents();

            if (rebeam) {
                helper.autoBeam(startTime, endTime, GROUP_TYPE_BEAMED);
                helper.autoSlur(startTime, endTime, true);
            }

            // Kick the event loop.
            qApp->processEvents();
        }
    } else {
        if (makeViable)
            helper.makeNotesViable(getStartTime(), getEndTime(), true);

        // Kick the event loop.
        qApp->processEvents();

        if (decounterpoint)
            helper.deCounterpoint(getStartTime(), getEndTime());

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
        if (rebeam || makeViable || decounterpoint) {
            if (m_progressDialog) {
                m_progressDialog->setValue(
                        m_progressTotal + m_progressPerCall / 2);
            }
        }
    }

    if (m_progressDialog  &&  m_progressDialog->wasCanceled())
        throw CommandCancelled();
}

void
EventQuantizeCommand::makeQuantizer(const QString &settingsGroup,
                                    QuantizeScope scope)
{
    // See QuantizeParameters::getQuantizer() which is quite similar.
    // ??? We should factor them out into a single factory function in
    //     Quantizer.

    // ??? Communicating via the .conf file is very confusing and could lead
    //     to bugs down the road.  Make the connection between the dialogs
    //     and this command more explicit.  Pass a quantization parameters
    //     struct or something so that this isn't mysterious.  Restrict the
    //     setting/getting of .conf info to the UI for persistence purposes
    //     only.
    QSettings settings;
    settings.beginGroup(settingsGroup);

    const bool notationDefault =
        (scope == QUANTIZE_NOTATION_ONLY  ||
         scope == QUANTIZE_NOTATION_DEFAULT);
    const int type =
            settings.value("quantizetype", notationDefault ? 2 : 0).toInt();

    const timeT defaultUnit =
        Note(Note::Demisemiquaver).getDuration();
    const timeT unit = settings.value("quantizeunit", (int)defaultUnit).toInt();

    bool notationOnly;
    if (scope == QUANTIZE_NOTATION_ONLY) {
        notationOnly = true;
    } else {
        notationOnly = qStrToBool(
                settings.value("quantizenotationonly", notationDefault));
    }

    const bool durations =
            qStrToBool(settings.value("quantizedurations", false));
    const int simplicity =
            settings.value("quantizesimplicity", 13).toInt();
    const int maxTuplet =
            settings.value("quantizemaxtuplet", 3).toInt();
    const bool counterpoint =
            qStrToBool(settings.value("quantizecounterpoint", false));
    const bool articulate =
            qStrToBool(settings.value("quantizearticulate", true));
    const int swing = settings.value("quantizeswing", 0).toInt();
    const int iterate = settings.value("quantizeiterate", 100).toInt();

    settings.endGroup();

    m_quantizer = nullptr;

    // BasicQuantizer
    if (type == 0) {
        if (notationOnly) {
            m_quantizer = std::shared_ptr<Quantizer>(new BasicQuantizer(
                    Quantizer::RawEventData,
                    Quantizer::NotationPrefix,
                    unit, durations, swing, iterate));
        } else {
            m_quantizer = std::shared_ptr<Quantizer>(new BasicQuantizer(
                    Quantizer::RawEventData,
                    Quantizer::RawEventData,
                    unit, durations, swing, iterate));
        }
    } else if (type == 1) {  // LegatoQuantizer
        if (notationOnly) {
            m_quantizer = std::shared_ptr<Quantizer>(new LegatoQuantizer(
                    Quantizer::RawEventData,
                    Quantizer::NotationPrefix, unit));
        } else {
            m_quantizer = std::shared_ptr<Quantizer>(new LegatoQuantizer(
                    Quantizer::RawEventData,
                    Quantizer::RawEventData, unit));
        }
    } else {  // NotationQuantizer

        std::shared_ptr<NotationQuantizer> notationQuantizer;

        if (notationOnly) {
            notationQuantizer = std::shared_ptr<NotationQuantizer>(
                    new NotationQuantizer());
        } else {
            notationQuantizer = std::shared_ptr<NotationQuantizer>(
                    new NotationQuantizer(
                            Quantizer::RawEventData,
                            Quantizer::RawEventData));
        }

        notationQuantizer->setUnit(unit);
        notationQuantizer->setSimplicityFactor(simplicity);
        notationQuantizer->setMaxTuplet(maxTuplet);
        notationQuantizer->setContrapuntal(counterpoint);
        notationQuantizer->setArticulate(articulate);

        m_quantizer = notationQuantizer;
    }

    setName(getGlobalName(m_quantizer));
}


}
