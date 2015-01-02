/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2015 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_EVENTQUANTIZECOMMAND_H
#define RG_EVENTQUANTIZECOMMAND_H

#include "document/BasicCommand.h"
#include "base/Event.h"

#include <QObject>
#include <QString>


namespace Rosegarden
{

class Segment;
class Quantizer;
class EventSelection;


class EventQuantizeCommand : public QObject, public BasicCommand
{
    Q_OBJECT

public:
    enum QuantizeScope {
        QUANTIZE_NORMAL,            /// Quantize the event's performed times
        QUANTIZE_NOTATION_DEFAULT,  /// Notation only unless overridden by settings
        QUANTIZE_NOTATION_ONLY      /// Notation only always
    };

    /// Quantizer must be on heap (EventQuantizeCommand dtor will delete)
    EventQuantizeCommand(Segment &segment,
                         timeT startTime,
                         timeT endTime,
                         Quantizer *);
    
    /// Quantizer must be on heap (EventQuantizeCommand dtor will delete)
    EventQuantizeCommand(EventSelection &selection,
                         Quantizer *);

    /// Constructs own quantizer based on QSettings data in given group
    EventQuantizeCommand(Segment &segment,
                         timeT startTime,
                         timeT endTime,
                         QString settingsGroup,
                         QuantizeScope scope);
    
    /// Constructs own quantizer based on QSettings data in given group
    EventQuantizeCommand(EventSelection &selection,
                         QString settingsGroup,
                         QuantizeScope scope);

    ~EventQuantizeCommand();
    
    static QString getGlobalName(Quantizer *quantizer = 0);
    void setProgressTotal(int total, int perCall) { m_progressTotal = total;
                                                    m_progressPerCall = perCall; };

signals:
    void setValue(int);

protected:
    virtual void modifySegment();

private:
    Quantizer *m_quantizer; // I own this
    EventSelection *m_selection;
    QString m_settingsGroup;
    int m_progressTotal;
    int m_progressPerCall;

    /// Sets to m_quantizer as well as returning value
    Quantizer *makeQuantizer(QString, QuantizeScope);
};

// Collapse equal-pitch notes into one event
//

}

#endif
