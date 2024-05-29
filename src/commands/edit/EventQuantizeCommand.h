/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2024 the Rosegarden development team.

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
#include <QPointer>
#include <QString>

class QProgressDialog;


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

    EventQuantizeCommand(Segment &segment,
                         timeT startTime,
                         timeT endTime,
                         std::shared_ptr<Quantizer> quantizer);

    EventQuantizeCommand(EventSelection &selection,
                         std::shared_ptr<Quantizer> quantizer);

    /// Constructs own Quantizer based on QSettings data in given group
    /**
     * ??? Quantization parameters should not be passed from the UI to this
     *     command via QSettings/.conf file.  We need a more direct approach
     *     like a parameters struct.  QSettings should be used for UI
     *     persistence only.
     */
    EventQuantizeCommand(Segment &segment,
                         timeT startTime,
                         timeT endTime,
                         const QString &settingsGroup,
                         QuantizeScope scope);

    /// Constructs own Quantizer based on QSettings data in given group
    /**
     * ??? Quantization parameters should not be passed from the UI to this
     *     command via QSettings/.conf file.  We need a more direct approach
     *     like a parameters struct.  QSettings should be used for UI
     *     persistence only.
     */
    EventQuantizeCommand(EventSelection &selection,
                         const QString &settingsGroup,
                         QuantizeScope scope);

    ~EventQuantizeCommand() override;

    static QString getGlobalName(
            std::shared_ptr<Quantizer> quantizer = std::shared_ptr<Quantizer>());

    void setProgressDialog(QPointer<QProgressDialog> progressDialog)
            { m_progressDialog = progressDialog; }
    void setProgressTotal(int total, int perCall)
    {
        m_progressTotal = total;
        m_progressPerCall = perCall;
    }

protected:

    void modifySegment() override;

private:

    EventSelection *m_selection{nullptr};
    QString m_settingsGroup;
    std::shared_ptr<Quantizer> m_quantizer;
    void makeQuantizer(const QString &settingsGroup, QuantizeScope);

    QPointer<QProgressDialog> m_progressDialog;
    int m_progressTotal{0};
    int m_progressPerCall{0};

};


}

#endif
